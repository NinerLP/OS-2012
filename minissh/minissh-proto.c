#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <iostream>
#include <pty.h>

const int BUF_SIZE = 4096;

struct s_buffer {
    char *buff;
    size_t pos;
    bool in_dead;
    bool out_dead;
};

using namespace std;


int pull_data(pollfd *src, s_buffer *buff, pollfd *dest) {
    if ((buff->pos < BUF_SIZE) && (src->revents & POLLIN)) {
	    int cnt =  read(src->fd, buff->buff + buff->pos, BUF_SIZE - buff->pos);
	    if (cnt <= 0) {
		buff->in_dead = true;
	    } else {
	    	buff->pos+=cnt;
	    } 
    }

    if (buff->pos > 0) {// && (dest->revents & POLLOUT)) {
	    int cnt = write(dest->fd, buff->buff, buff->pos);
	    if (cnt < 0) {
		buff->out_dead = true;
	    }
	    if (cnt > 0) {
		memmove(buff->buff, buff->buff+cnt, buff->pos - cnt);
	    }	    
	    buff->pos-=cnt;
    }

    if ((buff->pos < BUF_SIZE) && (!buff->out_dead)) {
	src->events |= POLLIN;
    } else {
	src->events &= ~POLLIN;
    }

    return 0;
}

void handle_client(int cfd) {
    setsid();
    int amaster = 0;
    int aslave = 0;
    char name[4096];
    if (openpty(&amaster, &aslave, name, NULL, NULL) == -1) {
	perror("openpty failed");
	exit(1);    
    }
    int shellpid = fork();
    if (shellpid != 0) {
	close(aslave);
	pollfd pfds[2];
	pfds[0].fd = cfd;
	pfds[0].events = POLLIN | POLLERR;// | POLLOUT; 
	pfds[0].revents = 0;
	pfds[1].fd = amaster;
        pfds[1].events = POLLIN | POLLERR;// | POLLOUT;
	pfds[1].revents = 0;
	int ret;
	struct s_buffer bufftom;
	bufftom.buff = (char*) malloc(BUF_SIZE);
	struct s_buffer bufffrm;
	bufffrm.buff = (char*) malloc(BUF_SIZE);
	while (!bufftom.in_dead && !bufftom.out_dead && !bufffrm.in_dead && !bufffrm.out_dead) {
	    ret = poll(pfds, 2, -1);
	    if (!ret) {
		continue;
	    }
	    if (ret < 0) {
	        //error
	        perror("something bad with poll");
	        exit(1);
	    }
	    if (pfds[0].revents & POLLERR || pfds[1].revents & POLLERR ) {
		perror("poll error");
	    	exit(1);
	    }
	    if (pfds[0].revents & POLLIN) {// && pfds[1].revents & POLLOUT) {
		pull_data(&pfds[0],&bufftom,&pfds[1]);
	    }
	    if (pfds[1].revents & POLLIN) {// && pfds[0].revents & POLLOUT) {
		pull_data(&pfds[1],&bufffrm,&pfds[0]);
	    }
 	}
	kill(shellpid, SIGTERM);
	exit(0);
    }
    close(amaster);
    close(cfd);

    //proto

    int pty_fd = open(name, O_RDWR);
    close(pty_fd);

    dup2(aslave, 0);
    dup2(aslave, 1);
    dup2(aslave, 2);
    printf("execlp");
    execlp("bash", "bash", NULL);
    close(aslave);
    _exit(6);
}

int main()
{
    struct addrinfo hints;
    struct addrinfo *servinfo;
    int status;
    vector<int> fds;
    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_UNSPEC; //don’tcareIPv4orIPv6
    hints.ai_socktype=SOCK_STREAM;//TCPstreamsockets
    hints.ai_flags=AI_PASSIVE; //fillinmyIPforme
    if ((status=getaddrinfo(NULL,"3490",&hints,&servinfo)) !=0){
        fprintf(stderr,"getaddrinfo error:%s\n",gai_strerror(
                    status));
        exit(1);
    }
    while (servinfo) {
        if (servinfo == NULL)
        {
            printf("servinfo == NULL\n");
            exit(1);
        }
        fds.push_back(socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol));
        if (fds.back() < 0)
        {
            perror("sockfd < 0");
            exit(1);
        }
        if (bind(fds.back(), servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        {
           // perror("bind < 0");
           // exit(1);
        }
        int yes = 1;
        if (setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        {
            perror("setsockopt < 0");
            exit(1);
        }
	if (listen(fds.back(), 5) < 0)
	{
    	    perror("listen < 0");
    	    exit(1);
	}
	servinfo = servinfo->ai_next;
    }
    freeaddrinfo(servinfo);
    if (fds.empty()) {
        perror("no binds");
	exit(1);
    }
    int fd = -1;
    vector<pollfd> pollfds(fds.size());
    for (int i = 0; i < pollfds.size(); i++) {
        pollfds[i].fd = fds[i];
        pollfds[i].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
	pollfds[i].revents = 0;
    }

    while (true) {
	int ret = poll(pollfds.data(), pollfds.size(), 1);
	if (ret < 0) {
	    //error
	    perror("something bad with poll");
	    exit(1);
	}
	if (ret == 0) {
	    continue;
	}
	for (int i = 0; i < pollfds.size(); i++) {
	    if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
		//pollfds[i].events = 0;
		//close(fds[i]);
		cerr << "listen error" << endl;
		exit(1);
	    }
	    if (pollfds[i].revents & POLLIN) {
	        int cfd = accept(fds[i], NULL, NULL);
	        if (cfd < 0) {
		    perror("fd < 0");
		    continue;
		}
	        int pid = fork();
        	if (pid == 0) {
                    for (int i = 0; i < fds.size(); i++) {
			close(fds[i]);
		    }
		    handle_client(cfd);
                }
                close(cfd);
	    }
	}
    }
}
