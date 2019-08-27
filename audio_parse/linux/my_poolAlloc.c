#include "my_poolAlloc.h"

#ifdef NO_INLINE
#define inline
#endif

void * pa_allocate(struct PoolAllocator *_this, size_t cnt);
void pa_deallocate(struct PoolAllocator *_this, void *p, size_t);
void pa_freeAllObjs_(struct PoolAllocator *_this);

int PoolAllocator_pobj2pitemDistance = 0;

struct LinkedQueue *lq_init(struct LinkedQueue *_this)
{
	_this->first = _this->last = NULL;
	_this->size_ = 0;
	return _this;
}

void lq_push(struct LinkedQueue *_this, struct LinkedQueueItem *_item)
{
	assert((_this->size_ == 0 && _this->first == NULL && _this->last == NULL) 
		|| (_this->size_ == 1 && _this->first == _this->last && _this->first->next == NULL && _this->first->pre == NULL)
		|| (_this->size_ > 1 && _this->first->pre == NULL && _this->last->next == NULL && _this->first->next->pre == _this->first && _this->last->pre->next == _this->last));
	assert(_item != NULL && _item->next == NULL && _item->pre == NULL);
	if (_item != NULL)
	{			
		_item->pre = _this->last;
		_item->next = NULL;
		if (_this->last != NULL)
		{
			_this->last->next = _item;
		}
		else
		{
			assert(_this->first == NULL && _this->size_ == 0);
			_this->first = _this->last = _item;
		}
		_this->last = _item;
		_this->size_ ++;
	}
}

struct LinkedQueueItem *lq_pop2(struct LinkedQueue *_this, struct LinkedQueueItem *_item)
{
	assert((_this->size_ == 0 && _this->first == NULL && _this->last == NULL) 
		|| (_this->size_ == 1 && _this->first == _this->last && _this->first->next == NULL && _this->first->pre == NULL)
		|| (_this->size_ > 1 && _this->first->pre == NULL && _this->last->next == NULL && _this->first->next->pre == _this->first && _this->last->pre->next == _this->last));
	assert(_item != NULL && ((_item->next != NULL)?(_item->next->pre == _item):(_item == _this->last))
		&& ((_item->pre != NULL)?(_item->pre->next == _item):(_item == _this->first)));
	if (_item != NULL)
	{
		if (_item == _this->last)
		{
			_this->last = _item->pre;				
		}
		if (_item == _this->first)
		{
			_this->first = _item->next;				
		}
		if (_item->next != NULL)
		{
			assert(_item->next->pre == _item);
			_item->next->pre = _item->pre;
			_item->next = NULL;
		}
		if (_item->pre != NULL)
		{
			assert(_item->pre->next == _item);
			_item->pre->next = _item->next;
			_item->pre = NULL;
		}
		_this->size_ --;
	}
	return _item;
}

struct LinkedQueueItem *lq_pop(struct LinkedQueue *_this)
{
	struct LinkedQueueItem *r = NULL;
	struct LinkedQueueItem *newLast;
	assert((_this->size_ == 0 && _this->first == NULL && _this->last == NULL) 
		|| (_this->size_ == 1 && _this->first == _this->last && _this->first->next == NULL && _this->first->pre == NULL)
		|| (_this->size_ > 1 && _this->first->pre == NULL && _this->last->next == NULL && _this->first->next->pre == _this->first && _this->last->pre->next == _this->last));
	if (_this->last != NULL)
	{
		r = _this->last;
		newLast = _this->last->pre;
		_this->last->pre = NULL;
		assert(_this->last->next == NULL);
		if (newLast != NULL)
		{
			newLast->next = NULL;
		}
		else
		{
			_this->first = NULL;
		}
		_this->last = newLast;
		_this->size_ -- ;
	}
	return r;
}

inline int lq_size(struct LinkedQueue *_this)
{
	return _this->size_;
}

bool lq_check(struct LinkedQueue *_this)
{
	struct LinkedQueueItem *itor;
	assert((_this->size_ == 0 && _this->first == NULL && _this->last == NULL) 
		|| (_this->size_ == 1 && _this->first == _this->last && _this->first->next == NULL && _this->first->pre == NULL)
		|| (_this->size_ > 1 && _this->first->pre == NULL && _this->last->next == NULL && _this->first->next->pre == _this->first && _this->last->pre->next == _this->last));
	itor = _this->first;
	while (itor != NULL)
	{
		assert(((itor->pre == NULL && itor == _this->first) || (itor->pre->next == itor))
			&& ((itor->next == NULL && itor == _this->last) || (itor->next->pre == itor)));
		itor = itor->next;
	}
	return true;
}

struct PoolAllocator *pa_init(struct PoolAllocator *_this, size_t _sizeofT, paf_destroy _fdestory)
{
	assert(_fdestory == NULL);
	_this->currBlockIdx = 0;
	_this->objIdx = 0;
	_this->sizeofT = _sizeofT;
	vector_init(&_this->blocks, sizeof(void *));
	return _this;
}
void pa_finalize(struct PoolAllocator *_this)
{
	int i;
	for (i = 0; i < vector_size(&_this->blocks); i ++)
	{
		myfree(*(void **)vector_at(&_this->blocks, i));
	}
	vector_finalize(&_this->blocks);
}
void * pa_new(struct PoolAllocator *_this)
{
	int blockObjIdx = (_this->objIdx++)%PoolAllocatorObjCountPerBlock;
	int currBlockIdx = (blockObjIdx == 0 && _this->objIdx > 1)?(++_this->currBlockIdx):_this->currBlockIdx;
	char *currBlock = (char *)((vector_size(&_this->blocks) > currBlockIdx)?(*(void **)vector_at(&_this->blocks, currBlockIdx)):NULL);
	if (currBlock == NULL)
	{
		currBlock = (char *)mymalloc(_this->sizeofT * PoolAllocatorObjCountPerBlock);
		vector_push_back(&_this->blocks, &currBlock);
	}
	return currBlock + (blockObjIdx * _this->sizeofT);
}
void pa_delete(struct PoolAllocator *_this, void * p)
{

}
void pa_freeAllObjs(struct PoolAllocator *_this)
{
	_this->currBlockIdx = 0;
	_this->objIdx = 0;
}