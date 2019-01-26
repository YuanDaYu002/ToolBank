#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H


/*
缓存池初始化
返回：
	成功：  0
	失败：-1
*/
int mem_poll_init(void);

/*
内存申请	
*/
void *mem_poll_Alloc(int size);


/*
内存释放
*/
void mem_poll_Free(void *ptr);

/*
缓存池的销毁
*/
void mem_poll_destory(void);


#endif


