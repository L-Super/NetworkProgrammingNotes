

## epoll_create ()
创建一个 epoll 实例，返回一个引用新的 epoll 实例的文件描述符。当不再需要时，应该使用 `close()` 关闭 `epoll_create()` 返回的文件描述符。当所有文件描述符引用为 0 时，则内核将销毁实例，并释放相关的资源以供重用。

```c
#include <sys/epoll.h>
int epoll_create(int size);
int epoll_create1(int flags);
//成功时返回 epoll 的文件描述符，失败时返回 -1
```
+ size：epoll 实例的大小
+ flags：如果 falgs 为 0，epoll_create1 () 与 epoll_create () 相同。可以增加如 `EPOLL_CLOEXEC` 的额外选项

> 注：从 Linux 开始 2.6.8，则忽略 size 参数，但必须大于 0;

## epoll_ctl ()
```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//成功时返回 0 ，失败时返回 -1
```
+ epfd：用于注册监视对象的 epoll 例程的文件描述符，即 `epoll_create()` 创建的 epoll 文件描述符
+ op：用于监视对象的添加、删除或更改等操作类型
+ fd：需要注册的监视对象文件描述符
+ event：监视对象的事件类型

`op` 操作类型：
+ EPOLL_CTL_ADD： 向 epoll 实例注册 fd对应的事件； EPOLL_CTL_DEL：向 epoll 实例删除文件描述符对应的事件； EPOLL_CTL_MOD： 修改文件描述符对应的事件。