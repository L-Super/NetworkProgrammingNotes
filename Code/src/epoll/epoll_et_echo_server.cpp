//
// Created by LMR on 2023/2/26.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <string>

#define PROT 12345
#define BUF_SIZE 10
#define EPOLL_MAX_SIZE 50

// 设置非阻塞
void set_nonblocking_mode(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

void error_handling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	char buf[BUF_SIZE];

	struct sockaddr_in server_addr{};//{}列表初始化默认为0
	struct sockaddr_in client_addr{};

	int server_sock = socket(PF_INET, SOCK_STREAM, 0);

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PROT);
	if (bind(server_sock, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		error_handling("bind() error.");
	if (listen(server_sock, 5) == -1)
		error_handling("listen() error");

	set_nonblocking_mode(server_sock);

	int epoll_fd = epoll_create(1);
	epoll_event event;
	epoll_event epollEvents[EPOLL_MAX_SIZE];
//	epoll_event *epollEvents = (epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_MAX_SIZE);
	event.data.fd = server_sock;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &event);

	while (true) {
		// 获取改变了的文件描述符，返回数量
		int event_cnt = epoll_wait(epoll_fd, epollEvents, EPOLL_MAX_SIZE, -1);
		if (event_cnt == -1) {
			puts("epoll_wait() error");
			break;
		}
		printf("return event num:%d\n", event_cnt);

		for (int i = 0; i < event_cnt; i++) {
			if (epollEvents[i].data.fd == server_sock) //客户端请求连接时
			{
				socklen_t client_addr_len = sizeof(client_addr);
				auto client_sock_fd = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);

				set_nonblocking_mode(client_sock_fd);

				event.events = EPOLLIN | EPOLLET; // ET模式
				event.data.fd = client_sock_fd; // 把客户端套接字添加进去
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock_fd, &event);

				printf("connected client fd: %d \n", client_sock_fd);
			}
			else if (epollEvents[i].events & EPOLLIN)
			{
				while (true) {
					bzero(buf, sizeof(buf));
					int str_len = read(epollEvents[i].data.fd, buf, BUF_SIZE - 1);
					if (str_len == 0) { // close request
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, epollEvents[i].data.fd, nullptr); //从epoll中删除套接字
						close(epollEvents[i].data.fd);
						printf("closed client fd: %d \n", epollEvents[i].data.fd);
						break;
					}
					else if (str_len == -1 && errno == EAGAIN) { //读取了输入缓冲的全部数据
						printf("read all data from the input buffer, errno: %d\n", errno);
						break;
					}
					else { //str_len > 0
						write(epollEvents->data.fd, buf, str_len);
						printf("written. client fd %d. data: %s\n", epollEvents[i].data.fd, buf);
					}
				}
			}
			else
			{   //其他事件
				printf("other happened\n");
			}
		}
	}
	close(server_sock);
	close(epoll_fd);

	return 0;
}