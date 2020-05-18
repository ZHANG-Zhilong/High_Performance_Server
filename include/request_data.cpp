//
// Created by 张志龙 on 2020/5/17.
//

#include "request_data.h"
#include "util.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>


void accept_request(void *arg) {
    int client = (intptr_t) arg;
    char buf[1024];
    size_t numchars;
    char method[1024];
    char url[1024];
    char path[1024];
    size_t i, j;
    struct stat st{};
    bool cgi = false;  //becomes true is server decides this is a cgi program.
    char *query_string = nullptr;
    numchars = get_line(client, buf, sizeof(buf));
    i = 0;
    while (!ISspace(buf[i]) && i < sizeof(method) - 1) {
        method[i] = buf[i];
        i++;
    }
    j = i;
    method[i] = '\0';
    if (strcasecmp(method, "GET") != 0 || strcasecmp(method, "POST") != 0) {
        unimplemented(client);
        return;
    }
    if (strcasecmp(method, "POST") == 0) {
        cgi = true;
    }

    i = 0;
    while (ISspace(buf[j]) && j < numchars - 1) {
        j++;
    }
    while (!ISspace(buf[j]) && j < numchars - 1 && i < sizeof(url) - 1) {
        url[i++] = buf[j++];
    }
    url[i] = '\0';
    //http://www.xxx.com/Show.asp?id=77&nameid=2905210001&page=1
    //此处问号用于传递参数，将show.asp文件和后面的id、nameid、page等连接起来
    if (strcasecmp(method, "GET") == 0) {
        query_string = url;
        while ((*query_string) != '?' && (*query_string) != '\0') {
            query_string++;
        }
        if (*query_string == '?') {
            cgi = true;
            *query_string = '\0';
            query_string++;
        }
    }
    sprintf(path, "htdocs%s", url);
    if (path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    if (stat(path, &st) == -1) {
        while (numchars > 0 && strcmp("\r\n", buf) != 0) {  /*read and discard request headers*/
            numchars = get_line(client, buf, sizeof(buf));
        }
        not_found(client);
    } else {
        if (S_ISDIR(st.st_mode)) {
            strcat(path, "/index.html");
        }
        if (S_IXOTH & st.st_mode || S_IXUSR & st.st_mode || S_IXGRP & st.st_mode) {
            cgi = true;
        }
        if (!cgi) {
            serv_file(client, path);
        } else {
            execute_cgi(client, path, method, query_string);
        }
        close(client);
    }
}

void bad_request(int client) {
    char buf[1024];
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\r");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type:text/html\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(client, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(client, buf, sizeof(buf), 0);
}

void cannot_execute(int client) {
    char buf[1024];
    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-type:text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<P>Error prohibited CGI execution.\r\n");
    send(client, buf, strlen(buf), 0);
}

void headers(int client, const char *filename) {
    char buf[2014];
    (void) filename;    //could use filename to determine file type.
    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);

}

void execute_cgi(int, const char *, const char *, const char *) {}

void not_found(int) {}

void serv_file(int, const char *) {}

void unimplemented(int) {}
