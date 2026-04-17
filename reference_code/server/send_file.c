#include "send_file.h"
#include <my_header.h>

// 处理客户端请求的核心业务逻辑
void send_file(int fd){
    
    // ================= 接收客户端想要下载的文件名 =================
    int req_file_name_len = 0;
    // 使用 MSG_WAITALL 保证完整读到一个 int (4字节)
    // 如果在读满4字节前客户端断开了连接，ret 会 <= 0
    int ret = recv(fd, &req_file_name_len, sizeof(int), MSG_WAITALL);
    if (ret <= 0) {
        printf("客户端断开连接或接收客户端发送的请求文件名长度失败\n");
        return; // 直接返回，外层 Worker 会负责 close(fd)
    }
    
    char file_name[256] = {0};
    ret = recv(fd, file_name, req_file_name_len, MSG_WAITALL);
    if (ret <= 0) {
        printf("接收客户端发送的请求文件名失败\n");
        return;
    }
    
    // 手动添加字符串结束符 \0，确保作为 C 字符串使用时不会越界
    // 用 [] 包裹打印，方便排查由于隐藏空格或换行符导致的找不到文件的问题
    file_name[req_file_name_len] = '\0';
    printf(">> 客户端请求下载文件: [%s]\n", file_name);
    // ==================================================================
    
    
    // 尝试在服务端本地打开客户端请求的这个文件 (只读模式)
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        perror("open file error");
        // 如果文件不存在或无法打开，发送一个长度为 0 的标志给客户端，
        // 客户端拿到长度 0 后就会知道文件不存在，从而安全断开。
        // MSG_NOSIGNAL：非常关键！当对端断开连接时，继续 send 会触发 SIGPIPE 信号导致服务端整个进程崩溃。
        // 使用 MSG_NOSIGNAL 可以在对端关闭时仅返回 -1，而不是引发崩溃。
        int err_len = 0;
        send(fd, &err_len, sizeof(int), MSG_NOSIGNAL);
        return; 
    }    
    
    // 使用 fstat 获取已打开文件的元信息（如大小、权限等）
    struct stat st;
    fstat(file_fd, &st);
    off_t file_size = st.st_size; // 获取文件内容的实际大小 (字节)
    
    int file_name_len = strlen(file_name);
    // 协议格式设计：将[文件名的长度] + [文件名] + [文件内容总大小]依次发给客户端
    
    // 这里还要注意：如果对一个已经断开的文件描述符执行两次以及以上的
    // 写操作会触发SIGPIPE信号，导致程序崩溃，为了解决这个问题，
    // 可以将send的第四个参数设置为MSG_NOSIGNAL
    
    // 发送文件名的长度
    send(fd, &file_name_len, sizeof(int), MSG_NOSIGNAL);
    // 发送文件名字符串
    send(fd, file_name, file_name_len, MSG_NOSIGNAL);
    // 发送文件的总大小 (客户端拿到这个值后去 ftruncate 并 mmap)
    send(fd, &file_size, sizeof(off_t), MSG_NOSIGNAL);
    
    // ================== 发送文件本体内容 ==================
    // 核心优化：使用 sendfile 函数实现【零拷贝】(Zero-Copy) 技术
    // 传统方式：磁盘 -> 内核缓冲区 -> 用户缓冲区 -> socket缓冲区 -> 网卡 (多次拷贝，多次上下文切换)
    // sendfile：磁盘 -> 内核缓冲区 -> socket缓冲区 -> 网卡 (数据无需进入用户态程序，极大提高传输效率)
    sendfile(fd, file_fd, NULL, file_size);
    printf(">> 文件 [%s] 发送完毕!\n\n", file_name);
    
    // 释放服务端本地打开的文件描述符
    close(file_fd);

}
