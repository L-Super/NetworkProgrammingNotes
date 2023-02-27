/**
 * epoll LT模式(水平触发)
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define BUF_SIZE 10
#define EPOLL_MAX_SIZE 50
#define PORT 12345

void error_handling(const char *message);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr{}, clnt_adr{};
	socklen_t adr_sz;
	int str_len, i;
	char buf[BUF_SIZE];

//	if (argc != 2) {
//		printf("Usage : %s <port> \n", argv[0]);
//		exit(1);
//	}
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(PORT);

	if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	//1.动态分配
//	struct epoll_event *ep_events;
//	ep_events = (epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_MAX_SIZE);
	//2.分配数组，此方法有差异，会在for (i = 0; i < event_cnt; i++)阻塞，原因未明，可能边缘触发下有效
	struct epoll_event ep_events[EPOLL_MAX_SIZE];

	struct epoll_event event;
	int epfd, event_cnt;

	epfd = epoll_create(EPOLL_MAX_SIZE); // 可以忽略这个参数，填入的参数为操作系统参考
//	epfd = epoll_create1(0); //同epoll_create()

	event.events = EPOLLIN; // 需要读取数据的情况
	event.data.fd = serv_sock;
	epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event); // epfd中添加文件描述符serv_sock，目的是监听event中的事件

	int num{0};
	while (true) {
		// 获取改变了的文件描述符，返回数量
		event_cnt = epoll_wait(epfd, ep_events, EPOLL_MAX_SIZE, -1);
		if (event_cnt == -1) {
			puts("epoll_wait() error");
			break;
		}
		printf("return event num:%d\n", event_cnt);

		for (i = 0; i < event_cnt; i++) {
			//if(events[i].events & EPOLLIN)
			if (ep_events[i].data.fd == serv_sock) //客户端请求连接时
			{
				adr_sz = sizeof(clnt_adr);
				clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz);
				event.events = EPOLLIN; // 默认LT模式
				event.data.fd = clnt_sock; // 把客户端套接字添加进去
				epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
				printf("connected client fd: %d \n", clnt_sock);
			}
			else //是客户端套接字时
			{
				bzero(buf, sizeof(buf));
				str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
				printf("read data: \"%s\" str len: %d. num: %d\n", buf, str_len, num);
				if (str_len == 0) {
					epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL); //从epoll中删除套接字
					close(ep_events[i].data.fd);
					printf("closed client fd: %d \n", ep_events[i].data.fd);
				}
				else {
					write(ep_events[i].data.fd, buf, str_len);
					printf("written. num: %d\n", num);
				}
			}
			num++;
		}
	}
	close(serv_sock);
	close(epfd);

	return 0;
}

void error_handling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

//TODO：
//2.epoll_event 动态分配与数组的差异

