//
// Created by 张志龙 on 2020/5/14.
//
#ifndef HIGH_PERFORMANCE_SERVER_THREAD_POOL_H
#define HIGH_PERFORMANCE_SERVER_THREAD_POOL_H

#define THREADPOOL_INVALID -1
#define THREADPOOL_LOCK_FAILURE -2
#define THREADPOOL_QUEUE_FULL -3
#define THREADPOOL_SHUTDOWN -4
#define THREADPOOL_THREAD_FAILURE -5
#define THREADPOOL_GRACEFUL 1

#define MAX_THREADS 1024
#define MAX_QUEUE 65535

#define RUNNING 1
#define SHUTDOWN 0
#define DEFAULT_SLEEP 3              //10s检测一次
#define MIN_WAIT_TASK_NUM 10    //如果queue_size大于该值，增加线程数量
#define DEFAULT_THREAD_VARY 10  //每次创建和销毁线程的数量


typedef enum{
    immediately_shutdown = 1,
    graceful_shutdown=2
}pool_shutdown_t;


struct thread_pool_task_t {
    void *(*function)(void *);
    void *arg;
};

typedef struct thread_pool_task_t thread_pool_task_t;
typedef struct thread_pool_t thread_pool_t;

thread_pool_t *
thread_pool_create(
        int thread_min_num,
        int thread_max_num,
        int queue_max_size
);

int
thread_pool_add_task(
        thread_pool_t *pool,
        void *(*function)(void *arg),
        void *arg
);

int
thread_pool_destroy(
        thread_pool_t *pool
);


#endif //HIGH_PERFORMANCE_SERVER_THREAD_POOL_H
