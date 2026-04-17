#include "queue.h"

/*
 * 函数名：queue_init
 * 功能：初始化任务队列。
 * 参数：queue - 队列对象指针。
 * 返回值：无。
 */
void queue_init(queue_t *queue)
{
    // 链式队列的初始状态：头尾都为空，元素个数为 0。
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
}

/*
 * 函数名：queue_push
 * 功能：把新的客户端 fd 放到队列尾部。
 * 参数：
 *   queue     - 任务队列。
 *   client_fd - 客户端连接 fd。
 * 返回值：成功返回 0，失败返回 -1。
 */
int queue_push(queue_t *queue, int client_fd)
{
    queue_node_t *node = NULL;

    // 为新任务申请一个结点，结点里只保存客户端 fd。
    node = (queue_node_t *)calloc(1, sizeof(queue_node_t));
    if (node == NULL) {
        return -1;
    }

    node->client_fd = client_fd;
    node->next = NULL;

    // 如果队列原本为空，头尾都指向新结点；
    // 否则执行普通的尾插法。
    if (queue->size == 0) {
        queue->head = node;
        queue->tail = node;
    } else {
        queue->tail->next = node;
        queue->tail = node;
    }

    ++queue->size;
    return 0;
}

/*
 * 函数名：queue_pop
 * 功能：从队列头部取出一个客户端 fd。
 * 参数：
 *   queue     - 任务队列。
 *   client_fd - 输出参数，用于带回取出的 fd。
 * 返回值：成功返回 0，失败返回 -1。
 */
int queue_pop(queue_t *queue, int *client_fd)
{
    queue_node_t *node = NULL;

    // 空队列没有可取的数据，直接返回失败。
    if (queue->size == 0) {
        return -1;
    }

    // 取队头结点，因为这里实现的是 FIFO 队列。
    node = queue->head;
    if (client_fd != NULL) {
        *client_fd = node->client_fd;
    }

    queue->head = node->next;
    // 如果删掉后队列空了，尾指针也必须一起置空。
    if (queue->head == NULL) {
        queue->tail = NULL;
    }

    --queue->size;
    free(node);
    return 0;
}
