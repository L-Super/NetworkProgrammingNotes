
```cpp
#include <sys/socket.h>
ssize_t send(int sockfd, const void* buf, size_t nbytes, int flags);
//成功时返回发送的字节数，失败时返回-1
```
+ sockfd：表示与数据传输对象的连接的套接字文件描述符
+ buf：保存待传输数据的缓冲地址值
+ nbytes：待传输的字节数
+ flags：传输数据时指定的可选项信息