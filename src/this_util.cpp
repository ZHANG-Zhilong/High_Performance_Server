#include "this_util.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

int readn(int fd, void *buff, size_t n) {
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t char_nums = 0;
    char *ptr = (char *) buff;
    while (nleft > 0) {
        nread = read(fd, ptr, nleft);
        if (nread < 0) {
            if (errno == EINTR) {
                nread = 0;
            } else if (errno == EAGAIN) {
                return char_nums;
            } else {
                return -1;
            }
        } else if (nread == 0) {
            break;
        }
        char_nums += nread;
        nleft -= nread;
        ptr += nread;
    }
    return char_nums;
}

int writen(int fd, void *buff, size_t n) {
    size_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char *ptr = (char *) buff;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0) {
                if (errno == EINTR || errno == EAGAIN) {
                    nwritten = 0;
                    continue;
                } else
                    return -1;
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return writeSum;
}

void handle_for_sigpipe() {
    struct sigaction sa{};
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, nullptr))
        return;
}

int setSocketNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

int get_line(int sock, char *buf, int size) {
    int i = 0;
    char c = '\0';
    int n = 0;
    //读不到下一个字节，添加LF
    //读到CR且下一个是LF
    //读到CR但下一个不是LF
    while (i < size - 1 && c != '\n') {
        n = recv(sock, &c, 1, 0);
        if (n > 0) {
            if (c == '\r') {
                n = recv(sock, &c, 1, MSG_PEEK);
                if (n > 0 && c == '\n') {
                    recv(sock, &c, 1, 0);
                } else {
                    c = '\n';
                }
                buf[i++] = c;
            }
        } else {
            c = '\n';
        }
    }
    buf[i] = '\0';
    return 0;
}

