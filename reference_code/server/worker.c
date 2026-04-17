#include "worker.h"
#include "queue.h"
#include "thread_pool.h"
#include "send_file.h"
#include <pthread.h>

// 消费者（工作线程）的入口函数
void *thread_func(void *arg){
    // 通过 pthread_create 的第四个参数将 thread_pool_t 指针传过来，
    // 在这里进行强制类型转换解包，获得线程池的操作句柄。
    thread_pool_t *pool = (thread_pool_t *)arg;

    // 工作线程进入无限循环，不断地去队列取任务、执行任务
    while(1){
        int fd = 0;
        // 1. 尝试从队列拿数据前，必须先获取互斥锁（保护共享的队列结构）
        pthread_mutex_lock(&pool->lock);
        
        // 2. 检查队列是否为空
        // 面试常考题：什么是虚假唤醒(Spurious Wakeup)？为什么要用 while 而不是 if？
        // 解答：当多个线程因队列为空都在 pthread_cond_wait 处睡眠时，如果主线程通过 signal 唤醒了一个线程，
        // 在某些极端情况下（例如系统中断），可能会导致多个线程同时被唤醒。
        // 或者，某个线程被唤醒后，还没来得及加锁，任务就被别的线程抢先拿走了。
        // 如果使用 if，被唤醒的线程会直接往下执行，尝试从空队列里 deQueue，从而导致段错误。
        // 使用 while 时，线程被唤醒后会再次检查条件 (size == 0)。如果不满足要求，它会回去继续睡眠。
        while(pool->queue.size == 0 && pool->exitFlag == 0){
            // pthread_cond_wait 做了三件事：
            // 1. 自动解开互斥锁 (让主线程能加锁入队)
            // 2. 让当前线程挂起睡眠
            // 3. 当收到 signal / broadcast 唤醒信号并抢到锁时，重新对互斥锁加锁并往下执行
            pthread_cond_wait(&pool->cond, &pool->lock);
        }
        
        // 3. 检查是否收到了退出指令
        if(pool->exitFlag == 1){
            // 优雅退出原则：死前必须把手上占有的锁释放掉！
            // 否则带着锁自杀，其他等待这把锁的线程会产生死锁，永远卡死。
            pthread_mutex_unlock(&pool->lock);
            pthread_exit((void *)NULL); // 结束本线程
        }

        // 4. 走到这里说明：队列不为空 (size > 0)，且不是要退出
        // 从队列头部取出客户端的网络文件描述符
        fd = pool->queue.head->fd;
        
        // 执行出队操作，将结点从队列中抹去
        deQueue(&pool->queue);

        // 5. 任务提取完毕，立即释放锁，让其他线程可以去队列取任务，提高并发度
        pthread_mutex_unlock(&pool->lock);
        
        // ================= 真正的业务处理 =================
        // 利用文件描述符与客户端进行完整交互 (收文件名 -> 查文件 -> 发送文件数据)
        // 注意：这里的 send_file 内部已经妥善处理了 MSG_WAITALL 和 MSG_NOSIGNAL。
        // 即使发送过程中客户端异常断开，引发的错误也仅在内部 return，不会让当前工作线程崩溃。
        send_file(fd);
        
        // 任务处理完毕，关闭与这个客户端通信的连接，释放资源
        close(fd);
        // 执行完后，while(1) 循环又回到顶端，继续抢锁取下一个任务
    }

    return NULL;
}
