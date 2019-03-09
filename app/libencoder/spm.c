
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "spm.h"

typedef struct __tag_STREAM_LIST_NODE {
 	unsigned int ref_count;
	unsigned int length; /*pack中的data length*/
	struct __tag_STREAM_LIST_NODE *next;
	ENC_STREAM_PACK pack;
} STREAM_LIST_NODE;

typedef struct {
	unsigned int free;
	STREAM_LIST_NODE *free_list;
} STREAM_FREE_LIST;

typedef struct __tag_SPM_CONTEXT {
	unsigned int max; /*pack总数*/
	unsigned int count; /*已经分配的PACK数目*/
	unsigned int used_mem; /*已经分配的内存大小*/

	pthread_mutex_t free_lock;
	pthread_cond_t free_cond;
} SPM_CONTEXT;

static SPM_CONTEXT spm_ctx = {0};

/*
	function:  spm_alloc
	description:  空闲码流包请求接口
	args:
	return:
		non-NULL  success  指向申请到的空闲包的指针
		NULL    fail
 */
ENC_STREAM_PACK *spm_alloc(unsigned int length)
{
	if (spm_ctx.max == 0)
		return NULL;

	STREAM_LIST_NODE *node = (STREAM_LIST_NODE *) malloc(sizeof (STREAM_LIST_NODE) + length);

	
	if (node == NULL) {
		ERROR_LOG("malloc pack failed, length = %d\n", length);
		return NULL;
	}

	pthread_mutex_lock(&spm_ctx.free_lock);
	while (spm_ctx.count == spm_ctx.max) {
		FATAR_LOG("SPM no free packet left, wait for free signal\n");
		pthread_cond_wait(&spm_ctx.free_cond, &spm_ctx.free_lock);
	}

	//printf("spm_ctx.count++ !\n");
	spm_ctx.count++;
	spm_ctx.used_mem += length;
	pthread_mutex_unlock(&spm_ctx.free_lock);

	node->pack.data = (HLE_U8 *) node + sizeof (STREAM_LIST_NODE);
	node->next = NULL;
	node->ref_count = 0;
	node->length = length;
	return &node->pack;
}



#define OFFSET_OF(TYPE, MEMBER)    ((unsigned int) &((TYPE *)0)->MEMBER)
#define CONTANER_OF(PTR, TYPE, MEMBER)  ((TYPE *)((char *)PTR - OFFSET_OF(TYPE, MEMBER)))

/*
	function:  spm_inc_pack_ref
	description:  增加包引用计数接口(+1)
	args:
		ENC_STREAM_PACK *pack  码流包
	return:
		0  success
		-1  fail
 */
int spm_inc_pack_ref(ENC_STREAM_PACK *pack)
{
	if (pack == NULL)
		return -1;
	if (spm_ctx.max <= 0)
		return -1;
	//依据pack的地址反推 node 的首地址
	STREAM_LIST_NODE *node = CONTANER_OF(pack, STREAM_LIST_NODE, pack);
	__sync_add_and_fetch(&node->ref_count, 1);

	return 0;
}

//debug
int spm_pack_print_ref(ENC_STREAM_PACK *pack)
{
	if (pack == NULL)
		return -1;
	if (spm_ctx.max <= 0)
		return -1;

	STREAM_LIST_NODE *node = CONTANER_OF(pack, STREAM_LIST_NODE, pack);
	printf("node->ref_count = %d\n",node->ref_count);

	return 0;
}

/*
	function:  spm_dec_pack_ref
	description:  减少包引用计数接口(-1)，减完后如果引用计数为0则释放该包
	args:
		ENC_STREAM_PACK *pack  码流包
	return:
		0  success
		-1  fail
 */
int debug_i = 0;
int spm_dec_pack_ref(ENC_STREAM_PACK *pack)
{
	if (pack == NULL)
		return -1;
	if (spm_ctx.max <= 0)
		return -1;

	STREAM_LIST_NODE *node = CONTANER_OF(pack, STREAM_LIST_NODE, pack);
	//debug*********
	//printf("spm_ctx.count = %d  spm_ctx.max = %d ref_count = %d \n",spm_ctx.count,spm_ctx.max,node->ref_count);
	//**************

	if (__sync_sub_and_fetch(&node->ref_count, 1) <= 0) 
	{
		int trigger = 0; 
		pthread_mutex_lock(&spm_ctx.free_lock);

		//if (spm_ctx.count == spm_ctx.max) /*trigger a signal if free list was empty before add this node*/
		if (spm_ctx.count >= spm_ctx.max)
			trigger = 1;
		//debug
		debug_i++;
		if(40 == debug_i)
		{
			printf("pm_ctx.count--! spm_ctx.count = %d\n",spm_ctx.count);
			debug_i = 0;
		}
		
		
		spm_ctx.count--;
		spm_ctx.used_mem -= node->length;
		pthread_mutex_unlock(&spm_ctx.free_lock);

		free(node);


		if (trigger)
		{
			printf("pthread_cond_signal !\n");
			pthread_cond_signal(&spm_ctx.free_cond);
		}

	}

	return 0;
}

/*
	function:  spm_init
	description:  码流包管理模块初始化接口
	args:
		int pack_count  预分配的码流包个数

	return:
		0  success
		-1  fail
 */
int spm_init(int pack_count)
{
	if (pack_count <= 0)
		return -1;

	if (spm_ctx.max != 0) {
		ERROR_LOG("Do not init twice!\n");
		return -1;
	}

	spm_ctx.max = pack_count;
	spm_ctx.count = 0;
	spm_ctx.used_mem = 0;
	pthread_mutex_init(&spm_ctx.free_lock, NULL);
	pthread_cond_init(&spm_ctx.free_cond, NULL);

	return 0;
}

/*
	function:  spm_exit
	description:  码流包管理模块去初始化接口
	args:

	return:
 */
void spm_exit(void)
{
	if (spm_ctx.max <= 0)
		return;

	pthread_mutex_lock(&spm_ctx.free_lock);
	if (spm_ctx.count != 0) {
		pthread_mutex_unlock(&spm_ctx.free_lock);
		ERROR_LOG("Not all pack returned to spm, can not free it now!\n");
		return;
	}
	pthread_mutex_unlock(&spm_ctx.free_lock);

	pthread_cond_destroy(&spm_ctx.free_cond);
	pthread_mutex_destroy(&spm_ctx.free_lock);
	spm_ctx.max = 0;
}

void spm_debug_info(void)
{
	DEBUG_LOG("--------------------spm_debug_info-------------------\n");
	pthread_mutex_lock(&spm_ctx.free_lock);
	DEBUG_LOG("max %d, count %d, used_mem %u\n", spm_ctx.max, spm_ctx.count, spm_ctx.used_mem);
	pthread_mutex_unlock(&spm_ctx.free_lock);
}

//test
#if 0
#define PACK_COUNT  10

int main(int argc, char *argv[])
{
	spm_init(PACK_COUNT + 5);
	spm_init(50);

	//spm_exit();
	int i;
	for (i = 0; i < PACK_COUNT; i++) {
		ENC_STREAM_PACK *p = spm_alloc(3000);
		printf("in %p\n", p);
		spm_enqueue(p);
	}

	for (i = 0; i < PACK_COUNT; i++) {
		ENC_STREAM_PACK *p = spm_dequeue();
		printf("out %p\n", p);
		spm_free(p);
	}

	spm_exit();

	return 0;
}
#endif




