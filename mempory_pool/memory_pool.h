#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#define USE_MEM_POOL     0      //使用缓存池,总控开关（主要用来给音视频编码帧提供内存空间）

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


