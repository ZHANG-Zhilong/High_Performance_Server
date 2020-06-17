#ifndef UTIL
#define UTIL
#include <cstdlib>
#include <sys/socket.h>

int readn(int fd, void *buff, size_t n);
int get_line(int sock, char* buf, int size);
int writen(int fd, void *buff, size_t n);
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);


#endif