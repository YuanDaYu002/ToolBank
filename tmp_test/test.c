 
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
	int i = 0;
	for(i = 1;i<16;i++)
	{
		if(i=5)
		{
			break;
		}
	}	
	printf("i = %d\n",i);
		
return 0;

}


