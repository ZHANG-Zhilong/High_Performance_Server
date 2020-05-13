#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include "wrap.h"

#define SERV_PORT 8899

using namespace std;

struct s_info {
	sockaddr_in clieaddr;
	int connfd;
};

void* do_work(void* arg){
	cout<<pthread_self()<<endl;
	struct s_info* ts = (struct s_info*)arg;
	char buf[128];
	char clie_IP [INET_ADDRSTRLEN];  //#define IENT_ADDRSTRLEN 16   "[ + d"
	int n = 0;

	while(1){
		int n = Read(ts->connfd, buf, 128);
		if(n == 0){
			cout<<"the client is closed, id: "<<ts->connfd;
			break;
		}
		
		cout<<"client IP: "
			<<inet_ntop(AF_INET, &ts->clieaddr.sin_addr.s_addr, clie_IP, sizeof(clie_IP))
			<<"port: "
			<<ntohs(ts->clieaddr.sin_port);

		for(int i = 0; i< n;i++){
			buf[i] = toupper(buf[i]);
		}
		Write(ts->connfd, buf, sizeof(buf));
	}

	Close(ts->connfd);
	return NULL;
}


int main(void){
	
#ifdef DEBUG
	cout<<"DEBUG"<<endl;
#endif

	int i = 0;
	int lfd, cfd;
	struct sockaddr_in serv_addr, clie_addr;
	pthread_t tid;
	struct s_info ts[256];
	char buf[128], clie_IP[128];

	lfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, "192.168.3.22", &serv_addr.sin_addr.s_addr);

	Bind(lfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	Listen(lfd, 128);

	cout<<"accepting client ..."<<endl;
	
	while(1){     //创建子进程，揽活
		socklen_t clie_addr_len = sizeof(clie_addr);
		cfd = Accept(lfd, (struct sockaddr*) &clie_addr, &clie_addr_len);
		ts[i].clieaddr = clie_addr;
		ts[i].connfd = cfd;

		//达到线程最大数量时，pthread_create出错处理，增加服务器稳定性
		pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
		pthread_detach(tid);    //子线程分离，避免🧟‍♂️僵尸线程
		i++;
	}
	return 0;
}










