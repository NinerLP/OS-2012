#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUF_SIZE 1024

struct s_buff {
    char buff[BUF_SIZE];
    size_t pos;
    int in_dead;
};

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
//	printf("%d\n", num);
	struct pollfd* fds =(struct pollfd*)malloc(sizeof(struct pollfd)*num*2);
	int i;
	struct s_buff *buffs = (struct s_buff*)malloc(sizeof(struct s_buff)*num);
	for (i = 0; i < num; i++) {
		fds[2*i].fd = atoi(argv[2*i+1]);
		fds[2*i].events = POLLIN;
		fds[2*i+1].fd = atoi(argv[2*i+2]);
		fds[2*i+1].events = POLLOUT;
		buffs[i].in_dead = 0;
		buffs[i].pos = 0;
//		printf("From fd %d to %d\n", fds[2*i].fd, fds[2*i+1].fd);
	}
        
	int parsed = 0;
	int res;
	while (parsed < num) {
//		printf("inb4 poll\n");
		res = poll(fds,num*2,-1);
		if (res <= 0) {
			//errors
		} else {
			//printf("Dun gufed %d\n", res);
			for (i = 0; i < num; i++) {
				if (fds[2*i].revents & POLLIN && buffs[i].pos < BUF_SIZE) {
					//ready to write
					int cnt = read(fds[2*i].fd, buffs[i].buff + buffs[i].pos, BUF_SIZE - buffs[i].pos);
					if (cnt <= 0) {
						buffs[i].in_dead = 1;
						fds[2*i].events = 0;
						//parsed++;
					} else {
						buffs[i].pos+=cnt;
					}
					//printf("read %d pos %d\n ", cnt, (int) buffs[i]->pos);
				}
				if (fds[2*i+1].revents & POLLOUT && buffs[i].pos > 0) {
					//printf("inb4 write pos %d\n", buffs[i]->pos);
					int cnt = write(fds[2*i+1].fd, buffs[i].buff, buffs[i].pos);
					//printf("wrote %d\n", cnt);
					if (cnt > 0) {
						memmove(buffs[i].buff, buffs[i].buff + cnt, buffs[i].pos - cnt);
					}
					buffs[i].pos -= cnt;
				}
				if (buffs[i].pos == 0 && buffs[i].in_dead) {
					fds[2*i+1].events = 0;
					parsed++;
				}
			}
//			printf("pass, parsed %d\n", parsed);
		}
	}
//	printf("done");
	return 0;
}
