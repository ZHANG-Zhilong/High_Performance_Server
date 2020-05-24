#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <queue>
#include <sys/socket.h>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "request_data.h"
#include "socket_tool.h"
#include "thread_pool.h"
#include "util.h"
#include "epoll_tool.h"

using namespace std;

#define THREADPOOL_THREAD_NUM 4
#define QUEUE_SIZE 65535
#define PORT 8899
#define ASK_STATIC_FILE 1
#define ASK_IMAGE_STITCH 2
#define PATH "/"
#define TIMER_TIME_OUT 500

extern pthread_mutex_t qlock;
extern struct epoll_events *events;
extern priority_queue<mytimer *, deque<mytimer *>, timerCmp> my_timer_queue;

void accept_connection(int listen_fd, int epoll_fd, const string &path);

void my_handler(void *args);

void handle_expired_event();

void handle_events(int epoll_fd, int listen_fd,
                   struct epoll_event *events, int events_num,
                   const string &path, thread_pool_t *tp);

int main(int argc, char *argv[]) {

    return 0;
}

void my_handler(void *args) {
    auto *request = (requestData *) args;
    request->handle_request();
}

void accept_connection(int listen_fd, int epoll_fd, const string &path) {
    struct sockaddr_in client_addr{};
    memset(&client_addr, 0, sizeof(sockaddr_in));
    socklen_t client_addr_len = 0;
    int accept_fd = 0;
    while ((accept_fd = accept(listen_fd, (struct sockaddr *) &client_addr,
                               &client_addr_len)) > 0) {
        if (set_socket_unblocking(accept_fd) < 0) {
            perror("accept failed");
            return;
        }
        auto *req_info = new requestData(epoll_fd, accept_fd, path);
        uint32_t epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
        epoll_add(epoll_fd, accept_fd, static_cast<void *> (req_info), epo_event);

        auto *mtimer = new mytimer(req_info, TIMER_TIME_OUT);
        req_info->add_timer(mtimer);
        pthread_mutex_lock(&qlock);
        my_timer_queue.push(mtimer);
        pthread_mutex_unlock(&qlock);
    }
}

void handle_events(int epoll_fd, int listen_fd,
                   struct epoll_event *events, int events_num,
                   const string &path, thread_pool_t *tp) {
    for (int i = 0; i < events_num; i++) {
        auto request = (requestData *) (events[i].data.ptr);
        int fd = request->get_fd();

        if (fd == listen_fd) {
            accept_connection(listen_fd, epoll_fd, path);
        } else {
            if (events[i].events & EPOLLERR
                || events[i].events & EPOLLHUP
                || !(events[i].events & EPOLLIN)) {
                printf("error event");
                delete request;
                continue;
            }
            //加入线程池之前将timer和request分离
            request->separate_timer();
            int rc = thread_pool_add_task(tp, my_handler, events[i].data.ptr);
        }
    }
}

void handle_expired_event() {
    pthread_mutex_lock(&qlock);
    while (!my_timer_queue.empty()) {
        mytimer *timer_now_ptr = my_timer_queue.top();
        if (timer_now_ptr->is_deleted() || !timer_now_ptr->is_valid()) {
            my_timer_queue.pop();
            delete timer_now_ptr;
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&qlock);
}

