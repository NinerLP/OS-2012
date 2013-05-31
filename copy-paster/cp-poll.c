#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, const char* argv[] ) {
	/* so we have file descriptor list in argv
	 * 2n of fd, and we use sendfile(argv[2k-1],argv[2k],NULL, sizeoffd?
	 * poll usage, do i just make arry of structs 
	 * fd - events - revents for each fd, where i set events POLLIN and POLLOUT for 2k and 2k-1 fds?
	 * FROM 2k-1 TO 2k
	*/
	
	//rough shit that may be working or may be not working at all
	int num = (argc-1)/2;
	struct stat* buf = (struct stat*)malloc(sizeof(struct stat));
	printf("%d\n", num);
	struct pollfd* fds =(struct pollfd*)malloc(sizeof(struct pollfd)*num*2);
	int i;
	for (i = 0; i < num; i++) {
		fds[2*i].fd = atoi(argv[2*i+1]);
		fds[2*i].events = POLLIN;
		fds[2*i+1].fd = atoi(argv[2*i+2]);
		fds[2*i+1].events = POLLOUT;
		printf("From fd %d to %d\n", fds[2*i].fd, fds[2*i+1].fd);
	}
	int parsed = 0;
	int res;
	while (parsed < num) {
		res = poll(fds,num*2,-1);
		if (res < 0) {
			//error
		} else {
			//printf("Dun gufed %d\n", res);
			for (i = 0; i < num; i++) {
			//	if (fds[2*i].revents) {
			//		printf("fD %d rdy\n", fds[2*i-1].fd);
			//	}
			//	if (fds[2*i+1].revents) {
			//		printf("fD %d rdy\n", fds[2*i].fd);
			//	}
				if ((fds[2*i].revents & POLLIN) && (fds[2*i+1].revents & POLLOUT)) {
					fstat(fds[2*i].fd,buf);
					sendfile(fds[2*i+1].fd,fds[2*i].fd, NULL, buf->st_size);
					fds[2*i].events = 0;
					fds[2*i+1].events = 0;
					parsed++;
				}
			}
		}
	}
	return 0;
}
