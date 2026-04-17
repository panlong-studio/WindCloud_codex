#include "queue.h"

// 生产者调用：将新的文件描述符(fd)加入队列尾部
int enQueue(queue_t *pQueue, int fd){
    
    // 为新结点分配内存并初始化
    node_t *pNew = (node_t *)calloc(1, sizeof(node_t));
    pNew->fd = fd;
    
    // 链表尾插法
    if(pQueue->size == 0){
        // 如果队列为空，头尾指针都指向这个新结点
        pQueue->head = pNew;
        pQueue->end = pNew;
    }else{
        // 否则追加到队尾，并更新 end 指针
        pQueue->end->pNext = pNew;
        pQueue->end = pNew;
    }
    pQueue->size++; // 结点数加一
    
    return 0;
}

// 消费者调用：从队列头部取出一个任务并删除结点
int deQueue(queue_t *pQueue){
    
    // 防御性编程：如果队列为空，直接返回 -1
    if(pQueue->size == 0){
        return -1;
    }
    
    // 记录要删除的头结点
    node_t *p = pQueue->head;
    // 头指针向后移位
    pQueue->head = p->pNext;
    
    // 如果队列中本来只有一个结点，
    // 删完之后队列就空了，需将尾指针置空
    if(pQueue->size == 1){
        pQueue->end = NULL;
    }

    pQueue->size--; // 结点数减一
    free(p);        // 释放弹出的结点的内存，防止内存泄漏

    return 0;
}
