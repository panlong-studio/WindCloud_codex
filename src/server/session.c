#include "session.h"
#include "config.h"

/*
 * 函数名：session_init
 * 功能：初始化一个新的客户端会话。
 * 参数：
 *   session   - 会话对象。
 *   client_fd - 客户端 fd。
 * 返回值：无。
 */
void session_init(session_t *session, int client_fd)
{
    // 新连接刚建立时，默认一定是“未登录”状态。
    memset(session, 0, sizeof(*session));
    session->client_fd = client_fd;
    session->is_login = 0;
    // 虚拟路径默认从根目录开始显示。
    strcpy(session->current_path, "/");
}

/*
 * 函数名：session_login
 * 功能：根据用户名完成伪登录，并初始化用户目录。
 * 参数：
 *   session   - 会话对象。
 *   user_name - 登录用户名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_login(session_t *session, const char *user_name)
{
    int ret = 0;

    // 这里只做第一期的“伪登录”：
    // 只检查用户名是否在 users.conf 中存在，不校验密码。
    if (!config_user_exists(user_name)) {
        return -1;
    }

    // 用户名记录到会话结构体里，后续日志和路径拼接都要用到。
    strncpy(session->user_name, user_name, sizeof(session->user_name) - 1);
    session->user_name[sizeof(session->user_name) - 1] = '\0';

    // user_root 是这个用户真实文件根目录，例如 ./data/users/alice
    snprintf(session->user_root, sizeof(session->user_root), "%s/%s", config_get_data_root(), session->user_name);
    ret = ensure_dir(session->user_root);
    if (ret != 0) {
        return -1;
    }

    // 登录成功后，会话状态切换到“已登录”，并把当前虚拟路径重置为根目录。
    session->is_login = 1;
    strcpy(session->current_path, "/");
    return 0;
}

/*
 * 函数名：session_build_current_real_path
 * 功能：把当前虚拟路径转换成真实物理路径。
 * 参数：
 *   session   - 会话对象。
 *   real_path - 输出真实路径缓冲区。
 *   size      - 缓冲区大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_build_current_real_path(const session_t *session, char *real_path, int size)
{
    // 这里是“虚拟路径 -> 真实路径”的核心转换函数。
    // 例如：
    //   user_root     = ./data/users/alice
    //   current_path  = /dir1
    // 转换结果：
    //   real_path     = ./data/users/alice/dir1
    if (session == NULL || real_path == NULL || size <= 0) {
        return -1;
    }

    if (session->is_login == 0) {
        return -1;
    }

    if (strcmp(session->current_path, "/") == 0) {
        // 当前在虚拟根目录时，真实路径就是用户根目录本身。
        snprintf(real_path, (size_t)size, "%s", session->user_root);
    } else {
        // 否则直接把 current_path 接到 user_root 后面。
        snprintf(real_path, (size_t)size, "%s%s", session->user_root, session->current_path);
    }

    return 0;
}

/*
 * 函数名：session_build_target_path
 * 功能：在当前目录下拼接目标文件或目录的真实路径。
 * 参数：
 *   session   - 会话对象。
 *   name      - 目标名字。
 *   real_path - 输出真实路径缓冲区。
 *   size      - 缓冲区大小。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_build_target_path(const session_t *session, const char *name, char *real_path, int size)
{
    char current_real_path[MAX_PATH_SIZE] = {0};

    // 先得到“当前目录”的真实路径，再在后面拼接目标名字。
    if (session_build_current_real_path(session, current_real_path, sizeof(current_real_path)) != 0) {
        return -1;
    }

    snprintf(real_path, (size_t)size, "%s/%s", current_real_path, name);
    return 0;
}

/*
 * 函数名：session_cd_child
 * 功能：把当前虚拟路径切换到下一级目录。
 * 参数：
 *   session  - 会话对象。
 *   dir_name - 子目录名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_cd_child(session_t *session, const char *dir_name)
{
    char temp[MAX_PATH_SIZE] = {0};
    int need_len = 0;

    if (session == NULL || dir_name == NULL) {
        return -1;
    }

    // 先做长度预估，避免后面 strcat 越界。
    if (strcmp(session->current_path, "/") == 0) {
        need_len = (int)strlen(dir_name) + 2;
    } else {
        need_len = (int)strlen(session->current_path) + (int)strlen(dir_name) + 2;
    }
    if (need_len >= (int)sizeof(temp)) {
        return -1;
    }

    // 当前在 / 时，进入子目录后的虚拟路径应为 /xxx；
    // 当前在 /a 时，进入子目录后的虚拟路径应为 /a/xxx。
    if (strcmp(session->current_path, "/") == 0) {
        strcpy(temp, "/");
        strcat(temp, dir_name);
    } else {
        strcpy(temp, session->current_path);
        strcat(temp, "/");
        strcat(temp, dir_name);
    }

    if (strlen(temp) >= sizeof(session->current_path)) {
        return -1;
    }

    // 最后再把新的虚拟路径写回 session。
    strcpy(session->current_path, temp);
    return 0;
}

/*
 * 函数名：session_cd_parent
 * 功能：把当前虚拟路径返回到上一级目录。
 * 参数：session - 会话对象。
 * 返回值：成功返回 0，失败返回 -1。
 */
int session_cd_parent(session_t *session)
{
    char *last_slash = NULL;

    if (session == NULL) {
        return -1;
    }

    // 已经在根目录时，再 cd .. 也还是留在根目录。
    if (strcmp(session->current_path, "/") == 0) {
        return 0;
    }

    // 从右往左找最后一个 '/'，把最后一级目录“截掉”。
    last_slash = strrchr(session->current_path, '/');
    if (last_slash == session->current_path) {
        strcpy(session->current_path, "/");
    } else if (last_slash != NULL) {
        *last_slash = '\0';
    }

    return 0;
}
