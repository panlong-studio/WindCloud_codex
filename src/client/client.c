#include "client.h"
#include "config.h"
#include "file_transfer.h"
#include "protocol.h"

/*
 * 函数名：client_need_arg
 * 功能：检查某些命令是否缺少参数。
 * 参数：
 *   cmd - 命令字。
 *   arg - 参数字符串。
 * 返回值：
 *   参数满足返回 1，参数缺失返回 0。
 */
static int client_need_arg(const char *cmd, const char *arg)
{
    // 这些命令如果没有参数就没法执行，所以在客户端本地先拦住，
    // 避免把明显错误的请求发给服务端。
    if ((strcmp(cmd, "cd") == 0 ||
         strcmp(cmd, "mkdir") == 0 ||
         strcmp(cmd, "rm") == 0 ||
         strcmp(cmd, "puts") == 0 ||
         strcmp(cmd, "gets") == 0) &&
        (arg == NULL || arg[0] == '\0')) {
        printf("命令缺少参数，请重新输入。\n");
        return 0;
    }
    return 1;
}

/*
 * 函数名：client_local_file_ready
 * 功能：上传前检查本地文件是否存在，且为普通文件。
 * 参数：file_name - 本地文件名。
 * 返回值：成功返回 1，失败返回 0。
 */
static int client_local_file_ready(const char *file_name)
{
    struct stat st;

    // puts 上传的是“客户端本地文件”，所以应该先在本机检查。
    if (stat(file_name, &st) == -1) {
        printf("本地文件不存在：%s\n", file_name);
        return 0;
    }
    if (!S_ISREG(st.st_mode)) {
        printf("只能上传普通文件：%s\n", file_name);
        return 0;
    }
    return 1;
}

/*
 * 函数名：client_connect_server
 * 功能：根据配置文件中的 IP 和端口连接服务端。
 * 参数：无。
 * 返回值：成功返回已连接的 socket fd，失败返回 -1。
 */
int client_connect_server(void)
{
    int server_fd = -1;
    struct sockaddr_in server_addr;

    // 客户端是主动连接方，所以这里创建普通 TCP socket 即可。
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return -1;
    }

    // 目标地址来自配置文件，客户端和服务端共用同一份 mycloud.conf。
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((unsigned short)config_get_port());
    server_addr.sin_addr.s_addr = inet_addr(config_get_ip());

    // connect 成功后，server_fd 就可以用于后续全部通信。
    if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

/*
 * 函数名：client_login
 * 功能：循环读取用户名并向服务端发起伪登录。
 * 参数：
 *   server_fd - 服务端套接字。
 *   user_name - 登录成功后输出用户名。
 *   size      - user_name 缓冲区大小。
 * 返回值：登录成功返回 0，失败或退出返回 -1。
 */
int client_login(int server_fd, char *user_name, int size)
{
    char input[MAX_NAME_SIZE] = {0};
    char request[MAX_LINE_SIZE] = {0};
    char response[MAX_LINE_SIZE] = {0};

    while (1) {
        // 登录阶段单独循环，直到成功或用户主动输入 quit。
        printf("请输入用户名(输入 quit 退出)：");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            return -1;
        }

        // 去掉输入中的空白干扰，避免 "alice\n" 之类的问题。
        trim_newline(input);
        trim_space(input);
        if (input[0] == '\0') {
            continue;
        }

        if (strcmp(input, "quit") == 0) {
            return -1;
        }

        // 按服务端约定，登录命令格式固定为 "login 用户名"。
        snprintf(request, sizeof(request), "login %s", input);
        if (protocol_send_string(server_fd, request) != 0) {
            return -1;
        }
        if (protocol_recv_string(server_fd, response, sizeof(response)) < 0) {
            return -1;
        }

        // 只要服务端返回 "OK ..."，就认为登录成功。
        printf("%s\n", response);
        if (strncmp(response, "OK", 2) == 0) {
            strncpy(user_name, input, (size_t)size - 1);
            user_name[size - 1] = '\0';
            return 0;
        }
    }
}

/*
 * 函数名：client_read_command
 * 功能：从标准输入读取一行命令。
 * 参数：
 *   input - 输出缓冲区。
 *   size  - 缓冲区大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int client_read_command(char *input, int size)
{
    // 这里封装成函数，主要是为了把“读取 + 去换行 + 去空格”这组重复操作统一起来。
    if (fgets(input, size, stdin) == NULL) {
        return -1;
    }
    trim_newline(input);
    trim_space(input);
    return 0;
}

/*
 * 函数名：client_run_loop
 * 功能：登录成功后进入命令循环，持续与服务端交互。
 * 参数：
 *   server_fd - 服务端套接字。
 *   user_name - 当前用户名，用于显示提示符。
 * 返回值：无。
 */
void client_run_loop(int server_fd, const char *user_name)
{
    while (1) {
        char input[MAX_LINE_SIZE] = {0};
        char cmd[MAX_CMD_SIZE] = {0};
        char arg[MAX_ARG_SIZE] = {0};
        char response[MAX_RESPONSE_SIZE] = {0};

        // 提示符里带上用户名，让交互看起来更像一个简化版 shell。
        printf("%s@mycloud$ ", user_name);
        fflush(stdout);

        if (client_read_command(input, sizeof(input)) != 0) {
            break;
        }
        if (input[0] == '\0') {
            continue;
        }

        // 先在客户端本地做一轮基础校验：
        // 1. 能否解析出命令
        // 2. 是否是第一阶段支持的命令
        // 3. 是否缺少必要参数
        if (client_parse_command(input, cmd, arg) <= 0) {
            printf("无效命令，请重新输入。\n");
            continue;
        }
        if (!client_is_valid_command(cmd)) {
            printf("不支持的命令：%s\n", cmd);
            continue;
        }
        if (!client_need_arg(cmd, arg)) {
            continue;
        }

        // puts 特殊一点，因为它依赖客户端本地文件存在。
        if (strcmp(cmd, "puts") == 0 && !client_local_file_ready(arg)) {
            continue;
        }

        // 普通命令流程：先把整条命令字符串发给服务端。
        if (protocol_send_string(server_fd, input) != 0) {
            printf("发送请求失败。\n");
            break;
        }

        // 服务端会先回一条文本响应。
        if (protocol_recv_string(server_fd, response, sizeof(response)) < 0) {
            printf("接收服务端响应失败。\n");
            break;
        }

        if (strcmp(cmd, "puts") == 0) {
            printf("%s\n", response);
            if (strncmp(response, "OK", 2) == 0) {
                // 只有服务端明确说“准备好接收文件”后，
                // 客户端才开始按协议发送文件大小和文件内容。
                if (client_send_file(server_fd, arg) != 0) {
                    printf("上传文件失败。\n");
                    break;
                }
                // 上传完毕后，服务端还会再回一条结果消息。
                if (protocol_recv_string(server_fd, response, sizeof(response)) < 0) {
                    printf("接收上传结果失败。\n");
                    break;
                }
                printf("%s\n", response);
            }
        } else if (strcmp(cmd, "gets") == 0) {
            printf("%s\n", response);
            if (strncmp(response, "OK", 2) == 0) {
                // 下载的顺序相反：客户端先收到 OK，再进入收文件流程。
                if (client_recv_file(server_fd, arg, "download") != 0) {
                    printf("下载文件失败。\n");
                    break;
                }
                printf("OK download success\n");
            }
        } else {
            // 普通文本命令（pwd/ls/mkdir/rm/cd/quit）直接打印即可。
            printf("%s\n", response);
            if (strcmp(cmd, "quit") == 0) {
                break;
            }
        }
    }
}
