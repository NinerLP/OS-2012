#include "autofd.h"
#include <iostream>
#include <utility>

int main(void) {
/*	try {
		autofd test(-1);
		std::cout << *test;
	} catch (autofdException e) {
		std::cout << e.what();
	}*/
	
	autofd test2(autofd(2));
	autofd test3(3);	
	
	std::cout << *test2 << " " << *test3 << std::endl;	
	return 0;
}
