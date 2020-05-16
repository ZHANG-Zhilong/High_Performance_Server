//
// Created by 张志龙 on 2020/5/12.
//

#include "socket_tool.h"
#include "util.h"
#include <iostream>
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
using namespace std;

void perror_exit(const char *s)
{
    cerr<<s<<endl;
    exit(-1);
}

int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;

    again:
    if ((n = accept(fd, sa, salenptr)) < 0) {
        if ((errno == ECONNABORTED) || (errno == EINTR))
            goto again;
        else
            perror_exit("accept error");
    }
    return n;
}

int Bind(int fd, const struct sockaddr *sa, socklen_t socklen)
{
    int n;
    if ((n = bind(fd, sa, socklen)) < 0){
        perror_exit("bind error");
        printf("bind error  %s",  strerror(errno));
    }
    return n;
}

int Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    int n;
    n = connect(fd, sa, salen);
    if (n < 0) {
        perror_exit("connect error");
    }

    return n;
}

int Listen(int fd, int backlog)
{
    int n;

    if ((n = listen(fd, backlog)) < 0)
        perror_exit("listen error");

    return n;
}

int Socket(int family, int type, int protocol)
{
    int n;

    if ((n = socket(family, type, protocol)) < 0)
        perror_exit("socket error");

    return n;
}

/*参三: 应该读取的字节数*/                          //socket 4096  readn(cfd, buf, 4096)   nleft = 4096-1500

/*readline --- fgets*/


