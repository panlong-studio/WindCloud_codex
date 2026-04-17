#include "log.h"

/*
 * 日志模块使用两个静态全局变量：
 * g_log_fp    : 日志文件句柄
 * g_log_level : 当前允许输出的最低日志级别
 */
static FILE *g_log_fp = NULL;
static int g_log_level = LOG_LEVEL_INFO;

/*
 * 函数名：log_level_to_string
 * 功能：把日志级别整数转换成字符串。
 * 参数：level - 日志级别。
 * 返回值：返回对应的字符串常量。
 */
static const char *log_level_to_string(int level)
{
    // 这里把整数级别转成字符串，方便最终日志格式更直观。
    if (level == LOG_LEVEL_DEBUG) {
        return "DEBUG";
    }
    if (level == LOG_LEVEL_ERROR) {
        return "ERROR";
    }
    return "INFO";
}

/*
 * 函数名：log_prepare_dir
 * 功能：从日志文件路径中拆出目录并确保目录存在。
 * 参数：file_path - 日志文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
static int log_prepare_dir(const char *file_path)
{
    char dir_path[MAX_PATH_SIZE] = {0};
    char *last_slash = NULL;

    // 把完整文件路径拷贝到临时数组中，因为下面要原地修改字符串。
    strncpy(dir_path, file_path, sizeof(dir_path) - 1);
    last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        // 找到最后一个 '/' 后，把它截断，剩下的就是目录路径。
        *last_slash = '\0';
        if (dir_path[0] != '\0') {
            return ensure_dir(dir_path);
        }
    }
    return 0;
}

/*
 * 函数名：log_init
 * 功能：初始化日志文件和日志级别。
 * 参数：
 *   file_path - 日志文件路径。
 *   level     - 当前日志级别。
 * 返回值：成功返回 0，失败返回 -1。
 */
int log_init(const char *file_path, int level)
{
    // 先记住当前日志级别，后面的 log_write 会按它过滤日志。
    g_log_level = level;

    if (log_prepare_dir(file_path) != 0) {
        return -1;
    }

    // 追加模式打开日志文件，避免每次启动都把旧日志清空。
    g_log_fp = fopen(file_path, "a");
    if (g_log_fp == NULL) {
        perror("fopen log file");
        return -1;
    }

    return 0;
}

/*
 * 函数名：log_write
 * 功能：按日志级别输出日志。
 * 参数：
 *   level  - 日志级别。
 *   file   - 源文件名。
 *   func   - 函数名。
 *   line   - 行号。
 *   format - 格式化字符串。
 * 返回值：无。
 */
void log_write(int level, const char *file, const char *func, int line, const char *format, ...)
{
    char time_buf[64] = {0};
    time_t now = 0;
    struct tm *time_info = NULL;
    va_list args;
    va_list args_copy;

    // 如果本条日志级别低于配置级别，直接丢弃。
    if (level < g_log_level) {
        return;
    }

    // 先把当前时间格式化成可读字符串。
    now = time(NULL);
    time_info = localtime(&now);
    if (time_info != NULL) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", time_info);
    } else {
        strcpy(time_buf, "unknown-time");
    }

    // 先往终端打印一份，方便开发时实时观察。
    printf("[%s] [%s] [%s:%s:%d] ", time_buf, log_level_to_string(level), file, func, line);

    va_start(args, format);
    va_copy(args_copy, args);
    vprintf(format, args);
    printf("\n");
    fflush(stdout);

    // 再往日志文件写一份，方便事后排查。
    if (g_log_fp != NULL) {
        fprintf(g_log_fp, "[%s] [%s] [%s:%s:%d] ", time_buf, log_level_to_string(level), file, func, line);
        vfprintf(g_log_fp, format, args_copy);
        fprintf(g_log_fp, "\n");
        fflush(g_log_fp);
    }

    va_end(args_copy);
    va_end(args);
}

/*
 * 函数名：log_close
 * 功能：关闭日志文件。
 * 参数：无。
 * 返回值：无。
 */
void log_close(void)
{
    if (g_log_fp != NULL) {
        // 程序退出前关闭文件，确保缓冲区内容真正落盘。
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
}
