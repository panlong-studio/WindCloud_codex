#ifndef __SESSION_H__
#define __SESSION_H__

#include "common.h"

/*
 * session_t
 * 功能：描述“一个客户端连接当前处于什么状态”。
 * 说明：
 *   第一阶段服务端是长连接模式，
 *   所以同一个 client_fd 在整个会话过程中会持续保存这些状态信息。
 */
typedef struct session_s {
    /* 当前客户端套接字 */
    int client_fd;
    /* 是否已经成功登录：0 未登录，1 已登录 */
    int is_login;
    /* 当前登录用户名 */
    char user_name[MAX_NAME_SIZE];
    /* 当前虚拟路径，例如 "/" 或 "/dir1" */
    char current_path[MAX_PATH_SIZE];
    /* 当前用户真实根目录，例如 "./data/users/alice" */
    char user_root[MAX_PATH_SIZE];
} session_t;

/*
 * 函数名：session_init
 * 功能：初始化一个新会话。
 * 参数：
 *   session   - 会话对象。
 *   client_fd - 客户端连接 fd。
 * 返回值：无。
 */
void session_init(session_t *session, int client_fd);

/*
 * 函数名：session_login
 * 功能：根据用户名完成伪登录并设置用户根目录。
 * 参数：
 *   session   - 会话对象。
 *   user_name - 登录用户名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_login(session_t *session, const char *user_name);

/*
 * 函数名：session_build_current_real_path
 * 功能：把当前虚拟路径转换成真实路径。
 * 参数：
 *   session   - 会话对象。
 *   real_path - 输出真实路径缓冲区。
 *   size      - 缓冲区大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_build_current_real_path(const session_t *session, char *real_path, int size);

/*
 * 函数名：session_build_target_path
 * 功能：在当前目录下拼接某个目标文件或目录的真实路径。
 * 参数：
 *   session   - 会话对象。
 *   name      - 目标名称。
 *   real_path - 输出真实路径缓冲区。
 *   size      - 缓冲区大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_build_target_path(const session_t *session, const char *name, char *real_path, int size);

/*
 * 下面两个函数只负责更新 session 里的“虚拟路径”：
 * - session_cd_child : 进入下一级
 * - session_cd_parent: 返回上一级
 * 真正的目录是否存在，需要在 handle.c 里先做检查。
 */
int session_cd_child(session_t *session, const char *dir_name);
int session_cd_parent(session_t *session);

#endif
