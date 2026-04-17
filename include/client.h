#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "common.h"

/*
 * client.h
 * 功能：声明客户端交互流程相关接口。
 * 说明：
 *   客户端主流程大致如下：
 *   1. client_connect_server 连接服务端
 *   2. client_login 登录
 *   3. client_run_loop 进入命令循环
 */

/*
 * 函数名：client_connect_server
 * 功能：根据配置文件连接服务端。
 * 参数：无。
 * 返回值：成功返回已连接的 socket fd，失败返回 -1。
 */
int client_connect_server(void);

/*
 * 函数名：client_login
 * 功能：执行客户端登录流程。
 * 参数：
 *   server_fd - 服务端连接 fd。
 *   user_name - 登录成功后输出用户名。
 *   size      - user_name 缓冲区大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int client_login(int server_fd, char *user_name, int size);

/*
 * 函数名：client_run_loop
 * 功能：登录成功后进入命令交互循环。
 * 参数：
 *   server_fd - 服务端连接 fd。
 *   user_name - 当前用户名。
 * 返回值：无。
 */
void client_run_loop(int server_fd, const char *user_name);

/*
 * 下面三个函数主要服务于“命令行输入处理”：
 * - client_read_command 负责读一行
 * - client_parse_command 负责拆成命令和参数
 * - client_is_valid_command 负责判断是否是本期支持的命令
 */
int client_read_command(char *input, int size);
int client_parse_command(const char *input, char *cmd, char *arg);
int client_is_valid_command(const char *cmd);

#endif
