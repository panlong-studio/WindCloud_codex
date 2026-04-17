#include "client.h"
#include "common.h"

/*
 * 函数名：client_parse_command
 * 功能：解析客户端输入命令。
 * 参数：
 *   input - 原始输入字符串。
 *   cmd   - 输出命令字。
 *   arg   - 输出参数。
 * 返回值：返回解析到的字段数量。
 */
int client_parse_command(const char *input, char *cmd, char *arg)
{
    // 这里直接复用公共工具函数 split_command，
    // 这样客户端和服务端的切分规则保持一致。
    return split_command(input, cmd, arg);
}

/*
 * 函数名：client_is_valid_command
 * 功能：判断命令是否属于第一期支持的命令。
 * 参数：cmd - 命令字。
 * 返回值：支持返回 1，不支持返回 0。
 */
int client_is_valid_command(const char *cmd)
{
    if (cmd == NULL || cmd[0] == '\0') {
        return 0;
    }

    // 这里只校验第一阶段支持的命令。
    // 以后如果二期、三期加命令，也是在这里继续扩展。
    if (strcmp(cmd, "pwd") == 0 ||
        strcmp(cmd, "ls") == 0 ||
        strcmp(cmd, "cd") == 0 ||
        strcmp(cmd, "mkdir") == 0 ||
        strcmp(cmd, "rm") == 0 ||
        strcmp(cmd, "puts") == 0 ||
        strcmp(cmd, "gets") == 0 ||
        strcmp(cmd, "quit") == 0) {
        return 1;
    }

    return 0;
}
