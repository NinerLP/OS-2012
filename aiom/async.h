#ifndef __ASYNC_H_
#define __ASYNC_H_

#include <exception>
#include <unistd.h>
#include <string>
#include <functional>
#include <utility>
#include <sys/epoll.h>
#include <sys/types.h>
#include "epollfd.h"


struct asyncException : public std::exception {
	std::string s;
	asyncException(std::string ss) : s(ss) {}
	const char* what() const throw() { return s.c_str(); }
};

class async {
public:
	async(epollfd *epfd, int fd, __uint32_t what, std::function<void(void)> cont);
	~async();
	bool isDone();
	void reset();
	void myContinuation();
private:
	bool done;
	int fd;
	__uint32_t what;
	epollfd *epfd;
	std::function<void(void)> cont;
};

async::async(epollfd *epfd, int fd, __uint32_t what, std::function<void(void)> cont) : epfd(epfd), fd(fd), what(what), cont(cont), done(false) {
	std::function<void(void)> f = std::bind(&async::myContinuation, this);
	epfd->subscribe(fd,what,f);
}

async::~async() {
	if (!done) {
		epfd->unsubscribe(fd, what);
	}
}

bool async::isDone() {
	return done;
}

void async::reset() {
	if (done) {
		std::function<void(void)> f = std::bind(&async::myContinuation, this);
		epfd->subscribe(fd,what,f);
	}
}

void async::myContinuation() {
	done = true;
	cont();
}

#endif

