
#include <stdio.h>
#include <unistd.h>
#include "memory_pool.h"




void main(void)
{

	int ret = mem_poll_init();
	if(ret < 0)
	{

		printf("init failed !\n");
		return;
	}

	unsigned int i = 0;

	while(1)
	{
		i++;
		if(i > 50) break;

		unsigned char* ptr1 = (unsigned char*)mem_poll_Alloc(1024);
		if(NULL == ptr1)
		{
			printf("mem_poll_Alloc 1 failed !\n");
			break;
		}

		// unsigned char* ptr2 = (unsigned char*)mem_poll_Alloc(1000);
		// if(NULL == ptr2)
		// {
		// 	printf("mem_poll_Alloc 2 failed !\n");
		// 	break;
		// }

		// unsigned char* ptr3 = (unsigned char*)mem_poll_Alloc(1024);
		// if(NULL == ptr3)
		// {
		// 	printf("mem_poll_Alloc 3 failed !\n");
		// 	break;
		// }

		// unsigned char* ptr4 = (unsigned char*)mem_poll_Alloc(1024*3);
		// if(NULL == ptr4)
		// {
		// 	printf("mem_poll_Alloc 4 failed !\n");
		// 	break;
		// }
		sleep(1);

		mem_poll_Free(ptr1);
		// mem_poll_Free(ptr2);
		// mem_poll_Free(ptr3);
		// mem_poll_Free(ptr4);


	}


	mem_poll_destory();

}