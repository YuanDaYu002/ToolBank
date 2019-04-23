 
/***************************************************************************
* @file:my_malloc.h 
* @author:   
* @date:  4,11,2019
* @brief:  对malloc系列函数进行封装，方便查找内存泄露位置
* @attention:
***************************************************************************/

#ifndef _MY_MALLOC_H
#define _MY_MALLOC_H

#define MAX_MALLOC_TIMES 	600  //最大的malloc次数（泛指 malloc calloc realloc 操作次数）
// 600 * 36/1024 = 21 kB 大小
typedef struct _malloc_mem_info_t
{
	void* address;				//malloc空间的地址记录
	char file_line[32];			//记录文件名和行号，格式 my_malloc.c:66(代表在my_malloc.c文件66行处的malloc内存没有释放)
}malloc_mem_info_t;



#endif
