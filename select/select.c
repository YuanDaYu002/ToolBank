/*********************************************************
				select()实现多路复用 
1.	create socket
2.	bind		
3.	select
*********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define BUF_SIZE 100

int main()
{
	//socket
	int iServer = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == iServer)
	{
		return -1;
	}
	printf("socket ok\r\n");
	
	//bind
	struct sockaddr_in stServer;
	memset(&stServer, 0, sizeof(struct sockaddr_in));
	stServer.sin_family = AF_INET;
	stServer.sin_port = htons(8866);
	stServer.sin_addr.s_addr = INADDR_ANY;
	int iRet = bind(iServer, (struct sockaddr *)&stServer, sizeof(struct sockaddr_in));
	if (-1 == iRet)
	{
		return -1;
	}
	printf("bind ok\r\n");
	
	//listen
	iRet = listen(iServer, 5);
	if (-1 == iRet)
	{
		return -1;
	}
	printf("listen ok\r\n");
	
	
	//不同的部分：
	//select（）
	fd_set stRSet;//fd_set是一个结构体类型
	FD_ZERO(&stRSet);//清除stRSet中所有的文件描述
	char buf[BUF_SIZE];
	while(1)
	{
		FD_SET(0, &stRSet);//0：标准输入，即将标准输入加入到stRSet
		FD_SET(iServer, &stRSet);//将iServer文件描述符加入到stRSet
		iRet = select(iServer+1, &stRSet, NULL, NULL, NULL);
		if (iRet <= 0)
		{
			continue;
		}
		if (FD_ISSET(iServer, &stRSet))//判断iServer是否在stRSet中
		{
			int iClient = accept(iServer, NULL, NULL);
			if (iClient < 0)
			{
				continue;
			}
			memset(buf, 0, BUF_SIZE);
			recv(iClient, buf, BUF_SIZE, 0);
			printf("iserver %s\r\n", buf);
			send(iClient, buf, BUF_SIZE, 0);
			close(iClient);
		}
		if (FD_ISSET(0, &stRSet))//判断标准输入是否在stRSet中
		{
			memset(buf, 0, BUF_SIZE);
			fgets(buf, BUF_SIZE, stdin);
			printf("stdin %s\r\n", buf);
		}
	}
	return 0;
}
