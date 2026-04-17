#include "epoll.h"
#include <my_header.h>

// 辅助函数：将文件描述符(fd)添加到 epoll 实例的红黑树上进行监听
void add_epoll_fd(int epfd, int fd){
    struct epoll_event evt;
    memset(&evt, 0, sizeof(evt));
    evt.events = EPOLLIN; // 监听读事件 (有数据可读、有新连接到来)
    evt.data.fd = fd;     // 绑定需要监听的文件描述符，方便触发时知道是哪个fd
    
    // EPOLL_CTL_ADD: 注册新的fd到epfd中
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &evt);
    ERROR_CHECK(ret, -1, "epoll_ctl_add");
}

// 辅助函数：将文件描述符(fd)从 epoll 实例的红黑树上移除，取消监听
void del_epoll_fd(int epfd, int fd){
    struct epoll_event evt;
    evt.events = EPOLLIN; //读事件
    evt.data.fd = fd;     //监听的文件描述符
    
    // EPOLL_CTL_DEL: 从epfd中删除一个fd
    int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &evt);
    ERROR_CHECK(ret, -1, "epoll_ctl_del");
}
