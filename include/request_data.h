//
// Created by 张志龙 on 2020/5/17.
//

#ifndef HIGH_PERFORMANCE_SERVER_REQUEST_DATA_H
#define HIGH_PERFORMANCE_SERVER_REQUEST_DATA_H
#include <iostream>
#define ISspace(x) isspace((int) x)
#define SERVER_STRING "Server name"
#define STDIN 0
#define STDOUT 1
#define STDERR 2

void accept_request(void*);
void bad_request(int);
void cannot_execute(int);
void execute_cgi(int, const char*, const char*, const char*);
void headers(int, const char*);
void not_found(int);
void serv_file(int, const char*);
void unimplemented(int);


#endif //HIGH_PERFORMANCE_SERVER_REQUEST_DATA_H
