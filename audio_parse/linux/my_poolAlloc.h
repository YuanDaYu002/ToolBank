#ifndef MY_POOLALLOC_H
#define MY_POOLALLOC_H

#include <stdlib.h>
#include <stdio.h>
#include "base.h"
#include "my_vector.h"

struct LinkedQueueItem
{
	struct LinkedQueueItem *next;
	struct LinkedQueueItem *pre;
};

struct LinkedQueue
{
	struct LinkedQueueItem *first;
	struct LinkedQueueItem *last;
	int size_;
};

struct LinkedQueue *lq_init(struct LinkedQueue *_this);
void lq_push(struct LinkedQueue *_this, struct LinkedQueueItem *_item);
struct LinkedQueueItem *lq_pop2(struct LinkedQueue *_this, struct LinkedQueueItem *_item);
struct LinkedQueueItem *lq_pop(struct LinkedQueue *_this);
int lq_size(struct LinkedQueue *_this);
bool lq_check(struct LinkedQueue *_this);

#define PoolAllocatorObjCountPerBlock 64
typedef void (*paf_destroy)(void*);

struct PoolAllocator {
	struct myvector blocks;
	int sizeofT;
	int currBlockIdx;
	int objIdx;
};    
struct PoolAllocator *pa_init(struct PoolAllocator *_this, size_t _sizeofT, paf_destroy _fdestory);
void pa_finalize(struct PoolAllocator *_this);
void * pa_new(struct PoolAllocator *_this);
void pa_delete(struct PoolAllocator *_this, void * p);
void pa_freeAllObjs(struct PoolAllocator *_this);

#endif
