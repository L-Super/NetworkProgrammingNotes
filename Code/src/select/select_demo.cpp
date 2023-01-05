//
// Created by Listening on 2023/1/5.
// select demo 如果运行后在标准输入流输入数据，就会在标准输出流输出数据，但是如果 5 秒没有输入数据，就提示超时。
//

#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>

#define BUF_SIZE 30

int main(int argc, char *argv[])
{
	fd_set reads, temps;
	int result, str_len;
	char buf[BUF_SIZE];
	struct timeval timeout;

	FD_ZERO(&reads);   //初始化变量
	int fd{0};//标准输入
	FD_SET(fd, &reads); //将文件描述符0对应的位设置为1

	//如果此处设置后，while里没重新设置，将会被默认为0
//	timeout.tv_sec=5;
//	timeout.tv_usec=0;

	while (1)
	{
		temps = reads; //为了防止调用了select 函数后，位的内容改变，先提前存一下
		//每次都要初始化其值，否则timeout被默认初始化为0。
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		result = select(fd+1, &temps, 0, 0, &timeout); //如果控制台输入数据，则返回大于0的数，没有就会超时
		if (result == -1)
		{
			puts("select error!");
			break;
		}
		else if (result == 0)
		{
			puts("Time-out!");
		}
		else
		{
			if (FD_ISSET(fd, &temps)) //验证发生变化的值是否是标准输入端
			{
				str_len = read(fd, buf, BUF_SIZE);
				buf[str_len] = 0;
				printf("message from console: %s", buf);
			}
		}
	}
	return 0;
}
