 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

typedef unsigned int  uint32; 

 // 长整型大小端互换
#define BigLittleSwap32(A)  ((((uint32)(A) & 0xff000000) >> 24) | \
                            (((uint32)(A) & 0x00ff0000) >> 8) | \
                            (((uint32)(A) & 0x0000ff00) << 8) | \
                            (((uint32)(A) & 0x000000ff) << 24))

int main()
{
	//大小端测试
	unsigned int  bytes = 0x1234;
	unsigned char mdatbox[4] = {0};
        mdatbox[0] = (bytes >> 24) & 0xFF;
        mdatbox[1] = (bytes >> 16) & 0xFF;
        mdatbox[2] = (bytes >> 8)  & 0xFF;
        mdatbox[3] = (bytes) & 0xFF;
		int i = 0;
		printf(">> transfer:");
		for (i = 0;i<4;i++)
		{
			printf("%d ",mdatbox[i]);
			
		}
		
		unsigned int tmp = BigLittleSwap32(bytes);
		unsigned char* p = (unsigned char*)&tmp;
		printf("BigLittleSwap32 transfer:");
		for (i = 0;i<4;i++)
		{
			printf("%d ",p[i]);
			
		}
		printf ("\n");
		
		
		
return 0;

}


