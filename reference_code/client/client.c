#include <my_header.h>

int main(int argc, char *argv[]){                                  
    
    // 1. 校验命令行参数，确保用户输入了要下载的文件名
    if (argc != 2) {
        printf("用法: %s <要下载的文件名 (例如 A.txt)>\n", argv[0]);
        return -1;
    }
    
    char *req_file_name = argv[1]; // 用户请求的文件名
    
    // 硬编码服务端的IP和端口号
    char *ip = "192.168.193.128";
    char *port = "12345";

    // 2. 创建客户端的套接字 (IPv4, TCP协议)
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    ERROR_CHECK(client_fd, -1, "socket");

    // 3. 配置服务端的地址信息结构体
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // 初始化清空内存

    addr.sin_family = AF_INET;            // 使用IPv4地址族
    addr.sin_addr.s_addr = inet_addr(ip); // 将点分十进制的IP字符串转换为网络字节序的整型IP
    addr.sin_port = htons(atoi(port));    // 将主机字节序的端口号转换为网络字节序 (大端)
    
    // 4. 发起连接请求，底层会进行TCP三次握手
    int ret = connect(client_fd, (struct sockaddr *)&addr, sizeof(addr));
    ERROR_CHECK(ret, -1, "connect");
    // 客户端的代码走到这里就说明TCP三次握手已经建立成功了，
    // 已经可以与服务器进行全双工的数据传输

    // ==================== 向服务端发送请求的文件名 ====================
    // 为了防止TCP粘包问题，采用“长度 + 内容”的协议格式发送
    int req_file_name_len = strlen(req_file_name);
    // 先发送文件名的长度
    send(client_fd, &req_file_name_len, sizeof(int), 0);
    // 再发送文件名的实际字符串内容
    send(client_fd, req_file_name, req_file_name_len, 0);
    // ==================================================================
    
    // 5. 准备接收服务端回传的文件数据
    // 先读取服务端发来的文件名字长度
    int file_name_len = 0;
    // MSG_WAITALL: 必须接收满 sizeof(int) 个字节才返回，防止网络拥塞导致只收到一半
    ret = recv(client_fd, &file_name_len, sizeof(int), MSG_WAITALL);
    printf("从服务器接收到的文件名长度: %d\n", file_name_len);
    
    // 如果服务端返回的长度为0，是服务端约定的错误码，说明服务端没有这个文件
    if (ret == 0 || file_name_len <= 0) {
        printf("服务端不存在该文件或连接已断开!\n");
        close(client_fd);
        return -1;
    }

    // 根据服务端发来的文件名长度，接收对应的文件名内容
    char file_name[100] = {0};
    recv(client_fd, file_name, file_name_len, MSG_WAITALL);

    // 接收即将下载的文件大小 (off_t 用于表示大文件的大小)
    off_t file_size = 0;
    recv(client_fd, &file_size, sizeof(off_t), MSG_WAITALL);
    printf("fi
        le_size: %ld \n", file_size);
    
    // 6. 接收文件内容并写入本地磁盘 (使用 mmap 内存映射提高写入效率)
    // 在本地以读写模式创建（或覆盖）同名文件，权限为 0600 (属主可读可写)
    int file_fd = open(file_name, O_RDWR|O_CREAT, 0600);
    
    // 刚创建的文件大小为0，必须将其截断(扩大)到服务端告知的文件大小，
    // 否则后面的 mmap 会因为越界访问而导致段错误 (Segmentation fault)
    ftruncate(file_fd, file_size);
    
    // 内存映射：将磁盘文件映射到进程的虚拟内存空间中
    // PROT_READ|PROT_WRITE: 映射区可读可写
    // MAP_SHARED: 对映射区的修改会同步到磁盘文件中
    char *p = (char *)mmap(NULL, file_size, PROT_READ|PROT_WRITE, MAP_SHARED, file_fd, 0);
    // 直接将网络接收到的数据写到内存映射区，底层会自动同步到磁盘，省去了一次系统调用(write)
    // MSG_WAITALL: 等到接收完毕 file_size 指定长度的数据，如果等不到就一直阻塞
    recv(client_fd, p, file_size, MSG_WAITALL);
    
    // 7. 收尾工作：解除内存映射，关闭文件描述符
    munmap(p, file_size);
    close(file_fd);
    close(client_fd);
    printf("下载完成!\n");
     
    return 0;
}

