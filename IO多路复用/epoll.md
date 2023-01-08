

## epoll_create ()

```c
#include <sys/epoll.h>
int epoll_create(int size);
//成功时返回 epoll 的文件描述符，失败时返回 -1
```
+ size：epoll 实例的大小
> 注：从 Linux 开始 2.6.8，则忽略 size 参数，但必须大于 0;

返回文件描述符用于所有对 epoll 接口的后续调用如果需要，epoll _ create ()返回的文件描述符应该使用 close (2)关闭。当所有文件描述符引用对于一个 epoll 实例已经关闭，内核将销毁实例，并释放相关的资源以供重用。