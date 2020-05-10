#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

#define SERV_IP "127.0.0.1"
#define SERV_PORT 9527

using namespace std;

int main(int argc, char* argv[]){
	
	int cfd;
	struct sockaddr_in serv_addr;
	socklen_t serv_addr_len;
	char buf[BUFSIZ];
	
	cfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,SERV_IP, &serv_addr.sin_addr.s_addr);
	connect(cfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr));

	while(1){
		fgets(buf, sizeof(buf), stdin);
		write(cfd, buf, strlen(buf));
		int n = read(cfd, buf, sizeof(buf));
		write(STDOUT_FILENO, buf, n);
	}
	close (cfd);


	
	return 0;
}