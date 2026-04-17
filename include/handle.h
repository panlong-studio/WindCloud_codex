#ifndef __HANDLE_H__
#define __HANDLE_H__

#include "common.h"
#include "session.h"

/*
 * handle.h
 * 功能：声明服务端命令处理模块的对外接口。
 * 说明：
 *   handle_request 是总入口，
 *   内部再根据命令字符串分发到不同的 handle_xxx 函数。
 */

/*
 * 函数名：handle_request
 * 功能：处理单个客户端连接上的完整命令循环。
 * 参数：client_fd - 客户端连接 fd。
 * 返回值：无。
 */
void handle_request(int client_fd);

/*
 * 下面这些函数分别对应第一阶段支持的具体命令。
 * 它们都围绕 session 工作：
 * - 读 session 中的当前状态
 * - 按命令修改状态或操作文件
 * - 把结果返回给客户端
 */
int handle_login(session_t *session, const char *arg);
int handle_pwd(session_t *session);
int handle_ls(session_t *session);
int handle_cd(session_t *session, const char *arg);
int handle_mkdir(session_t *session, const char *arg);
int handle_rm(session_t *session, const char *arg);
int handle_puts(session_t *session, const char *arg);
int handle_gets(session_t *session, const char *arg);

#endif
