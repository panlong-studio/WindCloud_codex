#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "common.h"

/*
 * queue_node_t
 * 功能：任务队列中的单个结点。
 * 说明：
 *   第一阶段队列里保存的任务非常简单，就是一个客户端连接 fd。
 *   后面如果项目变复杂，也可以把这里扩展成更大的任务结构体。
 */
typedef struct queue_node_s {
    /* 要处理的客户端连接 */
    int client_fd;
    /* 指向下一个结点 */
    struct queue_node_s *next;
} queue_node_t;

/*
 * queue_t
 * 功能：链式任务队列。
 * 说明：
 *   线程池中的主线程负责入队，工作线程负责出队，
 *   所以这个结构体会被多个线程共享。
 */
typedef struct queue_s {
    /* 队头指针：出队从这里取 */
    queue_node_t *head;
    /* 队尾指针：入队往这里插 */
    queue_node_t *tail;
    /* 当前队列中的任务个数 */
    int size;
} queue_t;

/*
 * 函数名：queue_init
 * 功能：初始化队列。
 * 参数：queue - 队列对象。
 * 返回值：无。
 */
void queue_init(queue_t *queue);

/*
 * 函数名：queue_push
 * 功能：把一个客户端任务追加到队尾。
 * 参数：
 *   queue     - 队列对象。
 *   client_fd - 客户端连接 fd。
 * 返回值：成功返回 0，失败返回 -1。
 */
int queue_push(queue_t *queue, int client_fd);

/*
 * 函数名：queue_pop
 * 功能：从队头取出一个客户端任务。
 * 参数：
 *   queue     - 队列对象。
 *   client_fd - 输出参数，用来带回取出的 fd。
 * 返回值：成功返回 0，失败返回 -1。
 */
int queue_pop(queue_t *queue, int *client_fd);

#endif
