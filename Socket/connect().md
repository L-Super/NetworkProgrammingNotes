建立连接
```c
#include <sys/socket.h>
int connect(int sock, struct sockaddr *servaddr, socklen_t addrlen);
/*
成功时返回0，失败返回-1
*/
```

+ sock:客户端套接字文件描述符
+ servaddr: 保存目标服务器端地址信息的变量地址值
+ addrlen: 以字节为单位传递给第二个结构体参数 servaddr 的变量地址长度