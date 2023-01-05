//
// Created by Listening on 2023/1/5.
//

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <iostream>

#define MAXLINE 80
#define SERV_PORT 12345

const char client()
{
	struct sockaddr_in servaddr;
	char buf[MAXLINE];
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	servaddr.sin_port = htons(SERV_PORT);

	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	const char* str = {"hello"};
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);
		write(sockfd,str,sizeof(str));
		int n = read(sockfd, buf, MAXLINE);
		if (n == 0)
			printf("the other side has been closed.\n");
		else
		{
			std::cout<<std::this_thread::get_id()<<": send msg"<<std::endl;
		}

	close(sockfd);
}

int main(int argc, char *argv[])
{
	std::vector<std::thread> threads;
	for(int i=0;i<100;i++)
	{
		threads.emplace_back(std::thread(client));
	}
	for (auto& t:threads)
	{
		t.join();
	}

	return 0;
}
