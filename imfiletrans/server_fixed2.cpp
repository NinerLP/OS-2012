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
#include <map>

#define PORT "3490"
#define BUF_SIZE 32*1024*1024


using namespace std;

int CURR_ID = 1;

struct buffer{
    int id;
    char* buf;
    int pos;
    bool senderDone;
    bool receiveDone;
    buffer() {
	buf = (char*)  malloc(BUF_SIZE);
	pos = 0;
	senderDone = false;
	receiveDone = false;
    }
};

map<int, buffer *> buffs;

void dataFromSender(pollfd src, buffer *buff) {
    if (!buff->senderDone) {
	if ((buff->pos < BUF_SIZE) && (src.revents & POLLIN)) {
	    int cnt = read(src.fd, buff->buf + buff->pos, BUF_SIZE - buff->pos);
	    printf("%d\n",cnt);
	    if (cnt <= 0) {
	        buff->senderDone = true;
            } else {
	        buff->pos+=cnt;
	    }
	}
        if ((src.revents & POLLERR)) {
            buff->senderDone = true;
	}
	//printf("some sender arrived %d", buff->pos);
    }
}

void dataToReceiver(pollfd dst, buffer *buff) {
    //printf("Start receiving, %d pos\n", buff->pos);
    if (!buff->receiveDone) {
	if ((buff->pos > 0) && (dst.revents & POLLOUT)) {
	    //printf("Writing");
	    int cnt = write(dst.fd, buff->buf, buff->pos);
	    if (cnt < 0) {
	        buff->receiveDone = true;
	    }
	    if (cnt > 0) {
		memmove(buff->buf, buff->buf + cnt, buff->pos - cnt);
	    }
	    buff->pos-=cnt;
	}
	if (buff->senderDone && buff->pos == 0) {
	    buff->receiveDone = true;
	}
    }
}

/*
void handle_client(int cfd) {
    printf(buf);
    printf("\n");
    if (!strcmp(buf,"send\0") || !strcmp(buf,"sendN")) {
	printf("sender connected\n");
	int bid = ++CURR_ID;
	char idBUF[10];
	sprintf(idBUF, "%d\n", bid);
	write(cfd,idBUF,strlen(idBUF));
	buffer *senderBuffer = new buffer;
	buffs[bid] = senderBuffer;
	dataFromSender(cfd, senderBuffer);	
	//printf(senderBuffer->buf);
	//printf("\n");
	//printf("%d\n", senderBuffer->pos);
    } else if (!strcmp(buf,"recv\0") || !strcmp(buf,"recvN")) {
	printf("receiver connected\n");
	read(cfd, buf, 4);
	int id = atoi(buf);
	//printf("%d\n",id);
	//printf(buffs[id]->buf);
	//printf("\n");
	//printf("%d\n",buffs[id]->pos);
	dataToReceiver(cfd, buffs[id]);
    } else {
	perror("wrong connection target\n");
	exit(1);
    }
    memset(&buf, 0, 5);
}*/

int main() {
    struct addrinfo hints;
    struct addrinfo *servinfo;
    int status;
    vector<int> fds;
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype=SOCK_STREAM;
    hints.ai_flags=AI_PASSIVE;
    if ((status=getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
	fprintf(stderr,"getaddrinfo error:%s\n",gai_strerror(status));
	exit(1);
    }

    while (servinfo) {
	fds.push_back(socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol));
	if (fds.back() < 0) {
	    perror("sockfd < 0");
	    exit(1);
	}	
        if (bind(fds.back(), servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
	    //perror("bind < 0");
	    //exit(1);
	}
	int yes = 1;
	if (setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
	    perror("setsockopt < 0");
	    exit(1);
	}
	if (listen(fds.back(), 5) < 0) {
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
    vector<bool> send(fds.size());
    vector<int> ids(fds.size());
    vector<int> op(fds.size()); //0 - accept, 1 - first read, 2 - rotating
    char buf[5];
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
	    perror("poll error");
	    exit(1);
	}
	if (ret > 0) {
	    for (int i = 0; i < pollfds.size(); i++) {
		if (op[i] == 0) {
	            if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
	    	        perror("listen error");
	    	        exit(1);
	            }
	            if (pollfds[i].revents & POLLIN) {
		        int cfd = accept(fds[i], NULL, NULL);
		        if (cfd < 0) {
		            perror("fd < 0");
		            continue;
		        }
			pollfds.push_back(pollfd());
			pollfds.back().fd = cfd;
			pollfds.back().events = POLLIN;
			pollfds.back().revents = 0;
			op.push_back(1);
			ids.push_back(-1);
		    }
		} else if (op[i] == 1) {
		    if (pollfds[i].revents & POLLIN) {

    		        read(pollfds[i].fd,&buf,5);
		        printf("%s\n", buf);
		        if (!strcmp(buf,"send\0")) {
		            printf("sender connected\n");
		            pollfds.push_back(pollfd());
		            pollfds.back().fd = pollfds[i].fd;
		            pollfds.back().events = POLLIN | POLLERR;
		            pollfds.back().revents = 0;
		            int bid = CURR_ID; 
		            CURR_ID++;
		            char idBUF[10];
		            sprintf(idBUF, "%d\n", bid);
		            write(pollfds[i].fd, idBUF,strlen(idBUF));
		            buffer *senderBuffer = new buffer;
		            buffs[bid] = senderBuffer;
		            ids.push_back(bid);
		            send.push_back(true);
			    op.push_back(2);
		        } else if (!strcmp(buf, "recv\0")) {
		            printf("receiver connected\n");
		            pollfds.push_back(pollfd());
		            pollfds.back().fd = pollfds[i].fd;
		            pollfds.back().events = POLLOUT | POLLERR;
		            pollfds.back().revents = 0;
		            read(pollfds[i].fd, &buf, 4);
			    printf("%s\n", buf);
		            int id = atoi(buf);
		            ids.push_back(id);
		            send.push_back(false);
			    op.push_back(2);
		        } else {
		            perror("wrong connection target");
		        }
		        memset(&buf, 0, 5);
		    }
		} else if (op[i] == 2) {
	            if (send[i]) {
		        //sender
		        if (pollfds[i].revents) {
	    	            dataFromSender(pollfds[i], buffs[ids[i]]);
		        }
	            } else {
	                //receiver
		        if (pollfds[i].revents) {
		            dataToReceiver(pollfds[i], buffs[ids[i]]);
		        }
		    }
   	            if (send[i]) {
	   	        if (buffs[ids[i]]->senderDone) {
		             pollfds[i].events = 0;
		             close(pollfds[i].fd);
		        }
	            } else {
		        if (buffs[ids[i]]->receiveDone) {
		            pollfds[i].events = 0;
		            close(pollfds[i].fd);
		        }
	            }
		}   
	    }	
	}	
    }
}


