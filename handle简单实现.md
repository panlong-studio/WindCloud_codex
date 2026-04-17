逻辑简化：
1. **客户端只发字符串：** 比如发过来 `"cd mydir"` 或者 `"ls"`。
2. **服务器按字符串处理：** 用原生的 `sscanf` 或 `strtok` 把命令和参数拆开。
3. **返回结果也是字符串：** 也是用“先发长度，再发内容”的简单逻辑。

---

### 第一部分：头文件 `handle.h`

我们只需要向外暴露一个 `handle_request` 函数，供线程池里的 worker 调用。

```c
#ifndef __HANDLE_H__
#define __HANDLE_H__

#include <my_header.h> // 包含你们自己封装的基础头文件

// 假设服务器根目录下有一个 upload 文件夹，用来统一存放客户端文件
#define SERVER_BASE_DIR "./upload"

// 处理客户端请求的总入口函数
void handle_request(int client_fd);

// ----------------- 以下是内部处理函数的声明 -----------------
// (只在 handle.c 内部使用，其实可以不写进头文件，但写出来更清晰)
void handle_pwd(int client_fd, char *current_path);
void handle_ls(int client_fd, char *current_path);
void handle_cd(int client_fd, char *current_path, char *arg);
void handle_mkdir(int client_fd, char *current_path, char *arg);
void handle_rm(int client_fd, char *current_path, char *arg);
void handle_gets(int client_fd, char *current_path, char *arg);
void handle_puts(int client_fd, char *current_path, char *arg);

#endif
```

---

### 第二部分：核心实现 `handle.c`

这里面的每一行代码我都加了详细的注释，逻辑完全符合你们第一期项目的需求。

```c
#include "handle.h"
#include <dirent.h> // 处理目录(opendir, readdir)需要用到

// ==========================================================
// 辅助函数：把客户端的信息发回去
// 为了防止粘包，我们依旧采用你们熟悉的 "先发长度，再发内容" 的方式
// ==========================================================
void send_response(int client_fd, const char *msg) {
    int len = strlen(msg);
    // 1. 先发送字符串的长度
    send(client_fd, &len, sizeof(int), MSG_NOSIGNAL);
    // 2. 再发送字符串的实际内容
    send(client_fd, msg, len, MSG_NOSIGNAL);
}

// ==========================================================
// 核心总控函数：被工作线程(worker)调用
// ==========================================================
void handle_request(int client_fd) {
    // 1. 记录这个客户端当前所在的“虚拟目录”
    // 刚连接时，默认在根目录（即只显示一个 "/"）
    char current_path[512] = "/"; 

    printf("[工作线程] 开始为一个新客户端(fd=%d)提供服务...\n", client_fd);

    // 2. 进入死循环，不断接收这个客户端发来的命令，直到客户端断开
    while (1) {
        int cmd_len = 0;
        // 第一步：接收客户端发来的命令长度
        int ret = recv(client_fd, &cmd_len, sizeof(int), MSG_WAITALL);
        
        // 如果 recv 返回 0，说明客户端主动关闭了终端或断开了连接
        if (ret == 0) {
            printf("[工作线程] 客户端(fd=%d)已断开连接。\n", client_fd);
            break; // 跳出 while 循环，结束服务
        }

        // 第二步：根据收到的长度，接收具体的命令字符串
        char cmd_buf[1024] = {0}; // 用来装完整的命令字符串，比如 "cd abc"
        recv(client_fd, cmd_buf, cmd_len, MSG_WAITALL);
        
        printf("[工作线程] 收到客户端发送的命令: %s\n", cmd_buf);

        // 第三步：解析命令字符串 (拆分出 指令 和 参数)
        char cmd[32] = {0};  // 装指令，比如 "cd", "ls"
        char arg[256] = {0}; // 装参数，比如 "abc", "a.txt"
        
        // sscanf 是极其好用的字符串提取工具。
        // "%s %s" 表示按空格提取两个字符串。
        // 如果客户端只发了 "ls"，那么 sscanf 只会提取到 cmd 里，arg 保持为空
        sscanf(cmd_buf, "%s %s", cmd, arg);

        // 第四步：根据不同的指令，调用不同的处理函数
        if (strcmp(cmd, "pwd") == 0) {
            handle_pwd(client_fd, current_path);
        } 
        else if (strcmp(cmd, "ls") == 0) {
            handle_ls(client_fd, current_path);
        } 
        else if (strcmp(cmd, "cd") == 0) {
            handle_cd(client_fd, current_path, arg);
        } 
        else if (strcmp(cmd, "mkdir") == 0) {      // <---- 补充这里！
            handle_mkdir(client_fd, current_path, arg);
        }
        else if (strcmp(cmd, "rm") == 0) {
            handle_rm(client_fd, current_path, arg);
        } 
        else if (strcmp(cmd, "gets") == 0) {
            handle_gets(client_fd, current_path, arg);
        } 
        else if (strcmp(cmd, "puts") == 0) {
            handle_puts(client_fd, current_path, arg);
        } 
        else {
            // 如果是乱敲的命令，给个错误提示
            send_response(client_fd, "无效的命令，请重新输入！\n");
        }
    }

    // 循环结束（客户端断开），记得释放文件描述符
    close(client_fd);
}


// ==========================================================
// 具体命令 1：处理 pwd
// ==========================================================
void handle_pwd(int client_fd, char *current_path) {
    // pwd 最简单，直接把存下来的 current_path 字符串发给客户端即可
    send_response(client_fd, current_path);
}

// ==========================================================
// 具体命令 2：处理 ls
// ==========================================================
void handle_ls(int client_fd, char *current_path) {
    // 1. 拼接真实的物理路径。
    // 服务器真正的文件夹是 ./upload，我们要加上客户端当前的虚拟路径
    // 比如 current_path 是 "/abc"，真实路径就是 "./upload/abc"
    char real_path[1024] = {0};
    sprintf(real_path, "%s%s", SERVER_BASE_DIR, current_path);

    // 2. 打开这个目录
    DIR *dir = opendir(real_path);
    if (dir == NULL) {
        send_response(client_fd, "目录打开失败！\n");
        return;
    }

    // 3. 遍历目录，把所有文件名拼接到一个大字符串里
    char result[2048] = {0}; // 用来存放所有文件名的结果
    struct dirent *entry;
    
    // readdir 会逐个读取目录里的文件
    while ((entry = readdir(dir)) != NULL) {
        // 过滤掉隐藏文件 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        // 把文件名拼接到 result 后面，加个换行符或空格隔开
        strcat(result, entry->d_name);
        strcat(result, "  "); 
    }
    closedir(dir);

    // 4. 如果是空目录，给个提示
    if (strlen(result) == 0) {
        strcpy(result, "当前目录为空。");
    }

    // 5. 把拼接好的大字符串发给客户端
    send_response(client_fd, result);
}

// ==========================================================
// 具体命令 3：处理 cd
// ==========================================================
void handle_cd(int client_fd, char *current_path, char *arg) {
    // 客户端如果没有输入参数，默认报错
    if (strlen(arg) == 0) {
        send_response(client_fd, "用法: cd <目录名>\n");
        return;
    }

    // 暂时用一种极简的“字符串拼接”法处理路径。
    // 如果 cd .. ，咱们简单提示一下暂不支持，或者直接重置回根目录(降低复杂度)
    if (strcmp(arg, "..") == 0) {
        strcpy(current_path, "/"); // 为了代码简单，cd .. 直接回根目录
        send_response(client_fd, "已返回根目录\n");
        return;
    }

    // 拼接测试真实路径： "./upload" + 当前路径 + "/" + 目标文件夹
    char test_real_path[1024] = {0};
    if (strcmp(current_path, "/") == 0) {
        sprintf(test_real_path, "%s/%s", SERVER_BASE_DIR, arg);
    } else {
        sprintf(test_real_path, "%s%s/%s", SERVER_BASE_DIR, current_path, arg);
    }

    // 测试这个文件夹是否存在 (尝试打开它)
    DIR *dir = opendir(test_real_path);
    if (dir == NULL) {
        send_response(client_fd, "目录不存在！\n");
    } else {
        closedir(dir); // 测试成功，记得关掉
        
        // 更新客户端的“虚拟路径”字符串
        if (strcmp(current_path, "/") == 0) {
            sprintf(current_path, "/%s", arg);
        } else {
            strcat(current_path, "/");
            strcat(current_path, arg);
        }
        send_response(client_fd, "进入目录成功\n");
    }
}

// ==========================================================
// 具体命令 4：处理 mkdir (创建目录)
// ==========================================================
void handle_mkdir(int client_fd, char *current_path, char *arg) {
    // 1. 检查客户端是否输入了要创建的目录名字
    if (strlen(arg) == 0) {
        send_response(client_fd, "用法: mkdir <目录名>\n");
        return;
    }

    // 2. 拼接服务器上的真实物理路径
    char real_path[1024] = {0};
    if (strcmp(current_path, "/") == 0) {
        // 如果当前在根目录，直接拼接: ./upload/新目录名
        sprintf(real_path, "%s/%s", SERVER_BASE_DIR, arg);
    } else {
        // 如果在子目录，拼接: ./upload/当前路径/新目录名
        sprintf(real_path, "%s%s/%s", SERVER_BASE_DIR, current_path, arg);
    }

    // 3. 调用系统函数 mkdir 创建目录
    // 第二个参数 0775 是权限设置（类似 chmod 775），
    // 意味着拥有者和同组用户可读可写可执行，其他用户可读可执行。
    // 注意：如果是老代码报错，可能需要包含头文件 <sys/stat.h> 和 <sys/types.h>
    if (mkdir(real_path, 0775) == 0) {
        send_response(client_fd, "目录创建成功！\n");
    } else {
        // 如果目录已经存在，或者没有权限创建，mkdir 会返回 -1
        send_response(client_fd, "目录创建失败（可能已存在或无权限）。\n");
    }
}

// ==========================================================
// 具体命令 5：处理 rm
// ==========================================================
void handle_rm(int client_fd, char *current_path, char *arg) {
    if (strlen(arg) == 0) {
        send_response(client_fd, "用法: rm <文件名>\n");
        return;
    }

    // 拼接真实物理路径
    char real_path[1024] = {0};
    if (strcmp(current_path, "/") == 0) {
        sprintf(real_path, "%s/%s", SERVER_BASE_DIR, arg);
    } else {
        sprintf(real_path, "%s%s/%s", SERVER_BASE_DIR, current_path, arg);
    }

    // unlink 是 Linux 中删除文件的标准系统调用
    if (unlink(real_path) == 0) {
        send_response(client_fd, "文件删除成功！\n");
    } else {
        send_response(client_fd, "文件删除失败（可能文件不存在或无权限）。\n");
    }
}

// ==========================================================
// 具体命令 6：处理 gets (下载) —— 结合你之前的 sendfile
// ==========================================================
void handle_gets(int client_fd, char *current_path, char *arg) {
    if (strlen(arg) == 0) {
        send_response(client_fd, "用法: gets <文件名>\n");
        return;
    }

    // 1. 拼接并找到服务器上的真实文件
    char real_path[1024] = {0};
    if (strcmp(current_path, "/") == 0) {
        sprintf(real_path, "%s/%s", SERVER_BASE_DIR, arg);
    } else {
        sprintf(real_path, "%s%s/%s", SERVER_BASE_DIR, current_path, arg);
    }

    // 2. 尝试打开文件
    int file_fd = open(real_path, O_RDONLY);
    if (file_fd == -1) {
        // 如果文件不存在，发送约定好的错误码（比如发送文件大小为 0）
        off_t err_size = 0;
        send(client_fd, &err_size, sizeof(off_t), MSG_NOSIGNAL);
        return;
    }

    // 3. 获取文件大小
    struct stat st;
    fstat(file_fd, &st);
    off_t file_size = st.st_size;

    // 4. 发送文件总大小给客户端
    send(client_fd, &file_size, sizeof(off_t), MSG_NOSIGNAL);

    // 5. 零拷贝技术：将文件直接从内核发给网卡
    sendfile(client_fd, file_fd, NULL, file_size);
    
    printf("[工作线程] 文件 %s 发送完毕\n", arg);
    close(file_fd);
}

// ==========================================================
// 具体命令 7：处理 puts (上传) —— 结合你之前的 mmap
// ==========================================================
void handle_puts(int client_fd, char *current_path, char *arg) {
    if (strlen(arg) == 0) {
        printf("[工作线程] 客户端上传未指定文件名\n");
        return; 
    }

    // 1. 客户端那边会先发来这个文件有多大，我们先接收大小
    off_t file_size = 0;
    int ret = recv(client_fd, &file_size, sizeof(off_t), MSG_WAITALL);
    if (ret <= 0 || file_size == 0) {
        printf("客户端取消上传或文件为空\n");
        return;
    }

    // 2. 拼接服务器要保存的真实路径
    char real_path[1024] = {0};
    if (strcmp(current_path, "/") == 0) {
        sprintf(real_path, "%s/%s", SERVER_BASE_DIR, arg);
    } else {
        sprintf(real_path, "%s%s/%s", SERVER_BASE_DIR, current_path, arg);
    }

    // 3. 在服务器创建这个同名文件，准备写入
    int file_fd = open(real_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        perror("创建上传文件失败");
        return;
    }

    // 4. 将文件扩充到目标大小，防止 mmap 越界
    ftruncate(file_fd, file_size);

    // 5. 将文件映射到内存 (这段逻辑你们原先在 client.c 写过，现在搬到服务器用)
    char *p = (char *)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
    
    // 6. 接收网络数据，直接写进内存映射区，底层自动同步到磁盘
    recv(client_fd, p, file_size, MSG_WAITALL);

    // 7. 收尾：解除映射，关闭文件
    munmap(p, file_size);
    close(file_fd);
    
    printf("[工作线程] 文件 %s 成功接收并保存到服务器\n", arg);
}
```

### 三、 客户端配合修改建议 

写完这个模块，你需要和写**客户端(`client`)**的同学明确一下交互的“规矩”（接口说明文档）：

*   **对于普通命令（ls, cd, pwd, rm, mkdir）**
    1. 客户端读取键盘输入（比如 `cd xyz`）。
    2. 客户端计算字符串长度 `len`，先发送 `len`，再发 `"cd xyz"` 字符串。
    3. 客户端马上调用 `recv` 等待服务器的回复。服务器也是先发回复内容的 `len`，再发回复的字符串内容，客户端收到后 `printf` 打印即可。

*   **对于下载文件命令（gets xxx）**
    1. 客户端先发字符串 `"gets xxx"`。
    2. 客户端直接 `recv` 接收一个 `off_t` 的文件大小。
    3. 如果文件大小是 0，说明服务器没这个文件，退出下载。
    4. 如果大于 0，客户端使用 `ftruncate` 和 `mmap`，准备接收文件（和你们原本代码一模一样）。

*   **对于上传文件命令（puts xxx）**
    1. 客户端先发字符串 `"puts xxx"`。
    2. 客户端判断本地有没有这个文件，有的话用 `stat` 算出大小。
    3. 客户端把 `off_t` 的文件大小发过去。
    4. 客户端用 `sendfile` 把文件发给服务器（利用你们学过的零拷贝）。
