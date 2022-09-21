/****************************************************/
/************* datetime Example Server **************/
/****************************************************/
#include "datetime.h"
#include <time.h>
#include <signal.h>

void signal_chld(int sig){
	// TODO:杀死子进程
	wait();
	// exit(0); 
}

int main(int argc, char **argv)
{
	signal(SIGCHLD,signal_chld);

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
		if (p == 0)
		{
			close(listenfd);
			time(&ticks);
			printf("Got time!\n");
			struct tm *p;
			p = gmtime(&ticks);

			snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
			printf("Generate message!\n");

			sleep(10);
			write(connfd, buff, strlen(buff));
			close(connfd);
			exit(0);
		}
		else
		{
			close(connfd);
		}
	}
}
