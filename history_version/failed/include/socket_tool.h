//
// Created by 张志龙 on 2020/5/12.

#include <sys/socket.h>
#include <unistd.h>

#ifndef HIGH_PERFORMANCE_SERVER_WRAP_H
#define HIGH_PERFORMANCE_SERVER_WRAP_H

void perror_exit(const char *s);

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

int Bind(int fd, const struct sockaddr *sa, socklen_t salen);

int Connect(int fd, const struct sockaddr *sa, socklen_t salen);

int Listen(int fd, int backlog);

int Socket(int family, int type, int protocol);

int startup(u_short *, int max_listen);

void err_die(const char *);

#endif //HIGH_PERFORMANCE_SERVER_WRAP_H
