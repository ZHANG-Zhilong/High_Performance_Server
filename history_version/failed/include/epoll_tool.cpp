//
// Created by 张志龙 on 2020/5/13.
//

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <sys/epoll.h>
#include <cstring>
#include <ctime>
#include "socket_tool.h"
#include "epoll_tool.h"


//initialize struct my_event_t object.
void
event_set(
        struct my_event_t *ev,
        int fd,
        void (*call_back)(int, int, void *),
        void *arg) {

    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->status = NOT_IN_EPOLL_TREE;
    ev->self = arg;
    //memset(ev->buf, 0, sizeof(ev->buf));
    ev->last_active = time(nullptr);    //绝对时间,统计连接时长
}

//向epoll监听的红黑树中添加一个文件描述符
void event_add(int epfd, int events, struct my_event_t *my_event) {
    //events:  被添加描述符监听事件  一般为EPOLLIN
    struct epoll_event epollEvent{};
    int option = 0;
    epollEvent.data.ptr = my_event;
    epollEvent.events = events;
    my_event->events = events;
    my_event->last_active = time(nullptr);

    if (my_event->status == IN_EPOLL_TREE) {    //已经在epoll树中
        option = EPOLL_CTL_MOD;
    } else {                                    //not in epoll tree
        option = EPOLL_CTL_ADD;
        my_event->status = IN_EPOLL_TREE;
    }

    int ret = epoll_ctl(epfd, option, my_event->fd, &epollEvent);  //

    if (ret < 0) {
        printf("event add failed [fd=%d], events[%d]\n", my_event->fd, events);
    } else {
        printf("event add OK [fd=%d], op = %d, events[%0x]\n", my_event->fd, option, events);

    }
}

void event_remove(int epfd, struct my_event_t *my_event) {
    if (my_event->status == NOT_IN_EPOLL_TREE) {
        return;
    }
    struct epoll_event epollEvent{};
    epollEvent.data.ptr = my_event;
    my_event->status = NOT_IN_EPOLL_TREE;
    Epoll_ctl(epfd, EPOLL_CTL_DEL, my_event->fd, &epollEvent);   //摘下ev->fd
    printf("event removed [fd=%d], events[%0x]\n", my_event->fd, my_event->events);
}

int Epoll_create(int size) {
    int epfd = epoll_create(size);
    if (epfd <= 0) {
        printf("create efd in %s err %s\n", __func__, strerror(errno));
    }
    return epfd;
}

int Epoll_ctl(int epfd, int opt, int fd, struct epoll_event *events) {
    int ret = epoll_ctl(epfd, opt, fd, events);//注册lfd及对应结构体
    if (ret == -1) {
        std::cerr << "epoll_ctl error" << std::endl;
        exit(1);
    }
    return ret;
}

int Epoll_wait(int epfd, struct epoll_event *events, int max_events, int timeout) {
    int ready_fds = epoll_wait(epfd, events, max_events, timeout);
    if (ready_fds == -1) {
        printf("epoll_wait error in %s, errno: %s", __func__, strerror(errno));
        exit(1);
    }
    return ready_fds;
}


//struct epoll_event *events;

int epoll_init() {
    int epoll_fd = epoll_create(LISTENQ + 1);
    if (epoll_fd == -1)
        return -1;
    //events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * MAXEVENTS);
    events = new epoll_event[MAXEVENTS];
    return epoll_fd;
}

// 注册新描述符
int epoll_add(int epoll_fd, int fd, void *request, __uint32_t events) {
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    //printf("add to epoll %d\n", fd);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
        perror("epoll_add error");
        return -1;
    }
    return 0;
}

// 修改描述符状态
int epoll_mod(int epoll_fd, int fd, void *request, __uint32_t events) {
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event) < 0) {
        perror("epoll_mod error");
        return -1;
    }
    return 0;
}

// 从epoll中删除描述符
int epoll_del(int epoll_fd, int fd, void *request, __uint32_t events) {
    struct epoll_event event;
    event.data.ptr = request;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event) < 0) {
        perror("epoll_del error");
        return -1;
    }
    return 0;
}

// 返回活跃事件数
int my_epoll_wait(int epoll_fd, struct epoll_event *events, int max_events, int timeout) {
    int ret_count = epoll_wait(epoll_fd, events, max_events, timeout);
    if (ret_count < 0) {
        perror("epoll wait error");
    }
    return ret_count;
}
