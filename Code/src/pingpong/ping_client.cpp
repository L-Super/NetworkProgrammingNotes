//
// Created by LMR on 2023/1/17.
//
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <cstdarg>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>

struct messageObject{
	int type;
	char data[1024];
};

#define MSG_PING          1
#define MSG_PONG          2
#define MSG_TYPE1        11
#define MSG_TYPE2        21

#define    MAXLINE     4096
#define    KEEP_ALIVE_TIME  5
#define    KEEP_ALIVE_INTERVAL  3
#define    KEEP_ALIVE_PROBETIMES  3

#define    SERV_PORT      12345

void error(int status, int err, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (err)
		fprintf(stderr, ": %s (%d)\n", strerror(err), err);
	if (status)
		exit(status);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("usage: tcpclient <IPaddress>");
	}
	int socket_fd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

	socklen_t server_len = sizeof(server_addr);
	int connect_rt = connect(socket_fd, (struct sockaddr *) &server_addr, server_len);
	if (connect_rt < 0) {
		error(1, errno, "connect failed ");
	}
	char recv_line[MAXLINE + 1];
	int n;

	fd_set readmask;
	fd_set allreads;

	struct timeval tv;
	int heartbeats = 0;

	// 设置超时时间
	tv.tv_sec = KEEP_ALIVE_TIME;
	tv.tv_usec = 0;

	messageObject messageObject;

	FD_ZERO(&allreads);
	FD_SET(0, &allreads);
	FD_SET(socket_fd, &allreads);
	for (;;) {
		readmask = allreads;
		// 感知 I/O 事件。这里的 I/O 事件，除了套接字上的读操作之外，还有超时事件。
		// 当 KEEP_ALIVE_TIME 时间到达之后，select 函数会返回 0
		int rc = select(socket_fd + 1, &readmask, NULL, NULL, &tv);
		if (rc < 0) {
			error(1, errno, "select failed");
		}
		//timeout
		if (rc == 0) {
			if (++heartbeats > KEEP_ALIVE_PROBETIMES) {
				error(1, 0, "connection dead\n");
			}
			printf("sending heartbeat #%d\n", heartbeats);
			messageObject.type = htonl(MSG_PING);
			rc = send(socket_fd, (char *) &messageObject, sizeof(messageObject), 0);
			if (rc < 0) {
				error(1, errno, "send failure");
			}
			tv.tv_sec = KEEP_ALIVE_INTERVAL;
			continue;
		}
		// 客户端在接收到服务器端之后的处理。为了简单，这里就没有再进行报文格式的转换和分析。
		// 在实际的工作中，是需要对报文进行解析后处理的，只有是PONG 类型的回应，才认为是 PING 探活的结果。
		// 这里认为既然收到服务器端的报文，那么连接就是正常的，所以会对探活计数器和探活时间都置零，等待下一次探活时间的来临。
		if (FD_ISSET(socket_fd, &readmask)) {
			n = read(socket_fd, recv_line, MAXLINE);
			if (n < 0) {
				error(1, errno, "read error");
			} else if (n == 0) {
				error(1, 0, "server terminated \n");
			}
			printf("received heartbeat, make heartbeats to 0 \n");
			heartbeats = 0;
			tv.tv_sec = KEEP_ALIVE_TIME;
		}
	}

	return 0;
}