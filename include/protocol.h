#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "common.h"

/*
 * protocol.h
 * 功能：声明客户端和服务端通信时会共用的协议函数。
 * 说明：
 *   第一阶段协议非常简单：
 *   1. 文本命令和文本响应：先发长度，再发内容
 *   2. 文件传输：先发文件大小，再发文件内容
 *   这些接口就是为这套协议服务的。
 */

/*
 * 函数名：protocol_send_n
 * 功能：循环发送固定长度的数据。
 * 参数：
 *   fd  - 套接字文件描述符。
 *   buf - 待发送缓冲区。
 *   len - 待发送总字节数。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_send_n(int fd, const void *buf, size_t len);

/*
 * 函数名：protocol_recv_n
 * 功能：循环接收固定长度的数据。
 * 参数：
 *   fd  - 套接字文件描述符。
 *   buf - 接收缓冲区。
 *   len - 需要接收的总字节数。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_recv_n(int fd, void *buf, size_t len);

/*
 * 函数名：protocol_send_string
 * 功能：按“长度 + 正文”的格式发送字符串。
 * 参数：
 *   fd  - 套接字文件描述符。
 *   str - 待发送字符串。
 * 返回值：成功返回 0，失败返回 -1。
 */
int protocol_send_string(int fd, const char *str);

/*
 * 函数名：protocol_recv_string
 * 功能：按“长度 + 正文”的格式接收字符串。
 * 参数：
 *   fd      - 套接字文件描述符。
 *   buf     - 接收缓冲区。
 *   max_len - 接收缓冲区大小。
 * 返回值：成功返回拷贝到 buf 中的字节数，失败返回 -1。
 */
int protocol_recv_string(int fd, char *buf, int max_len);

/*
 * 文件上传下载时，文件大小要先发后收，
 * 所以下面这两个接口专门负责 off_t 类型的长度字段。
 */
int protocol_send_file_size(int fd, off_t file_size);
int protocol_recv_file_size(int fd, off_t *file_size);

#endif
