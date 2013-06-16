#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_EVENTS 10
#define BUF_SIZE 1024

struct s_buff {
    char buff[BUF_SIZE];
    size_t pos;
    int in_dead;
};

int main(int argc, const char* argv[]) {
    int num = (argc-1)/2;
    struct stat* buf = (struct stat*) malloc(sizeof(struct stat));
    printf("%d\n", num);
    int efd, i, s, fd;
    struct epoll_event event;
    struct epoll_event *events;

    efd = epoll_create1(0);
    if (efd == -1) {
	perror("epoll create1");
        exit(1);
    }

    for (i = 0; i < num; i++) {
	fd = atoi(argv[2*i+1]);
	struct s_buff* buffer =(struct s_buff*) malloc(sizeof(struct s_buff));
	event.data.fd = fd;
	event.data.ptr = buffer;
	event.events = EPOLLIN;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
	if (s == -1) {
	    perror("epoll_ctl 1");
	    exit(1);
	}
	fd = atoi(argv[2*i+2]);
	event.data.fd = fd;
	event.events = EPOLLOUT;
	event.data.ptr = buffer;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
	if (s == -1) {
	    perror("epoll_ctl 2");
	    exit(1);
	}
    } 
    
    events = calloc(MAX_EVENTS, sizeof(event));

    int parsed = 0;

    while (parsed < num) {
	int n, i;
	n = epoll_wait(efd, events, MAX_EVENTS, -1);
	for (i = 0; i < n; i++) {
	    if (events[i].events & EPOLLIN) {
	    	//some fd is ready for read
	    	struct s_buff* buff = (struct s_buff*) events[i].data.ptr;
	    	if (buff->pos < BUF_SIZE) {
		    int cnt = read(events[i].data.fd, buff->buff + buff->pos, BUF_SIZE - buff->pos);
		    if (cnt <= 0) {
		    	buff->in_dead = 1;
		    	//close(events[i].data.fd);
			epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &event);
		    } else {
		    	buff->pos+=cnt;
		    }
	    	}	 
	    } else if (events[i].events & EPOLLOUT) {
	    	struct s_buff* buff = (struct s_buff*) events[i].data.ptr;
	   	 if (buff->pos > 0) {
		    int cnt = write(events[i].data.fd, buff->buff, buff->pos);
		    if (cnt > 0) {
		    	memmove(buff->buff, buff->buff+cnt, buff->pos - cnt);
		    }
		    buff->pos -= cnt;
	    	}
	    	if (buff->pos == 0 && buff->in_dead) {
		    //close(events[i].data.fd);
		    epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &event);
		    parsed++;
		}
    	    }
	}
    }
    free(events);
    return 0;	
}
