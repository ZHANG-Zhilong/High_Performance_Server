#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "./wrap.h"

#define SERVER_PORT 8888

using namespace std;

int main(void){
	
#ifdef DEBUG
	cout<<"DEBUG"<<endl;
#endif
	int lfd, cfd;
	struct sockaddr_in serv_addr, clie_addr;

	lfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&serv_addr,sizeof(serv_addr));
	return 0;
}
