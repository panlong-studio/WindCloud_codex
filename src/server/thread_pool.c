#include "thread_pool.h"

/*
 * 函数名：thread_pool_init
 * 功能：初始化线程池，创建所有工作线程。
 * 参数：
 *   pool       - 线程池对象。
 *   thread_num - 线程数量。
 * 返回值：成功返回 0，失败返回 -1。
 */
int thread_pool_init(thread_pool_t *pool, int thread_num)
{
    int i = 0;

    // 先整体清零，保证结构体中的指针和计数都是可控的初始值。
    memset(pool, 0, sizeof(*pool));
    pool->thread_num = thread_num;
    pool->exit_flag = 0;
    queue_init(&pool->queue);

    // 线程池会被多个线程共享，所以互斥锁和条件变量都是必需的。
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&pool->cond, NULL) != 0) {
        pthread_mutex_destroy(&pool->mutex);
        return -1;
    }

    // 保存所有工作线程的线程 ID，方便退出时统一 pthread_join。
    pool->thread_ids = (pthread_t *)calloc((size_t)thread_num, sizeof(pthread_t));
    if (pool->thread_ids == NULL) {
        pthread_cond_destroy(&pool->cond);
        pthread_mutex_destroy(&pool->mutex);
        return -1;
    }

    // 逐个创建工作线程。每个线程拿到的都是同一个线程池对象，
    // 因为它们共享队列、锁、条件变量和退出标志。
    for (i = 0; i < thread_num; ++i) {
        if (pthread_create(&pool->thread_ids[i], NULL, thread_worker, pool) != 0) {
            // 如果线程创建到一半失败，已经创建出来的线程也要回收，
            // 所以这里直接复用 destroy 逻辑做清理。
            pool->thread_num = i;
            thread_pool_destroy(pool);
            return -1;
        }
    }

    return 0;
}

/*
 * 函数名：thread_pool_add_task
 * 功能：向线程池任务队列中添加一个客户端连接任务。
 * 参数：
 *   pool      - 线程池对象。
 *   client_fd - 客户端连接 fd。
 * 返回值：成功返回 0，失败返回 -1。
 */
int thread_pool_add_task(thread_pool_t *pool, int client_fd)
{
    int ret = 0;

    // 队列是共享资源，生产者（主线程）往里塞任务前必须加锁。
    pthread_mutex_lock(&pool->mutex);
    ret = queue_push(&pool->queue, client_fd);
    if (ret == 0) {
        // 队列里有新任务后，唤醒一个沉睡中的工作线程。
        pthread_cond_signal(&pool->cond);
    }
    pthread_mutex_unlock(&pool->mutex);

    return ret;
}

/*
 * 函数名：thread_pool_destroy
 * 功能：通知所有线程退出并回收线程池资源。
 * 参数：pool - 线程池对象。
 * 返回值：无。
 */
void thread_pool_destroy(thread_pool_t *pool)
{
    int i = 0;
    int client_fd = -1;

    // 先加锁设置退出标志，再广播唤醒所有等待中的线程。
    // 否则某些线程可能永远睡在 cond_wait 里，导致 join 卡死。
    pthread_mutex_lock(&pool->mutex);
    pool->exit_flag = 1;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    // 阻塞等待每个工作线程退出，确保线程资源真正回收。
    for (i = 0; i < pool->thread_num; ++i) {
        if (pool->thread_ids != NULL) {
            pthread_join(pool->thread_ids[i], NULL);
        }
    }

    // 如果队列里还残留未处理的 client_fd，统一 close 掉，
    // 防止文件描述符泄漏。
    while (queue_pop(&pool->queue, &client_fd) == 0) {
        close(client_fd);
    }

    // 最后释放线程池本身申请过的资源。
    free(pool->thread_ids);
    pool->thread_ids = NULL;
    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->mutex);
}
