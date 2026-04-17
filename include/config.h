#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "common.h"

/*
 * config_data_t
 * 功能：保存主配置文件 mycloud.conf 中解析出来的结果。
 * 说明：
 *   这个结构体只保存“主配置”，不保存 users.conf 里的用户名列表。
 *   用户名列表由 config.c 内部的静态数组单独维护。
 */
typedef struct config_data_s {
    /* 服务端监听 IP，例如 127.0.0.1 */
    char server_ip[64];
    /* 服务端监听端口 */
    int server_port;
    /* 线程池中工作线程数量 */
    int thread_num;
    /* 当前日志等级 */
    int log_level;
    /* 日志文件路径，例如 log/server.log */
    char log_file[MAX_PATH_SIZE];
    /* 用户数据根目录，例如 ./data/users */
    char data_root[MAX_PATH_SIZE];
} config_data_t;

/*
 * 函数名：config_load
 * 功能：加载主配置文件。
 * 参数：file_path - 主配置文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 * 说明：
 *   该函数主要服务于 mycloud.conf，
 *   负责读取 IP、端口、线程数、日志级别、数据目录等基础配置。
 */
int config_load(const char *file_path);

/*
 * 函数名：config_load_users
 * 功能：加载伪登录用户配置文件。
 * 参数：file_path - 用户配置文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 * 说明：
 *   第一阶段的“伪登录”就是靠这个函数把 users.conf 读进内存，
 *   后面登录时只检查用户名是否存在即可。
 */
int config_load_users(const char *file_path);

/*
 * 下面这些 getter 的作用很直接：
 * 让其他模块只关心“拿配置”，而不用关心配置到底保存在什么地方。
 * 这也是实际项目里很常见的一种封装方式。
 */
const char *config_get_ip(void);
int config_get_port(void);
int config_get_thread_num(void);
int config_get_loglevel(void);
const char *config_get_logfile(void);
const char *config_get_data_root(void);

/*
 * 函数名：config_user_exists
 * 功能：判断某个用户名是否在 users.conf 中存在。
 * 参数：user_name - 要检查的用户名。
 * 返回值：存在返回 1，不存在返回 0。
 */
int config_user_exists(const char *user_name);

/*
 * 这两个函数一般只在服务端初始化阶段使用：
 * 1. 先拿到用户总数
 * 2. 再按下标把每个用户名取出来，为其创建独立目录
 */
int config_get_user_count(void);
const char *config_get_user_name(int index);

#endif
