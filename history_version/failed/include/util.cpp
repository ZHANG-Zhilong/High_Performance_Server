//
// Created by 张志龙 on 2020/5/15.
//


#include <cerrno>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>

int set_socket_unblocking(int fd) {
    int flag = fcntl(fd, F_GETFL);
    if (flag == -1) {
        return -1;
    }
    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1) {
        return -1;
    }
}

void err_die(const char* sc){
    perror(sc);
    exit(1);
}

/********************
 * @param client    the client sock descriptor
 * @param resource  pointer for the file to cat
 */
void cat(int client, FILE* resource){
    char buf[1024];
    fgets(buf, sizeof(buf), resource);
    while(!feof(resource)){
        send(client, buf, sizeof(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

ssize_t Read(int fd, void *ptr, unsigned long nbytes) {
    ssize_t n;

    again:
    if ((n = read(fd, ptr, nbytes)) == -1) {
        if (errno == EINTR)
            goto again;
        else
            return -1;
    }

    return n;
}

ssize_t Write(int fd, const void *ptr, unsigned long nbytes) {
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

int Close(int fd) {
    int n;
    if ((n = close(fd)) == -1) {
        perror("close error");
        exit(-1);
    }
    return n;
}

ssize_t readn(int fd, void *buf, unsigned long n) {

    ssize_t read_num = 0;                            //int 实际读到的字节数
    unsigned long left_num = n;                  //n 未读取字节数
    unsigned long read_sum = 0;
    char *ptr = (char *) buf;

    while (left_num > 0) {
        read_num = read(fd, ptr, left_num);
        if (read_num < 0) {
            if (errno == EINTR) {
                read_num = 0;
            } else if (errno == EAGAIN) {
                return read_sum;
            } else {
                return -1;
            }
        } else if (read_num == 0) {
            break;
        }

        left_num -= read_num;                    //left_num = left_num - read_num
        read_sum += read_num;
        ptr += read_num;
    }
    return read_sum;
}

ssize_t Writen(int fd, const void *buf, u_long n) {
    u_long left_num = 0;
    ssize_t written_num = 0;
    ssize_t written_sum = 0;
    const char *ptr;

    ptr = (char *) buf;
    left_num = n;
    while (left_num > 0) {
        written_num = write(fd, ptr, left_num);
        if (written_num < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                written_num = 0;
                continue;
            } else {
                return -1;
            }
        }
        left_num -= written_num;
        ptr += written_num;
        written_sum += written_num;
    }
    return written_sum;
}

ssize_t my_read(int fd, char *ptr) {
    static int read_cnt;
    static char *read_ptr;
    static char read_buf[100];

    if (read_cnt <= 0) {
        again:
        if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {   //"hello\n"
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

size_t get_line(int sock, char *buf, size_t size) {
    char c = '\0';
    int n = 0;
    size_t i = 0;
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
            }
            buf[i++] = c;
        }
        c = '\n';
    }
    buf[i] = '\0';
    return i;
}