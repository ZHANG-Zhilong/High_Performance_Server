//
// Created by 张志龙 on 2020/5/14.
//
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <cstring>
#include "thread_pool.h"

using namespace std;

struct Msg {
    int id;
    char buf[BUFSIZ];
};
typedef struct Msg Msg;

void *process(void *arg) {    //thread_function,simulate processing task.
    auto *msg = (Msg *) arg;
    printf("thread 0%x working on task %d, %s\n",
           (uint) pthread_self(), msg->id, msg->buf);
    return nullptr;
}

void play_thread_pool() {
    thread_pool_t *thp = thread_pool_create(2, 4, 5); //create thread_pool
    printf("pool inited");
    const int len = 14;
    int *num = (int *) malloc(sizeof(int) * len);
    Msg *message = (Msg *) malloc(sizeof(Msg) * len);
    vector<string> str = {"faith", "hope","charity", "justice", "fortitude", "temperance", "prudence",
                          "gluttony", "greed", "sloth", "envy", "pride", "lust", "wrath"};

    for (int i = 0; i < len; i++) {
        message[i].id = i;
        const char *str2 = str[i].c_str();
        strncpy(message[i].buf, str2, sizeof(str2));
        printf("add task %d\n", i);
        thread_pool_add_task(thp, process, (void *) &message[i]);  // add task to thread_pool
    }
    sleep(4);
    thread_pool_destroy(thp);
}

namespace test_pthread {
    struct data {
        pthread_mutex_t lock;
        int a;
        int b;
    };

    pthread_t tid;

    void *th1(void *arg) {
        auto data = (struct data *) arg;
        cout << "i am here sub thread" << endl;
        pthread_mutex_lock(&data->lock);
        cout << "lock success" << endl;
        cout << data->a << endl;
        pthread_mutex_lock(&data->lock);  //⚠️
        data->b = 20;
        cout << data->b;
        pthread_mutex_unlock(&data->lock);
        pthread_mutex_unlock(&data->lock);
        cout << "unlock success" << endl;
        pthread_exit(nullptr);
        return (void *) 0;
    }

    void th2() {
        cout << "control thread" << endl;
        cout << pthread_self() << endl;
    }

    void play_pthread() {
        struct data *d = nullptr;
        d = (struct data *) malloc(sizeof(struct data));
        d->a = 10;
        pthread_mutex_init(&d->lock, nullptr);
        int ret = pthread_create(&tid, nullptr, th1, d);
        sleep(2);
        if (ret != 0) {
            cout << "create thread failed" << endl;
        }
        th2();
        pthread_join(tid, nullptr);
    }
}

int main(int argc, char *argv[]) {
    //test_pthread::play_pthread();
    play_thread_pool();

    return 0;
}

