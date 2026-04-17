# 网盘项目第一期

本项目是一个使用 C 语言实现的简化网盘项目。当前完成的是第一期，重点实现：

- 客户端与服务端长连接通信
- 伪登录与多用户目录隔离
- `pwd`、`ls`、`cd`、`mkdir`、`rm`、`puts`、`gets`、`quit`
- 配置文件加载
- 服务端日志记录

## 目录说明

- `config/mycloud.conf`：主配置文件
- `config/users.conf`：伪登录用户列表
- `docs/源码阅读顺序说明.md`：适合初学者的源码阅读路线
- `src/server/`：服务端源码
- `src/client/`：客户端源码
- `include/`：头文件
- `data/users/`：服务端各用户目录
- `download/`：客户端下载目录
- `log/server.log`：服务端日志

## 编译方法

```bash
make
```

编译完成后会生成：

- `bin/mycloud_server`
- `bin/mycloud_client`

## 启动方法

先启动服务端：

```bash
./bin/mycloud_server
```

再启动客户端：

```bash
./bin/mycloud_client
```

## 配置文件示例

`config/mycloud.conf`

```text
server_ip=127.0.0.1
server_port=12345
thread_num=4
log_level=INFO
log_file=log/server.log
data_root=./data/users
```

`config/users.conf`

```text
alice
bob
test
```

## 第一阶段命令说明

- `pwd`：查看当前虚拟路径
- `ls`：查看当前目录内容
- `cd 目录名`：进入子目录
- `cd ..`：返回上一级目录
- `mkdir 目录名`：创建目录
- `rm 文件名`：删除普通文件
- `puts 文件名`：上传本地普通文件
- `gets 文件名`：下载服务端文件到 `download/`
- `quit`：退出客户端

## 第一阶段限制

- 登录只检查用户名是否存在，不校验密码
- `cd` 只支持进入一级子目录和 `cd ..`
- `rm` 只删除普通文件，不删除目录
- 不实现断点续传、`mmap`、数据库和真实用户系统

## 已完成验证

已验证以下流程可以正常运行：

1. `alice` 登录
2. `pwd`
3. `ls`
4. `mkdir dir1`
5. `cd dir1`
6. `puts test.txt`
7. `gets test.txt`
8. `rm test.txt`
9. 不存在用户登录
10. 下载不存在文件
