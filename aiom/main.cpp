#include "autofd.h"
#include "epollfd.h"
#include "async.h"
#include <iostream>
#include <utility>
#include <functional>

void test() {
	printf("Test\n");
}

void test2() {
	printf("Test2\n");
}

int main(void) {
	epollfd *lol = new epollfd();
	async ass(lol, 0, EPOLLIN, test2);
	lol->cycle();
	lol->subscribe(1, EPOLLIN, test);
	while (!ass.isDone()) {
		lol->cycle();
	}
	if (ass.isDone()) {
		printf("Done");
	}
	return 0;
}
