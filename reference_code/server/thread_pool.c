#include "thread_pool.h"
#include "queue.h"
#include "worker.h"
#include <my_header.h>

// 初始化线程池结构体内部的所有数据
void init_thread_pool(thread_pool_t *pool, int num){
    
    // 初始化退出标志位: 0表示正常运行, 1表示让所有线程准备退出
    pool->exitFlag = 0; // 默认为0
    
    // 记录线程池中初始化的工作线程数目
    pool->thread_num = num;
    
    // 将任务队列内存清零，保证初始状态下 size=0，头尾指针为NULL
    // 这一步非常重要，防止野指针引发段错误
    memset(&pool->queue, 0, sizeof(queue_t));

    // 初始化互斥锁，用于保护共享的任务队列
    pthread_mutex_init(&pool->lock, NULL);

    // 初始化条件变量，用于实现线程之间的同步（阻塞沉睡与唤醒）
    pthread_cond_init(&pool->cond, NULL);

    // 在堆区分配一个数组，用于保存所有工作线程的 ID
    // 方便后续回收 (pthread_join)
    pool->thread_id_arr = (pthread_t *)malloc(num * sizeof(pthread_t));
    
    // 循环创建指定数量的子线程
    for(int idx=0; idx<num; ++idx){
        // 注意：将整个 pool 对象的指针作为第四个参数传入 thread_func 线程入口函数中。
        // 因为工作线程在执行时，需要用到 pool 里的锁、条件变量以及任务队列。
        int ret = pthread_create(&pool->thread_id_arr[idx], NULL, thread_func, (void *)pool);
        ERROR_CHECK(ret, -1, "pthread_create");
    }
}
