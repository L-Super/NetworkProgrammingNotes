结构体的定义如下

```c
struct sockaddr_in
{
    sa_family_t sin_family;  //地址族（Address Family）
    uint16_t sin_port;       //16 位 TCP/UDP 端口号
    struct in_addr sin_addr; //32位 IP 地址
    char sin_zero[8];        //不使用
};
```

该结构体中提到的另一个结构体 in_addr 定义如下，它用来存放 32 位IP地址

```c
struct in_addr
{
    in_addr_t s_addr; //32位IPV4地址
}
```
#### 结构体 sockaddr_in 的成员分析

- 成员 sin_family

每种协议适用的地址族不同，比如，IPV4 使用 4 字节的地址族，IPV6 使用 16 字节的地址族。

> 地址族
| 地址族（Address Family） | 含义                               |
| ------------------------ | ---------------------------------- |
| AF_INET                  | IPV4用的地址族                     |
| AF_INET6                 | IPV6用的地址族                     |
| AF_LOCAL                 | 本地通信中采用的 Unix 协议的地址族 |

AF_LOCAL 只是为了说明具有多种地址族而添加的。

- 成员 sin_port
  该成员保存 16 位端口号，重点在于，它以网络字节序保存。
- 成员 sin_addr
  该成员保存 32 为IP地址信息，且也以网络字节序保存
- 成员 sin_zero
  无特殊含义。只是为结构体 sockaddr_in 的大小与sockaddr结构体保持一致而插入的成员。填充零。

```c
#include <arpa/inet.h>  //这个头文件包含了<netinet/in.h>，不用再次包含了
struct sockaddr_in serv_addr;
bzero(&serv_addr, sizeof(serv_addr));
```

然后使用`bzero()`初始化这个结构体，这个函数在头文件`<string.h>`或`<cstring>`中。这里用到了《Effective C++》的准则：

> 条款04: 确定对象被使用前已先被初始化。如果不清空，使用gdb调试器查看addr内的变量，会是一些随机值，未来可能会导致意想不到的问题。


设置地址族、IP地址和端口：

```cpp
serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
serv_addr.sin_port = htons(8888);
```

# 字符串转[[网络字节序]]
## inet_addr()
sockaddr_in 中需要的是 32 位整数型，但是我们只熟悉点分十进制表示法，那么如何把类似于 201.211.214.36 转换为 4 字节的整数类型数据呢 

```C
#include <arpa/inet.h>
in_addr_t inet_addr(const char *string);
```
inet_addr 不仅可以转换地址，还可以检测有效性。
例如：
```c
char *addr1 = "1.2.3.4";
char *addr2 = "1.2.3.256";

unsigned long conv_addr = inet_addr(addr1);
if (conv_addr == INADDR_NONE)
    printf("Error occured! \n");
else
    printf("Network ordered integer addr: %#lx \n", conv_addr);

conv_addr = inet_addr(addr2);
if (conv_addr == INADDR_NONE)
    printf("Error occured! \n");
else
	printf("Network ordered integer addr: %#lx \n", conv_addr);
```
## inet_aton()
```c
#include <arpa/inet.h>
int inet_aton(const char *string, struct in_addr *addr);
/*
成功时返回 1 ，失败时返回 0
string: 含有需要转换的IP地址信息的字符串地址值
addr: 将保存转换结果的 in_addr 结构体变量的地址值
*/
```
例如：
```c
char *addr = "127.232.124.79";
struct sockaddr_in addr_inet;

if (!inet_aton(addr, &addr_inet.sin_addr))
	error_handling("Conversion error");
else
	printf("Network ordered integer addr: %#x \n", addr_inet.sin_addr.s_addr);
```