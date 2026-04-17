#ifndef __SOCKET_INIT_H__
#define __SOCKET_INIT_H__

#include "common.h"

/*
 * socket_init.h
 * 功能：声明服务端监听套接字初始化函数。
 * 说明：
 *   这个模块只负责“建 socket、bind、listen”，
 *   把网络初始化和业务逻辑拆开，主函数会更清晰。
 */

/*
 * 函数名：socket_init_listen
 * 功能：创建并初始化监听套接字。
 * 参数：
 *   ip   - 监听 IP。
 *   port - 监听端口。
 * 返回值：成功返回监听 fd，失败返回 -1。
 */
int socket_init_listen(const char *ip, int port);

#endif
