随IPv6出现的函数，对于IPv4地址和IPv6地址都适用，函数中p和n分别代表表达（presentation)和数值（numeric)。地址的表达格式通常是ASCII字符串，数值格式则是存放到套接字地址结构的二进制值。

头文件`#include <arpe/inet.h>`
```c
int inet_pton(int family, const char *strptr, void *addrptr);     
```
将点分十进制的ip地址转化为用于网络传输的数值格式。
返回值：若成功则为1，若输入不是有效的表达式则为0，若出错则为-1
``` c
const char * inet_ntop(int family, const void *addrptr, char *strptr, size_t len); 
```
将数值格式转化为点分十进制的ip地址格式
返回值：若成功则为指向结构的指针，若出错则为NULL

family参数既可以是AF_INET（ipv4）也可以是AF_INET6（ipv6）

```c
inet_pton(AF_INET, ip, &ser.sin_addr); // 代替ser.sin_addr.addr=inet_addr(ip);
    
char str[INET_ADDRSTRLEN];
char *ptr = inet_ntop(AF_INET,&ser.sin_addr, str, sizeof(str)); // 代替 ptr = inet_ntoa(ser.sin_addr)
```
