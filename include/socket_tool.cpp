//
// Created by 张志龙 on 2020/5/12.
//

#include "socket_tool.h"
#include "util.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
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

/***************
 * this function starts the process of listen for web connections
 * on a specified port. if the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual port.
 * @param port pointer to the variable containing the port to connect
 * @param max_listen variable is the max num to listen
 * @return  the socket
 */
int startup(u_short* port, const int max_listen){
    int httpd;
    int on = 1;
    struct sockaddr_in name{};

    httpd = socket(AF_INET, SOCK_STREAM, 0);
    if(httpd == -1){
        err_die("socket");
    }
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = htons(*port);
    if((setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR,&on, sizeof(on)))<0){
        err_die("setsocketopt failed.");
    }
    if(bind(httpd, (struct sockaddr*)&name, sizeof(name))<0){
        err_die("bind failed");
    }
    if(*port == 0){
        socklen_t namelen = sizeof(name);
        if(getsockname(httpd, (struct sockaddr*)&name, &namelen)==-1){
            err_die("getsockname");
        }
        *port = ntohs(name.sin_port);
    }
    if(listen(httpd, max_listen)<0){
        err_die("listen");
    }
    return httpd;
}

