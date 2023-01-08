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
		str = {"hello"};

	printf("sizeof(str): %d. sizeof(str.c_str()): %d. str.size(): %d\n",
		   sizeof(str),
		   sizeof(str.c_str()),
		   str.size());
	write(sockfd, str.c_str(), str.size());
	int n = read(sockfd, buf, MAXLINE);
	if (n == 0)
		printf("the socket has been closed.Receive data: %s\n", buf);


	close(sockfd);
}