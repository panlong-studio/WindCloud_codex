#include "thread_pool.h"
#include "handle.h"
#include "log.h"

/*
 * 函数名：thread_worker
 * 功能：工作线程入口函数，不断从队列中取客户端并处理。
 * 参数：arg - 线程池对象指针。
 * 返回值：线程退出时返回 NULL。
 */
void *thread_worker(void *arg)
{
    // 线程入口函数拿到的是 void *，要先还原成真正的线程池类型。
    thread_pool_t *pool = (thread_pool_t *)arg;

    while (1) {
        int client_fd = -1;

        // 取任务前先加锁，因为队列是多个线程共享的。
        pthread_mutex_lock(&pool->mutex);
        // 这里必须用 while，不能用 if。
        // 原因和 reference_code 中写的一样：线程被唤醒后，条件不一定仍然成立，
        // 所以醒来之后还得重新检查一次队列是否真的有任务。
        while (pool->queue.size == 0 && pool->exit_flag == 0) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        // 如果线程醒来后发现“队列为空且线程池准备退出”，
        // 就说明已经没有继续工作的必要了，当前线程可以安全结束。
        if (pool->queue.size == 0 && pool->exit_flag == 1) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }

        // 走到这里说明队列里确实有任务，把一个 client_fd 取出来。
        queue_pop(&pool->queue, &client_fd);
        pthread_mutex_unlock(&pool->mutex);

        if (client_fd >= 0) {
            LOG_INFO("worker start handle client_fd=%d", client_fd);
            // 一个工作线程会负责这个客户端连接的整个会话，
            // 直到客户端 quit 或断开连接为止。
            handle_request(client_fd);
            close(client_fd);
            LOG_INFO("worker finish client_fd=%d", client_fd);
        }
    }

    return NULL;
}
