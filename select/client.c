/*********************************************************
				select()实现多路复用的客户端程序
1.	create socket
2.	connect		
3.	send
4.	recv
*********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 100

int main()
{
	//create socket
	int iClient = socket(AF_INET, SOCK_STREAM, 0);//SOCK_STREAM:流式套接字，TCP协议用
	if (-1 == iClient)
	{
		return -1;
	}
	printf("socket ok\r\n");
	
	//connect
	struct sockaddr_in stServer;
	memset(&stServer, 0, sizeof(struct sockaddr_in));
	stServer.sin_family = AF_INET;
	stServer.sin_port = htons(8866);
	stServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	int iRet = connect(iClient, (struct sockaddr *)&stServer, sizeof(struct sockaddr));
	if (-1 == iRet)
	{
		perror("connect");
		return -1;
	}
	printf("connect ok\r\n");
	
	//send
	char buf[BUF_SIZE] = {0};
	gets(buf);
	send(iClient, buf, BUF_SIZE, 0);
	memset(buf, 0, BUF_SIZE);
	
	//recv
	iRet = recv(iClient, buf, BUF_SIZE, 0);
	printf("%d %s\r\n", iRet, buf);
	return 0;
}
