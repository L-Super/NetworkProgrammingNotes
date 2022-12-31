
```cpp
#include <sys/socket.h>
ssize_t send(int sockfd, const void* buf, size_t nbytes, int flags);
//成功时返回发送的字节数，失败时返回-1
```
+ sockfd：表示与数据传输对象的连接的套接字文件描述符
+ buf：保存待传输数据的缓冲地址值
+ nbytes：待传输的字节数
+ flags：传输数据时指定的可选项信息（可利用位或运算同时传递多个信息）

send & recv 函数的可选项意义：

| 可选项（Option） | 含义                                                     | send（支持） | recv（支持） |
| --------------- | -------------------------------------------------------- | ---- | ---- |
| MSG_OOB         | 用于传输带外数据（Out-of-band data）                        | √    | √    |
| MSG_PEEK        | 验证输入缓冲中是否存在接受的数据                             | X    | √    |
| MSG_DONTROUTE   | 数据传输过程中不参照本地路由（Routing）表，在本地（Local）网络中寻找目的地 | √  | X    |
| MSG_DONTWAIT    | 调用 I/O 函数时不阻塞，用于使用非阻塞（Non-blocking）I/O   | √    | √     |
| MSG_WAITALL     | 防止函数返回，直到接收到全部请求的字节数                    | X    | √    |