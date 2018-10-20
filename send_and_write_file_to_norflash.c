/************************************************************************
*文件名：send_and_write_file_to_norflash.c
*创建者：yuyuanda
*创见时间：2018/10/11
*文件说明：
功能：发送文件到给设备：client端程序
	专门烧写系统文件（ipc18.bin）用，提高开发调试速度，比ftp传输文件
	再烧写的方式要快，因接收的数据不保存成文件，而是直接从内存写入norflash
使用：配合设备端（服务端）命令 “RecvWriteNor” 使用
	1.设备端运行命令“RecvWriteNor”等待接收数据；
	2.虚拟机（linux主机）下运行客户端程序 send_and_write_file_to_norflash
		格式：send_and_write_file_to_norflash [file name] [deviceIP]
		eg： ./send_and_write_file_to_norflash ipc18.bin 192.168.3.82
注意：目前没有加入文件校验机制
************************************************************************/

#include<stdlib.h>//不加会报隐式声明与内建函数'exit'不兼容
#include<stdio.h>
#include<sys/socket.h>
#include<string.h>
#include<netinet/in.h>//因用了地址

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




//3333是主机字节序
#define portnum  5555

#define ERRASE_SIZE   (7*1024*1024)		   //文件缓存空间大小

int main(int argc,char**argv)
{
	if(argc < 3)
	{
		printf("usage:%s [filename] [device ip]\n",argv[0]);
		return -1;
	}
	int sockfd,checkConnect;
	struct sockaddr_in sever_addr; //服务器的IP地址
	
	//1.创建套接字
	if((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		printf("create socket error!\n");
		exit(1);
	}
	
	//2.1设置需要连接的服务器地址
	bzero(&sever_addr,sizeof(struct sockaddr_in));
	sever_addr.sin_family = AF_INET;
	sever_addr.sin_port = htons(portnum);//host to net  (short) 先要将主机字节序转换成网络字节序
	sever_addr.sin_addr.s_addr =  inet_addr(argv[2]);//绑定IP地址
	
	
	//2.2连接服务器
	checkConnect = connect(sockfd,(struct sockaddr*)(&sever_addr),sizeof(struct sockaddr));
	if(checkConnect < 0)
	{
		printf("connect error!\n");
		exit(1);
	}
	
	
	/*分配缓存空间*/
	char * read_buffer = (char*)malloc(ERRASE_SIZE);
	if(NULL == read_buffer)
	{
		perror("malloc failed !\n");
		return -1;
	}
	memset(read_buffer,0,ERRASE_SIZE);
	
	//打开文件
	int read_file = open((const char *)argv[1], O_RDONLY);
	if( read_file < 0)
	{
		
		printf("open %s  error !\n",argv[1]);
		return -1;
	}
	
	/*读取文件至缓存空间*/
	unsigned int size = read(read_file,read_buffer,ERRASE_SIZE);
	printf("read size = %d bytes\n",size);
	
	//发送文件大小信息
	unsigned int  size_bak = size;
	printf("sizeof(unsigned long) = %d\n",sizeof(unsigned long));
	if(send(sockfd,&size_bak,sizeof(size_bak),0) < 0)
	{
		perror("send file size failed!\n");
		return -1;
	}
	printf("send file size success!\n");
	
	//3.发送数据到服务器
	int countbytes = 0;
	int sendbytes = 0;
	usleep(100);
	while(1)
	{
		if(0 == size) break;
		
		while(1)
		{
			sendbytes  = send(sockfd,read_buffer + countbytes,size,0);
			if(sendbytes < 0)
			{
				perror("send failed!\n");
				return -1;
			}
			
			break;
		}
	
		countbytes = countbytes + sendbytes;
		printf("countbytes = %d\n",countbytes);
		size = size - countbytes;
		usleep(100);
		
	}
	printf("success send(%ld)bytes!\n",countbytes);
	
	
	//4.关闭连接
	close(sockfd);
	free(read_buffer);
	return 0;
	
}
