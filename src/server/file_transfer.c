#include "file_transfer.h"
#include "protocol.h"

/*
 * 函数名：server_recv_file
 * 功能：接收客户端上传的文件，并保存到指定路径。
 * 参数：
 *   client_fd - 客户端套接字。
 *   file_path - 服务端保存文件的完整路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int server_recv_file(int client_fd, const char *file_path)
{
    int file_fd = -1;
    off_t file_size = 0;
    off_t received = 0;
    char buffer[FILE_BUFFER_SIZE] = {0};

    // 上传协议约定：
    // 客户端先发文件大小，再发文件内容。
    if (protocol_recv_file_size(client_fd, &file_size) != 0) {
        return -1;
    }

    // 服务端按覆盖写方式打开目标文件。
    // 第一阶段不做断点续传，所以直接 O_TRUNC 即可。
    file_fd = open(file_path, O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (file_fd == -1) {
        perror("open upload file");
        return -1;
    }

    // 循环接收，直到收到的字节数达到 file_size 为止。
    while (received < file_size) {
        ssize_t ret = 0;
        size_t need = (size_t)(file_size - received);

        // 一次最多读 FILE_BUFFER_SIZE，避免缓冲区越界。
        if (need > sizeof(buffer)) {
            need = sizeof(buffer);
        }

        ret = recv(client_fd, buffer, need, 0);
        if (ret <= 0) {
            close(file_fd);
            return -1;
        }

        // 接收到多少，就原样写入本地文件。
        if (write(file_fd, buffer, (size_t)ret) != ret) {
            close(file_fd);
            return -1;
        }

        received += ret;
    }

    close(file_fd);
    return 0;
}

/*
 * 函数名：server_send_file
 * 功能：把服务端文件发送给客户端。
 * 参数：
 *   client_fd - 客户端套接字。
 *   file_path - 服务端要发送的文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int server_send_file(int client_fd, const char *file_path)
{
    int file_fd = -1;
    struct stat st;
    off_t sent = 0;
    char buffer[FILE_BUFFER_SIZE] = {0};

    // 下载流程的第一步：先打开服务端文件。
    file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        perror("open download file");
        return -1;
    }

    // 用 fstat 拿到文件大小，客户端要先知道总长度才能正确接收。
    if (fstat(file_fd, &st) == -1) {
        close(file_fd);
        return -1;
    }

    // 下载协议约定：服务端先发文件大小，再发文件内容。
    if (protocol_send_file_size(client_fd, st.st_size) != 0) {
        close(file_fd);
        return -1;
    }

    // 循环从磁盘读，循环发到网络。
    while (sent < st.st_size) {
        ssize_t read_bytes = read(file_fd, buffer, sizeof(buffer));
        if (read_bytes < 0) {
            close(file_fd);
            return -1;
        }
        if (read_bytes == 0) {
            // 正常读到 EOF 就结束。
            break;
        }
        if (protocol_send_n(client_fd, buffer, (size_t)read_bytes) != 0) {
            close(file_fd);
            return -1;
        }
        sent += read_bytes;
    }

    close(file_fd);
    return 0;
}
