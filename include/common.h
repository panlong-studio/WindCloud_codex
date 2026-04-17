#ifndef __COMMON_H__
#define __COMMON_H__

/*
 * common.h
 * 功能：统一包含项目中常用的系统头文件、公共宏和公共工具函数声明。
 * 说明：
 *   这个头文件更像整个项目的“地基”：
 *   1. 先把常用系统头文件一次性包含好
 *   2. 再定义公共宏
 *   3. 最后声明多个模块都会用到的基础工具函数
 *   这样其他头文件只需要先 include common.h，就能少写很多重复内容。
 */

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * 下面这些宏属于“全局公共配置”：
 * - 配置文件路径
 * - 数组和缓冲区大小
 * - 日志级别常量
 * 这样做的好处是：项目里如果要统一调整大小，只改这里一处即可。
 */
#define CONFIG_FILE_PATH "config/mycloud.conf"
#define USER_FILE_PATH "config/users.conf"

#define MAX_LINE_SIZE 1024
#define MAX_NAME_SIZE 64
#define MAX_PATH_SIZE 512
#define MAX_CMD_SIZE 32
#define MAX_ARG_SIZE 256
#define MAX_RESPONSE_SIZE 8192
#define MAX_USERS 128
#define FILE_BUFFER_SIZE 4096
#define LISTEN_BACKLOG 100

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_ERROR 2

/*
 * 函数名：trim_newline
 * 功能：删除字符串结尾处的换行符。
 * 参数：str - 需要处理的字符串。
 * 返回值：无。
 * 说明：
 *   这个函数通常配合 fgets 使用。
 *   因为 fgets 读入的一行内容里往往会带一个 '\n'，如果不删掉，
 *   后面的字符串比较、路径拼接、命令解析都会受到影响。
 */
void trim_newline(char *str);

/*
 * 函数名：trim_space
 * 功能：删除字符串首尾多余空格。
 * 参数：str - 需要处理的字符串。
 * 返回值：无。
 * 说明：
 *   这里处理的是“首尾空格”，不会去删除字符串中间的空格。
 *   例如 "   alice   " 会变成 "alice"。
 */
void trim_space(char *str);

/*
 * 函数名：ensure_dir
 * 功能：确保目录存在；如果目录不存在，则按层级创建。
 * 参数：path - 目录路径，例如 "./data/users"。
 * 返回值：成功返回 0，失败返回 -1。
 * 说明：
 *   这个函数在服务端初始化、用户目录创建、客户端下载目录创建时都会用到。
 *   它相当于代码里封装了一版简单的“mkdir -p”。
 */
int ensure_dir(const char *path);

/*
 * 函数名：is_valid_name
 * 功能：检查文件名或目录名是否合法。
 * 参数：name - 待检查的名称。
 * 返回值：合法返回 1，不合法返回 0。
 * 说明：
 *   第一阶段为了降低复杂度，名字里不允许出现路径分隔符和空白字符，
 *   这样可以减少路径拼接时的歧义，也能顺手挡掉一部分非法输入。
 */
int is_valid_name(const char *name);

/*
 * 函数名：split_command
 * 功能：把一行命令拆成“命令字”和“参数”两部分。
 * 参数：
 *   input - 原始输入字符串。
 *   cmd   - 输出的命令字缓冲区。
 *   arg   - 输出的参数缓冲区。
 * 返回值：
 *   返回 sscanf 提取到的字段数量。
 * 示例：
 *   输入 "cd dir1" 后，cmd 为 "cd"，arg 为 "dir1"。
 * 说明：
 *   客户端和服务端都会复用这个函数，所以两边的命令切分规则是一致的。
 */
int split_command(const char *input, char *cmd, char *arg);

#endif
