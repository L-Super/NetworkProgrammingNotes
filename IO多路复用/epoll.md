

## epoll_create ()

```c
#include <sys/epoll.h>
int epoll_create(int size);
//成功时返回 epoll 的文件描述符，失败时返回 -1
```
+ size：epoll 实例的大小
> 注：从 Linux 开始 2.6.8，则忽略 size 参数，但必须大于 0;
