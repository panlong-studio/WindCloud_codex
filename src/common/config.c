#include "config.h"

/*
 * 说明：
 * 这里使用静态全局变量保存配置，方便服务端和客户端在任意模块中通过 getter 取值。
 * 第一阶段配置项很少，这种写法足够简单，也便于初学者理解。
 */
static config_data_t g_config;
static char g_users[MAX_USERS][MAX_NAME_SIZE];
static int g_user_count = 0;

/*
 * 函数名：config_parse_log_level
 * 功能：把日志级别字符串转换成整数常量。
 * 参数：level_str - 例如 "DEBUG"、"INFO"、"ERROR"。
 * 返回值：返回对应的日志级别整数。
 */
static int config_parse_log_level(const char *level_str)
{
    // 第一阶段只支持三种日志级别：
    // DEBUG、INFO、ERROR
    if (level_str == NULL) {
        return LOG_LEVEL_INFO;
    }

    if (strcmp(level_str, "DEBUG") == 0) {
        return LOG_LEVEL_DEBUG;
    }
    if (strcmp(level_str, "ERROR") == 0) {
        return LOG_LEVEL_ERROR;
    }
    return LOG_LEVEL_INFO;
}

/*
 * 函数名：config_set_default
 * 功能：设置默认配置，避免配置文件缺项时出现未初始化数据。
 * 参数：无。
 * 返回值：无。
 */
static void config_set_default(void)
{
    // 即使配置文件缺项，程序也可以依靠默认值继续跑起来，
    // 这样对初学者更友好，也更容易调试。
    memset(&g_config, 0, sizeof(g_config));
    strcpy(g_config.server_ip, "127.0.0.1");
    g_config.server_port = 12345;
    g_config.thread_num = 4;
    g_config.log_level = LOG_LEVEL_INFO;
    strcpy(g_config.log_file, "log/server.log");
    strcpy(g_config.data_root, "./data/users");
}

/*
 * 函数名：config_load
 * 功能：从主配置文件中读取 key=value 形式的配置项。
 * 参数：file_path - 主配置文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int config_load(const char *file_path)
{
    FILE *fp = NULL;
    char line[MAX_LINE_SIZE] = {0};

    // 每次 load 前先恢复默认值，再用配置文件里的内容覆盖它。
    config_set_default();

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror("fopen config file");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        char key[64] = {0};
        char value[MAX_PATH_SIZE] = {0};
        char *equal_pos = NULL;

        // 先做最基本的清洗：去掉换行和首尾空格。
        trim_newline(line);
        trim_space(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // 没有 '=' 的行不符合 key=value 约定，直接跳过。
        equal_pos = strchr(line, '=');
        if (equal_pos == NULL) {
            continue;
        }

        // "%[^=]" 的写法表示“读到等号前为止”。
        sscanf(line, "%63[^=]=%511[^\n]", key, value);
        trim_space(key);
        trim_space(value);

        // 按 key 名称分别写入配置结构体。
        if (strcmp(key, "server_ip") == 0) {
            strncpy(g_config.server_ip, value, sizeof(g_config.server_ip) - 1);
        } else if (strcmp(key, "server_port") == 0) {
            g_config.server_port = atoi(value);
        } else if (strcmp(key, "thread_num") == 0) {
            g_config.thread_num = atoi(value);
        } else if (strcmp(key, "log_level") == 0) {
            g_config.log_level = config_parse_log_level(value);
        } else if (strcmp(key, "log_file") == 0) {
            strncpy(g_config.log_file, value, sizeof(g_config.log_file) - 1);
        } else if (strcmp(key, "data_root") == 0) {
            strncpy(g_config.data_root, value, sizeof(g_config.data_root) - 1);
        }
    }

    fclose(fp);
    return 0;
}

/*
 * 函数名：config_load_users
 * 功能：从用户配置文件中读取所有允许登录的用户名。
 * 参数：file_path - 用户配置文件路径。
 * 返回值：成功返回 0，失败返回 -1。
 */
int config_load_users(const char *file_path)
{
    FILE *fp = NULL;
    char line[MAX_LINE_SIZE] = {0};

    // 重新加载前，先把用户数量归零。
    g_user_count = 0;

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror("fopen users file");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        trim_newline(line);
        trim_space(line);

        // 支持空行和注释行，便于用户手工维护 users.conf。
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        // 防止数组越界；第一阶段直接截断后续用户。
        if (g_user_count >= MAX_USERS) {
            break;
        }

        // 每行一个用户名，按顺序放入静态数组中。
        strncpy(g_users[g_user_count], line, MAX_NAME_SIZE - 1);
        g_users[g_user_count][MAX_NAME_SIZE - 1] = '\0';
        ++g_user_count;
    }

    fclose(fp);
    return 0;
}

/*
 * 函数名：config_get_ip
 * 功能：获取服务端监听 IP。
 * 参数：无。
 * 返回值：返回 IP 字符串指针。
 */
const char *config_get_ip(void)
{
    return g_config.server_ip;
}

/*
 * 函数名：config_get_port
 * 功能：获取服务端监听端口。
 * 参数：无。
 * 返回值：返回端口号整数。
 */
int config_get_port(void)
{
    return g_config.server_port;
}

/*
 * 函数名：config_get_thread_num
 * 功能：获取线程池线程数量。
 * 参数：无。
 * 返回值：返回线程数量。
 */
int config_get_thread_num(void)
{
    return g_config.thread_num;
}

/*
 * 函数名：config_get_loglevel
 * 功能：获取当前日志等级。
 * 参数：无。
 * 返回值：返回日志级别整数。
 */
int config_get_loglevel(void)
{
    return g_config.log_level;
}

/*
 * 函数名：config_get_logfile
 * 功能：获取日志文件路径。
 * 参数：无。
 * 返回值：返回日志文件路径字符串。
 */
const char *config_get_logfile(void)
{
    return g_config.log_file;
}

/*
 * 函数名：config_get_data_root
 * 功能：获取服务端用户数据根目录。
 * 参数：无。
 * 返回值：返回根目录字符串。
 */
const char *config_get_data_root(void)
{
    return g_config.data_root;
}

/*
 * 函数名：config_user_exists
 * 功能：检查用户名是否在伪登录用户列表中。
 * 参数：user_name - 要检查的用户名。
 * 返回值：存在返回 1，不存在返回 0。
 */
int config_user_exists(const char *user_name)
{
    int i = 0;

    if (user_name == NULL || user_name[0] == '\0') {
        return 0;
    }

    // 线性查找已经够第一阶段使用：
    // 用户数量很少，没必要为了这个场景引入更复杂的数据结构。
    for (i = 0; i < g_user_count; ++i) {
        if (strcmp(g_users[i], user_name) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * 函数名：config_get_user_count
 * 功能：获取当前加载到内存中的用户数量。
 * 参数：无。
 * 返回值：返回用户数量。
 */
int config_get_user_count(void)
{
    return g_user_count;
}

/*
 * 函数名：config_get_user_name
 * 功能：按下标获取用户名。
 * 参数：index - 用户下标。
 * 返回值：成功返回用户名字符串，失败返回 NULL。
 */
const char *config_get_user_name(int index)
{
    if (index < 0 || index >= g_user_count) {
        return NULL;
    }
    return g_users[index];
}
