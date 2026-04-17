#ifndef __FILE_TRANSFER_H__
#define __FILE_TRANSFER_H__

#include "common.h"

/*
 * file_transfer.h
 * 功能：声明第一阶段上传下载相关接口。
 * 说明：
 *   这个头文件里既声明服务端的上传下载函数，
 *   也声明客户端的上传下载函数。
 *   名字虽然相似，但调用场景不同：
 *   - server_xxx 给服务端用
 *   - client_xxx 给客户端用
 */

/*
 * 函数名：server_recv_file
 * 功能：服务端接收客户端上传的文件。
 * 参数：
 *   client_fd - 客户端连接 fd。
 *   file_path - 服务端保存文件的完整路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int server_recv_file(int client_fd, const char *file_path);

/*
 * 函数名：server_send_file
 * 功能：服务端把文件发送给客户端。
 * 参数：
 *   client_fd - 客户端连接 fd。
 *   file_path - 服务端文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int server_send_file(int client_fd, const char *file_path);

/*
 * 函数名：client_send_file
 * 功能：客户端把本地文件上传给服务端。
 * 参数：
 *   server_fd - 服务端连接 fd。
 *   file_path - 本地文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int client_send_file(int server_fd, const char *file_path);

/*
 * 函数名：client_recv_file
 * 功能：客户端接收服务端文件并保存到下载目录。
 * 参数：
 *   server_fd    - 服务端连接 fd。
 *   file_name    - 文件名。
 *   download_dir - 下载目录。
 * 返回值：成功返回 0，失败返回 -1。
 */
int client_recv_file(int server_fd, const char *file_name, const char *download_dir);

#endif
