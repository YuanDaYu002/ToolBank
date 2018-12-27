/*******************************************************************
server端程序：
从设备端（client）接收一个文件到本地主机（功能和LiteOS自带的 http 命令一样）
1.主机端运行server程序在后台监听
2.设备端配好网络后，在设备端执行命令调用 client端程序
********************************************************************/
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>





typedef struct _file_info_t
{
	unsigned int	filesize;  		//文件大小
	unsigned char 	file_name[32];	//文件名
}file_info_t;
#define portnum  5555  //端口有可能端口被占用，可以改端口才能通
#define RECV_BUF_SIZE (1024*1024)

int  main(int argc,char**argv)
{
	char need_help;

	need_help = !strcmp(argv[2],"-h")||!strcmp(argv[2],"-help");
	printf("need_help = %d\n",need_help);	


		
	if(argc == 0||need_help)
	{
		
		printf("usage: tcp_send\n\
			Atteion : need to be used in conjunction with server process: tcp_recv\n\
			eg : \n\
			<linux computer>: tcp_recv\
			<board>: tcp_send /jffs0/fmp4.mp4 192.168.XXX.XXX\n\n");
		return 0;
	}
	

	
	int sockfd;
	int new_fd;//建立连接后会返回一个新的fd
	struct sockaddr_in sever_addr; //服务器的IP地址
	struct sockaddr_in client_addr; //设备端的IP地址
	char *buffer = (char*)malloc(RECV_BUF_SIZE);
	if(NULL == buffer)
	{
		perror("malloc failed!\n");
		return -1;
	}
	char *p_buffer = buffer;
	memset(buffer,0,RECV_BUF_SIZE);
	int nbyte;
	int checkListen;
	file_info_t file_info = {0}; //用于接收文件描述信息
	unsigned int countbytes = 0;
	int tmp_file; //临时文件描述符
	
	/***为了消除accept函数第3个参数的类型不匹配问题的警告***********/
	int sockaddr_size = sizeof(struct sockaddr);
	/******************************************************/
	
	//1.创建套接字
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("create socket error!\n");
		return -1;
	}
	printf("create socket success!\n");

	//2.1设置要绑定的服务器地址
	bzero(&sever_addr,sizeof(struct sockaddr_in));
	sever_addr.sin_family = AF_INET;
	sever_addr.sin_port = htons(portnum);//将主机字节序转换成网络字节序
	sever_addr.sin_addr.s_addr = htonl(INADDR_ANY);//绑定任意的IP地址，要和网络上所有的IP地址通讯
	
	
	//2.2绑定地址
	if(bind(sockfd,(struct sockaddr*)&sever_addr,sizeof(struct sockaddr)) < 0)
	{
		printf("bind socket error!\n");
		return -1;
	}
	printf("bind socket success!\n");
	
	//3.监听端口
	printf("into listen portnum(%d)......!\n",portnum);
	checkListen = listen(sockfd,5);
	if(checkListen < 0)
	{
		perror("socket listen error!\n");
		return -1;
	}
	
	do
	{
		//4.等待连接
		new_fd = accept(sockfd,(struct sockaddr*)(&client_addr),(socklen_t *)(&sockaddr_size));
		if(new_fd<0)
		{
			printf("accept eror!\n");
			return -1;
		}
		//printf("sever get connection from %s\n",inet_ntoa(client_addr.sin_addr.s_addr));
		
		//5.接收文件大小信息
		recv(new_fd,&file_info,sizeof(file_info),0);
		printf("receive file size = %ld bytes\n",file_info.filesize);
		printf("receive file name = %s \n",file_info.file_name);
		if(file_info.filesize <= 0)return -1;
		
		//解析出文件的最终名字（如若文件名带路径分隔符“/”，则需要截取最后一段）
		unsigned char*name = file_info.file_name;
		unsigned char*flag = NULL;
		while(1)
		{
			flag = strstr(name,"/");
			if(NULL == flag)
			{
				printf("final name : %s\n",name);
				memcpy(file_info.file_name,name,strlen(name)+1);
				break;
			}
			else
			{
				name = flag + 1; //跳过“/”
			}
			
		}

		
		//创建本地对应的文件
		tmp_file = open(file_info.file_name, O_RDWR| O_CREAT,0660);
		if( tmp_file < 0)
		{
			
			printf("open [%s]  error !\n",file_info.file_name);
			return -1;
		}
		printf("open [%s]  success !\n",file_info.file_name);

		//5.1接收数据
		while(1)
		{
			if(countbytes >= file_info.filesize)break;
			nbyte = recv(new_fd,p_buffer,file_info.filesize - countbytes,0);
			countbytes = countbytes + nbyte;
			//接收到的数据及时写入到文件
			unsigned int tmp_ret = write(tmp_file, p_buffer, nbyte);
			if(tmp_ret < 0)
			{
				perror("write file error!\n");
				return -1;
			}
			
		}
		printf("success ! server received (%d)bytes!\n",countbytes);

	
	}while(0);

	
	close(tmp_file);
	close(new_fd);
	close(sockfd);
	free(buffer);

	return 0;
}


