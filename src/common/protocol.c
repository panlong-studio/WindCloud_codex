#include "protocol.h"

/*
 * 函数名：protocol_send_n
 * 功能：循环发送指定长度的数据，直到全部发送完成。
 * 参数：
 *   fd  - 套接字文件描述符。
 *   buf - 待发送缓冲区。
 *   len - 待发送字节数。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_send_n(int fd, const void *buf, size_t len)
{
    size_t total = 0;
    const char *p = (const char *)buf;

    // send 一次不一定能把所有字节都发完，
    // 所以这里要自己维护 total，循环发到完整长度为止。
    while (total < len) {
        ssize_t ret = send(fd, p + total, len - total, MSG_NOSIGNAL);
        if (ret <= 0) {
            return -1;
        }
        total += (size_t)ret;
    }

    return 0;
}

/*
 * 函数名：protocol_recv_n
 * 功能：循环接收指定长度的数据，直到全部接收完成。
 * 参数：
 *   fd  - 套接字文件描述符。
 *   buf - 接收缓冲区。
 *   len - 需要接收的字节数。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_recv_n(int fd, void *buf, size_t len)
{
    size_t total = 0;
    char *p = (char *)buf;

    // recv 也一样，一次不一定能把目标长度全部收满。
    // 所以只要协议要求“必须读满 N 个字节”，就需要这种循环接收。
    while (total < len) {
        ssize_t ret = recv(fd, p + total, len - total, 0);
        if (ret <= 0) {
            return -1;
        }
        total += (size_t)ret;
    }

    return 0;
}

/*
 * 函数名：protocol_send_string
 * 功能：按“长度 + 内容”的协议发送字符串。
 * 参数：
 *   fd  - 套接字文件描述符。
 *   str - 待发送字符串。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_send_string(int fd, const char *str)
{
    int len = 0;

    // 约定：先发 int 长度，再发字符串正文。
    // 这是本项目第一阶段最核心的文本协议格式。
    if (str == NULL) {
        len = 0;
        return protocol_send_n(fd, &len, sizeof(int));
    }

    len = (int)strlen(str);
    if (protocol_send_n(fd, &len, sizeof(int)) != 0) {
        return -1;
    }
    // 空字符串只需要发一个长度 0 即可，正文部分可以省略。
    if (len == 0) {
        return 0;
    }

    return protocol_send_n(fd, str, (size_t)len);
}

/*
 * 函数名：protocol_recv_string
 * 功能：按“长度 + 内容”的协议接收字符串。
 * 参数：
 *   fd      - 套接字文件描述符。
 *   buf     - 接收缓冲区。
 *   max_len - 缓冲区最大长度。
 * 返回值：成功返回实际拷贝的长度，失败返回 -1。
 */
int protocol_recv_string(int fd, char *buf, int max_len)
{
    int len = 0;
    int copy_len = 0;
    int remain = 0;
    char temp[256] = {0};

    // 第一步先收长度字段。
    if (protocol_recv_n(fd, &len, sizeof(int)) != 0) {
        return -1;
    }

    // 长度出现负数一定是协议异常。
    if (len < 0) {
        return -1;
    }

    if (max_len <= 0) {
        return -1;
    }

    // 如果对端发来的是空字符串，这里直接返回。
    if (len == 0) {
        buf[0] = '\0';
        return 0;
    }

    // 如果对端发的正文比本地缓冲区更长，
    // 本地只保留能装下的部分，多余数据继续从网络里“吞掉”，
    // 这样不会把后续协议读乱。
    copy_len = len;
    if (copy_len >= max_len) {
        copy_len = max_len - 1;
    }

    if (protocol_recv_n(fd, buf, (size_t)copy_len) != 0) {
        return -1;
    }
    buf[copy_len] = '\0';

    // remain 表示还剩多少字节虽然本地不要，但协议层必须把它们读走。
    remain = len - copy_len;
    while (remain > 0) {
        int once = remain;
        if (once > (int)sizeof(temp)) {
            once = (int)sizeof(temp);
        }
        if (protocol_recv_n(fd, temp, (size_t)once) != 0) {
            return -1;
        }
        remain -= once;
    }

    return copy_len;
}

/*
 * 函数名：protocol_send_file_size
 * 功能：发送文件大小。
 * 参数：
 *   fd        - 套接字文件描述符。
 *   file_size - 文件大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_send_file_size(int fd, off_t file_size)
{
    // 文件大小本质上也是一个固定长度字段，这里继续复用 send_n。
    return protocol_send_n(fd, &file_size, sizeof(off_t));
}

/*
 * 函数名：protocol_recv_file_size
 * 功能：接收文件大小。
 * 参数：
 *   fd        - 套接字文件描述符。
 *   file_size - 输出的文件大小指针。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_recv_file_size(int fd, off_t *file_size)
{
    if (file_size == NULL) {
        return -1;
    }
    // 与发送端保持对应：必须完整收满 sizeof(off_t) 个字节。
    return protocol_recv_n(fd, file_size, sizeof(off_t));
}
