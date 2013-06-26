#include "autofd.h"
#include "epollfd.h"
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
	epollfd lol;
	lol.subscribe(1,EPOLLIN, test);
	lol.cycle();
	lol.subscribe(1,EPOLLIN, test2);
	lol.cycle();
	lol.unsubscribe(1,EPOLLOUT);
	lol.cycle();
	return 0;
}
