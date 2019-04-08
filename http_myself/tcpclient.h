#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__
 
#include <netinet/in.h>
#include <sys/socket.h>
 
typedef struct _tcpclient{
    int socket; 			//本地客户端的socket
    int remote_port;		//远端服务端的 端口	
    char remote_ip[16];		//远端服务端的ip
    struct sockaddr_in _addr;	//套接字属性
    int connected;			//是否已经连接
} tcpclient;
 
int tcpclient_create(tcpclient *, const char *host, int port);
int tcpclient_conn(  tcpclient *);
int tcpclient_recv(  tcpclient *, char **lpbuff,    int size);
int tcpclient_send(  tcpclient *, char *buff,       int size);
int tcpclient_close( tcpclient *);
 
#endif

