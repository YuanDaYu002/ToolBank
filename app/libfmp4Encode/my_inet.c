
#include "my_inet.h"

#define LOCAL_PLAY_MODE 0   //本地播放模式（小端模式，不进行大端转换） 
typedef unsigned short int uint16;
typedef unsigned long int uint32;

// 短整型大小端互换
#define BigLittleSwap16(A)  ((((uint16)(A) & 0xff00) >> 8) | \
                            (((uint16)(A) & 0x00ff) << 8))

 // 长整型大小端互换
#define BigLittleSwap32(A)  ((((uint32)(A) & 0xff000000) >> 24) | \
                            (((uint32)(A) & 0x00ff0000) >> 8) | \
                            (((uint32)(A) & 0x0000ff00) << 8) | \
                            (((uint32)(A) & 0x000000ff) << 24))


// 本机大端返回1，小端返回0
int checkCPUendian(void)
{
       union{
                unsigned long int i;
                unsigned char s[4];
              }c;

       c.i = 0x12345678;
       return (0x12 == c.s[0]);
}

 
// 模拟htonl函数，本机字节序转网络字节序
unsigned long int t_htonl(unsigned long int h)
{
       #if LOCAL_PLAY_MODE
              return h;
       #endif
       // 若本机为大端，与网络字节序同，直接返回
       // 若本机为小端，转换成大端再返回
       return checkCPUendian() ? h : BigLittleSwap32(h);
}

// 模拟ntohl函数，网络字节序转本机字节序
unsigned long int t_ntohl(unsigned long int n)
{
#if LOCAL_PLAY_MODE
              return n;
#endif

       // 若本机为大端，与网络字节序同，直接返回
       // 若本机为小端，网络数据转换成小端再返回
       return checkCPUendian() ? n : BigLittleSwap32(n);
}

 
// 模拟htons函数，本机字节序转网络字节序
unsigned short int t_htons(unsigned short int h)
{
#if LOCAL_PLAY_MODE
              return h;
#endif

       // 若本机为大端，与网络字节序同，直接返回
       // 若本机为小端，转换成大端再返回
       return checkCPUendian() ? h : BigLittleSwap16(h);
}

// 模拟ntohs函数，网络字节序转本机字节序
unsigned short int t_ntohs(unsigned short int n)
{
#if LOCAL_PLAY_MODE
              return n;
#endif

       // 若本机为大端，与网络字节序同，直接返回
       // 若本机为小端，网络数据转换成小端再返回
       return checkCPUendian() ? n : BigLittleSwap16(n);
}



// 模拟ntohll函数，网络字节序转本机字节序
unsigned long long t_ntohll(unsigned long long val)
  {
     
   if(0 == checkCPUendian())//小端机器
      {
         return (((unsigned long long )t_htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)t_htonl((int)(val >> 32));
     }
     else 
     {
         return val;
     }
 }

 // 模拟htonll函数，本机字节序转网络字节序
 unsigned long long t_htonll(unsigned long long val)
  {
     if (0 == checkCPUendian())//小端机器
      {
         return (((unsigned long long )t_htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)t_htonl((int)(val >> 32));
     }
     else 
     {
         return val;
     }
 }

