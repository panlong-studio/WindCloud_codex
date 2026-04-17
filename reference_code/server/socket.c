#include <my_header.h>
#include "socket.h"

// 封装套接字的创建、绑定、监听流程
void init_socket(int *fd, char *ip, char *port){                                  
    
    // 1. 创建用于监听的 TCP 套接字
    *fd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(*fd, -1, "socket");

    // 2. 设置端口复用 (SO_REUSEADDR / SO_REUSEPORT)
    // 作用：当服务器主动关闭连接（如重启服务端）时，处于 TIME_WAIT 状态的端口可以被立即重用，
    // 避免出现 "Address already in use" 的报错导致服务器无法马上启动。
    int opt = 1;
    setsockopt(*fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    // 3. 填充服务器的网络地址结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // 初始化清空内存
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip); // 字符串IP转整型网络字节序
    addr.sin_port= htons(atoi(port));     // 字符串端口转整型网络字节序
    
    // 4. 将套接字与指定的IP和端口进行绑定
    int ret = bind(*fd, (struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "bind");

    // 5. 将套接字设为被动监听状态
    // 参数 100：内核维护的全连接队列(已完成三次握手)与半连接队列的最大长度参考值
    ret = listen(*fd, 100);
    ERROR_CHECK(ret, -1, "listen");
    
}

