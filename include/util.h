//
// Created by 张志龙 on 2020/5/15.
//
#include<fcntl.h>
#include<iostream>

#ifndef HIGH_PERFORMANCE_SERVER_UTIL_H
#define HIGH_PERFORMANCE_SERVER_UTIL_H

void err_die(const char *sc);

ssize_t Readn(int fd, void *buf, size_t n);

ssize_t Writen(int fd, const void *buf, size_t n);

ssize_t Read(int fd, void *ptr, size_t n);

ssize_t Write(int fd, const void *ptr, size_t n);

int Close(int fd);

ssize_t my_read(int fd, char *ptr);

size_t get_line(int fd, char *buf, size_t size);

int set_fd_unblocked(int fd);

void cat(int client, FILE* resource);


#endif //HIGH_PERFORMANCE_SERVER_UTIL_H
