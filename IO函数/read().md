```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t nbytes);
/*
成功时返回接收的字节数（但遇到文件结尾则返回 0），失败时返回 -1
*/
```
+ fd : 显示数据接收对象的文件描述符
+ buf : 要保存接收的数据的缓冲地址值
+ nbytes : 要接收数据的最大字节数

阻塞模式和非阻塞模式下的不同行为特性：

| 系统内核缓冲区状态 | 阻塞模式         | 非阻塞模式                           |
| ------------------ | ---------------- | ------------------------------------ |
| 接收缓冲区有数据   | 立即返回         | 立即返回                             |
| 接收缓冲区无数据   | 一直等待数据到来 | 立即返回，带有EWOULDBLOCK或EAGIN错误 |

