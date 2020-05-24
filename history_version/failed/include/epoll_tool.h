//
// Created by 张志龙 on 2020/5/13.
//

#ifndef HIGH_PERFORMANCE_SERVER_EPOLL_TOOL_H
#define HIGH_PERFORMANCE_SERVER_EPOLL_TOOL_H

#include <sys/socket.h>
#include <unistd.h>

const int MAXEVENTS = 5000;
const int LISTENQ = 1024;
#define IN_EPOLL_TREE 1
#define NOT_IN_EPOLL_TREE 0

struct my_event_t;

//描述就绪文件描述符信息
struct my_event_t {
    int fd;                 //要监听的文件描述符
    int events;             //对应的监听事件
    int status;             //1->在红黑树上（监听）    0->不在树上（不监听）
    char buf[BUFSIZ];
    int buf_len;
    long last_active;       //记录每次加入树上的g_efg时间值
    void (*call_back)(int fd, int events, void *arg);

    void *self;              //泛型参数
};

void event_set(struct my_event_t *ev, int fd, void (*call_back)(int, int, void *), void *arg);

void event_add(int epfd, int events, struct my_event_t *my_event);

void event_remove(int epfd, struct my_event_t *my_event);

int Epoll_create(int size);

int Epoll_ctl(int epfd, int op, int fd, struct epoll_event *events);

int Epoll_wait(int epfd, struct epoll_event *events, int max_events, int timeout);


int epoll_init();

int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events);

int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events);

int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events);

int my_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout);


#endif //HIGH_PERFORMANCE_SERVER_EPOLL_TOOL_H
