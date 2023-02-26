将套接字与特定的 IP 地址和端口绑定起来
它既可以用于连接的（流式）套接字，也可以用于无连接的（数据报）套接字。
```c
#include <sys/socket.h>
int bind(int sockfd, struct sockaddr *myaddr, socklen_t addrlen);//成功时返回0，失败时返回-1
```

例如：
```c
bind(sockfd, (sockaddr* )&serv_addr, sizeof(serv_addr));
```

# 为什么使用 sockaddr_in 而不使用 sockaddr

bind() 第二个参数的类型为 sockaddr，而代码中却使用 [[sockaddr_in结构体|sockaddr_in]]，然后再强制转换为 sockaddr，这是为什么呢？

sockaddr 结构体的定义如下：

```c
struct sockaddr{    
    sa_family_t  sin_family;   //地址族（Address Family），也就是地址类型    
    char         sa_data[14];  //IP地址和端口号
};
```

下图是 sockaddr 与 sockaddr_in 的对比（括号中的数字表示所占用的字节数）：

![sockaddr 与 sockaddr_in 的对比](../images/sockaddr与sockaddr_in的对比.png)

sockaddr 和 sockaddr_in 的长度相同，都是16字节，只是将IP地址和端口号合并到一起，用一个成员 sa_data 表示。

要想给 sa_data 赋值，必须同时指明IP地址和端口号，例如”127.0.0.1:80“，遗憾的是，没有相关函数将这个字符串转换成需要的形式，也就很难给 sockaddr 类型的变量赋值，所以使用 sockaddr_in 来代替。这两个结构体的长度相同，强制转换类型时不会丢失字节，也没有多余的字节。

可以认为，sockaddr 是一种通用的结构体，可以用来保存多种类型的IP地址和端口号，而 sockaddr_in 是专门用来保存 IPv4 地址的结构体。另外还有 sockaddr_in6，用来保存 IPv6 地址，它的定义如下：

```c
struct sockaddr_in6 {     
    sa_family_t sin6_family;  //(2)地址类型，取值为AF_INET6    
    in_port_t sin6_port;  //(2)16位端口号    
    uint32_t sin6_flowinfo;  //(4)IPv6流信息    
    struct in6_addr sin6_addr;  //(4)具体的IPv6地址    
    uint32_t sin6_scope_id;  //(4)接口范围ID
};
```

正是由于通用结构体 sockaddr 使用不便，才针对不同的地址类型定义了不同的结构体。
