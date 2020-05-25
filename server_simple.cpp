#include <netinet/in.h>
#include <cstdio>
#include <cstring>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <queue>
#include <vector>

#include "this_epoll.h"
#include "this_util.h"
#include "requestData.h"
#include "threadpool.h"

using namespace std;

const int THREADPOOL_THREAD_NUM = 4;
const int QUEUE_SIZE = 65535;

const int PORT = 8888;
const int ASK_STATIC_FILE = 1;
const int ASK_IMAGE_STITCH = 2;

const string PATH = "/";

const int TIMER_TIME_OUT = 500;

extern pthread_mutex_t qlock;
extern struct epoll_event *events;
extern priority_queue<mytimer *, deque<mytimer *>, timerCmp> myTimerQueue;

void acceptConnection(int listen_fd, int epoll_fd, const string &path);

int socket_bind_listen(int port) {
    // 检查port值，取正确区间范围
    if (port < 1024 || port > 65535) return -1;

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = 0;
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

    // 消除bind时"Address already in use"错误
    int op = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) == -1)
        return -1;

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr{};
    bzero((char *) &server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short) port);
    if (bind(listen_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) ==
        -1)
        return -1;

    // 开始监听，最大等待队列长为LISTENQ
    if (listen(listen_fd, LISTENQ) == -1) return -1;

    if (listen_fd == -1) {
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

void myHandler(void *args) {
    auto *req_data = (requestData *) args;
    req_data->handleRequest();
}

void acceptConnection(int listen_fd, int epoll_fd, const string &path) {
    struct sockaddr_in client_addr{};
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = 0;
    int accept_fd = 0;
    while ((accept_fd = accept(listen_fd,
                               (struct sockaddr *) &client_addr,
                               &client_addr_len)) > 0) {

        int ret = setSocketNonBlocking(accept_fd);
        if (ret < 0) {
            perror("Set non block failed!");
            return;
        }

        auto *req_info = new requestData(epoll_fd, accept_fd, path);

        uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
        epoll_add(epoll_fd, accept_fd, static_cast<void *>(req_info), _epo_event);
        // 新增时间信息
        auto *mtimer = new mytimer(req_info, TIMER_TIME_OUT);
        req_info->addTimer(mtimer);
        pthread_mutex_lock(&qlock);
        myTimerQueue.push(mtimer);
        pthread_mutex_unlock(&qlock);
    }
}

// 分发处理函数
//void handle_events(int epoll_fd, int listen_fd, struct epoll_event *events,
//                   int events_num, const string &path, threadpool_t *tp) {

void handle_events(int epoll_fd, int listen_fd, struct epoll_event *events,
                   int events_num, const string &path, ThreadPool &pool) {
    for (int i = 0; i < events_num; i++) {
        // 获取有事件产生的描述符
        auto *request = (requestData *) (events[i].data.ptr);
        int fd = request->get_fd();

        // 有事件发生的描述符为监听描述符
        if (fd == listen_fd) {
            acceptConnection(listen_fd, epoll_fd, path);
        } else {
            // 排除错误事件
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN))) {
                printf("error event\n");
                delete request;
                continue;
            }

            // 将请求任务加入到线程池中
            // 加入线程池之前将Timer和request分离
            request->separateTimer();
            //int rc = threadpool_add(tp, myHandler, events[i].data.ptr, 0);
            pool.add(myHandler, events[i].data.ptr, 0);
        }
    }
}

/* 处理逻辑是这样的~
因为(1) 优先队列不支持随机访问
(2) 即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构。
所以对于被置为deleted的时间节点，会延迟到它(1)超时 或
(2)它前面的节点都被删除时，它才会被删除。
一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除。
这样做有两个好处：
(1) 第一个好处是不需要遍历优先队列，省时。
(2)
第二个好处是给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，
就不用再重新申请requestData节点了，这样可以继续重复利用前面的requestData，减少了一次delete和一次new的时间。
*/

void handle_expired_event() {
    pthread_mutex_lock(&qlock);
    while (!myTimerQueue.empty()) {
        mytimer *timer_now_ptr = myTimerQueue.top();
        if (timer_now_ptr->isDeleted() || !timer_now_ptr->isvalid()) {
            myTimerQueue.pop();
            delete timer_now_ptr;
        } else {
            break;
        }
    }
    pthread_mutex_unlock(&qlock);
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int main() {

    //handle_for_sigpipe();
    int epoll_fd = epoll_init();
    if (epoll_fd < 0) {
        perror("epoll init failed");
        return 1;
    }
    //threadpool_t *threadpool =
    //       threadpool_create(THREADPOOL_THREAD_NUM, QUEUE_SIZE, 0);

    ThreadPool threadpool;
    threadpool.create(THREADPOOL_THREAD_NUM, QUEUE_SIZE, 0);
    int listen_fd = socket_bind_listen(PORT);
    if (listen_fd < 0) {
        perror("socket bind failed");
        return 1;
    }
    if (setSocketNonBlocking(listen_fd) < 0) {
        perror("set socket non block failed");
        return 1;
    }
    uint32_t event = EPOLLIN | EPOLLET;
    auto *req = new requestData();
    req->set_fd(listen_fd);
    epoll_add(epoll_fd, listen_fd, static_cast<void *>(req), event);
    while (true) {
        int events_num = my_epoll_wait(epoll_fd, events, MAXEVENTS, -1);
        if (events_num == 0) continue;
        printf("%d\n", events_num);
        handle_events(epoll_fd, listen_fd,
                      events, events_num, PATH, threadpool);
        handle_expired_event();
    }
    return 0;
}

#pragma clang diagnostic pop