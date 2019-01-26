/*******************************************************************************
				该缓存池结构主要针对小内存申请。
外部申请时：
1. >= 128K，直接调用系统的malloc函数进行开辟;
2. <  128K，由本模块管理的内存空间进行分配。
H264/AAC/g711 等编码帧数据大小基本符合小内存申请（从几百字节到几十KB）。

简介：

chunk：缓存块（指定为 N * BIG_UNIT）
	缓存池由若干个 chunk组成 ，chunk之间使用单链表连接，如若当前chunk剩余空间不足，
	会自动再向系统申请新chunk（chunk个数会动态增长，可用宏控制最大增长数量）插到
	chunk链表的尾部。这样方便在销毁缓存池的时候统一释放。
node: 缓存块节点（从chunk划分出来的）
	用户每次申请内存，如若回收数组中（最适合档位下）没有对应大小块的node，则需要从
	chunk剩余的空间里划分对档位的块（不够则需要向系统申请新chunk），如果有则直接从
	回收数组对应档位下划分一个node返回，并将该node从链表中删除。
 注意：node 内库空间的分配要按照档位大小来，在满足用户需求的情况下再向上取整
 	  （靠近大一级的档位），否则当该node回收后下次再分配出去就可能出错！！！	
*******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "memory_pool.h"
#include "typeport.h"


/*---最小的单元块 bytes（基本单元块）-2K------*/ 
#define SMALL_UNIT 		(2*1024)    

/*---最大的单元块 bytes（最大单元块）-128K ---*/
#define BIG_UNIT 		(128*1024) 

/*---管理区间----------------------------------------------------------------------
按照最小单元块的增长步长，从最小单元快到最大单元快之间划分的不同大小缓存块区间档位，
缓存池只按照这些档位大小的块进行分配，方便管理
---------------------------------------------------------------------------------*/
#define MANAGE_RANGE	(BIG_UNIT/SMALL_UNIT)  	//64个档位

#define CHUNK_SIZE  	(1024*512)   			//每个chunk块大小，chunk大小不能小于 BIG_UNIT

#define MAX_CHUNK_NUM  	(3) 					//chunk的最大个数，缓存池最大空间：CHUNK_SIZE * MAX_CHUNK_NUM。

/*----------------------------------------------------
chunk 之间连接的（单链表）管理结构
----------------------------------------------------*/
typedef struct _chunk_t
{
	  char				*freesp; //当前可用内存起始位置
	  struct _chunk_t 	*next;	 //下一个chunk的起始地址
}chunk_t;

static chunk_t 	*chunk_head; 	  //chunk链表的头

static int 		chunk_free_size;  //当前申请的chunk还剩余多少空间没被划分成node
static int		cur_chunk_num = 0;	//当前已经申请的chunk个数
/*----------------------------------------------------
node 之间的管理结构，和单链表指针数组构成哈希表管理结构
-----------------------------------------------------*/
typedef struct _free_node_t
{
	unsigned int 			tag;	//该node空间的对应管理数组的下标值(档位值)
	struct _free_node_t* 	next;	//nod节点的后继
}free_node_t;
//可用节点，管理数组（单链表指针数组）,回收数组(哈希表)
static free_node_t* free_node_array[MANAGE_RANGE] = {0};


void *mem_poll_Alloc_node(int size,char tag);


int mem_poll_init(void)
{
	/*初始化回收数组*/
	int i = 1;
	for(i = 1; i < MANAGE_RANGE; i++)
	{
		free_node_array[i] = NULL; 
	}
	
	/*初始化第一个回收数组*/
	if(chunk_head == NULL)
	{
		chunk_head = (chunk_t *)malloc(CHUNK_SIZE); //分配第一个chunk
		if(chunk_head < 0)
		{
			ERROR_LOG("malloc failed!\n");
			return -1;
		}
		/*初始化chunk_head*/
		chunk_head->freesp = (char *)chunk_head + sizeof(chunk_t);
		chunk_head->next = NULL; //第一次申请的chunk即为最后一个chunk节点（next为NULL）
		chunk_free_size = CHUNK_SIZE - sizeof(chunk_t);
	}
	printf("chunk_free_size = %d\n",chunk_free_size);
	return 0;
}

static int print_count = 0;  //调试，控制打印次数
void *mem_poll_Alloc(int size)
{
	void *ret = NULL;
	char i;
	/*计算所需大小对应回收数组的最合适“档位”（下标）值*/
	for(i = 1; i < MANAGE_RANGE; i++)
	{
		if(size <= i * SMALL_UNIT)
		{
			break;//break后i就是当前的i值，i++不会执行
		}	
	}
	print_count++;
	if(print_count >=30)
	{
		DEBUG_LOG("cur_chunk_num(%d) tag = %d\n",cur_chunk_num,i);
		print_count = 0;
	}
	
	if(i == MANAGE_RANGE)//超出了最大单元块的大小,采用系统malloc
	{
		free_node_t *tmp = (free_node_t *)malloc(size + sizeof(free_node_t));
		if(tmp == NULL)
		{
			ERROR_LOG("malloc failed!\n");
			return NULL;
		}
		cur_chunk_num ++;
		tmp->tag = MANAGE_RANGE;//超出（包括）BIG_UNIT大小的内存申请，tag统一标记为MANAGE_RANGE
		tmp->next = NULL;
		ret = (char*)tmp + sizeof(free_node_t);
		printf("alloc from malloc!\n");
	}
	else //采用缓存池的内存空间
	{
		ret = mem_poll_Alloc_node(size,i);
	}
	return ret;
}

/********************************************
功能：	缓存池内存空间申请接口
参数：
		@<size>:所需内存空间大小
		@<tag>：应分配的基本单元个数
注意：该接口的输入参数size必须小于 BIG 的值
********************************************/
void *mem_poll_Alloc_node(int size,char tag)
{
	if(size >= BIG_UNIT)
	{
		printf("\n<ERROR> memory pool Alloc error!\n");
		return NULL;
	}
	
	void *ret = NULL;
	if(free_node_array[tag] == NULL)//回收数组中size 大小对应的链表节点为空 
	{
		//if(size + sizeof(free_node_t) > chunk_free_size)//表述错误//当前chunk 剩余空间不满足空间需求，申请一个新chunk！
		if(tag * SMALL_UNIT + sizeof(free_node_t) > chunk_free_size)
		{
			printf("into 0002\n");
			
			if(cur_chunk_num == MAX_CHUNK_NUM)//chunk个数已经达到上限
			{
				printf("chunk limit !! ");
				return NULL;
			}
			
			/*1.将剩余部分放到对应档位的链表中去*/
			unsigned int tmp_tag = (chunk_free_size-sizeof(free_node_t))/SMALL_UNIT;
			free_node_t* free_node = (free_node_t*)chunk_head->freesp;
			free_node->tag = tmp_tag;
			free_node->next = free_node_array[tmp_tag];
			free_node_array[tmp_tag] = free_node;

			/*2.之前的chunk已经用完，重新申请一个新chunk*/
			chunk_t *ctmp;
			ctmp = (chunk_t*)malloc(CHUNK_SIZE);
			cur_chunk_num ++;
			ctmp->freesp = (char *)ctmp + sizeof(chunk_t);
			ctmp->next = chunk_head; //新申请的chunk节点插入到上一个chunk节点的头部（单链表头插法）
			chunk_head = ctmp;
			chunk_free_size = CHUNK_SIZE - sizeof(chunk_t);
			printf("malloc a new chunk...\n");

			/*3.从新chunk分配所需内存*/
			free_node_t* alloc_node = (free_node_t*)chunk_head->freesp;
			//unsigned int alloc_node_size = size + sizeof(free_node_t); //错误,得按照档位来分配大小,不然下一次该node给别人用有可能空间不足。
			unsigned int alloc_node_size = tag * SMALL_UNIT + sizeof(free_node_t);
			alloc_node->tag = tag;
			alloc_node->next = NULL; //分出去给用户使用的节点都得置空
			ret = (char*)alloc_node + sizeof(free_node_t);
			chunk_head->freesp += alloc_node_size;
			chunk_free_size -= alloc_node_size;
		}
		else//剩余空间够分配“对应档位”大小的节点，直接从剩余空间划分新的节点出来。
		{
			
			free_node_t* new_node = (free_node_t*)chunk_head->freesp;
			//unsigned int new_node_size = size + sizeof(free_node_t);//错误,得按照档位来分配大小
			unsigned int new_node_size = tag * SMALL_UNIT + sizeof(free_node_t);
			new_node->next = NULL;
			new_node->tag = tag;
			
			ret = (char*)new_node + sizeof(free_node_t);
			chunk_head->freesp += new_node_size;
			chunk_free_size -= new_node_size;
			
			//printf("alloc from chunk! new_node_size(%d)\n",new_node_size);
		}
	}
	else //回收数组tag档位的链表上还有有空闲的节点
	{
		
		/*删除该链表第一个节点，将当中指向的内存分配出去*/
		free_node_t* delet_node = free_node_array[tag];
		free_node_array[tag] = free_node_array[tag]->next;

		delet_node->next = NULL;//分出去的node的next都得置空。
		delet_node->tag = tag;
		ret = (char*)delet_node + sizeof(free_node_t);
		//printf("alloc from free_node_array\n");

		/*如果是最后一个节点，需要置空 next*/
	}
	
	return ret;
}


void mem_poll_Free(void *ptr)
{
	free_node_t *tmp = (free_node_t*)((char *)ptr - sizeof(free_node_t));//往前偏移 free_node_t 大小字节，读取描述信息
	unsigned int tag = tmp->tag;
	if(tag == MANAGE_RANGE)//该段内存长度 >= 128K,使用系统free
	{
		printf("use free\n");
		free(tmp);
	}
	else //该段内存长度 < 128K,使用自己的free（即重新挂载到回收数组 free_node_array 中）
	{
		//printf("free ptr to %d\n",tag);
		/*单链表的头插法*/
		tmp->next = free_node_array[tag];
		free_node_array[tag] = tmp;
	}
}

void mem_poll_destory(void)
{
	chunk_t *tmp;
	while(chunk_head != NULL)
	{
		/*单链表的释放*/
		tmp = chunk_head->next;
		free(chunk_head);
		chunk_head = tmp;
	}
	memset(free_node_array,0,sizeof(free_node_array));
	printf("destory memory pool success !\n");
}






