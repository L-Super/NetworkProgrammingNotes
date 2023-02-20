//
// Created by LMR on 2023/1/8.
//

#include <netinet/in.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <string>

#define MAXLINE 80
#define SERV_PORT 12345
int main(int argc, char *argv[])
{
	struct sockaddr_in servaddr;
	char buf[MAXLINE];
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	servaddr.sin_port = htons(SERV_PORT);

	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	std::string str;
	if (argc > 1)
		str = argv[1];
	else
		str = {"helloworld!I'm Leo"};

	write(sockfd, str.c_str(), str.size());
	printf("send data: %s\n", str.c_str());
	auto send_len = str.size();
	int recv_len = 0;
	while (recv_len < send_len) {
		int n = read(sockfd, &buf[recv_len], MAXLINE - 1);
		if (n == -1)
			printf("read() error\n");
		recv_len += n;

	}

	printf("the socket has been closed.Receive data: %s\n", buf);

	close(sockfd);
}