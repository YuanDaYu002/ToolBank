#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "tcpclient.h"
 
#define BUFFER_SIZE 1024
 
/*******************************************************************************
*@ Description    : TCP 客户端（设备端）socket 的创建
*@ Input          :<host>远端服务端的域名
					<port>远端服务端的端口号
*@ Output         :<pclient> 传入的客户端信息指针（返回）
*@ Return         :成功：0 失败：负数
*@ attention      :
*******************************************************************************/
int tcpclient_create(tcpclient *pclient,const char *host, int port){
    struct hostent *he;
 
    if(pclient == NULL) return -1;
    memset(pclient,0,sizeof(tcpclient));
 
    if((he = gethostbyname(host))==NULL){
        return -2;
    }
 
    pclient->remote_port = port;
    strcpy(pclient->remote_ip,inet_ntoa( *((struct in_addr *)he->h_addr) ));
 
    pclient->_addr.sin_family = AF_INET;
    pclient->_addr.sin_port = htons(pclient->remote_port);
    pclient->_addr.sin_addr = *((struct in_addr *)he->h_addr);
 
    if((pclient->socket = socket(AF_INET,SOCK_STREAM,0))==-1){
        return -3;
    }
 
    /*TODO:是否应该释放内存呢?*/
 
    return 0;
}
 
int tcpclient_conn(tcpclient *pclient){
    if(pclient->connected)
        return 1;
 
    if(connect(pclient->socket, (struct sockaddr *)&pclient->_addr,sizeof(struct sockaddr))==-1){
        return -1;
    }
 
    pclient->connected = 1;
 
    return 0;
}

/*******************************************************************************
*@ Description    :数据接收函数（TCP）
*@ Input          :<pclient>远端URL解析后的描述信息
					<size> 期望接收到的数据长度（传入0则代表只进行一次recv）
*@ Output         :<lpbuff>返回的数据（函数内部自动分配内存）
*@ Return         :成功：实际接收到的数据长度（> 0）
					失败：内存不足(-2)
*@ attention      :
*******************************************************************************/ 
int tcpclient_recv(tcpclient *pclient,char **lpbuff,int size)
{
    int recvnum=0; //已经接收到的数据长度
	int tmpres=0;
    char buff[BUFFER_SIZE];
 
    *lpbuff = NULL;
 
    while(recvnum < size || size==0)
	{
        tmpres = recv(pclient->socket, buff,BUFFER_SIZE,0);
        if(tmpres <= 0)
            break;
        recvnum += tmpres;
 
        if(*lpbuff == NULL)
		{
            *lpbuff = (char*)malloc(recvnum+1);
            if(*lpbuff == NULL)
                return -2;
        }
		else
		{
            *lpbuff = (char*)realloc(*lpbuff, recvnum+1);
            if(*lpbuff == NULL)
                return -2;
        }
        memcpy(*lpbuff+recvnum-tmpres,buff,tmpres);
    }
	
    if (*lpbuff != NULL)
    {
        ((*lpbuff)[recvnum]) = 0;
    }
	
    return recvnum;
}

/*******************************************************************************
*@ Description    :TCP发送函数
*@ Input          :<pclient>TCP 目标地址的信息（URL解析而来）
					<buff>数据 buf 
					<size>数据长度
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
int tcpclient_send(tcpclient *pclient,char *buff,int size){
    int sent=0,tmpres=0;
 
    while(sent < size){
        tmpres = send(pclient->socket,buff+sent,size-sent,0);
        if(tmpres == -1){
            return -1;
        }
        sent += tmpres;
    }
    return sent;
}

/*******************************************************************************
*@ Description    :
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
int tcpclient_close(tcpclient *pclient){
    close(pclient->socket);
    pclient->connected = 0;
    return 0;
}


