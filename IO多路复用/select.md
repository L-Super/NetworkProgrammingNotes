使用 select 函数，通知内核挂起进程，当一个或多个 I/O 事件发生后，控制权返还给应用程序，由应用程序进行 I/O 事件的处理。

I/O 事件的类型非常多，比如： 
+ 标准输入文件描述符准备好可以读。
+ 监听套接字准备好，新的连接已经建立成功。
+ 已连接套接字准备好可以写。
+ 如果一个 I/O 事件等待超过了 10 秒，发生了超时事件。


select 函数的调用过程如下图所示：

![](../images/select_process.png)

## 函数声明

```c
#include <sys/select.h>
#include <sys/time.h>

int select(int maxfd, fd_set *readset, fd_set *writeset,
           fd_set *exceptset, const struct timeval *timeout);

//发生错误时返回 -1, 超时时返回 0,。因发生关注的事件返回时，返回大于 0 的值，该值是发生事件的文件描述符数。
```

+ maxfd: 监视对象文件描述符数量，值最大描述符编号值加 1（因为描述符编号从 0 开始，所以在最大描述符值加 1）
+ readset: 通知内核在哪些文件描述符上检测数据可以读
+ writeset: 通知内核在哪些文件描述符上检测数据可以写
+ exceptset: 通知内核在哪些文件描述符上检测数据有异常发生
+ timeout: 设置超时时间。


设置描述符集合宏：
-   `FD_ZERO(fd_set *fdset)`：将 fd_set 变量所指的位全部初始化成0
-   `FD_SET(int fd,fd_set *fdset)` ：在参数 fdset 中注册文件描述符 fd 的信息（设置设置为 1）
-   `FD_CLR(int fd,fd_set *fdset)` ：从参数 fdset 中清除文件描述符 fd 的信息（设置为 0）
-   `FD_ISSET(int fd,fd_set *fdset)` ：若参数 fdset 中包含文件描述符 fd 的信息，则返回「真」（判断判断 fd 是否为 1）

timeval 结构体时间：
```c
struct timeval { 
	long tv_sec; /* seconds */ 
	long tv_usec; /* microseconds */ 
};
```
+ 设置成 `NULL`，表示如果没有 I/O 事件发生，则 select 一直等待下去；
+ 设置一个非零的值，这个表示等待固定的一段时间后从 select 阻塞调用中返回；
+ 将 tv_sec 和 tv_usec 都设置成 0，表示根本不等待，检测完毕立即返回。这种情况使用得比较少。