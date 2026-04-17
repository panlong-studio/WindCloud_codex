#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include "queue.h"
#include <my_header.h>

typedef struct thread_pool{

    //子线程的数目
    int thread_num;

    //每个线程的id
    pthread_t *thread_id_arr;

    //队列
    queue_t queue;

    //互斥锁
    pthread_mutex_t lock;

    //条件变量
    pthread_cond_t cond;

    //退出标志位(当标志位为1时,子线程需要退出)
    int exitFlag;
}thread_pool_t;

//初始化线程池中的数据
void init_thread_pool(thread_pool_t *pool, int num);

#endif
