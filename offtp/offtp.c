#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <vector>
#include <iostream>
#include <pty.h>
#include <errno.h>
#include <sys/stat.h>

using namespace std;

#define PORT "3491"
#define FNAME_LEN 200
#define BUF_SIZE 1024

struct s_buffer {
	char data[BUF_SIZE];
	size_t pos;
	int fd_int;
	int fd_out;
};

int read_to_buf(s_buffer* buf, int fd) {
	if (buf->pos < BUF_SIZE) {
		int cnt = read(fd, buf->data + buf->pos, BUF_SIZE - buf->pos);
		if (cnt <= 0) {
			perror("read fuped");
			return -1;
		} else {
		buf->pos += cnt;
		return cnt;
		}
	}
	return 0;
}

int read_from_buf(s_buffer* buf, int fd) {
	if (buf->pos > 0) {
		int cnt = write(fd, buf->data, buf->pos);
		if (cnt <= 0) {
			perror("write fuped");
		} 
		if (cnt > 0) {
			memmove(buf->data, buf->data + cnt, buf->pos -cnt);
			buf->pos -= cnt;
			return cnt;
		}
	}
	return 0;
}

void handle_client(int cfd) {
	char filename[FNAME_LEN+1];
	//printf("dir: ");
	//printf(get_current_dir_name());
	//printf("\n");
	read(cfd, &filename, FNAME_LEN);
	int ps = 0;
	//printf(filename); printf("\n");
/*	while (!strcmp(filename, '\0')) {
		int rd = read(cfd, &filename+ps, FNAME_LEN - ps);
		if (rd < FNAME_LEN) {
			ps += rd;
		}
		printf(filename);
		printf("\n");
	}*/
	struct stat* sbuf = (struct stat*) malloc(sizeof(struct stat));
	char message[2];
	int fd = open(filename, O_RDONLY);	
if (fd < 0) {
		message[0] = 'E'; message[1] = 'R';
		write(cfd, message, 2);
		write(cfd, strerror(errno),strlen(strerror(errno)));
		close(cfd);
		return;
	}
	//writing
	fstat(fd,sbuf);
	s_buffer* buf =(s_buffer*) malloc(BUF_SIZE);
	message[0] = 'O'; message[1] = 'K';
	char sizeBUF[40];
	memset(&sizeBUF,0,40);
	sprintf(sizeBUF,"%d\0",sbuf->st_size);
	write(cfd,message,2);
	write(cfd,sizeBUF,40);

	int cnt = 1;
	while (cnt > 0) {
		cnt = read_to_buf(buf, fd);
		printf("read %d\n", cnt);
		cnt = read_from_buf(buf, cfd);
		printf("wrote %d\n", cnt);
	}
	free(buf);
	printf("I WROTE EVERYSHIT\n");
}

int main() {

	//daemon
	int dpid = fork();
	if (dpid != 0) {
		waitpid(dpid, NULL, 0);
		exit(0);
	}
	int sid = setsid();
	if (sid < 0) {
		perror("session creation error");
		exit(1);
	}

	//prepare network
	struct addrinfo hints;
	struct addrinfo *servinfo;
	int status;
	vector<int> fds;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	while (servinfo) {
		fds.push_back(socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol));
		if (fds.back() < 0) {
			perror("sockfd < 0");
			exit(1);
		}
		if (bind(fds.back(), servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		//	perror("bind < 0");
		//	exit(1);
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
	vector<pollfd> pollfds(fds.size());
	for (int i = 0; i < pollfds.size(); i++) {
		pollfds[i].fd = fds[i];
		pollfds[i].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
		pollfds[i].revents = 0;
	}

	//main loop serve
	while (true) {
		int ret = poll(pollfds.data(), pollfds.size(), -1);
		if (ret < 0) {
			perror("something w/ poll");
			exit(1);
		}
		if (ret == 0) {
			continue;
		}
		int status;
		wait3(&status, WNOHANG, NULL);
		for (int i = 0; i < pollfds.size(); i++) {
			if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
				cerr << "listen error" << endl;
				pollfds[i].events = 0;
				close(fds[i]);
				_exit(1);
			}
			if (pollfds[i].revents & POLLIN) {
				int cfd = accept(fds[i], NULL, NULL);
				if (cfd < 0) {
					perror("fd < 0");
					continue;
				}
				printf("accepted %d\n", cfd);
				int pid = fork();
				if (pid == 0) {
					for (int i = 0; i < fds.size(); i++) {
						close(fds[i]);
					}
					
					handle_client(cfd);
					return 0;
				}
				close(cfd);
		//		pollfds[i].events = 0;
			}
		}
	}
}
