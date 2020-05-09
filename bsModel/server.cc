#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
using namespace std;

#define SERV_PORT 9527
#define SERV_IP  "127.0.0.1"

int main(void)
{ 
	int cfd, lfd ;
	char buf[BUFSIZ], clie_IP[BUFSIZ];

	struct sockaddr_in serv_addr, clie_addr;
	socklen_t clie_addr_len= sizeof(clie_addr);

	lfd = socket(AF_INET,SOCK_STREAM, 0);   //check
	if(lfd==-1) cout<<"socket error"<<endl; 

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(SERV_PORT);
	int ret = bind(lfd,(struct sockaddr*)&serv_addr, sizeof(serv_addr));  //check
	if(ret == -1)  cout<<"bind err"<<endl;

	int lis = listen(lfd, 128);
	if(lis==-1) cout<<"listen err"<<endl;

	cout<<"waiting for client..."<<endl;
	cfd = accept(lfd,(struct sockaddr*)&clie_addr, &clie_addr_len);
	if(cfd==-1) cout<<"accept error"<<endl;


	printf("server IP: %s, server port: %d\n",
		inet_ntop(AF_INET, &serv_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP)),
		ntohs(serv_addr.sin_port));

	printf("client IP: %s, client port: %d\n",
		inet_ntop(AF_INET, &clie_addr.sin_addr.s_addr, clie_IP, sizeof(clie_IP)),
		ntohs(clie_addr.sin_port));

	while(1){
		int n = read(cfd, buf, sizeof(buf));
		for(int i = 0; i< n; i++){
			buf[i] = toupper(buf[i]);
		}
		int m = write(cfd, buf, n);
		if(m==-1) cout<<"write err"<<endl;
	}

	close(lfd);
	close(cfd);
}
