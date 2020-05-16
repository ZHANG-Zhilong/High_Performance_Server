//
// Created by 张志龙 on 2020/5/14.
//

#include "thread_pool.h"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <pthread.h>


//thread poll information
struct thread_pool_t {

    pthread_mutex_t lock;                   //mutex to this struct body
    pthread_mutex_t busy_thread_counter;    //mutex to busy_thread_num
    pthread_cond_t queue_spare;             //队列有空余 add task while queue has spare position.
    pthread_cond_t queue_not_empty;         //alert thread that pool comes task.

    pthread_t thread_mgr_tid;               //管理线程 thread_mgr
    pthread_t *thread_worker_tids;          //数组，存放线程池中线程tid
    thread_pool_task_t *task_queue;         //任务队列

    int min_thread_num;
    int max_thread_num;                      //capacity
    int live_thread_num;
    int busy_thread_num;
    int waited_thread_num;

    int task_queue_front;
    int task_queue_rear;                          //tail
    int task_queue_size;                          //队列中实际任务的数量
    int task_queue_capacity;                      //capacity

    int status;                               //flag, thread_pool usage status, 1 or 0
};

[[noreturn]] void *thread_pool_thread(void *thread_pool);

void *schedule_thread(void *thread_pool);

bool thread_alive(pthread_t tid);

int thread_pool_free(thread_pool_t *thread_pool);


thread_pool_t *
thread_pool_create(
        int thread_min_num,
        int thread_max_num,
        int queue_max_size
) {
    if (thread_min_num < 1 || thread_min_num > thread_max_num) {
        std::cerr << "input error in function: " << __func__ << std::endl;
        return nullptr;
    }
    thread_max_num = thread_max_num > MAX_THREADS ? MAX_THREADS : thread_max_num;
    queue_max_size = queue_max_size > MAX_QUEUE ? MAX_QUEUE : queue_max_size;

    auto *pool = (thread_pool_t *) malloc(sizeof(thread_pool_t));
    if (pool == nullptr) {
        return nullptr;
    }

    do {

        //ALLOCATION for threads and task_queue
        pool->thread_worker_tids = (pthread_t *) malloc(sizeof(pthread_t) * thread_max_num);
        pool->task_queue = (thread_pool_task_t *) malloc(sizeof(thread_pool_task_t) * queue_max_size);
        if (pool->thread_worker_tids == nullptr || pool->task_queue == nullptr) {
            std::cerr << "malloc failed in function" << __func__ << std::endl;
            break;
        }
        bzero(pool->thread_worker_tids, sizeof(pool->thread_worker_tids) * thread_max_num);
        bzero(pool->task_queue, sizeof(thread_pool_task_t) * queue_max_size);

        /*initialization*/
        pool->min_thread_num = thread_min_num;
        pool->max_thread_num = thread_max_num;
        pool->busy_thread_num = 0;
        pool->live_thread_num = 0;
        pool->task_queue_capacity = queue_max_size;
        pool->task_queue_size = 0;
        pool->task_queue_front = 0;
        pool->task_queue_rear = 0;
        pool->status = RUNNING;


        // mutex and cond
        if (pthread_mutex_init(&pool->lock, nullptr) != 0 ||
            pthread_mutex_init(&pool->busy_thread_counter, nullptr) != 0 ||
            pthread_cond_init(&pool->queue_not_empty, nullptr) != 0 ||
            pthread_cond_init(&pool->queue_spare, nullptr) != 0
                ) {
            std::cerr << "init the lock or cond failed";
            break;
        }

        //create thread
        pthread_create(&pool->thread_mgr_tid, nullptr, schedule_thread, (void *) pool);  //pool manager
        for (int i = 0; i < thread_min_num; i++) {
            pthread_create(&pool->thread_worker_tids[i], nullptr, thread_pool_thread,
                           (void *) pool);  //pool is this pool
#ifdef DEBUG
            printf("start thread 0x%x...\n", (uint) pool->thread_worker_tids[i]);
#endif
        }
        return pool;

    } while (false);

    thread_pool_free(pool);
}


int                     //向线程池中添加任务
thread_pool_add_task(
        thread_pool_t *pool,
        void *(*function)(void *),
        void *arg
) {

    int err = 0;

    if (pool == nullptr || function == nullptr) {
        return THREADPOOL_INVALID;
    }
    if (pthread_mutex_lock(&pool->lock) != 0) {
        return THREADPOOL_LOCK_FAILURE;
    }


    //Are we full?
    while (pool->status == RUNNING && pool->task_queue_size == pool->task_queue_capacity) {
        pthread_cond_wait(&pool->queue_spare, &pool->lock);
    }
    //Are we running?
    if (pool->status != RUNNING) {
        pthread_mutex_unlock(&pool->lock);
        return THREADPOOL_SHUTDOWN;
    }
    //clear previous call-back function arg
//    if (pool->task_queue[pool->task_queue_rear].arg != nullptr) {
//        free(&pool->task_queue[pool->task_queue_rear].arg);
//        pool->task_queue[pool->task_queue_rear].arg = nullptr;
//    }

    //register function and arg in task queue, and move rear
    pool->task_queue[pool->task_queue_rear].function = function;
    pool->task_queue[pool->task_queue_rear].arg = arg;
    int next = (pool->task_queue_rear + 1) % pool->task_queue_capacity;
    pool->task_queue_rear = next;
    pool->task_queue_size++;

    //added task, alert thread that the task queue is not empty
    pthread_cond_signal(&pool->queue_not_empty);
    if (pthread_mutex_unlock(&pool->lock) != 0) {
        err = THREADPOOL_LOCK_FAILURE;
    }
    return err;
}

void *schedule_thread(void *thread_pool) {

    auto pool = (struct thread_pool_t *) thread_pool;
    while (pool->status == RUNNING) {

        sleep(DEFAULT_SLEEP);     //manage pool interval

        pthread_mutex_lock(&(pool->lock));

//        if(pthread_mutex_trylock(&pool->lock)!=0){
//            continue;
//        }
        int task_queue_size = pool->task_queue_size;                  // 关注 任务数
        int live_thread_num = pool->live_thread_num;                  // 存活 线程数
        pthread_mutex_unlock(&(pool->lock));

//        if(pthread_mutex_trylock(&pool->busy_thread_counter)!=0){
//            continue;
//        }
        pthread_mutex_lock(&(pool->busy_thread_counter));
        int active_thread_num = pool->busy_thread_num;                // 忙着的线程数
        pthread_mutex_unlock(&(pool->busy_thread_counter));

        //create new thread_worker_tids
        pthread_mutex_lock(&pool->lock);
        //任务队列数量大于最小线程池数量，存活线程数量小于线程池容量（线程池最大线程数量）
        if (task_queue_size >= MIN_WAIT_TASK_NUM && live_thread_num < pool->max_thread_num) {
            //每次增加的线程数量为DEFAULT_THREAD
            for (int i = 0, add = 0; i < pool->max_thread_num && add < DEFAULT_THREAD_VARY
                                     && pool->live_thread_num < pool->max_thread_num; i++) {
                //注意三个约束条件
                if (pool->thread_worker_tids[i] == 0 || !thread_alive(pool->thread_worker_tids[i])) {
                    pthread_create(&pool->thread_worker_tids[i], nullptr, thread_pool_thread, (void *) pool);
                    add++;
                    pool->live_thread_num++;
                }
            }

        }
        pthread_mutex_unlock(&pool->lock);

        //destroy extra thread_worker_tids   活跃线程*2<存活线程数 && 存活线程数>最小线程数量
        if (active_thread_num * 2 < live_thread_num && active_thread_num > pool->min_thread_num) {

            pthread_mutex_lock(&pool->lock);
            pool->waited_thread_num = DEFAULT_THREAD_VARY;  //destroy 10 thread_worker_tids;
            pthread_mutex_unlock(&pool->lock);

            for (int i = 0; i < DEFAULT_THREAD_VARY; i++) {
                //inform resting thread, let him kill himself.
                pthread_cond_signal(&pool->queue_not_empty);
            }
        }
    }
    printf("schedule thread exiting...\n");
    pthread_exit(nullptr);
}

[[noreturn]] void *thread_pool_thread(void *thread_pool) {  //工作线程

    auto pool = (struct thread_pool_t *) thread_pool;
    thread_pool_task_t task{};

    while (true) {
        //线程创建，若任务队列为空，说明无任务，阻塞等待。若有任务，继续执行
        pthread_mutex_lock(&pool->lock);

        //任务队列中无任务，阻塞在条件变量中，若此时被唤醒，自行了结
        while (pool->status == RUNNING && pool->task_queue_size == 0) {
            printf("thread 0x%x is waiting\n", (uint) pthread_self());
            //条件变量阻塞等待，释放已掌握mutex，等待触发条件重新获取互斥锁
            pthread_cond_wait(&pool->queue_not_empty, &pool->lock);

            //清除指定数目的空闲进程，此时线程为阻塞等待状态，是空闲线程
            if (pool->waited_thread_num > 0) {
                pool->waited_thread_num--;
                //检查线程池中，库存（存活）线程数量，不小于最小线程数量
                if (pool->live_thread_num > pool->min_thread_num) {
                    printf("thread 0x%x exited.", (uint) pthread_self());
                    pool->live_thread_num--;
                    pthread_mutex_unlock(&pool->lock);
                    pthread_exit(nullptr);
                }
            }
        }

        if (pool->status == SHUTDOWN) {
            pthread_mutex_unlock(&pool->lock);
            printf("thread 0x%x exiting, pool closing.\n", (uint) pthread_self());
            pthread_exit(nullptr);
        }

        task.function = pool->task_queue[pool->task_queue_front].function;
        task.arg = pool->task_queue[pool->task_queue_front].arg;
        pool->task_queue_front = (pool->task_queue_front + 1) % pool->task_queue_capacity;
        pool->task_queue_size--;

        pthread_cond_broadcast(&pool->queue_spare);//广播告知队列未满

        pthread_mutex_unlock(&pool->lock);//取出任务后，立即释放互斥量

        printf("thread 0x%x start working\n", (uint) pthread_self());
        pthread_mutex_lock(&pool->busy_thread_counter);
        pool->busy_thread_num++;                                    //busy ++
        pthread_mutex_unlock(&pool->busy_thread_counter);
        task.function(task.arg);                 //call task call back function

        //任务结束处理
        printf("thread 0x%x done\n", (uint) pthread_self());
        pthread_mutex_lock(&pool->busy_thread_counter);
        pool->busy_thread_num--;                                    //busy--
        pthread_mutex_unlock(&pool->busy_thread_counter);
    }
}

int
thread_pool_destroy(
        thread_pool_t *pool
) {

    int err = 0;
    if (pool == nullptr) {
        return THREADPOOL_INVALID;
    }
    //销毁线程池时未获取互斥量   ⚠️
    do {

        if (pool->status == SHUTDOWN) {
            err = THREADPOOL_SHUTDOWN;
            break;
        }
        pool->status = SHUTDOWN;

        pthread_join(pool->thread_mgr_tid, nullptr);
        printf("joined manager thread.\n");

        //wakeup all waited thread.
        for (int i = 0; i < pool->live_thread_num; i++) {
            pthread_cond_broadcast(&pool->queue_not_empty);
        }
        printf("ready for join all threads, %d threads in total.\n", pool->live_thread_num);
        for (int i = 0; i < pool->live_thread_num; i++) {
            printf("id: 0x%lx\n", pool->thread_worker_tids[i]);
        }

        for (int i = 0; i < pool->max_thread_num; i++) {
            if (pool->thread_worker_tids[i] != 0) {
                //pthread_join(pool->thread_worker_tids[i], nullptr);  调用join会阻塞在这里
                if (pthread_detach(pool->thread_worker_tids[i]) != 0) {
                    err = THREADPOOL_THREAD_FAILURE;
                }
            }
        }

        //if everything is okay.
        if (err == 0) {
            thread_pool_free(pool);
        }
        printf("pool destroyed.\n");

    } while (0);

    return err;
}


int thread_pool_free(thread_pool_t *thread_pool) {
    if (thread_pool == nullptr) {
        return -1;
    }
    if (thread_pool->task_queue != nullptr) {
        free(thread_pool->task_queue);
    }
    if (thread_pool->thread_worker_tids != nullptr) {
        free(thread_pool->thread_worker_tids);
        pthread_mutex_lock(&thread_pool->lock);
        pthread_mutex_destroy(&thread_pool->lock);
        pthread_mutex_lock(&thread_pool->busy_thread_counter);
        pthread_mutex_destroy(&thread_pool->busy_thread_counter);
        pthread_cond_destroy(&thread_pool->queue_spare);
        pthread_cond_destroy(&thread_pool->queue_not_empty);
    }
    free(thread_pool);
    thread_pool = nullptr;
    return 0;
}


bool thread_alive(pthread_t tid) {
    //ESRCH  无此线程
    int status = kill(tid, 0);
    return ESRCH != status;
}








