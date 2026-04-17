#include "client.h"
#include "config.h"

/*
 * 函数名：main
 * 功能：客户端程序入口，负责加载配置、连接服务端和进入交互循环。
 * 参数：无。
 * 返回值：成功返回 0，失败返回非 0。
 */
int main(void)
{
    int server_fd = -1;
    char user_name[MAX_NAME_SIZE] = {0};

    // 客户端同样先读配置文件，因为服务端地址在里面。
    if (config_load(CONFIG_FILE_PATH) != 0) {
        fprintf(stderr, "load config failed\n");
        return 1;
    }

    // 先建立 TCP 连接，再进入登录流程。
    server_fd = client_connect_server();
    if (server_fd == -1) {
        return 1;
    }

    // 登录失败或用户主动退出时，客户端直接结束。
    if (client_login(server_fd, user_name, sizeof(user_name)) != 0) {
        close(server_fd);
        return 0;
    }

    // 登录成功后，进入主命令循环。
    client_run_loop(server_fd, user_name);
    close(server_fd);
    return 0;
}
