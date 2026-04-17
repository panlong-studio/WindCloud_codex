# Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 完成网盘项目第一期的客户端与服务端基础版本，实现伪登录、多用户目录隔离、基础命令交互、普通文件上传下载、配置加载和日志记录。

**Architecture:** 项目采用顶层 `include/` + `src/` 的 C 工程结构。服务端复用参考代码中的线程池模型，每个工作线程负责一个客户端会话；客户端使用简单命令行交互，通过长度加字符串的文本协议与服务端通信，上传下载在命令确认后再传文件数据。

**Tech Stack:** C、POSIX Socket、pthread、epoll、标准文件与目录操作接口、Makefile

---

### Task 1: 建立项目目录与配置基线

**Files:**
- Create: `config/mycloud.conf`
- Create: `config/users.conf`
- Create: `include/`
- Create: `src/common/`
- Create: `src/server/`
- Create: `src/client/`
- Create: `bin/`
- Create: `build/`
- Create: `log/`
- Create: `data/users/`
- Create: `download/`

- [ ] **Step 1: 创建第一期目录结构**

Run: `mkdir -p bin build config docs log data/users download include src/common src/server src/client`
Expected: 所有目录创建成功

- [ ] **Step 2: 写入主配置文件**

内容包含：
- `server_ip`
- `server_port`
- `thread_num`
- `log_level`
- `log_file`
- `data_root`

- [ ] **Step 3: 写入伪用户配置文件**

示例用户：
- `alice`
- `bob`
- `test`

### Task 2: 修订第一期项目描述文档

**Files:**
- Modify: `项目描述文档.md`

- [ ] **Step 1: 删除超出第一期范围的数据库和第二期内容混入**

- [ ] **Step 2: 按确认后的真实目录结构重写目录说明**

- [ ] **Step 3: 补充多用户伪登录、会话模块、协议模块、文件传输模块**

- [ ] **Step 4: 统一配置文件名为 `mycloud.conf`，补充 `users.conf` 说明**

### Task 3: 先写构建与头文件骨架

**Files:**
- Create: `Makefile`
- Create: `include/common.h`
- Create: `include/config.h`
- Create: `include/log.h`
- Create: `include/protocol.h`
- Create: `include/queue.h`
- Create: `include/thread_pool.h`
- Create: `include/socket_init.h`
- Create: `include/session.h`
- Create: `include/handle.h`
- Create: `include/file_transfer.h`
- Create: `include/client.h`

- [ ] **Step 1: 写顶层 Makefile**

- [ ] **Step 2: 定义公共宏、结构体和函数声明**

- [ ] **Step 3: 确保头文件边界与职责清晰**

### Task 4: 为公共基础模块先写失败验证，再实现

**Files:**
- Create: `src/common/config.c`
- Create: `src/common/log.c`
- Create: `src/common/protocol.c`
- Create: `src/common/utils.c`

- [ ] **Step 1: 先编写一个最小编译验证目标**

Run: `make`
Expected: 在实现缺失前编译失败，提示未定义函数或缺失源文件

- [ ] **Step 2: 实现配置模块**

- [ ] **Step 3: 实现日志模块**

- [ ] **Step 4: 实现协议模块**

- [ ] **Step 5: 实现公共工具模块**

- [ ] **Step 6: 重新执行 `make`，确认公共模块可编译**

### Task 5: 先为服务端主链路写失败验证，再实现服务端模块

**Files:**
- Create: `src/server/main.c`
- Create: `src/server/socket_init.c`
- Create: `src/server/queue.c`
- Create: `src/server/thread_pool.c`
- Create: `src/server/worker.c`
- Create: `src/server/session.c`
- Create: `src/server/handle.c`
- Create: `src/server/file_transfer.c`

- [ ] **Step 1: 扩充 Makefile 让服务端目标进入编译**

Run: `make server`
Expected: 在服务端源文件未实现完整前失败

- [ ] **Step 2: 复用参考代码思路实现队列和线程池**

- [ ] **Step 3: 实现 socket 初始化和服务端主流程**

- [ ] **Step 4: 实现会话管理与虚拟路径处理**

- [ ] **Step 5: 参考 `handle简单实现.md` 实现命令处理**

- [ ] **Step 6: 实现上传下载**

- [ ] **Step 7: 重新执行 `make server`，确认服务端编译通过**

### Task 6: 先为客户端主链路写失败验证，再实现客户端模块

**Files:**
- Create: `src/client/main.c`
- Create: `src/client/client.c`
- Create: `src/client/command.c`
- Create: `src/client/file_transfer.c`

- [ ] **Step 1: 扩充 Makefile 让客户端目标进入编译**

Run: `make client`
Expected: 在客户端源文件未实现完整前失败

- [ ] **Step 2: 实现连接、登录和命令发送**

- [ ] **Step 3: 实现客户端命令解析**

- [ ] **Step 4: 实现上传下载逻辑**

- [ ] **Step 5: 重新执行 `make client`，确认客户端编译通过**

### Task 7: 端到端联调验证

**Files:**
- Modify: `README.md` 或新增第一期验证说明

- [ ] **Step 1: 启动服务端**

Run: `./bin/mycloud_server`
Expected: 服务端读取配置成功并开始监听

- [ ] **Step 2: 启动客户端并登录**

Run: `./bin/mycloud_client`
Expected: 能输入用户名并登录

- [ ] **Step 3: 验证基础命令**

命令：
- `pwd`
- `ls`
- `mkdir dir1`
- `cd dir1`
- `pwd`

- [ ] **Step 4: 验证文件上传下载**

命令：
- `puts test.txt`
- `ls`
- `gets test.txt`
- `rm test.txt`

- [ ] **Step 5: 验证异常场景**

场景：
- 不存在用户名
- 未登录执行命令
- 不存在文件
- 不存在目录

### Task 8: 完成说明与收尾验证

**Files:**
- Modify: `README.md`

- [ ] **Step 1: 补充运行方法**

- [ ] **Step 2: 补充配置说明**

- [ ] **Step 3: 补充第一期支持命令与限制说明**

- [ ] **Step 4: 执行完整 `make clean && make` 验证构建**
