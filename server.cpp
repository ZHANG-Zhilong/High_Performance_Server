#include <iostream>
#include <cstring>
#include <cctype>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "./include/wrap.h"

#define SERV_PORT 8899
#define OPEN_MAX 5000
#define MAX_LINE 8192
#define IN_EPOLL_TREE 1
#define NOT_IN_EPOLL_TREE 0

using namespace std;

//描述就绪文件描述符信息
struct event_t {
    void *self;              //泛型参数
    int fd;                 //要监听的文件描述符
    int events;             //对应监听事件
    void (*call_back)(void* self, int fd, int events);

    int status;             //1->在红黑树上（监听）    0->不在树上（不监听）
    char buf[BUFSIZ];
    int buf_len;
    long last_active;       //记录每次加入树上的g_efg时间值
};

void event_set(struct event_t *event, void *self, int fd, void(*call_back)(void *, int, int)) {
    event->fd = fd;
    event->events = 0;
    event->call_back = call_back;
    event->status = NOT_IN_EPOLL_TREE;
    memset(event->buf, 0, sizeof(event->buf));
    event->buf_len = BUFSIZ;
    event->last_active = time(nullptr);
    event->self = self;
}

void event_add(int epfd, int events, void *event_ptr) {
    auto event_data = (struct event_t *) event_ptr;
    event_data->last_active = time(nullptr);

    epoll_event listen_event{};
    listen_event.events = events;
    listen_event.data.ptr = event_ptr;

    int op = 0;
    if (event_data->status == IN_EPOLL_TREE) {
        op = EPOLL_CTL_MOD;
    } else {
        op = EPOLL_CTL_ADD;
        event_data->status = NOT_IN_EPOLL_TREE;
    }

    int ret = epoll_ctl(epfd, op, event_data->fd, &listen_event);
    if (ret > 0) {
        printf("event add failed, fd:%d, event:%d", event_data->fd, events);
    } else {
        printf("event added , op:%d, fd:%d, event:%d", op, event_data->fd, events);
    }
}

void event_remove(int epfd, int events, void *event_ptr) {
    auto event_data = (struct event_t *) event_ptr;
    if (event_data->status == NOT_IN_EPOLL_TREE) {
        return;
    }
    epoll_event listen_event{};
    listen_event.events = EPOLLOUT;
    listen_event.data.ptr = event_data;
    int op = EPOLL_CTL_DEL;
    Epoll_ctl(epfd, op, event_data->fd, &listen_event);
    event_data = NOT_IN_EPOLL_TREE;
}

int init_listen_socket(sockaddr_in &serv_addr, u_short user_serv_port);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main(int argc, char *argv[]) {

    int lfd, cfd;   //listen_fd, client_fd
    char buf[MAX_LINE], client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in server_sockaddr = {}, client_sockaddr = {};

    lfd = init_listen_socket(server_sockaddr, SERV_PORT);

    int epfd = Epoll_create(OPEN_MAX);  //创建epoll模型，efd为红黑树根节点
    struct epoll_event lfd_event, ep[OPEN_MAX];

    Epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &lfd_event);//注册lfd及对应结构体


    while (1) {
        //epoll 为server阻塞监听事件，ep为struct epoll_event类型数组
        //OPEN_MAX为数组容量，-1表永久阻塞, >=0表示等待时间（ms）
        int ready_fds = epoll_wait(epfd, ep, OPEN_MAX, -1);

        for (int i = 0; i < ready_fds; i++) {

            if (!(ep[i].events & EPOLLIN)) {  //如果不是读事件，继续轮询
                continue;
            }

            if (ep[i].data.fd == lfd) {    // lfd 满足事件

                socklen_t client_sockaddr_len = sizeof(client_sockaddr);
                cfd = Accept(lfd, (struct sockaddr *) &client_sockaddr, &client_sockaddr_len);
                lfd_event.events = EPOLLIN;  //默认水平触发，而非边沿触发
                lfd_event.data.fd = cfd;
                Epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &lfd_event);

                printf("new connection! %d[%s:%d]\n", cfd,
                       inet_ntop(AF_INET, &client_sockaddr.sin_addr, client_ip, sizeof(client_ip)),
                       ntohs(client_sockaddr.sin_port));
            } else {                            //not lfd ,客户端有数据流入

                int touched_fds = ep[i].data.fd;
                int n = Read(touched_fds, buf, sizeof(buf));
                //int n = recv(touched_fds, buf, sizeof(buf), 0);
                if (n == 0) {                    //client closed the connection.
                    int ret = Epoll_ctl(epfd, EPOLL_CTL_DEL, touched_fds, nullptr);
                    Close(touched_fds);
                    cout << "client closed, id: " << touched_fds << endl;
                } else if (n < 0) {    //client error
                    printf("read error, %s", strerror(errno));
                    Epoll_ctl(epfd, EPOLL_CTL_DEL, touched_fds, nullptr);
                    Close(touched_fds);
                } else {
                    for (int i = 0; i < n; i++) {
                        buf[i] = toupper(buf[i]);
                    }
                    //Write(touched_fds, buf, n);
                    int ret = send(touched_fds, buf, n, 0);
                    if (ret <= 0) {
                        cerr << "send err" << endl;
                    }
                }
            }
        }
    }
    return 0;
}

#pragma clang diagnostic pop


int init_listen_socket(sockaddr_in &serv_addr, u_short user_serv_port) {

    int lfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  //reuse user_serv_port

    int flag = fcntl(lfd, F_GETFL);
    flag|= O_NONBLOCK;
    fcntl(lfd, F_SETFL, flag);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(user_serv_port);

    Bind(lfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    Listen(lfd, OPEN_MAX);

    return lfd;
}
