#ifndef _MY_INET_H
#define _MY_INET_H

// 本机大端返回1，小端返回0
int checkCPUendian(void);

// 模拟htonl函数，本机字节序转网络字节序
unsigned long int t_htonl(unsigned long int h);

// 模拟ntohl函数，网络字节序转本机字节序
unsigned long int t_ntohl(unsigned long int n);

 
// 模拟htons函数，本机字节序转网络字节序
unsigned short int t_htons(unsigned short int h);


// 模拟ntohs函数，网络字节序转本机字节序
unsigned short int t_ntohs(unsigned short int n);

// 模拟ntohll函数，网络字节序转本机字节序
unsigned long long t_ntohll(unsigned long long val);

// 模拟htonll函数，本机字节序转网络字节序
unsigned long long t_htonll(unsigned long long val);




#endif

