让套接字进入被动监听状态
```c
#include <sys/socket.h>
int listen(int sockfd, int backlog);
//成功时返回0，失败时返回-1
```
+ sockfd 为需要进入监听状态的套接字
+ backlog 为请求队列的最大长度。在Linux中表示已完成(ESTABLISHED)且未accept的队列大小，这个参数的大小决定了可以接收的并发数目。
如果将 backlog 的值设置为 SOMAXCONN，就由系统来决定请求队列长度。

> 请求队列:
> 当套接字正在处理客户端请求时，如果有新的请求进来，套接字是没法处理的，只能把它放进缓冲区，待当前请求处理完毕后，再从缓冲区中读取出来处理。如果不断有新的请求进来，它们就按照先后顺序在缓冲区中排队，直到缓冲区满。这个缓冲区，就称为请求队列（Request Queue）。

当请求队列满时，就不再接收新的请求，对于 Linux，客户端会收到 ECONNREFUSED 错误。队列满了之后，用户发送的SYN，会被丢弃，用户端未收到ACK回应会重新发SYN。