

```c
#include <sys/select.h>
#include <sys/time.h>

int select(int maxfd, fd_set *readset, fd_set *writeset,
           fd_set *exceptset, const struct timeval *timeout);

//发生错误时返回 -1, 超时时返回 0,。因发生关注的事件返回时，返回大于 0 的值，该值是发生事件的文件描述符数。
```

+ maxfd: 监视对象文件描述符数量
+ readset: 将所有关注「是否存在待读取数据」的文件描述符注册到 fd_set 型变量，并传递其地址值。
+ writeset: 将所有关注「是否可传输无阻塞数据」的文件描述符注册到 fd_set 型变量，并传递其地址值。
+ exceptset: 将所有关注「是否发生异常」的文件描述符注册到 fd_set 型变量，并传递其地址值。
+ timeout: 调用 select 函数后，为防止陷入无限阻塞的状态，传递超时 (time-out) 信息
