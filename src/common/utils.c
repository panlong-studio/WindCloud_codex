#include "common.h"

/*
 * 函数名：trim_newline
 * 功能：删除字符串尾部的换行符和回车符。
 * 参数：str - 需要处理的字符串。
 * 返回值：无。
 */
void trim_newline(char *str)
{
    size_t len = 0;

    if (str == NULL) {
        return;
    }

    // 从尾部开始删，兼容 Linux 的 '\n' 和可能出现的 '\r\n'。
    len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[len - 1] = '\0';
        --len;
    }
}

/*
 * 函数名：trim_space
 * 功能：删除字符串首尾的空格和制表符。
 * 参数：str - 需要处理的字符串。
 * 返回值：无。
 */
void trim_space(char *str)
{
    int start = 0;
    int end = 0;
    int len = 0;

    if (str == NULL) {
        return;
    }

    len = (int)strlen(str);
    if (len == 0) {
        return;
    }

    // start 从左往右找第一个非空白字符。
    while (str[start] != '\0' && (str[start] == ' ' || str[start] == '\t')) {
        ++start;
    }

    // end 从右往左找最后一个非空白字符。
    end = (int)strlen(str) - 1;
    while (end >= start && (str[end] == ' ' || str[end] == '\t')) {
        str[end] = '\0';
        --end;
    }

    // 如果前面有多余空格，用 memmove 把有效内容整体前移。
    if (start > 0) {
        memmove(str, str + start, strlen(str + start) + 1);
    }
}

/*
 * 函数名：ensure_dir
 * 功能：确保目录存在，不存在时按层级创建。
 * 参数：path - 目录路径。
 * 返回值：成功返回 0，失败返回 -1。
 * 示例：ensure_dir("./data/users/alice");
 */
int ensure_dir(const char *path)
{
    char temp[MAX_PATH_SIZE] = {0};
    size_t len = 0;
    size_t i = 0;

    if (path == NULL || path[0] == '\0') {
        return -1;
    }

    // temp 是可修改副本，因为下面会临时把 '/' 改成 '\0'。
    strncpy(temp, path, sizeof(temp) - 1);
    len = strlen(temp);
    if (len == 0) {
        return -1;
    }

    // 如果路径末尾自带 '/'，先删掉，避免重复处理。
    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    // 逐层创建目录。
    // 例如 "./data/users/alice"：
    // 会先尝试创建 "./data"，再创建 "./data/users"，最后创建完整路径。
    for (i = 1; temp[i] != '\0'; ++i) {
        if (temp[i] == '/') {
            temp[i] = '\0';
            if (strlen(temp) > 0 && mkdir(temp, 0775) == -1 && errno != EEXIST) {
                return -1;
            }
            temp[i] = '/';
        }
    }

    if (mkdir(temp, 0775) == -1 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

/*
 * 函数名：is_valid_name
 * 功能：检查文件名或目录名是否合法。
 * 参数：name - 需要检查的名字。
 * 返回值：合法返回 1，不合法返回 0。
 */
int is_valid_name(const char *name)
{
    int i = 0;

    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    // "." 和 ".." 在目录操作里具有特殊含义，
    // 第一阶段不允许把它们当普通文件名或目录名使用。
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return 0;
    }

    // 为了避免路径穿越等问题，名字里不允许出现斜杠、反斜杠和空白字符。
    for (i = 0; name[i] != '\0'; ++i) {
        if (name[i] == '/' || name[i] == '\\' || name[i] == ' ' || name[i] == '\t') {
            return 0;
        }
    }

    return 1;
}

/*
 * 函数名：split_command
 * 功能：把输入命令拆成命令字和参数。
 * 参数：
 *   input - 原始输入。
 *   cmd   - 输出命令字。
 *   arg   - 输出参数。
 * 返回值：成功返回提取字段个数，失败返回 0。
 */
int split_command(const char *input, char *cmd, char *arg)
{
    int count = 0;

    // 先把输出缓冲区清成空串，避免上次调用残留内容影响本次结果。
    if (cmd != NULL) {
        cmd[0] = '\0';
    }
    if (arg != NULL) {
        arg[0] = '\0';
    }

    if (input == NULL || cmd == NULL || arg == NULL) {
        return 0;
    }

    // 第一阶段命令格式比较简单，所以 sscanf 足够用了：
    // 第一个 %s 取命令字，第二个 %[...] 取参数的剩余部分。
    count = sscanf(input, "%31s %255[^\n]", cmd, arg);
    trim_space(cmd);
    trim_space(arg);
    return count;
}
