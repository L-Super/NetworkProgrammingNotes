

```cpp
#include <sys/socket.h>  
ssize_t recv(int sockfd, void *buf, size_t nbytes, int flags);  
//成功时返回接收的字节数（收到 EOF 返回 0），失败时返回 -1  
```

+ sockfd: 表示数据接受对象的连接的套接字文件描述符 
+ buf: 保存接受数据的缓冲地址值 
+ nbytes: 可接收的最大字节数 
+ flags: 接收数据时指定的可选项参数 （可利用位或运算同时传递多个信息）

