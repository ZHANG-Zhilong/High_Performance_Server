#ifndef THREADPOOL
#define THREADPOOL

#include "requestData.h"
#include <pthread.h>
#include <cassert>
#include <queue>
#include <iostream>
#include <algorithm>
#include <memory>


const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

constexpr int RUNNING = 1;
constexpr int SHUTDOWN = 0;


typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
} threadpool_shutdown_t;


class MutexLock {
private:
    pthread_t holder_{};
    pthread_mutex_t mutex_{};

    MutexLock(const MutexLock &);

    MutexLock &operator=(const MutexLock &);

public:
    MutexLock();

    ~MutexLock();


    bool isLockedByThisThread() const;

    void unlock();

    void lock();

    pthread_mutex_t *getPthreadMutex();
};

class MutexLockGuard {
private:
    MutexLock &mutex_;

    MutexLockGuard(const MutexLockGuard &);

    MutexLockGuard &operator=(const MutexLockGuard &);

public:
    explicit MutexLockGuard(MutexLock &mutex);

    ~MutexLockGuard();

};

class Condition {
private:
    MutexLock &mutex_;
    pthread_cond_t pcond_{};

    Condition(const Condition &);

    Condition &operator=(const Condition &);

public:
    explicit Condition(MutexLock &mutex);

    ~Condition();

    void wait();

    void notify();

    void notifyAll();
};

typedef struct {

    void (*function)(void *);

    void *argument;

} threadpool_task_t;

struct threadpool_t {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count_in_pool;
    int queue_size;
    int head;
    int tail;
    int pending_task_count;
    int shutdown;
    int active_thread_num;
};

threadpool_t *threadpool_create(int thread_num, int queue_size, int flag);

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);

int threadpool_destroy(threadpool_t *pool, int flags);

int threadpool_free(threadpool_t *pool);

static void *threadpool_thread(void *threadpool);




class ThreadPool {
public:
    MutexLock lock;
    Condition queue_has_task;
    Condition queue_has_spare;
    pthread_t *threads{};
    threadpool_task_t *queue{};
    //std::shared_ptr<threadpool_task_t> queue2;
    //std::shared_ptr<pthread_t> threads2;
    size_t thread_count{};
    size_t queue_size = 0;
    size_t queue_front = 0;
    size_t queue_rear = 0;
    size_t waiting_task_num = 0;
    MutexLock active_thread_count_lock;
    size_t active_thread_count = 0;
    int status{};

public:
    explicit ThreadPool();

    ~ThreadPool();

    int create(uint32_t thread_num, uint32_t queue_size, int flag);

    int add(void (*function)(void *), void *argument, int flag);

    static void *do_thread(void *);
};


#endif