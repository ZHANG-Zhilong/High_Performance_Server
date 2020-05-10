#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include "wrap.h"

#define SERV_PORT 8899
#define OPEN_MAX 5000
#define MAX_LINE 8192

using namespace std;

int main(void){
	
#ifdef DEBUG
	cout<<"DEBUG"<<endl;
#endif

	int i = 0;
	int listenfd, clientfd;
	char buf[MAX_LINE], clie_IP[INET_ADDRSTRLEN];

	struct sockaddr_in serv_addr, clie_addr;
	int efd = 0;
	struct epoll_event tep, ep[OPEN_MAX];	//tep: param@epoll_ctl, ep[] param@epoll_wait

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	int opt = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //reuse port

	bzero(&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_PORT);
	//inet_pton(AF_INET, "192.168.3.22", &serv_addr.sin_addr.s_addr);

	Bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	Listen(listenfd, OPEN_MAX);
	efd = Epoll_create(OPEN_MAX);  //创建epoll模型，efd为红黑树根节点
	

	tep.events = EPOLLIN;  			//指定lfd的监听事件为读
	tep.data.fd = listenfd;		
	int ret = Epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);//注册lfd及对应结构体
	
	while(1){
		//epoll 为server阻塞监听事件，ep为struct epoll_event类型数组
		//OPEN_MAX为数组容量，-1表永久阻塞
		int nready = epoll_wait(efd, ep, OPEN_MAX, -1);

		for(int i = 0; i< nready; i++){
			
			if(!(ep[i].events & EPOLLIN)){  //如果不是读事件，继续轮询
				continue;
			}

			if(ep[i].data.fd == listenfd){    // listenfd 满足事件
				socklen_t clielen = sizeof(clie_addr);
				clientfd = Accept(listenfd, (struct sockaddr*)&clie_addr, &clielen); 
				cout<<"client IP: "
					<<inet_ntop(AF_INET, &clie_addr.sin_addr, clie_IP, sizeof(clie_IP))
					<<"port: "<<ntohs(clie_addr.sin_port);
				tep.events = EPOLLIN;
				tep.data.fd = clientfd;
				int ret = Epoll_ctl(efd, EPOLL_CTL_ADD, clientfd, &tep);

			}else{							//not listenfd ,客户端有数据流入
				int sockfd = ep[i].data.fd;
				int n = Read(sockfd, buf, sizeof(buf));

				if(n == 0){     				//client closed the connection.
					int ret = Epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					Close(sockfd);
					cout<<"client closed, id: "<<sockfd<<endl;
				}else if(n < 0){    //client error
					cerr<<"read n<0, errno: "<<endl;
					int ret = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					Close(sockfd);
				}else{
					for(int i = 0; i< n; i++){
						buf[i] = toupper(buf[i]);
					}
					Write(sockfd, buf, n);
				}
			}
		}
	}
	return 0;
}










