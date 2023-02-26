
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
注册监视对象文件描述符，添加或删除监控的事件
```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//成功时返回 0 ，失败时返回 -1
```
+ epfd：用于注册监视对象的 epoll 例程的文件描述符，即 `epoll_create()` 创建的 epoll 文件描述符
+ op：用于监视对象的添加、删除或更改等操作类型
+ fd：需要注册的监视对象文件描述符
+ event：监视对象的事件类型

`op` 操作类型：
+ EPOLL_CTL_ADD： 向 epoll 实例注册 fd 对应的事件
+ EPOLL_CTL_DEL：向 epoll 实例删除 fd 对应的事件
+ EPOLL_CTL_MOD： 修改文件描述符对应的事件

`event` 类型：
+ EPOLLIN：表示对应的 fd 可以读
+ EPOLLOUT：表示对应的 fd 可以写
+ EPOLLPRI：收到 OOB 数据的情况
+ EPOLLRDHUP：表示套接字的一端已经关闭，或者半关闭
+ EPOLLERR：发生错误的情况
+ EPOLLHUP：表示对应的文件描述字被挂起
+ EPOLLET：设置为边缘触发 `edge-triggered`，默认为水平触发 `level-triggered`
+ EPOLLONESHOT：发生一次事件后，相应文件描述符不再收到事件通知。因此需要向 epoll_ctl 函数的第二个参数传递 EPOLL_CTL_MOD ，再次设置事件。
可通过位运算同时传递多个上述参数。

`evnet` 定义：
```c
typedef union epoll_data {
   void        *ptr;
   int          fd;
   uint32_t     u32;
   uint64_t     u64;
} epoll_data_t;

struct epoll_event {
   uint32_t     events;      /* Epoll 事件 */
   epoll_data_t data;        /* 用户数据 */
};
```

`epoll_data_t` 是一个联合体，可以在这个结构体里设置用户需要的数据，使用最多的是 `fd`，表示事件所对应的文件描述符。`ptr` 成员可用来指定与 `fd` 相关的用户数据。但由于 `epoll_data_t` 是一个联合体，我们不能同时使用其 `ptr` 成员和 `fd` 成员。

## epoll_wait ()
```c
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
// 成功时返回发生事件的文件描述符个数，返回0表示的是超时时间到，失败时返回 -1
```

+ epfd : 表示事件发生监视范围的 epoll 例程的文件描述符
+ events : 保存发生事件的文件描述符集合的结构体首地址值
+ maxevents : `events` 中可以保存的最大事件数，必须大于 0
+ timeout : `epoll_wait` 阻塞调用的超时时间。如果设置为 -1，表示不超时；如果设置为 0，则立即返回，即使没有任何 I/O 事件发生。

## LT 和 ET 模式
epoll 对文件描述符的操作有两种模式：LT (Level Trigger 水平触发) 模式和 ET (Edge Trigger 边缘触发) 模式。LT 模式是默认的工作模式。

+ 对于水平触发模式，只要输入缓冲有数据就会一直通知该事件
+ 对于边缘触发模式，输入缓冲收到数据时仅注册一次该事件
或理解为电路中：
+ 水平模式的触发条件：①低电平→高电平；②处于高电平状态。 
+ 边缘模式的触发条件：低电平→高电平。
可见，ET 模式在很大程度上降低了同一个 epoll 事件被重复触发的次数，因此效率要比 LT 模式高。

> 注意：每个使用 ET 模式的文件描述符都应该是非阻塞的。如果文件描述将是阻塞的，那么读或写操作将会因为没有后续的事件而一直处于阻塞状态（饥渴状态）。

