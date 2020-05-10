#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include "wrap.h"

#define SERV_PORT 8899

using namespace std;

void wait_child(int signo){
	while(waitpid(0, NULL, WNOHANG)){}
	return;
}

int main(void){
	
#ifdef DEBUG
	cout<<"DEBUG"<<endl;
#endif
	pid_t pid ;
	int lfd, cfd;
	struct sockaddr_in serv_addr, clie_addr;
	socklen_t clie_addr_len;
	char buf[128], clie_IP[128];

	lfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, "192.168.3.22", &serv_addr.sin_addr.s_addr);
	Bind(lfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	Listen(lfd, 128);
	
	while(1){     //创建子进程，揽活
		clie_addr_len = sizeof(clie_addr);
		cfd = Accept(lfd, (struct sockaddr*) &clie_addr, &clie_addr_len);
		cout<<"i am here"<<endl;
		cout<<"client IP: "
			<<inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP))
			<<"port: "
			<<ntohs(clie_addr.sin_port);
		
		pid = fork();
		if(pid<0){
			cerr<<"fork err"<<endl;
			exit(1);
		}else if(pid == 0){   // son progress
			close(lfd);
			break;
		}else{   			 //father progress
		    close (cfd);
			signal(SIGCHLD, wait_child);
		}
	}

	if(pid == 0){  			//son progress
		int n = 0;
		while(1){
			n = Read(cfd, buf, sizeof(buf));
			if(n == 0){
				close(cfd);  //client closed.
				return 0;
			}else if(n == -1){
				cerr<<"read err"<<endl;
				exit(1);
			}else{
				for(int i = 0; i< n; i++){
					buf[i] = toupper(buf[i]);
				}
				write(cfd, buf, n);
			}
		}
	}

	
	return 0;
}










