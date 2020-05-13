#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <cctype>
#include <sys/epoll.h>
#include <fcntl.h>
#include "wrap.h"

#define SERVER_PORT 8899
#define OPEN_MAX 5000
#define MAX_LINE 4

using namespace std;

//描述就绪文件描述符信息
struct my_event_s{
    int fd;                 //要监听的文件描述符
    int events;             //对应的监听事件
    void* arg;              //泛型参数
    void (*call_back)(int fd, int events, void* arg);
    int status;             //1->在红黑树上（监听）    0->不在树上（不监听）
    char buf[BUFSIZ];
    int len;
    long last_active;       //记录每次加入树上的g_efg时间值
};

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main(void){

#ifdef DEBUG
    cout<<"DEBUG"<<endl;
#endif


    struct sockaddr_in server_sock_addr, client_sock_addr;
    socklen_t client_addr_len;
    int server_listen_fd, connect_fd;
    char buf[MAX_LINE], client_ip[INET_ADDRSTRLEN];
    int ep_fd = 0, flag = 0;

    server_listen_fd = Socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //reuse port

    bzero(&server_sock_addr, sizeof(server_sock_addr));
    server_sock_addr.sin_family = AF_INET;
    server_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sock_addr.sin_port = htons(SERVER_PORT);
    //inet_pton(AF_INET, "192.168.3.22", &server_sock_addr.sin_addr.s_addr);

    Bind(server_listen_fd, (struct sockaddr*) &server_sock_addr, sizeof(server_sock_addr));
    Listen(server_listen_fd, OPEN_MAX);

    struct epoll_event epollEvent;
    struct epoll_event vecEpollEvent[OPEN_MAX];	//epollEvent: param@epoll_ctl, vecEpollEvent[] param@epoll_wait

    ep_fd = Epoll_create(OPEN_MAX);  //创建epoll模型，efd为红黑树根节点
    epollEvent.events = EPOLLIN|EPOLLET; //ET边沿触发，默认是水平触发，监听可读
    //epollEvent.events = EPOLLIN; //ET边沿触发，默认是水平触发，监听可读

    cout<<"Accepting connections..."<<endl;
    client_addr_len = sizeof(client_sock_addr);
    connect_fd = Accept(server_listen_fd, (struct sockaddr*)& client_sock_addr, &client_addr_len);
    cout<<"received ip: "
        <<inet_ntop(AF_INET, &client_sock_addr.sin_addr, client_ip, sizeof(client_ip))
        <<" port: "
        <<ntohs(client_sock_addr.sin_port);

    flag = fcntl(connect_fd, F_GETFL);   //修改为非阻塞读
    flag |= O_NONBLOCK;
    fcntl(connect_fd, F_SETFL, flag);

    epollEvent.data.fd = connect_fd;
    Epoll_ctl(ep_fd, EPOLL_CTL_ADD, connect_fd, &epollEvent);  //将connect_fd加入监听红黑树

    while(1){
        cout<<"epoll_wait begin\n";
        int ret = epoll_wait(ep_fd, vecEpollEvent, OPEN_MAX, -1);  //最多10个，阻塞监听
        cout<<"epoll_wait end ret: "<<ret<<endl;

        if(vecEpollEvent[0].data.fd == connect_fd){
            int len = 0;
            while((len = Read(connect_fd, buf, MAX_LINE))>0){  //非阻塞读，轮询
                Write(STDOUT_FILENO, buf, len);
            }
        }
    }



    return 0;
}
#pragma clang diagnostic pop










