创建套接字
```c
#include <sys/socket.h>
int socket(int domain, int type, int protocol);
//成功时返回文件描述符，失败时返回-1
```

+ domain: 使用的协议族（Protocol Family）
+ type: 数据传输类型
+ protocol: 计算机间通信中使用的协议信息

```cpp
int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);  //创建TCP套接字
int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);  //创建UDP套接字
```

- 第一个参数：IP地址类型，AF_INET表示使用IPv4，如果使用IPv6请使用AF_INET6。
- 第二个参数：数据传输方式，SOCK_STREAM表示流格式、面向连接，多用于TCP。SOCK_DGRAM表示数据报格式、无连接，多用于UDP。
- 第三个参数：协议，0表示根据前面的两个参数自动推导协议类型。设置为IPPROTO_TCP和IPPTOTO_UDP，分别表示TCP和UDP。