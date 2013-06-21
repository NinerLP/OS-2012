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
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <queue>
#include <string>
#include <pty.h>

using namespace std;

class client {
public:
	client() {}
	client(int fd, bool socket) : fd(fd), socket(socket) {}	
	queue<string> messageQueue;
	int fd;
	bool socket;
	//vector<string> subscribed;
};

class sigal {
public:
	sigal() {}
	sigal(string name, client * subscriber) : name(name) {
		subscribers.push_back(subscriber);
	}
	string name;
	vector<client *> subscribers;
};

int main() {
 /*   	int dpid = fork();
    	if (dpid != 0) {
        	waitpid(dpid, NULL, 0);
        	exit(0);
    	}
    	int sid = setsid();
	if (sid < 0) {
		perror("session creation error");
		exit(1);
    	}*/
    	struct addrinfo hints;
    	struct addrinfo *servinfo;
    	int status;
    	vector<int> fds;
    	memset(&hints,0,sizeof hints);
    	hints.ai_family=AF_UNSPEC; //don’tcareIPv4orIPv6
    	hints.ai_socktype=SOCK_STREAM;//TCPstreamsockets
    	hints.ai_flags=AI_PASSIVE; //fillinmyIPforme
    	if ((status=getaddrinfo(NULL,"3491",&hints,&servinfo)) !=0){
        	fprintf(stderr,"getaddrinfo error:%s\n",gai_strerror(
                    status));
        	exit(1);
   	}
    	while (servinfo) {
        	if (servinfo == NULL)
        	{
            		printf("servinfo == NULL\n");
            		exit(1);
        	}
        	fds.push_back(socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol));
        	if (fds.back() < 0)
        	{
            		perror("sockfd < 0");
            		exit(1);
        	}
        	if (bind(fds.back(), servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        	{
           	// perror("bind < 0");
           	// exit(1);
        	}
        	int yes = 1;
        	if (setsockopt(fds.back(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        	{
           		perror("setsockopt < 0");
            		exit(1);
        	}
		if (listen(fds.back(), 5) < 0)
		{
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

	printf("Prea alloc");
	vector<client* > clients;
	vector<pollfd> pollfds(fds.size());
	for (int i = 0; i < fds.size(); i++) {
		pollfds[i].fd = fds[i];
		pollfds[i].events = POLLIN | POLLERR | POLLHUP | POLLRDHUP | POLLNVAL;
		pollfds[i].revents = 0;
		clients.push_back(new client(fds[i], true));
	}

	vector<sigal* > signals;

	printf("Serv started");
	while (true) {
		int ret = poll(pollfds.data(), pollfds.size(), -1);
		if (ret < 0) {
			perror("Poll went wrong");
			exit(1);
		}
		if (ret == 0) {
			continue;
		}
		int status;
		wait3(&status, WNOHANG, NULL);
		for (int i = 0; i < pollfds.size(); i++) {
			if (pollfds[i].revents & (POLLERR | POLLHUP | POLLRDHUP | POLLNVAL)) {
				pollfds[i].events = 0;
				close(fds[i]);
				_exit(1);
			}	
			if (pollfds[i].revents & POLLIN && clients[i]->socket) {
				int cfd = accept(fds[i], NULL, NULL);
				if (cfd < 0) {
					perror("fd < 0");
					continue;
				}
				printf("Accepted %d\n", cfd);
				//accepted - now add it
				clients.push_back(new client(cfd, false));
				pollfd tmp;
				tmp.fd = cfd;
				tmp.events = POLLIN | POLLOUT;
				tmp.revents = 0;
				pollfds.push_back(tmp);
			}
			if (pollfds[i].revents & POLLIN && !clients[i]->socket) {
				//client has request
				//"lst\0\0\0\0\0\0\0 or "sub signame" or "uns signame" or "emt signame"
				//message size always 10, -> signal name size max  == 6, min 1
				char message[10];
				read(clients[i]->fd, &message, 10);
				string stmess = string(message);
				if (stmess.substr(0,3) == "lst") {
					printf("Client ordered a list\n");
					string resp = "Singals:";
					for (int k = 0; k < signals.size(); k++) {
						resp = resp + " " + signals[k]->name;
					}
					resp += "\n";
					clients[i]->messageQueue.push(resp);
					pollfds[i].events = POLLIN | POLLOUT;
				} else if (stmess.substr(0,3) == "sub") {
					string signame = stmess.substr(4,6);
					printf("Client subscribed to %s\n", signame.c_str());
					int signum = -1;
					for (int k = 0; k < signals.size(); k++) {
						if (signals[k]->name == signame) {
							signum = k;
							break;
						}
					}
					if (signum == -1) {
						signals.push_back(new sigal(signame, clients[i]));
					} else {
						bool already = false;
						for (int k = 0; k < signals[signum]->subscribers.size(); k++) {
							if (signals[signum]->subscribers[k] == clients[i]) {
								already = true;
								break;
							}
						}
						if (!already) {
							signals[signum]->subscribers.push_back(clients[i]);
						}
					}
				} else if (stmess.substr(0,3) == "uns") {
					string signame = stmess.substr(4,6);
					printf("Client unsubbed from %s\n", signame.c_str());
					int signum = -1;
					for (int k = 0; k < signals.size(); k++) {
						if (signals[k]->name == signame) {
							signum = k;
							break;
						}
					}
					if (signum != -1) {
						//at least such signal exists
						int clnum = -1;
						for (int k = 0; k < signals[signum]->subscribers.size(); k++) {
							if (signals[signum]->subscribers[k] == clients[i]) {
								clnum = k;
								break;
							}
						}
						if (clnum != -1) {
							signals[signum]->subscribers.erase(signals[signum]->subscribers.begin() + clnum);
						}
						if (signals[signum]->subscribers.size() == 0) {
							signals.erase(signals.begin() + signum);
						}
					}
				} else if (stmess.substr(0,3) == "emt") {
					string signame = stmess.substr(4,6);
					printf("Client emmited %s\n", signame.c_str());
					int signum = -1;
					for (int k = 0; k < signals.size(); k++) {
						if (signals[k]->name == signame) {
							signum = k;
							break;
						}
					}
					if (signum != -1) {
						for (int k = 0; k < signals[signum]->subscribers.size(); k++) {
							string resp = "Signal received: ";
							resp += signals[signum]->name;
							resp += "\n";
							signals[signum]->subscribers[k]->messageQueue.push(resp);
						}
					}
				}							
			}
			if (pollfds[i].revents & POLLOUT) {
				if (clients[i]->messageQueue.size() > 0) {
					string message = clients[i]->messageQueue.front();
					clients[i]->messageQueue.pop();
					write(clients[i]->fd, message.c_str(), message.length());
				}
			}				
		}
	}
} 
