#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUF_SIZE 32*1024*1024

void readTZ(int sockfd) {
    bool done = false;
    char buffer[256];
    while (!done) {
	int cnt = read(0, buffer, 256);
	write(sockfd, buffer, cnt);
//	for (int i = 0; i < cnt; i++) {
	   // if (buffer[i] == '\0') {
//		done = true;
//	    }
//	}
        if (cnt <= 0) {
	    done = true;
	}
    }
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    if (argc < 3) {
	fprintf(stderr,"%s hostname port\n", argv[0]);
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
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
	perror("connect error");
     printf("Sending!\n");
     char buf[5];
     sprintf(buf,"send\0");
     write(sockfd, buf, strlen(buf));
     bzero(buffer,256);
     read(sockfd,buffer,255);
     printf("%s\n", buffer);
     readTZ(sockfd);
     close(sockfd);
     return 0;
}
