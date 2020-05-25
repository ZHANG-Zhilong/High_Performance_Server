#include "threadpool.h"

threadpool_t *threadpool_create(int thread_num, int queue_size, int flag) {

    threadpool_t *pool;
    int i;
    do {
        if (thread_num <= 0 || thread_num > MAX_THREADS
            || queue_size <= 0 || queue_size > MAX_QUEUE) {
            return nullptr;
        }
        pool = (threadpool_t *) malloc(sizeof(threadpool_t));
        if (pool == nullptr) { break; }

        /* Initialize */
        pool->thread_count_in_pool = 0;
        pool->queue_size = queue_size;
        pool->head = pool->tail = pool->pending_task_count = 0;
        pool->shutdown = 0;
        pool->active_thread_num = 0;

        pool->threads = (pthread_t *) malloc(sizeof(pthread_t) * thread_num);
        pool->queue = (threadpool_task_t *)
                malloc(sizeof(threadpool_task_t) * queue_size);

        /* Initialize mutex and conditional variable first */
        if ((pthread_mutex_init(&(pool->lock), nullptr) != 0) ||
            (pthread_cond_init(&(pool->notify), nullptr) != 0) ||
            (pool->threads == nullptr) || (pool->queue == nullptr)) {
            break;
        }

        /* Start worker threads */
        for (i = 0; i < thread_num; i++) {
            if (pthread_create(&(pool->threads[i]), nullptr, threadpool_thread,
                               (void *) pool) != 0) {
                threadpool_destroy(pool, 0);
                return nullptr;
            }
            pool->thread_count_in_pool++;
            pool->active_thread_num++;
        }

        return pool;
    } while (false);

    if (pool != nullptr) {
        threadpool_free(pool);
    }
    return nullptr;
}

int threadpool_add(threadpool_t *pool, void (*function)(void *),
                   void *argument, int flags) {
    // printf("add to thread pool !\n");
    int err = 0;
    int next;
    //(void) flags;
    if (pool == nullptr || function == nullptr) {
        return THREADPOOL_INVALID;
    }
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return THREADPOOL_LOCK_FAILURE;
    }

    next = (pool->tail + 1) % pool->queue_size;
    do {
        /* Are we full ? */
        if (pool->pending_task_count == pool->queue_size) {
            err = THREADPOOL_QUEUE_FULL;
            break;
        }
        /* Are we shutting down ? */
        if (pool->shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        /* Add task to queue */
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->pending_task_count += 1;

        /* pthread_cond_broadcast */
        if (pthread_cond_signal(&(pool->notify)) != 0) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }
    } while (false);

    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = THREADPOOL_LOCK_FAILURE;
    }

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags) {
    printf("Thread pool destroy !\n");
    int i, err = 0;

    if (pool == nullptr) {
        return THREADPOOL_INVALID;
    }

    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        return THREADPOOL_LOCK_FAILURE;
    }

    do {
        /* Already shutting down */
        if (pool->shutdown) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }

        pool->shutdown =
                (flags & THREADPOOL_GRACEFUL) ? graceful_shutdown : immediate_shutdown;

        /* Wake up all worker threads */
        if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
            (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            err = THREADPOOL_LOCK_FAILURE;
            break;
        }

        /* Join all worker thread */
        for (i = 0; i < pool->thread_count_in_pool; ++i) {
            if (pthread_join(pool->threads[i], nullptr) != 0) {
                err = THREADPOOL_THREAD_FAILURE;
            }
        }
    } while (false);

    /* Only if everything went well do we deallocate the pool */
    if (!err) {
        threadpool_free(pool);
    }
    return err;
}

int threadpool_free(threadpool_t *pool) {
    if (pool == nullptr || pool->active_thread_num > 0) {
        return -1;
    }

    /* Did we manage to allocate ? */
    if (pool->threads) {
        free(pool->threads);
        free(pool->queue);

        /* Because we allocate pool->threads after initializing the
           mutex and condition variable, we're sure they're
           initialized. Let's lock the mutex just in case. */
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
    return 0;
}

static void *threadpool_thread(void *threadpool) {
    auto *pool = (threadpool_t *) threadpool;
    threadpool_task_t task;

    for (;;) {

        pthread_mutex_lock(&(pool->lock));

        while ((pool->pending_task_count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if ((pool->shutdown == immediate_shutdown) ||
            ((pool->shutdown == graceful_shutdown) && (pool->pending_task_count == 0))) {
            break;
        }

        /* Grab our task */
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->pending_task_count -= 1;

        /* Unlock */
        pthread_mutex_unlock(&(pool->lock));

        /* Get to work */
        (*(task.function))(task.argument);
    }

    --pool->active_thread_num;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(nullptr);
    return nullptr;
}

ThreadPool::ThreadPool() :
        lock(), active_thread_count_lock(),
        queue_has_task(lock), queue_has_spare(lock) {
    queue_front = 0;
    queue_rear = 0;
    waiting_task_num = 0;
}

ThreadPool::~ThreadPool() {
    this->status = SHUTDOWN;
    this->queue_has_task.notifyAll();
    for (int i = 0; i < this->thread_count; i++) {
        pthread_join(threads[i], nullptr);
    }
    delete[] this->queue;
    delete[]  this->threads;
}

int ThreadPool::create(uint32_t thread_num, uint32_t queue_capacity, int flag = 0) {
    if (thread_num == 0 || queue_capacity == 0) {
        thread_num = 3;
        queue_capacity = 3;
        std::cerr << "set as default in thread pool" << std::endl;
    }
    thread_num = std::min((int) thread_num, MAX_THREADS);
    queue_size = std::min((int) queue_capacity, MAX_QUEUE);
    try {
        threads = new pthread_t[thread_num]();
        queue = new threadpool_task_t[queue_capacity]();
    } catch (const std::bad_alloc &e) {
        std::cerr << e.what() << std::endl;
        exit(1);
    }
    status = RUNNING;
    for (int i = 0; i < thread_num; i++) {
        if (pthread_create(&threads[i], nullptr, do_thread, this) != 0) {
            return -1;
        }
        thread_count++;
        active_thread_count++;
    }
    return 0;
}

int ThreadPool::add(void(*function)(void *), void(*argument), int flag = 0) {
    if (function == nullptr || argument == nullptr) {
        std::cerr << "add err, int function: " << __func__ << std::endl;
        return -1;
    }
    do {
        MutexLockGuard guard(this->lock);
        if (this->status == RUNNING && this->queue_size == this->waiting_task_num) {
            this->queue_has_spare.wait();
        }
        if (this->status == SHUTDOWN) {
            break;
        }
        size_t next = (this->queue_rear + 1) % this->queue_size;  //rear指向下一个位置，而下一个位置还没有任务
        this->queue[this->queue_rear].function = function;
        this->queue[this->queue_rear].argument = argument;
        this->queue_rear = next;
        this->waiting_task_num++;
        this->queue_has_task.notify();
    } while (false);
    return 0;
}

void *do_thread(void *arg) {
    auto *pool = (ThreadPool *) arg;
    threadpool_task_t task{};
    while (true) {
        {
            MutexLockGuard guard(pool->lock);
            while (pool->status == RUNNING && pool->waiting_task_num == 0) {
                pool->queue_has_task.wait();
            }
            if (pool->status == SHUTDOWN) {
                pthread_exit(nullptr);
            }
            task.function = pool->queue[pool->queue_front].function;
            task.argument = pool->queue[pool->queue_front].argument;
            size_t next = (pool->queue_front + 1) % pool->queue_size;
            pool->queue_front = next;
            pool->waiting_task_num--;
            pool->queue_has_spare.notify();
        }
        {
            MutexLockGuard guard(pool->active_thread_count_lock);
            pool->active_thread_count++;
        }
        (*task.function)(task.argument);
        {
            MutexLockGuard guard(pool->active_thread_count_lock);
            pool->active_thread_count--;
        }
    }
    pthread_exit(nullptr);
}

MutexLock::MutexLock() {
    pthread_mutex_init(&mutex_, nullptr);
}

MutexLock::~MutexLock() {
    assert(holder_ == 0);
    pthread_mutex_destroy(&mutex_);
}

bool MutexLock::isLockedByThisThread() const {
    return holder_ == pthread_self();
}

void MutexLock::lock() {
    pthread_mutex_lock(&mutex_);
    holder_ = pthread_self();
}

void MutexLock::unlock() {
    pthread_mutex_unlock(&mutex_);
    holder_ = 0;
}

pthread_mutex_t *MutexLock::getPthreadMutex() {
    return &mutex_;
}

MutexLockGuard::MutexLockGuard(MutexLock &mutex) :
        mutex_(mutex) {
    mutex_.lock();
}

MutexLockGuard::~MutexLockGuard() {
    mutex_.unlock();
}

#define MutexLockGuard(x) static_assert(false, "missing mutex guard var name")

Condition::Condition(MutexLock &mutex) : mutex_(mutex) {
    pthread_cond_init(&pcond_, nullptr);
}

Condition::~Condition() {
    pthread_cond_destroy(&pcond_);
}

void Condition::wait() {
    pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
}

void Condition::notify() {
    pthread_cond_signal(&pcond_);
}

void Condition::notifyAll() {
    pthread_cond_broadcast(&pcond_);
}
