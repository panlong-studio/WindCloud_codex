#include "handle.h"
#include "config.h"
#include "file_transfer.h"
#include "log.h"
#include "protocol.h"

/*
 * 函数名：handle_send_error
 * 功能：向客户端发送错误文本。
 * 参数：
 *   session - 当前会话。
 *   msg     - 错误信息。
 * 返回值：成功返回 0，失败返回 -1。
 */
static int handle_send_error(session_t *session, const char *msg)
{
    // 服务端统一通过字符串响应把错误原因告诉客户端，
    // 这样客户端终端上能直接看到清晰提示。
    return protocol_send_string(session->client_fd, msg);
}

/*
 * 函数名：handle_need_arg
 * 功能：检查命令是否带了参数。
 * 参数：
 *   session - 当前会话。
 *   arg     - 命令参数。
 *   usage   - 用法提示。
 * 返回值：
 *   参数存在返回 1，参数缺失返回 0。
 */
static int handle_need_arg(session_t *session, const char *arg, const char *usage)
{
    if (arg == NULL || arg[0] == '\0') {
        // 某些命令例如 cd/mkdir/rm/puts/gets 必须带参数，
        // 如果用户没带参数，就直接返回用法提示。
        handle_send_error(session, usage);
        return 0;
    }
    return 1;
}

/*
 * 函数名：handle_request
 * 功能：处理单个客户端连接的完整请求循环。
 * 参数：client_fd - 当前客户端套接字。
 * 返回值：无。
 */
void handle_request(int client_fd)
{
    session_t session;
    char input[MAX_LINE_SIZE] = {0};

    // 一个 client_fd 对应一个 session。
    // 从这里开始，后续这个连接上的所有命令都围绕 session 进行。
    session_init(&session, client_fd);

    while (1) {
        char cmd[MAX_CMD_SIZE] = {0};
        char arg[MAX_ARG_SIZE] = {0};
        int count = 0;
        int ret = 0;

        // 协议层：先收长度，再收完整字符串。
        ret = protocol_recv_string(client_fd, input, sizeof(input));
        if (ret < 0) {
            if (session.is_login) {
                LOG_INFO("user=%s disconnected", session.user_name);
            } else {
                LOG_INFO("client_fd=%d disconnected before login", client_fd);
            }
            break;
        }

        // 把一行命令拆成“命令字 + 参数”。
        count = split_command(input, cmd, arg);
        if (count <= 0) {
            if (protocol_send_string(client_fd, "ERROR invalid command") != 0) {
                break;
            }
            continue;
        }

        // 登录前后日志格式略有区别：
        // 登录后更关心 user_name，登录前只能记录 client_fd。
        if (session.is_login) {
            LOG_INFO("user=%s cmd=%s arg=%s", session.user_name, cmd, arg);
        } else {
            LOG_INFO("client_fd=%d cmd=%s arg=%s", client_fd, cmd, arg);
        }

        // 整个 handle_request 的核心就是“命令分发”：
        // 根据 cmd 的内容，把请求转交给对应处理函数。
        if (strcmp(cmd, "quit") == 0) {
            protocol_send_string(client_fd, "OK bye");
            break;
        } else if (strcmp(cmd, "login") == 0) {
            if (handle_login(&session, arg) != 0) {
                break;
            }
        } else {
            // 除 login / quit 之外，其他命令都要求先登录。
            if (session.is_login == 0) {
                if (protocol_send_string(client_fd, "ERROR please login first") != 0) {
                    break;
                }
                continue;
            }

            if (strcmp(cmd, "pwd") == 0) {
                if (handle_pwd(&session) != 0) {
                    break;
                }
            } else if (strcmp(cmd, "ls") == 0) {
                if (handle_ls(&session) != 0) {
                    break;
                }
            } else if (strcmp(cmd, "cd") == 0) {
                if (handle_cd(&session, arg) != 0) {
                    break;
                }
            } else if (strcmp(cmd, "mkdir") == 0) {
                if (handle_mkdir(&session, arg) != 0) {
                    break;
                }
            } else if (strcmp(cmd, "rm") == 0) {
                if (handle_rm(&session, arg) != 0) {
                    break;
                }
            } else if (strcmp(cmd, "puts") == 0) {
                if (handle_puts(&session, arg) != 0) {
                    break;
                }
            } else if (strcmp(cmd, "gets") == 0) {
                if (handle_gets(&session, arg) != 0) {
                    break;
                }
            } else {
                if (protocol_send_string(client_fd, "ERROR invalid command") != 0) {
                    break;
                }
            }
        }
    }
}

/*
 * 函数名：handle_login
 * 功能：处理登录命令。
 * 参数：
 *   session - 当前会话。
 *   arg     - 登录用户名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_login(session_t *session, const char *arg)
{
    if (!handle_need_arg(session, arg, "ERROR usage: login <user_name>")) {
        return 0;
    }

    // 为了让第一期实现简单稳定，这里限制用户名只能是简单名字，
    // 不允许出现空格、斜杠等容易把路径搞乱的字符。
    if (!is_valid_name(arg)) {
        return handle_send_error(session, "ERROR invalid user name");
    }

    if (session->is_login) {
        return handle_send_error(session, "ERROR already login");
    }

    // 真正的“伪登录”逻辑在 session_login 里：
    // 检查用户是否存在，并初始化用户目录。
    if (session_login(session, arg) != 0) {
        LOG_ERROR("login failed, user=%s", arg);
        return handle_send_error(session, "ERROR user not found");
    }

    LOG_INFO("login success, user=%s", session->user_name);
    return protocol_send_string(session->client_fd, "OK login success");
}

/*
 * 函数名：handle_pwd
 * 功能：把当前虚拟路径返回给客户端。
 * 参数：session - 当前会话。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_pwd(session_t *session)
{
    // pwd 最简单，直接返回当前维护在 session 里的虚拟路径。
    return protocol_send_string(session->client_fd, session->current_path);
}

/*
 * 函数名：handle_ls
 * 功能：列出当前目录下的文件和目录名称。
 * 参数：session - 当前会话。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_ls(session_t *session)
{
    char real_path[MAX_PATH_SIZE] = {0};
    char result[MAX_RESPONSE_SIZE] = {0};
    size_t used = 0;
    DIR *dir = NULL;
    struct dirent *entry = NULL;

    // 先把当前虚拟路径转换成真实目录路径。
    if (session_build_current_real_path(session, real_path, sizeof(real_path)) != 0) {
        return handle_send_error(session, "ERROR get current path failed");
    }

    // 打开目录，逐项读取目录内容。
    dir = opendir(real_path);
    if (dir == NULL) {
        return handle_send_error(session, "ERROR open directory failed");
    }

    while ((entry = readdir(dir)) != NULL) {
        int written = 0;

        // "." 和 ".." 是系统默认目录项，通常不展示给客户端。
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 用一个大字符串把目录内容拼起来，客户端收到后直接打印即可。
        written = snprintf(result + used, sizeof(result) - used, "%s\n", entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(result) - used) {
            // 如果结果太长，这里直接停止拼接，避免数组越界。
            break;
        }
        used += (size_t)written;
    }

    closedir(dir);

    if (used == 0) {
        // 空目录单独给一个友好提示，比返回空字符串更直观。
        strcpy(result, "当前目录为空");
    }

    return protocol_send_string(session->client_fd, result);
}

/*
 * 函数名：handle_cd
 * 功能：处理目录切换命令。
 * 参数：
 *   session - 当前会话。
 *   arg     - 目录参数，可以是目录名或 ".."。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_cd(session_t *session, const char *arg)
{
    char real_path[MAX_PATH_SIZE] = {0};
    struct stat st;

    if (!handle_need_arg(session, arg, "ERROR usage: cd <dir_name>")) {
        return 0;
    }

    // 第一阶段支持 cd .. 返回上一级。
    if (strcmp(arg, "..") == 0) {
        session_cd_parent(session);
        return protocol_send_string(session->client_fd, "OK cd success");
    }

    // 目录名同样做简单合法性校验，避免路径被恶意拼坏。
    if (!is_valid_name(arg)) {
        return handle_send_error(session, "ERROR invalid directory name");
    }

    // 拼接目标真实路径后，用 stat 检查它是否存在且是不是目录。
    if (session_build_target_path(session, arg, real_path, sizeof(real_path)) != 0) {
        return handle_send_error(session, "ERROR build path failed");
    }

    if (stat(real_path, &st) == -1 || !S_ISDIR(st.st_mode)) {
        return handle_send_error(session, "ERROR directory not exist");
    }

    // 检查通过后，才真正更新 session 中保存的当前虚拟路径。
    session_cd_child(session, arg);
    return protocol_send_string(session->client_fd, "OK cd success");
}

/*
 * 函数名：handle_mkdir
 * 功能：在当前目录下创建一级新目录。
 * 参数：
 *   session - 当前会话。
 *   arg     - 目录名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_mkdir(session_t *session, const char *arg)
{
    char real_path[MAX_PATH_SIZE] = {0};

    if (!handle_need_arg(session, arg, "ERROR usage: mkdir <dir_name>")) {
        return 0;
    }

    // mkdir 只允许创建“当前目录下的一级目录”。
    if (!is_valid_name(arg)) {
        return handle_send_error(session, "ERROR invalid directory name");
    }

    if (session_build_target_path(session, arg, real_path, sizeof(real_path)) != 0) {
        return handle_send_error(session, "ERROR build path failed");
    }

    // 如果目录已经存在，给更具体的提示，便于客户端理解失败原因。
    if (mkdir(real_path, 0775) == -1) {
        if (errno == EEXIST) {
            return handle_send_error(session, "ERROR directory already exist");
        }
        return handle_send_error(session, "ERROR mkdir failed");
    }

    LOG_INFO("user=%s mkdir=%s", session->user_name, arg);
    return protocol_send_string(session->client_fd, "OK mkdir success");
}

/*
 * 函数名：handle_rm
 * 功能：删除当前目录下的普通文件。
 * 参数：
 *   session - 当前会话。
 *   arg     - 文件名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_rm(session_t *session, const char *arg)
{
    char real_path[MAX_PATH_SIZE] = {0};
    struct stat st;

    if (!handle_need_arg(session, arg, "ERROR usage: rm <file_name>")) {
        return 0;
    }

    // 第一阶段 rm 只处理普通文件，不支持删目录。
    if (!is_valid_name(arg)) {
        return handle_send_error(session, "ERROR invalid file name");
    }

    if (session_build_target_path(session, arg, real_path, sizeof(real_path)) != 0) {
        return handle_send_error(session, "ERROR build path failed");
    }

    // 先判断目标是否存在。
    if (stat(real_path, &st) == -1) {
        return handle_send_error(session, "ERROR file not exist");
    }

    // 如果它是目录，直接拒绝。
    if (S_ISDIR(st.st_mode)) {
        return handle_send_error(session, "ERROR cannot remove directory");
    }

    if (unlink(real_path) == -1) {
        return handle_send_error(session, "ERROR remove file failed");
    }

    LOG_INFO("user=%s rm=%s", session->user_name, arg);
    return protocol_send_string(session->client_fd, "OK rm success");
}

/*
 * 函数名：handle_puts
 * 功能：处理客户端上传文件请求。
 * 参数：
 *   session - 当前会话。
 *   arg     - 文件名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_puts(session_t *session, const char *arg)
{
    char real_path[MAX_PATH_SIZE] = {0};

    if (!handle_need_arg(session, arg, "ERROR usage: puts <file_name>")) {
        return 0;
    }

    // puts 的第一阶段流程：
    // 1. 检查文件名是否合法
    // 2. 给客户端一个“可以开始上传”的确认消息
    // 3. 再真正进入文件接收流程
    if (!is_valid_name(arg)) {
        return handle_send_error(session, "ERROR invalid file name");
    }

    if (session_build_target_path(session, arg, real_path, sizeof(real_path)) != 0) {
        return handle_send_error(session, "ERROR build path failed");
    }

    // 这条 OK 响应很重要，它相当于协议里的“握手”：
    // 只有客户端收到了这条消息，才会继续发送文件大小和文件内容。
    if (protocol_send_string(session->client_fd, "OK ready to receive file") != 0) {
        return -1;
    }

    if (server_recv_file(session->client_fd, real_path) != 0) {
        LOG_ERROR("user=%s upload failed, file=%s", session->user_name, arg);
        return handle_send_error(session, "ERROR upload failed");
    }

    LOG_INFO("user=%s upload success, file=%s", session->user_name, arg);
    return protocol_send_string(session->client_fd, "OK upload success");
}

/*
 * 函数名：handle_gets
 * 功能：处理客户端下载文件请求。
 * 参数：
 *   session - 当前会话。
 *   arg     - 文件名。
 * 返回值：成功返回 0，失败返回 -1。
 */
int handle_gets(session_t *session, const char *arg)
{
    char real_path[MAX_PATH_SIZE] = {0};
    struct stat st;

    if (!handle_need_arg(session, arg, "ERROR usage: gets <file_name>")) {
        return 0;
    }

    // gets 的处理顺序和 puts 类似：
    // 先检查，再发“准备发送文件”的确认消息，最后真的发文件。
    if (!is_valid_name(arg)) {
        return handle_send_error(session, "ERROR invalid file name");
    }

    if (session_build_target_path(session, arg, real_path, sizeof(real_path)) != 0) {
        return handle_send_error(session, "ERROR build path failed");
    }

    // 下载要求目标必须是普通文件。
    if (stat(real_path, &st) == -1 || !S_ISREG(st.st_mode)) {
        return handle_send_error(session, "ERROR file not exist");
    }

    if (protocol_send_string(session->client_fd, "OK ready to send file") != 0) {
        return -1;
    }

    if (server_send_file(session->client_fd, real_path) != 0) {
        LOG_ERROR("user=%s download failed, file=%s", session->user_name, arg);
        return -1;
    }

    LOG_INFO("user=%s download success, file=%s", session->user_name, arg);
    return 0;
}
