//
// Created by Listening on 2023/1/5.
//
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define BUF_SIZE 100

void error_handling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	struct timeval timeout;
	fd_set reads, cpy_reads;

	socklen_t adr_sz;
	int fd_max, str_len, fd_num;
	char buf[BUF_SIZE];
	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	FD_ZERO(&reads);
	FD_SET(serv_sock, &reads); //注册服务端套接字
	fd_max = serv_sock;

	while (1)
	{
		cpy_reads = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if ((fd_num = select(fd_max + 1, &cpy_reads, 0, 0, &timeout)) == -1) //开始监视,每次重新监听
			break;
		if (fd_num == 0)//超时
		{
			printf("timeout\n");
		}

		for (int fd = 0; fd < fd_max + 1; fd++)
		{
			if (FD_ISSET(fd, &cpy_reads)) //查找发生变化的套接字文件描述符
			{
				if (fd == serv_sock) //如果是服务端套接字时,受理连接请求
				{
					adr_sz = sizeof(clnt_adr);
					clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);

					FD_SET(clnt_sock, &reads); //注册一个clnt_sock
					if (fd_max < clnt_sock) //如果大于fd_max则更新最大文件描述符值
						fd_max = clnt_sock;
					printf("Connected client fd: %d \n", clnt_sock);
				}
				else //不是服务端套接字时
				{
					str_len = read(fd, buf, BUF_SIZE); //fd指的是当前发起请求的客户端
					if (str_len == 0)
					{
						FD_CLR(fd, &reads);
						close(fd);
						printf("closed client: %d \n", fd);
					}
					else
					{
						printf("send buf %s\n",buf);
						write(fd, buf, str_len);
					}
				}
			}
		}
	}
	close(serv_sock);
	return 0;
}