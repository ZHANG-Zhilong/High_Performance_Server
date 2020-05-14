//
// Created by 张志龙 on 2020/5/12.

#include <sys/socket.h>
#include <unistd.h>
#ifndef HIGH_PERFORMANCE_SERVER_WRAP_H
#define HIGH_PERFORMANCE_SERVER_WRAP_H

void perr_exit(const char *s);
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);
int Bind(int fd, const struct sockaddr *sa, socklen_t salen);
int Connect(int fd, const struct sockaddr *sa, socklen_t salen);
int Listen(int fd, int backlog);
int Socket(int family, int type, int protocol);
ssize_t Read(int fd, void *ptr, size_t nbytes);
ssize_t Write(int fd, const void *ptr, size_t nbytes);
int Close(int fd);
ssize_t Read_nline(int fd, void *vptr, size_t n);
ssize_t Writen(int fd, const void *vptr, size_t n);
ssize_t my_read(int fd, char *ptr);
ssize_t Readline(int fd, void *vptr, size_t maxlen);
int Epoll_create(int size);
int Epoll_ctl(int epfd, int op, int fd, struct epoll_event* events);
int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);

#endif //HIGH_PERFORMANCE_SERVER_WRAP_H
