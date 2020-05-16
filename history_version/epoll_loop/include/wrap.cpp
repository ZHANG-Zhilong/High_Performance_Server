//
// Created by 张志龙 on 2020/5/12.
//

#include "wrap.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
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

ssize_t Read(int fd, void *ptr, size_t nbytes)
{
    ssize_t n;

    again:
    if ( (n = read(fd, ptr, nbytes)) == -1) {
        if (errno == EINTR)
            goto again;
        else
            return -1;
    }

    return n;
}

ssize_t Write(int fd, const void *ptr, size_t nbytes)
{
    ssize_t n;

    again:
    if ((n = write(fd, ptr, nbytes)) == -1) {
        if (errno == EINTR)
            goto again;
        else
            return -1;
    }
    return n;
}

int Close(int fd)
{
    int n;
    if ((n = close(fd)) == -1)
        perror_exit("close error");

    return n;
}

/*参三: 应该读取的字节数*/                          //socket 4096  readn(cfd, buf, 4096)   nleft = 4096-1500
ssize_t Read_nline(int fd, void *vptr, size_t n)
{
    size_t  nleft;              //usigned int 剩余未读取的字节数
    ssize_t nread;              //int 实际读到的字节数
    char   *ptr;

    ptr = (char*)vptr;
    nleft = n;                  //n 未读取字节数

    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if (nread == 0)
            break;

        nleft -= nread;   //nleft = nleft - nread
        ptr += nread;
    }
    return n - nleft;
}

ssize_t Writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = (char*)vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

ssize_t my_read(int fd, char *ptr)
{
    static int read_cnt;
    static char *read_ptr;
    static char read_buf[100];

    if (read_cnt <= 0) {
        again:
        if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {   //"hello\n"
            if (errno == EINTR)
                goto again;
            return -1;
        } else if (read_cnt == 0)
            return 0;

        read_ptr = read_buf;
    }
    read_cnt--;
    *ptr = *read_ptr++;

    return 1;
}

/*readline --- fgets*/
//传出参数 vptr
ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t n, rc;
    char    c, *ptr;
    ptr = (char*)vptr;

    for (n = 1; n < maxlen; n++) {
        if ((rc = my_read(fd, &c)) == 1) {   //ptr[] = hello\n
            *ptr++ = c;
            if (c == '\n')
                break;
        } else if (rc == 0) {
            *ptr = 0;
            return n-1;
        } else
            return -1;
    }
    *ptr = 0;

    return n;
}

int Epoll_create(int size){
    int epfd = epoll_create(size);
    if(epfd <=0){
        printf("create efd in %s err %s\n", __func__, strerror(errno));
    }
    return epfd;
}

int Epoll_ctl(int epfd, int opt, int fd, struct epoll_event* events){
    int ret = epoll_ctl(epfd, opt, fd, events);//注册lfd及对应结构体
    if(ret ==-1){
        cerr<<"epoll_ctl error"<<endl;
        exit(1);
    }
    return ret;
}

int Epoll_wait(int epfd, struct epoll_event* events, int max_events, int timeout){
    int ready_fds = epoll_wait(epfd, events, max_events, timeout);
    if(ready_fds == -1){
        printf("epoll_wait error in %s, errno: %s", __func__, strerror(errno));
        exit(1);
    }
    return ready_fds;
}


