
/***************************************************************************
* @file:my_malloc.c 
* @author:   
* @date:  4,11,2019
* @brief:  对malloc系列函数进行封装，方便查找内存泄露位置
* @attention:
***************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "my_malloc.h"

//用来存储每次 malloc 的相关信息
malloc_mem_info_t *malloc_mem_info = NULL;

int my_malloc_init(void)
{
	malloc_mem_info = (malloc_mem_info_t*)calloc(MAX_MALLOC_TIMES*sizeof(malloc_mem_info_t),sizeof(char));
	if(NULL == malloc_mem_info) 
		return -1;

	return 0;
}

static int malloc_index = 0;
/*******************************************************************************
*@ Description    :
*@ Input          :<size> 需求内存大小
					<file_line> 文件名和行号，格式 "my_malloc.c:66"
								传入 __FILE__:__LINE__ 即可
*@ Output         :
*@ Return         :成功 ：返回地址 ；失败：NULL
*@ attention      :
*******************************************************************************/
void * my_malloc(int size,char* file_name)
{
	void * ptr = malloc(size);
	if(ptr)
	{
		//记录返回的地址，统一管理
		malloc_mem_info[malloc_index].address = ptr;
		strncpy(malloc_mem_info[malloc_index].file_line,file_name,sizeof(malloc_mem_info.file_line));
		malloc_index++;
		return ptr;
	}
	else
	{
		return NULL;
	}

}

void my_free(void *ptr)
{

}
void * my_calloc(int nmemb, int size)
{

}

void * my_realloc(void *ptr, int size)
{


}


