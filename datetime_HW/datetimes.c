/****************************************************/
/************* datetime Example Server **************/
/****************************************************/
#include "datetime.h"
#include <time.h>

int main(int argc, char **argv)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	char buff[MAXLINE];
	time_t ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(13444);

	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenfd, 1024);

	for (;;)
	{
		connfd = accept(listenfd, (struct sockaddr *)NULL, NULL);
		printf("Got message!\n");
		pid_t p = fork();
		if (p = 0)
		{
			close(listenfd);
			time(&ticks);
			printf("Got time!\n");
			struct tm *p;
			p = gmtime(&ticks);

			snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
			printf("Generate message!\n");

			sleep(10000);
			write(connfd, buff, strlen(buff));
			close(connfd);
		}
		else
		{
			close(connfd);
		}
	}
}
