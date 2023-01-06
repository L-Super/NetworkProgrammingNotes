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
-   `FD_ISSET(int fd,fd_set *fdset)` ：若参数 fdset 中包含文件描述符 fd 的信息，则返回「真」（判断 fd 是否为 1）

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
####  定义
`fd_set` 结构体定义：
```c
//typesizes.h
#define __FD_SETSIZE       1024
//sys/select.h
/* The fd_set member is required to be an array of longs.  */  
typedef long int __fd_mask;
/* Maximum number of file descriptors in `fd_set'.  */  
#define    FD_SETSIZE    __FD_SETSIZE
/* Some versions of <linux/posix_types.h> define this macros.  */  
#undef __NFDBITS  
/* It's easier to assume 8-bit bytes than to get CHAR_BIT.  */  
#define __NFDBITS  (8 * (int) sizeof (__fd_mask))

typedef struct  
  {  
    /* XPG4.2 requires this member name.  Otherwise avoid the name  
       from the global namespace.  */
#ifdef __USE_XOPEN  
    __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS];  
# define __FDS_BITS(set) ((set)->fds_bits)  
#else  
    __fd_mask __fds_bits[__FD_SETSIZE / __NFDBITS];  
# define __FDS_BITS(set) ((set)->__fds_bits)  
#endif  
  } fd_set;
```

简化版：
```c
#define __FD_SETSIZE       1024
typedef long int __fd_mask;
#define __NFDBITS  (8 * (int) sizeof (__fd_mask))
typedef struct  
  {  
    __fd_mask fds_bits[__FD_SETSIZE / __NFDBITS]; //fds_bits[1024 / (8* 4)]
	# define __FDS_BITS(set) ((set)->fds_bits)  
  } fd_set;
```
由以上定义可见，`fd_set` 结构体仅包含一个整型数组，该数组的每个元素的每一位 (bit) 标记一个文件描述符。`fd_set` 能容纳的文件描述符数量由 `FD_SETSIZE` 指定，这就限制了 select 能同时处理的文件描述符的总量。

**select 调用最多只支持 1024 个文件描述符**推理：
32 位系统下，数组长度为 `1024/(8*4)=32`，即 `fds_bits[32]`，而一个整型数可以通过位运算表示 32 个数，如 `fds_bits[1]` 可以位形式存 32 个数，那么数组总共能存 `32*32=1024` 个数，即 1024 个文件描述符。

> 一个 32 位的整型数可以表示 32 个描述字，例如第一个整型数表示 0-31 描述字，第二个整型数可以表示 32-63 描述字，以此类推。

访问 `fd_set ` 的宏：
```c
#define __FD_ELT(d)    ((d) / __NFDBITS)  
#define __FD_MASK(d)   ((__fd_mask) (1UL << ((d) % __NFDBITS)))

/* Access macros for `fd_set'.  */  
#define    FD_SET(fd, fdsetp) __FD_SET (fd, fdsetp)  
#define    FD_CLR(fd, fdsetp) __FD_CLR (fd, fdsetp)  
#define    FD_ISSET(fd, fdsetp)   __FD_ISSET (fd, fdsetp)  
#define    FD_ZERO(fdsetp)       __FD_ZERO (fdsetp)

#define __FD_SET(d, set) \  
  ((void) (__FDS_BITS (set)[__FD_ELT (d)] |= __FD_MASK (d)))  
#define __FD_CLR(d, set) \  
  ((void) (__FDS_BITS (set)[__FD_ELT (d)] &= ~__FD_MASK (d)))  
#define __FD_ISSET(d, set) \  
  ((__FDS_BITS (set)[__FD_ELT (d)] & __FD_MASK (d)) != 0)

#if defined __GNUC__ && __GNUC__ >= 2  
# define __FD_ZERO(fdsp) \  
  do {                               \  
    int __d0, __d1;                            \  
    __asm__ __volatile__ ("cld; rep; " __FD_ZERO_STOS              \  
           : "=c" (__d0), "=D" (__d1)               \  
           : "a" (0), "0" (sizeof (fd_set)           \  
                 / sizeof (__fd_mask)),         \  
             "1" (&__FDS_BITS (fdsp)[0])                \  
           : "memory");                   \  
  } while (0)  
  
#else  /* ! GNU CC */  
  
/* We don't use `memset' because this would require a prototype and  
   the array isn't too big.  */
# define __FD_ZERO(set)  \  
  do {                               \    
	  unsigned int __i;                          \    
	  fd_set *__arr = (set);                      \
	  for (__i = 0; __i < sizeof (fd_set) / sizeof (__fd_mask); ++__i)   \
		  __FDS_BITS (__arr)[__i] = 0;                   \  
  } while (0) 
#endif /* GNU CC */
```

## 示例
监听标准输入，输入数据，就会在标准输出流输出数据，但是超时没有输入数据，就提示超时。

```preview
path: code/src/select/select_demo.cpp
start: 10
```

除 fd+1 外，还需要注意的是，设置超时时间，需要在 `while` 里每次都要赋值赋值，否则 timeout 被默认初始化为 0。

服务端：
```preview
path: code/src/select/select_echo_server.cpp
```
