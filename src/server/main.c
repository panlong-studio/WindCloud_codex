#include "common.h"
#include "config.h"
#include "log.h"
#include "socket_init.h"
#include "thread_pool.h"

/*
 * 这三个静态全局变量只在 main.c 内部使用：
 * g_stop      : 服务端是否收到退出指令
 * g_listen_fd : 当前监听套接字，便于收到信号后主动 close
 * g_pool      : 线程池对象，主线程和退出流程都要访问它
 */
static volatile sig_atomic_t g_stop = 0;
static int g_listen_fd = -1;
static thread_pool_t g_pool;

/*
 * 函数名：server_signal_handler
 * 功能：处理 Ctrl+C，通知服务端退出。
 * 参数：sig - 信号编号。
 * 返回值：无。
 */
static void server_signal_handler(int sig)
{
    (void)sig;
    // 收到 Ctrl+C 后，不在信号处理函数里做复杂逻辑，
    // 这里只做两件最核心的事情：
    // 1. 设置退出标志
    // 2. 主动关闭 listen_fd，让阻塞中的 accept 尽快返回
    g_stop = 1;
    if (g_listen_fd != -1) {
        close(g_listen_fd);
        g_listen_fd = -1;
    }
}

/*
 * 函数名：server_prepare_user_dirs
 * 功能：为配置文件中的每个用户准备独立目录。
 * 参数：无。
 * 返回值：成功返回 0，失败返回 -1。
 */
static int server_prepare_user_dirs(void)
{
    int i = 0;
    char user_dir[MAX_PATH_SIZE] = {0};

    // 先确保公共的用户根目录存在，例如 ./data/users
    if (ensure_dir(config_get_data_root()) != 0) {
        return -1;
    }

    // 再根据 users.conf 中的每个用户名，逐个创建个人目录。
    // 第一阶段虽然是“伪登录”，但目录隔离要先做好，
    // 这样不同用户的文件不会混在一起。
    for (i = 0; i < config_get_user_count(); ++i) {
        snprintf(user_dir, sizeof(user_dir), "%s/%s", config_get_data_root(), config_get_user_name(i));
        if (ensure_dir(user_dir) != 0) {
            return -1;
        }
    }

    return 0;
}

/*
 * 函数名：server_init_env
 * 功能：初始化服务端运行环境。
 * 参数：无。
 * 返回值：成功返回 0，失败返回 -1。
 */
static int server_init_env(void)
{
    // 初始化顺序是有讲究的：
    // 1. 先读主配置，拿到日志路径、端口、线程数等信息
    // 2. 再读用户配置
    // 3. 初始化日志，方便后面的错误能写入日志文件
    // 4. 创建数据目录
    // 5. 最后初始化线程池
    if (config_load(CONFIG_FILE_PATH) != 0) {
        return -1;
    }
    if (config_load_users(USER_FILE_PATH) != 0) {
        return -1;
    }
    if (log_init(config_get_logfile(), config_get_loglevel()) != 0) {
        return -1;
    }
    if (ensure_dir("log") != 0) {
        return -1;
    }
    if (server_prepare_user_dirs() != 0) {
        return -1;
    }
    if (thread_pool_init(&g_pool, config_get_thread_num()) != 0) {
        return -1;
    }

    return 0;
}

/*
 * 函数名：server_main_loop
 * 功能：主线程循环接收新连接，并投递给线程池。
 * 参数：无。
 * 返回值：无。
 */
static void server_main_loop(void)
{
    // 主线程只做两件事：
    // 1. accept 新连接
    // 2. 把连接丢进线程池
    // 真正的业务处理放到工作线程里完成，这样主线程可以持续接新客户。
    while (!g_stop) {
        int client_fd = -1;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        char client_ip[64] = {0};

        // accept 默认是阻塞的：
        // 没有新连接时，主线程就停在这里等待。
        client_fd = accept(g_listen_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            // 如果是我们主动退出导致的 close(listen_fd)，这里直接结束循环。
            if (g_stop) {
                break;
            }
            // 如果是被信号中断，就重新进下一轮等待。
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        // 把客户端网络地址转成人能看懂的文本形式，便于日志记录。
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        LOG_INFO("client connected: %s:%d", client_ip, ntohs(client_addr.sin_port));

        // accept 只是拿到连接，真正处理这个连接的整个会话
        // 交给线程池中的某个工作线程来做。
        if (thread_pool_add_task(&g_pool, client_fd) != 0) {
            LOG_ERROR("thread pool add task failed");
            close(client_fd);
        }
    }
}

/*
 * 函数名：main
 * 功能：服务端程序入口，完成初始化、监听和退出清理。
 * 参数：无。
 * 返回值：成功返回 0，失败返回非 0。
 */
int main(void)
{
    // 注册 Ctrl+C 的信号处理函数，便于做相对完整的退出清理。
    signal(SIGINT, server_signal_handler);

    if (server_init_env() != 0) {
        fprintf(stderr, "server init failed\n");
        return 1;
    }

    // 监听套接字需要放到环境初始化后，
    // 因为 IP/端口来自配置文件。
    g_listen_fd = socket_init_listen(config_get_ip(), config_get_port());
    if (g_listen_fd == -1) {
        thread_pool_destroy(&g_pool);
        log_close();
        return 1;
    }

    LOG_INFO("server start, ip=%s port=%d thread_num=%d",
             config_get_ip(),
             config_get_port(),
             config_get_thread_num());

    // 进入服务端主循环，直到收到退出信号。
    server_main_loop();

    // 下面是退出清理逻辑：
    // 1. 关闭监听 fd
    // 2. 通知线程池退出并回收线程
    // 3. 关闭日志
    if (g_listen_fd != -1) {
        close(g_listen_fd);
    }
    thread_pool_destroy(&g_pool);
    LOG_INFO("server stop");
    log_close();
    return 0;
}
