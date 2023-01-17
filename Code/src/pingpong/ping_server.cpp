//
// Created by LMR on 2023/1/17.
//

#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <stdarg.h>

struct messageObject
{
	int type;
	char data[1024];
};

#define MSG_PING          1
#define MSG_PONG          2
#define MSG_TYPE1        11
#define MSG_TYPE2        21

#define    SERV_PORT      12345

static int count;

static void sig_int(int signo)
{
	printf("\nreceived %d datagrams\n", count);
	exit(0);
}

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
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERV_PORT);

	int rt1 = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (rt1 < 0) {
		error(1, errno, "bind failed ");
	}

	int rt2 = listen(listenfd, 1024);
	if (rt2 < 0) {
		error(1, errno, "listen failed ");
	}

	signal(SIGINT, sig_int);
	signal(SIGPIPE, SIG_IGN);

	int connfd;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
		error(1, errno, "bind failed ");
	}

	messageObject message;
	count = 0;
	int sleepingTime = 1;

	for (;;) {
		int n = read(connfd, (char *)&message, sizeof(messageObject));
		if (n < 0) {
			error(1, errno, "error read");
		}
		else if (n == 0) {
			error(1, 0, "client closed \n");
		}

		printf("received %d bytes\n", n);
		count++;

		switch (ntohl(message.type)) {
		case MSG_TYPE1 :
			printf("process  MSG_TYPE1 \n");
			break;
		case MSG_TYPE2 :
			printf("process  MSG_TYPE2 \n");
			break;
		case MSG_PING: {
			printf("process  MSG_PING \n");
			messageObject pong_message;
			pong_message.type = MSG_PONG;
			sleep(sleepingTime);
			ssize_t rc = send(connfd, (char *)&pong_message, sizeof(pong_message), 0);
			if (rc < 0)
				error(1, errno, "send failure");
			break;
		}
		default :
			error(1, 0, "unknown message type (%d)\n", ntohl(message.type));
		}
	}
}
