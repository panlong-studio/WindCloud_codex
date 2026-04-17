#include "socket_init.h"

/*
 * 函数名：socket_init_listen
 * 功能：创建、绑定并监听服务端套接字。
 * 参数：
 *   ip   - 监听 IP。
 *   port - 监听端口。
 * 返回值：成功返回监听 fd，失败返回 -1。
 */
int socket_init_listen(const char *ip, int port)
{
    int listen_fd = -1;
    int opt = 1;
    struct sockaddr_in server_addr;

    // 1. 创建 TCP 监听套接字。
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return -1;
    }

    // 2. 开启端口复用，避免服务端重启时因为 TIME_WAIT 导致端口短时间不可用。
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(listen_fd);
        return -1;
    }

    // 3. 填充服务端地址结构体。
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 4. bind 绑定指定 IP 和端口。
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return -1;
    }

    // 5. listen 把主动套接字变成被动套接字，开始接收客户端连接。
    if (listen(listen_fd, LISTEN_BACKLOG) == -1) {
        perror("listen");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}
