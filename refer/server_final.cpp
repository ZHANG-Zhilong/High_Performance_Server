#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <sys/epoll.h>
#include <fcntl.h>
#include <chrono>
#include "./include/wrap.h"
#include "./include/epoll_tool.h"

#define SERVER_PORT 8899
#define OPEN_MAX 5000
#define MAX_LINE 4

using namespace std;

int g_epfd;                                  //save retval of epoll_create
uint16_t g_user_port;
struct my_event_t g_events[OPEN_MAX + 1];     // +1 -->listen fd
void accept_connection(int lfd, int events, void *arg);
void receive_data(int fd, int events, void *my_event_s_arg);
void send_data(int fd, int events, void *my_event_t_arg);
int init_listen_socket(int epfd, u_short port);

void process_command_line(int argc, char* argv[]){
    for (int i = 0; i < argc; i++) {
        if ((string) argv[i] == "-p") {    //用户指定端口
            g_user_port = (uint16_t) stoi(argv[i + 1], nullptr, 10);
            if (g_user_port < 2000 || g_user_port > 60000) {
                g_user_port = SERVER_PORT;
            }
        }
    }

}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main(int argc, char *argv[]) {

#ifdef DEBUG
    cout<<"DEBUG"<<endl;
#endif

    process_command_line(argc, argv);    //处理命令行参数

    g_epfd = Epoll_create(OPEN_MAX + 1);
    struct epoll_event ready_events[OPEN_MAX + 1];  //监听事件节点数组
    init_listen_socket(g_epfd, SERVER_PORT);
    cout << "Server port is: " << SERVER_PORT<<endl;

    size_t check_pos = 0;
    while(1){

        //超时验证，每次测试100个连接，不测试listen_fd;
        //当客户端60秒内没有和服务器通信，则关闭此客户端连接
        long now_time = time(nullptr);
        for(size_t i = 0; i< 100; i++, check_pos ++){
            if(check_pos == OPEN_MAX){
                check_pos = 0;
            }
            if(g_events[check_pos].status == NOT_IN_EPOLL_TREE){
                continue;
            }
            long duration = now_time - g_events[check_pos].last_active;  //客户端不活跃的时间
            if(duration >=60){
                Close(g_events[check_pos].fd);
                printf("[fd = %d] timeout. \n", g_events[check_pos].fd);
                event_remove(g_epfd, &g_events[check_pos]);
            }
        }

        //监听g_epfd_tree，将满足事件的文件描述符添加到g_events数组中，若一秒钟都没有时间满足，返回0
        int ready_fds = Epoll_wait(g_epfd, ready_events, OPEN_MAX+1, 3000);

        for(int i = 0; i< ready_fds; i++){
            //使用自定义结构体指针my_event_t，接受联合体data中void* ptr成员
            auto my_event = (struct my_event_t*)ready_events[i].data.ptr;
            if((ready_events[i].events & EPOLLIN) && (my_event->events & EPOLLIN)){
                my_event->call_back(my_event->fd, ready_events[i].events, my_event->arg);
            }
            if((ready_events[i].events & EPOLLOUT) && (my_event->events & EPOLLOUT)){
                my_event->call_back(my_event->fd, ready_events[i].events, my_event->arg);
            }
        }
    }
    //退出前释放所有资源
    return 0;
}

#pragma clang diagnostic pop

void accept_connection(int lfd, int events, void *arg) {
    struct sockaddr_in client_sock{};
    bzero(&client_sock, sizeof(client_sock));
    socklen_t len = sizeof(client_sock);
    int cfd, i;
    cfd = accept(lfd, (struct sockaddr *) &client_sock, &len);
    if (cfd == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            //暂时不做出错处理  ❌❌❌❌❌❌
        }
        printf("%s: accept, %s\n", __func__, strerror(errno));
        return;
    }
    do {
        for (i = 0; i < OPEN_MAX; i++) {
            //从全局数组g_events中找一个空闲元素，类似select中找值为-1的元素
            if (g_events[i].status == NOT_IN_EPOLL_TREE) {
                break;
            }
        }
        if (i == OPEN_MAX) {
            printf("%s: max connection limit [%d]\n", __func__, OPEN_MAX);
            break;
        }
        int flag = 0;
        flag = fcntl(cfd, F_GETFL);
        flag |= O_NONBLOCK;
        if (fcntl(cfd, F_SETFL, flag) < 0) {
            printf("%s: fcntl set non_block failed, %s\n", __func__, strerror(errno));
        }
        //给cfd配置一个my_event_s结构体，回调函数设置为recvdada

        event_set(&g_events[i], cfd, receive_data, &g_events[i]);
        event_add(g_epfd, EPOLLIN, &g_events[i]);

    } while (0);
    printf("new connect [%s:%d] [time:%ld], pos[%d]\n",
           inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port),
           g_events[i].last_active, i);
}

int init_listen_socket(int epfd, u_short port) {

    int client_fd = Socket(AF_INET, SOCK_STREAM, 0);
    int flag = fcntl(client_fd, F_GETFL);    //set sock as unblock
    flag |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, flag);

    event_set(&g_events[OPEN_MAX], client_fd, accept_connection, &g_events[OPEN_MAX]);
    event_add(epfd, EPOLLIN, &g_events[OPEN_MAX]);

    struct sockaddr_in client_sock = {};
    memset(&client_sock, 0, sizeof(client_sock));
    client_sock.sin_family = AF_INET;
    client_sock.sin_addr.s_addr = INADDR_ANY;
    client_sock.sin_port = htons(SERVER_PORT);

    Bind(client_fd, (struct sockaddr *) &client_sock, sizeof(client_sock));
    Listen(client_fd, OPEN_MAX);
}

void receive_data(int fd, int events, void *my_event_s_arg) {

    auto *my_event = (struct my_event_t *) my_event_s_arg;
    int len = recv(fd, my_event->buf, my_event->buf_len, 0);    //读fd，存入数据
    //write(STDOUT_FILENO, my_event->buf, len);  //❌❤️
    event_remove(g_epfd, my_event);                              //摘下节点
    if (len > 0) {

        my_event->buf_len = len;
        my_event->buf[len] = '\0';                      //添加字符串结束标记  REFLECT
        printf("C[%d]: %s\n", fd, my_event->buf);

        event_set(my_event, fd, send_data, my_event);  //设置fd对应的回调函数为send_data
        event_add(g_epfd, EPOLLOUT, my_event);       //将fd加入epoll_tree上监听可写事件

    } else if (len == 0) {
        //Close(my_event->fd);
        //my_event - g_events 地址做差得到偏移元素的位置
        printf("[fd=%d] pos[%ld], closed\n", fd, my_event - g_events);
    } else {
        //Close(my_event->fd);
        printf("recv[fd = %d] error [%d]: %s\n", fd, errno, strerror(errno));
    }
}

void send_data(int fd, int events, void *my_event_t_arg) {
    auto *my_event = (struct my_event_t *) my_event_t_arg;
    int len = send(fd, my_event->buf, my_event->buf_len, 0);   //reflect
    //write(STDOUT_FILENO, my_event->buf, len);  //❌❤️
    if (len > 0) {
        printf("send[fd=%d], [%d]%s\n", fd, len, my_event->buf);
        event_remove(g_epfd, my_event);                    //摘下节点
        event_set(my_event, fd, receive_data, my_event);  //更改fd的回调函数为receive_data
        event_add(g_epfd, EPOLLIN, my_event);     //重新添加到epoll_tree上，设置监听事件为可读
    } else {
        //close(my_event->fd);                            //关闭连接
        event_remove(g_epfd, my_event);
        printf("send [fd=%d] error %s\n", fd, strerror(errno));
    }
}







