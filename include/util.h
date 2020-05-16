//
// Created by 张志龙 on 2020/5/15.
//
#include<fcntl.h>
#include "socket_tool.h"

#ifndef HIGH_PERFORMANCE_SERVER_UTIL_H
#define HIGH_PERFORMANCE_SERVER_UTIL_H

ssize_t Readn(int fd, void *buf, size_t n);

ssize_t Writen(int fd, const void *buf, size_t n);

ssize_t Read(int fd, void *ptr, size_t n);

ssize_t Write(int fd, const void *ptr, size_t n);

int Close(int fd);

ssize_t my_read(int fd, char *ptr);

ssize_t Readline(int fd, void *vptr, size_t maxlen);



#endif //HIGH_PERFORMANCE_SERVER_UTIL_H
