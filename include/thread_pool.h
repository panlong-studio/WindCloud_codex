#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include "common.h"
#include "queue.h"

/*
 * thread_pool_t
 * 功能：线程池核心结构体。
 * 说明：
 *   它把“线程数组 + 任务队列 + 互斥锁 + 条件变量 + 退出标志”这些内容放在一起，
 *   这样主线程和工作线程只要拿到一个 pool 指针，就能完成协作。
 */
typedef struct thread_pool_s {
    /* 工作线程数量 */
    int thread_num;
    /* 所有工作线程的线程 ID 数组 */
    pthread_t *thread_ids;
    /* 等待处理的客户端连接队列 */
    queue_t queue;
    /* 保护共享队列和退出标志的互斥锁 */
    pthread_mutex_t mutex;
    /* 队列为空时，工作线程会睡在这个条件变量上 */
    pthread_cond_t cond;
    /* 线程池是否准备退出：0 表示继续运行，1 表示准备退出 */
    int exit_flag;
} thread_pool_t;

/*
 * 函数名：thread_pool_init
 * 功能：初始化线程池并创建所有工作线程。
 * 参数：
 *   pool       - 线程池对象。
 *   thread_num - 线程数量。
 * 返回值：成功返回 0，失败返回 -1。
 */
int thread_pool_init(thread_pool_t *pool, int thread_num);

/*
 * 函数名：thread_pool_add_task
 * 功能：向任务队列中加入一个新的客户端连接。
 * 参数：
 *   pool      - 线程池对象。
 *   client_fd - 客户端连接 fd。
 * 返回值：成功返回 0，失败返回 -1。
 */
int thread_pool_add_task(thread_pool_t *pool, int client_fd);

/*
 * 函数名：thread_pool_destroy
 * 功能：通知线程池退出并回收资源。
 * 参数：pool - 线程池对象。
 * 返回值：无。
 */
void thread_pool_destroy(thread_pool_t *pool);

/*
 * 函数名：thread_worker
 * 功能：工作线程入口函数。
 * 参数：arg - 实际上传入的是 thread_pool_t *。
 * 返回值：线程退出时返回 NULL。
 */
void *thread_worker(void *arg);

#endif
