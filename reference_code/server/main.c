#include "epoll.h"
#include "queue.h"
#include "socket.h"
#include "thread_pool.h"
#include <my_header.h>

// 全局无名管道，用于父子进程间的信号传递（实现服务端的优雅退出）
int pipe_fd[2];

// 父进程收到 SIGINT (Ctrl+C) 信号时的处理函数
void func(int num){
    printf("num: %d\n", num);
    // 父进程不直接杀死子进程，而是往管道写端(pipe_fd[1])写入一个字节的内容，
    // 通知子进程准备退出。
    write(pipe_fd[1], "1", 1);
}

int main(int argc, char *argv[]){                                  
    
    // 1. 创建无名管道，pipe_fd[0]是读端，pipe_fd[1]是写端
    pipe(pipe_fd);
    
    // 2. 创建父子进程架构
    if(fork() != 0){
        // === 这里是父进程的逻辑 ===
        // 注册 SIGINT(2号信号，即Ctrl+C) 的处理函数
        signal(2, func);
        wait(NULL);// 阻塞等待子进程退出，回收子进程资源防止产生僵尸进程
        exit(0);   // 子进程死后，父进程也随之退出，整个服务安全终止
    }
    
    // === 下面全是子进程(也就是实际的Server工作进程)的逻辑 ===

    // 3. 让子进程脱离原有的前台进程组，成为新进程组的组长
    // 原因：在终端按 Ctrl+C 时，信号会发给前台进程组的所有进程。
    // 如果子进程不脱离，它也会直接收到 SIGINT 并立刻被强制杀死，无法完成“优雅退出”的清理工作。
    // 脱离后，只有父进程收到信号，父进程再通过管道通知子进程退出。
    setpgid(0, 0);
    
    // 4. 初始化线程池
    thread_pool_t pool;
    // 预先创建 4 个工作线程，以及初始化内部的锁和条件变量
    init_thread_pool(&pool, 4);
    
    // 5. 初始化网络套接字操作
    int listen_fd = 0;
    // 绑定并监听指定的IP和端口
    init_socket(&listen_fd, "192.168.193.128", "12345");

    // 6. 创建 epoll 实例 (IO多路复用)
    int epfd = epoll_create(1); // 这里的参数只要是大于0的整数即可
    ERROR_CHECK(epfd, -1, "epoll_create");

    // 将监听新连接的 listen_fd 挂树
    add_epoll_fd(epfd, listen_fd);

    // 将管道的读端 pipe_fd[0] 挂树（用于监听父进程发来的退出通知）
    add_epoll_fd(epfd, pipe_fd[0]);
    
    // 7. 进入 epoll 事件循环 (主Reactor)
    while(1){
        struct epoll_event lst[10];
        // epoll_wait 阻塞等待，直到有事件发生（新连接到来，或者管道有数据可读）
        int nready = epoll_wait(epfd, lst, 10, -1);
        ERROR_CHECK(nready, -1, "epoll_wait");
        printf("有 %d 个事件就绪\n", nready);
        
        for(int idx=0; idx<nready; ++idx){
            int fd = lst[idx].data.fd;
            
            // 场景 A：管道有数据可读，说明父进程让子进程退出！
            if(fd == pipe_fd[0]){
                char buf[10] = {0};
                read(fd, buf, sizeof(buf)); // 读走管道里的数据
                printf("子进程的main线程收到了父进程的信号\n");
                
                // === 开始执行优雅退出逻辑 ===
                // 加锁修改公共资源（退出标志位）
                pthread_mutex_lock(&pool.lock);
                pool.exitFlag = 1; // 将退出标志位设置为1，告知所有线程准备退出
                                   // 子线程(工作线程)在完成任务之后，就会退出

                // 此时可能有线程正在 pthread_cond_wait 睡眠等待任务，
                // 使用 broadcast 唤醒所有正在沉睡的线程，让它们看到 exitFlag == 1 然后退出
                pthread_cond_broadcast(&pool.cond);
                pthread_mutex_unlock(&pool.lock);
                
                // 回收所有子线程的资源 (阻塞等待它们退出完毕)
                for(int idx=0; idx<pool.thread_num; ++idx){
                    pthread_join(pool.thread_id_arr[idx], NULL);
                }
                // 子进程的主线程也退出
                pthread_exit((void *)NULL);
            }
            
            // 场景 B：listen_fd 有事件，说明有新的客户端连接请求！
            if(fd == listen_fd){
                // 接收新连接，获取专门与该客户端通信的 conn_fd
                int conn_fd = accept(listen_fd, NULL, NULL);
                ERROR_CHECK(conn_fd, -1, "accept");
                
                // === 将生产者逻辑：新连接放入任务队列 ===
                pthread_mutex_lock(&pool.lock); // 访问共享队列，必须加锁
                
                // 核心操作：将 conn_fd 封装入队
                enQueue(&pool.queue, conn_fd);
                
                // 只要向队列里放入了任务，就发送 signal 信号
                // 唤醒其中一个处于沉睡状态的工作线程
                pthread_cond_signal(&pool.cond);

                // 操作完毕，解锁
                pthread_mutex_unlock(&pool.lock);
                //-----结束与队列打交道-----
            }
        }
    }
    
    return 0;
}

