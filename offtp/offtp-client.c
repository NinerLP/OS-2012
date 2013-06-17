#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

#define BUF_SIZE 1024

void readTZ(int sockfd) {
//   printf("Start reading\n");
    bool done = false;
    char message[2];
    //int fd = open("result.file", O_CREATE);
    int read_size = 0;
    int expected_size;
    char sbuf[40];
    char buffer[BUF_SIZE];
    int cnt = read(sockfd, message, 2);
    if (cnt < 2 || strcmp(message,"ER")) {
	//error
	cnt = read(sockfd, buffer, BUF_SIZE);
	write(1, buffer, cnt);
	printf("\n");
	return;
    }	
    cnt = read(sockfd, sbuf, 40);
    expected_size = atoi(sbuf);
    int wfile = open("downloaded", O_CREAT);
    while (!done) {
	int cnt = read(sockfd, buffer, BUF_SIZE);
	write(1, buffer, cnt);
//	printf("read %d\n", cnt);
	read_size += cnt;
	//for (int i = 0; i < cnt; i++) {
	//    if (buffer[i] == '\0') {
	//	done = true;
	//    }
	//}
        if (cnt <= 0) {
	    done = true;
	}
    }
	printf("READ: %d EXPECTED: %d\n",read_size, expected_size);
    if (read_size == expected_size) {
	printf("Download OK\n");
    } else {
	printf("Donwload failed\n");
    }
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    printf("test\n");
    char buffer[BUF_SIZE];
    char buf[200];
    if (argc < 4) {
	fprintf(stderr,"%s hostname port id\n", argv[0]);
	exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
	perror("Error opening socket");
	exit(1);
    }
    server = gethostbyname(argv[1]);
    if (server == NULL) {
	fprintf(stderr, "ERROR, no such host\n");
	exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr));

     sprintf(buf,"%s", argv[3]);
     write(sockfd, buf, strlen(buf));
	printf("\n");
     readTZ(sockfd);

     close(sockfd);
     return 0;
}
