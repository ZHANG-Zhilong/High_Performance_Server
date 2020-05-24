#ifndef THREADPOOL
#define THREADPOOL

#include "requestData.h"
#include <pthread.h>
#include <cassert>


const int THREADPOOL_INVALID = -1;
const int THREADPOOL_LOCK_FAILURE = -2;
const int THREADPOOL_QUEUE_FULL = -3;
const int THREADPOOL_SHUTDOWN = -4;
const int THREADPOOL_THREAD_FAILURE = -5;
const int THREADPOOL_GRACEFUL = 1;

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown = 2
} threadpool_shutdown_t;

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

typedef struct {
    void (*function)(void *);

    void *argument;
} threadpool_task_t;

struct threadpool_t {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
};

threadpool_t *threadpool_create(int thread_count, int queue_size, int flags);

int threadpool_add(threadpool_t *pool, void (*function)(void *), void *argument, int flags);

int threadpool_destroy(threadpool_t *pool, int flags);

int threadpool_free(threadpool_t *pool);

static void *threadpool_thread(void *threadpool);

class MutexLock {
private:
    pthread_t holder_{};
    pthread_mutex_t mutex_{};

public:
    MutexLock();

    ~MutexLock();

    MutexLock(MutexLock &) = default;

    MutexLock &operator=(const MutexLock &) = delete;

    bool isLockedByThisThread() const;

    void unlock();

    void lock();

    pthread_mutex_t *getPthreadMutex();
};

class MutexLockGuard {
private:
    MutexLock &mutex_;
public:
    explicit MutexLockGuard(MutexLock &mutex);

    ~MutexLockGuard();

    MutexLockGuard(MutexLockGuard &) = delete;

    MutexLockGuard &operator=(const MutexLockGuard &) = delete;
};


class Condition {
private:
    MutexLock &mutex_;
    pthread_cond_t pcond_{};
public:
    Condition(MutexLock &);

    ~Condition();

    Condition(Condition &) = delete;

    Condition &operator=(const Condition &) = delete;

    void wait();

    void notify();

    void notifyAll();
};

#endif