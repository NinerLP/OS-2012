#include <string>
#include <exception>
#include <unistd.h>

class autofd {
public:
	autofd(int fd);
	autofd(autofd && o);
	~autofd();
	int getfd();
	void trash();
	int operator *();
private:
	int fd;
	autofd();
	autofd (const autofd & o);
	autofd & operator = (const autofd &);
};

struct autofdException : public std::exception {
	std::string s;
	autofdException(std::string ss) : s(ss) {}
	const char* what() const throw() { return s.c_str(); }
};

autofd::autofd(int nfd) {
	if (nfd <= 0) {
		throw autofdException("Wrong file descriptor!");
	} else {
		fd = nfd;
	}
}

autofd::autofd(autofd && o) {
	if (*o <= 0) {
		throw autofdException("Moving wrong file descriptor!");
	} else {
		fd = *o;
		o.trash();
	}
}

autofd::~autofd() {
	if (fd > 0) {
		close(fd);
	}
}

int autofd::getfd() {
	return fd;
}

int autofd::operator*() {
	return fd;
}

void autofd::trash() {
	fd = -1;
}


