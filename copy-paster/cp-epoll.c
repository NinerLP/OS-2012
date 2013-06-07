#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_EVENTS 10

int main(int argc, const char* argv[]) {
    int num = (argc-1)/2;
    struct stat* buf = (struct stat*) malloc(sizeof(struct stat));
    printf("%d\n", num);
    int efd, i, s, fd;
    struct epoll_event event;
    struct epoll_events *events;

    efd = epoll_create1(0);
    if (efd == -1) {
	perror("epoll create1");
        exit(1);
    }

    for (i = 0; i < num; i++) {
	fd = atoi(argv[2*i+1]);
	event.data.fd = fd;
	event.events = EPOLLIN;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
	if (s == -1) {
	    perror("epoll_ctl 1");
	    exit(1);
	}
	fd = atoi(argv[2*i+2]);
	event.data.fd = fd;
	event.events = EPOLLOUT;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
	if (s == -1) {
	    perror("epoll_ctl 2");
	    exit(1);
	}
    } 
    
    events = calloc(MAX_EVENTS, sizeof(event));

    int parsed = 0;

    while (parsed < num) {
	int n;
	n = epoll_wait(efd, events, MAX_EVENTS, -1);
	/*now events contains fds with events, but how do we go throught
	 *them, in poll they are all sorted in correct order, and we could
	 *check them in pairs, but epoll doesn't specify order, i think
	 *we can try doing it in a poll way, by creating an array of
	 *bool ready[], where we fill true if fd is ready, but then we 
	 *lose the advantage of epoll, that is, not having to linearly 
	 *go through pollfds and check revents.
	 *ofc we can put additional data into our epoll structure for each fd
	 *but atm I don't see the way it will help, knowing your sibling fd
	 *(the one you have to read or write to) does not seem to help */
    }	
}
