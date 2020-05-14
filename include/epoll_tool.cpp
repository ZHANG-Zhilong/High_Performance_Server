//
// Created by 张志龙 on 2020/5/13.
//

#include <iostream>
#include <cstdio>
#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include "wrap.h"
#include "epoll_tool.h"


//initialize struct my_event_t object.
void event_set(struct my_event_t *ev, int fd, void (*call_back)(int, int, void*), void*arg){
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->status = NOT_IN_EPOLL_TREE;
    ev->arg = arg;
    memset(ev->buf, 0, sizeof(ev->buf));
    ev->buf_len = BUFSIZ;
    ev->last_active = time(nullptr);    //绝对时间,统计连接时长
}

//向epoll监听的红黑树中添加一个文件描述符
void event_add(int epfd, int events, struct my_event_t* my_event){
    //events:  被添加描述符监听事件  一般为EPOLLIN
    struct epoll_event epollEvent = {0, nullptr};
    int option = 0;
    epollEvent.data.ptr = my_event;
    epollEvent.events = events;
    my_event->events = events;
    my_event->last_active = time(nullptr);

    if(my_event->status == IN_EPOLL_TREE){    //已经在epoll树中
        option = EPOLL_CTL_MOD;
    }else{                                    //not in epoll tree
        option = EPOLL_CTL_ADD;
        my_event->status = IN_EPOLL_TREE;
    }

    int ret = epoll_ctl(epfd, option, my_event->fd, &epollEvent);  //

    if(ret<0){
        printf("event add failed [fd=%d], events[%d]\n", my_event->fd, events);
    }else{
        printf("event add OK [fd=%d], op = %d, events[%0x]\n", my_event->fd, option, events);

    }
}

void event_remove(int epfd, struct my_event_t* my_event){
    if(my_event->status == NOT_IN_EPOLL_TREE){
        return;
    }
    struct epoll_event epollEvent = {0, nullptr};
    epollEvent.data.ptr = my_event;
    my_event->status = NOT_IN_EPOLL_TREE;
    Epoll_ctl(epfd, EPOLL_CTL_DEL, my_event->fd, &epollEvent);   //摘下ev->fd
}

