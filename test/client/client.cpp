//
// Created by 张志龙 on 2020/5/16.
//

#include <cstdio>
#include <cstdlib>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main(){
    int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.3.25");
    serv_addr.sin_port = htons(8899);
    int len = sizeof(serv_addr);
    int ret = connect(serv_fd, (struct sockaddr*)&serv_addr, len);
    if(ret == -1){
        perror("oops client.\r\n");
    }
    std::string str = "fuck";
    const char* buf = str.c_str();
    char* buf2[BUFSIZ];
    write(serv_fd, buf, sizeof(buf));
    read(serv_fd, buf2, sizeof(buf2));
    printf("received from src: %s", *buf2);
    close(serv_fd);
    exit(0);
}