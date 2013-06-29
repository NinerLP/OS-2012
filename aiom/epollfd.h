#ifndef __EPOLLFD_H_
#define __EPOLLFD_H_

#include <exception>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <string>
#include <map>
#include <utility>
#include <functional>

struct epollfdException : public std::exception {
	std::string s;
	epollfdException(std::string ss) : s(ss) {}
	const char*  what() const throw() { return s.c_str(); }
};

class epollfd {
public:
	epollfd();
	~epollfd();
	int subscribe(int fd, __uint32_t what, std::function<void(void)> cont);
	int unsubscribe(int fd, __uint32_t what);
	void cycle();
private:
	int efd;
	epoll_event event;
	epoll_event *events;
	const int MAX_EVENTS = 10;
	std::map<std::pair<int, __uint32_t>, std::function<void(void)> > speshulMap;
	std::map<int, __uint32_t> allEvents; 
};

epollfd::epollfd() {
	efd = epoll_create1(0);
	if (efd == -1) {
		throw epollfdException("Error initialising epoll");
	}
	events = (epoll_event*) calloc(MAX_EVENTS, sizeof(event));
}

epollfd::~epollfd() {
	free(events);
}

int epollfd::subscribe(int fd, __uint32_t what, std::function<void(void)> cont) {
	event.data.fd = fd;
	if (allEvents.find(fd) != allEvents.end()) {
		//already in poll
		event.events = what | allEvents[fd];
		__uint32_t bkp = allEvents[fd];
		int s = epoll_ctl(efd,EPOLL_CTL_MOD,fd,&event);
		if  (s == -1) {
			throw epollfdException("epoll add error in subscribe, while adding more events");
		}
		allEvents[fd] = what | allEvents[fd];
		try {
			speshulMap[std::make_pair(fd, what)] =  cont;
		} catch (std::bad_alloc& e) {
			event.events = bkp;
			int s = epoll_ctl(efd,EPOLL_CTL_MOD, fd, &event);
			if (s == -1) {
				throw epollfdException("something terrible occured when adding more events");
			}
			allEvents[fd] = bkp;
		}
	} else {
		//has to be newly added
		event.events = what;
		int s = epoll_ctl(efd,EPOLL_CTL_ADD,fd,&event);
		if (s == -1) {
			throw epollfdException("epoll add error in subscribe, while creating new");
		}
		try {
			allEvents[fd] = what;
			speshulMap[std::make_pair(fd, what)] =  cont;
		} catch (std::bad_alloc& e) {
			//can't do something with map cancel epoll op
			int s = epoll_ctl(efd,EPOLL_CTL_DEL,fd, NULL);
			if (s == -1) {
				throw epollfdException("something terrible occured in epoll add new");
			}
		}
	}
	return 0;			 
}

int epollfd::unsubscribe(int fd, __uint32_t what) {
	//check if subscribed at all
	if (speshulMap.find(std::make_pair(fd, what)) != speshulMap.end()) {
		//actually is subscribed
		//now check if only that event
		if (allEvents[fd] == what) {
			//only that event just delete all
			epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
			allEvents.erase(allEvents.find(fd));
			speshulMap.erase(speshulMap.find(std::make_pair(fd,what)));
		} else {
			//some other too!
			event.data.fd = fd;
			event.events = allEvents[fd] & !what;
			epoll_ctl(efd, EPOLL_CTL_MOD, fd, &event);
			speshulMap.erase(speshulMap.find(std::make_pair(fd,what)));
		}
	} else {
		throw epollfdException("whoa, you are unsubscribing from shit you didn't sub to!");	
	}

	return 0;
}

void epollfd::cycle() {
	int n = epoll_wait(efd, events, MAX_EVENTS, 0);
	for (int i = 0; i < n; i++) {
		//to do : insert unsubscribes here
		int rfd = events[i].data.fd;
		if (events[i].events & EPOLLIN) {
			speshulMap.at(std::make_pair(rfd, EPOLLIN))();
			unsubscribe(rfd, EPOLLIN);
		}
		if (events[i].events & EPOLLOUT) {
			speshulMap[std::make_pair(rfd, EPOLLOUT)]();
			unsubscribe(rfd, EPOLLOUT);
		}
		if (events[i].events & EPOLLRDHUP) {
			speshulMap[std::make_pair(rfd, EPOLLRDHUP)]();
			unsubscribe(rfd, EPOLLRDHUP);
		}
		if (events[i].events & EPOLLPRI) {
			speshulMap[std::make_pair(rfd, EPOLLPRI)]();
			unsubscribe(rfd, EPOLLPRI);
		}
		if (events[i].events & EPOLLERR) {
			speshulMap[std::make_pair(rfd, EPOLLERR)]();
			unsubscribe(rfd, EPOLLERR);
		}
		if (events[i].events & EPOLLHUP) {
			speshulMap[std::make_pair(rfd, EPOLLHUP)]();
			unsubscribe(rfd, EPOLLHUP);
		}
		if (events[i].events & EPOLLET) {
			speshulMap[std::make_pair(rfd, EPOLLET)]();
			unsubscribe(rfd, EPOLLET);
		}
	}
}

#endif
