接收客户端请求
```c
#include <sys/socket.h>
int accept(int sockfd,struct sockaddr *addr,socklen_t *addrlen);
//成功时返回文件描述符，失败时返回-1
```
+ sock: 服务端套接字的文件描述符
+ addr: 保存发起连接请求的客户端地址信息的变量地址值
+ addrlen: 的第二个参数addr结构体的长度，但是存放有长度的变量地址。

accept() 返回一个新的套接字来和客户端通信，sockfd 是服务器端的套接字，addr 保存了客户端的IP地址和端口号。

[[listen()| `listen()`]] 只是让套接字进入监听状态，并没有真正接收客户端请求，listen() 后面的代码会继续执行，直到遇到 `accept()`。`accept()`函数会阻塞当前程序，直到有新的请求到来。