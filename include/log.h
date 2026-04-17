#ifndef __LOG_H__
#define __LOG_H__

#include "common.h"

/*
 * log.h
 * 功能：声明日志模块的对外接口。
 * 说明：
 *   日志模块既支持写终端，也支持写文件。
 *   其他模块正常情况下只需要关心两件事：
 *   1. 启动时调用 log_init
 *   2. 运行中通过 LOG_INFO / LOG_ERROR / LOG_DEBUG 这些宏写日志
 */

/*
 * 函数名：log_init
 * 功能：初始化日志模块。
 * 参数：
 *   file_path - 日志文件路径。
 *   level     - 当前日志等级。
 * 返回值：成功返回 0，失败返回 -1。
 * 说明：
 *   一般只在服务端启动阶段调用一次。
 */
int log_init(const char *file_path, int level);

/*
 * 函数名：log_write
 * 功能：按日志级别输出日志到终端和日志文件。
 * 参数：
 *   level    - 本条日志的级别。
 *   file     - 当前源文件名。
 *   func     - 当前函数名。
 *   line     - 当前代码行号。
 *   format   - 格式化字符串。
 *   ...      - 可变参数。
 * 返回值：无。
 * 说明：
 *   正常情况下不建议在业务代码里直接手写这些参数，
 *   更推荐使用下面的日志宏自动补齐文件名、函数名和行号。
 */
void log_write(int level, const char *file, const char *func, int line, const char *format, ...);

/*
 * 函数名：log_close
 * 功能：关闭日志文件。
 * 参数：无。
 * 返回值：无。
 * 说明：
 *   一般在服务端退出前调用，用来确保日志缓冲区内容真正落盘。
 */
void log_close(void);

/*
 * 这三个宏是日志模块最常用的入口。
 * 它们把 __FILE__、__func__、__LINE__ 自动带进去，
 * 所以日志里能直接看到“是哪个文件、哪个函数、哪一行”打出来的。
 */
#define LOG_DEBUG(format, ...) log_write(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) log_write(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) log_write(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#endif
