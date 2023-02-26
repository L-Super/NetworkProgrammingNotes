套接字可选项的读取和设置通过以下两个函数来完成

## getsockopt()

读取套接字可选项

```c
#include <sys/socket.h>

int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);
/*
成功时返回 0 ，失败时返回 -1
*/
```

+ sock: 用于查看选项套接字文件描述符
+ level: 要查看的可选项协议层
+ optname: 要查看的可选项名
+ optval: 保存查看结果的缓冲地址值
+ optlen: 向第四个参数传递的缓冲大小。调用函数，该变量中保存通过第四个参数返回的可选项信息的字节数。

## setsockopt()

更改可选项：

```c
#include <sys/socket.h>

int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);
/*
成功时返回 0 ，失败时返回 -1
*/
```

+ sock: 用于更改选项套接字文件描述符
+ level: 要更改的可选项协议层
+ optname: 要更改的可选项名
+ optval: 保存要更改的选项信息的缓冲地址值
+ optlen: 向第四个参数传递的可选信息的字节数。

```c
int main(int argc, char *argv[])
{
    int tcp_sock, udp_sock;
    int sock_type;
    socklen_t optlen;
    int state;

    optlen = sizeof(sock_type);
    tcp_sock = socket(PF_INET, SOCK_STREAM, 0);
    udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
    printf("SOCK_STREAM: %d\n", SOCK_STREAM);
    printf("SOCK_DGRAM: %d\n", SOCK_DGRAM);

    state = getsockopt(tcp_sock, SOL_SOCKET, SO_TYPE, (void *)&sock_type, &optlen);
    if (state)
        error_handling("getsockopt() error");
    printf("Socket type one: %d \n", sock_type);

    state = getsockopt(udp_sock, SOL_SOCKET, SO_TYPE, (void *)&sock_type, &optlen);
    if (state)
        error_handling("getsockopt() error");
    printf("Socket type two: %d \n", sock_type);
    return 0;
}
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
```