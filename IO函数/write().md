```c
#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t nbytes);
/*
成功时返回写入的字节数 ，失败时返回 -1
*/
```
+ fd : 显示数据传输对象的文件描述符
+ buf : 保存要传输数据的缓冲值地址
+ nbytes : 要传输数据的字节数