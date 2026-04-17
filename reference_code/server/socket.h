#ifndef __SOCKET_H__
#define __SOCKET_H__

//作用就是为了初始化socket网络编程中的基本操作
//包括socket函数创建用于监听的文件描述符，因为这个参数
//后续需要被监听，所以设置为传出参数，也就是设计为指针类型

//因为会绑定服务器的ip与端口号，为了让ip与端口号任意，所以
//设置为了函数的参数,就是为socket网络编程中bind服务的

//最后调用listen监听客户端（全连接队列与半连接队列）

void init_socket(int *fd, char *ip, char *port);

#endif
