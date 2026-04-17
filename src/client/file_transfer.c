#include "file_transfer.h"
#include "protocol.h"

/*
 * 函数名：client_send_file
 * 功能：读取本地文件并上传给服务端。
 * 参数：
 *   server_fd - 服务端套接字。
 *   file_path - 本地文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int client_send_file(int server_fd, const char *file_path)
{
    int file_fd = -1;
    struct stat st;
    off_t sent = 0;
    char buffer[FILE_BUFFER_SIZE] = {0};

    // 先打开本地文件，准备把它发给服务端。
    file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        perror("open local file");
        return -1;
    }

    if (fstat(file_fd, &st) == -1) {
        close(file_fd);
        return -1;
    }

    // 上传协议中，文件大小要先发给服务端。
    if (protocol_send_file_size(server_fd, st.st_size) != 0) {
        close(file_fd);
        return -1;
    }

    // 之后进入“读本地文件 -> 发网络”的循环。
    while (sent < st.st_size) {
        ssize_t read_bytes = read(file_fd, buffer, sizeof(buffer));
        if (read_bytes < 0) {
            close(file_fd);
            return -1;
        }
        if (read_bytes == 0) {
            // 正常读到 EOF。
            break;
        }
        if (protocol_send_n(server_fd, buffer, (size_t)read_bytes) != 0) {
            close(file_fd);
            return -1;
        }
        sent += read_bytes;
    }

    close(file_fd);
    return 0;
}

/*
 * 函数名：client_recv_file
 * 功能：从服务端接收文件并保存到下载目录。
 * 参数：
 *   server_fd    - 服务端套接字。
 *   file_name    - 文件名。
 *   download_dir - 下载目录。
 * 返回值：成功返回 0，失败返回 -1。
 */
int client_recv_file(int server_fd, const char *file_name, const char *download_dir)
{
    char file_path[MAX_PATH_SIZE] = {0};
    int file_fd = -1;
    off_t file_size = 0;
    off_t received = 0;
    char buffer[FILE_BUFFER_SIZE] = {0};

    // 下载目录可能不存在，所以先确保 download/ 已经创建好。
    if (ensure_dir(download_dir) != 0) {
        return -1;
    }

    // 拼接本地保存路径，例如 download/test.txt
    snprintf(file_path, sizeof(file_path), "%s/%s", download_dir, file_name);

    // 下载协议要求：先收文件大小，再收文件内容。
    if (protocol_recv_file_size(server_fd, &file_size) != 0) {
        return -1;
    }

    // 本地下载文件使用覆盖写方式。
    file_fd = open(file_path, O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (file_fd == -1) {
        perror("open download file");
        return -1;
    }

    // 循环接收，直到累计字节数达到 file_size。
    while (received < file_size) {
        ssize_t ret = 0;
        size_t need = (size_t)(file_size - received);

        if (need > sizeof(buffer)) {
            need = sizeof(buffer);
        }

        ret = recv(server_fd, buffer, need, 0);
        if (ret <= 0) {
            close(file_fd);
            return -1;
        }

        // 每次收多少，就写多少，保持逻辑最直接。
        if (write(file_fd, buffer, (size_t)ret) != ret) {
            close(file_fd);
            return -1;
        }

        received += ret;
    }

    close(file_fd);
    return 0;
}
