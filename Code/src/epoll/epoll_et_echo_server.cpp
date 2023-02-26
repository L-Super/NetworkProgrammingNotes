//
// Created by LMR on 2023/2/26.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <cstdlib>
#include <sys/epoll.h>

#define PROT 12345

void error_handling(const char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char *argv[])
{
	int server_sock = socket(PF_INET,SOCK_STREAM,0);
	struct sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PROT);

	if(bind(server_sock,(sockaddr*)&server_addr, sizeof(server_addr)) == -1)
		error_handling("bind() error");
	if(listen(server_sock,5) == -1)
		error_handling("listen() error");

	int epfd = epoll_create(1);

	return 0;
}