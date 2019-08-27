#include "log.h"
#include "buffer.h"

#ifdef NO_INLINE
#define inline
#endif

struct BufferData *sEmptyBuffer = NULL;

struct BufferData *bd_init(struct BufferData *_this, int maxBufferSize)
{
	_this->mFilledSize = 0;
	_this->floatBufferSize = 0;
	_this->floatOverlapSize = 0;
	_this->floatFilledSize = 0;
	_this->mMaxBufferSize = 0;

	bd_reset(_this);

	_this->mData = NULL;
	_this->floatData = NULL;
	if (maxBufferSize > 0)
	{
		_this->mMaxBufferSize = maxBufferSize;
		_this->floatBufferSize = _this->mMaxBufferSize/2;
	}
	return _this;
}

void bd_finalize(struct BufferData * _this)
{
	if (_this->mData != NULL)
	{
		myfree(_this->mData);
	}
	if (_this->floatData != NULL)
	{
		myfree(_this->floatData);
	}
}

inline struct BufferData *bd_getNullBuffer()
{
	if(sEmptyBuffer == NULL)sEmptyBuffer = _new(struct BufferData, bd_init, 0);
	return sEmptyBuffer;
}

inline char *bd_getData(struct BufferData * _this)
{
	if (_this->mData == NULL && _this->mMaxBufferSize > 0)
	{
		_this->mData = (char *)mymalloc(sizeof(char) *_this->mMaxBufferSize);
	}
	return _this->mData;
}

inline float *bd_getFloatData(struct BufferData * _this)
{
	if (_this->floatData == NULL && _this->floatBufferSize > 0)
	{
		_this->floatData = (float *)mymalloc(sizeof(float) * _this->mMaxBufferSize/2);
	}
	return _this->floatData;
}

inline bool bd_isNULL(struct BufferData * _this)
{
	return !(_this->mData != NULL || _this->floatData != NULL);
}

inline void bd_reset(struct BufferData * _this)
{
	_this->mFilledSize = 0;
	_this->floatOverlapSize = 0;
	_this->floatFilledSize = 0;
}

inline int bd_getMaxBufferSize(struct BufferData * _this)
{
	return _this->mMaxBufferSize;
}

inline void bd_setFilledSize(struct BufferData * _this, int size)
{
	_this->mFilledSize = size;
}

inline int bd_getFilledSize(struct BufferData * _this)
{
	return _this->mFilledSize;
}

inline int bd_getFloatFilledSize(struct BufferData * _this)
{
	return _this->floatFilledSize;
}

inline void bd_setFloatFilledSize(struct BufferData * _this, int _filledSize)
{
	_this->floatFilledSize = _filledSize;
}

inline int bd_getFloatBufferSize(struct BufferData * _this)
{
	return _this->floatBufferSize;
}

inline int bd_getFloatOverlapSize(struct BufferData * _this)
{
	return _this->floatOverlapSize;
}

inline void bd_setFloatOverlapSize(struct BufferData * _this, int _floatOverlapSize)
{
	_this->floatOverlapSize = _floatOverlapSize;
}

const char * Buffer_TAG = "struct Buffer";

void b_finalize(struct Buffer *_this)
{
	struct BufferData *pData = NULL;
	while(cq_size_approx(&_this->mProducerQueue) > 0)
	{
		if(cq_try_dequeue(&_this->mProducerQueue, &pData) && !bd_isNULL(pData))
			_delete(pData, bd_finalize);
	}

	while(cq_size_approx(&_this->mConsumeQueue) > 0)
	{
		if(cq_try_dequeue(&_this->mConsumeQueue, &pData) && !bd_isNULL(pData))
			_delete(pData, bd_finalize);
	}
	mtx_destroy(&_this->m);
	cnd_destroy(&_this->cond);
	cq_finalize(&_this->mProducerQueue);
	cq_finalize(&_this->mConsumeQueue);
}

struct Buffer *b_init(struct Buffer *_this, int bufferCount, int bufferSize)
{
	return b_init2(_this, bufferCount, bufferSize, false);
}

struct Buffer *b_init2(struct Buffer *_this, int bufferCount, int bufferSize, bool _createData)
{
	int i2 = 0;
	_this->mBufferSize = bufferSize;
	_this->mBufferCount = bufferCount;
	cq_init2(&_this->mProducerQueue, sizeof(struct BufferData *), _this->mBufferCount);
	cq_init2(&_this->mConsumeQueue, sizeof(struct BufferData *), _this->mBufferCount + 1);
	for (i2 = 0; i2 < _this->mBufferCount; ++i2)
	{
		struct BufferData *tempData = _new(struct BufferData, bd_init, _this->mBufferSize);		
		bool r = cq_try_enqueue(&_this->mProducerQueue, &tempData);
		assert(r);
		if(_createData)bd_getData(tempData);
	}
	mtx_init(&_this->m, mtx_plain);
	cnd_init(&_this->cond);
	return _this;
}

void b_reset(struct Buffer *_this)
{
	int size, i;
	debug(Buffer_TAG, "before reset total buffer count:%d, ProducerQueue Size:%d, ConsumeQueue Size:%d"
		, _this->mBufferCount
		,cq_size_approx(&_this->mProducerQueue)
		,cq_size_approx(&_this->mConsumeQueue));

#ifndef COND_NO_MUTEX
	mtx_lock(&_this->m);
#endif
	size = cq_size_approx(&_this->mProducerQueue);
	for (i = 0; i < size; ++i)
	{
		struct BufferData **pdata = (struct BufferData **)cq_peek(&_this->mProducerQueue);
		if (0 == pdata || 0 == (*pdata)->mData)
		{
			cq_pop(&_this->mProducerQueue);
		}
	}

	size = cq_size_approx(&_this->mConsumeQueue);
	for (i = 0; i < size; ++i)
	{
		struct BufferData *data = NULL;
		if (cq_try_dequeue(&_this->mConsumeQueue, &data) && 0 != data && 0 != data->mData)
		{
			bool r = cq_try_enqueue(&_this->mProducerQueue, &data);
			assert(r);
		}
	}
#ifndef COND_NO_MUTEX
	mtx_unlock(&_this->m);
#endif
	debug(Buffer_TAG, "after reset ProducerQueue Size:%d, ConsumeQueue Size:%d"
		, cq_size_approx(&_this->mProducerQueue)
		, cq_size_approx(&_this->mConsumeQueue));

	cnd_broadcast(&_this->cond);
}

int b_getEmptyCount(struct Buffer *_this)
{
	return cq_size_approx(&_this->mProducerQueue);
}

int b_getFullCount(struct Buffer *_this)
{
	return cq_size_approx(&_this->mConsumeQueue);
}

struct BufferData *b_getEmpty(struct Buffer *_this)
{
	
	struct BufferData *data = NULL;
#ifndef COND_NO_MUTEX
	mtx_lock(&_this->m);
#endif
	
	while (!cq_try_dequeue(&_this->mProducerQueue, &data))
	{
		cnd_wait(&_this->cond, &_this->m);
	}
#ifndef COND_NO_MUTEX
	mtx_unlock(&_this->m);
#endif
	cnd_broadcast(&_this->cond);
	return data;
}

struct BufferData * b_tryGetEmpty(struct Buffer *_this)
{
	struct BufferData *data = NULL;
	bool r = cq_try_dequeue(&_this->mProducerQueue, &data);
	if(r)
	{
		cnd_broadcast(&_this->cond);
		return data;
	}
	return NULL;
}

bool b_putEmpty(struct Buffer *_this, struct BufferData *data)
{
	if (0 == data)return false;
	
#ifndef COND_NO_MUTEX
	mtx_lock(&_this->m);
#endif
	
	while (!cq_try_enqueue(&_this->mProducerQueue, &data))
	{
		cnd_wait(&_this->cond, &_this->m);
	}
#ifndef COND_NO_MUTEX
	mtx_unlock(&_this->m);
#endif
	cnd_broadcast(&_this->cond);
	return true;
}

int getFullIdx = 0;
struct BufferData *b_getFull(struct Buffer *_this)
{
	
	struct BufferData *data = NULL;
#ifndef COND_NO_MUTEX
	mtx_lock(&_this->m);
#endif
	
	while (!cq_try_dequeue(&_this->mConsumeQueue, &data))
	{
		cnd_wait(&_this->cond, &_this->m);
	}
#ifndef COND_NO_MUTEX
	mtx_unlock(&_this->m);
#endif
	cnd_broadcast(&_this->cond);

	if (!bd_isNULL(data))
	{

		getFullIdx ++;
	}
	else
	{

	}
	return data;
}

int putFullIdx = 0;
bool b_putFull(struct Buffer *_this, struct BufferData *data)
{
	if (0 == data)return false;
	if (!bd_isNULL(data))
	{

		putFullIdx ++;
	}
	
#ifndef COND_NO_MUTEX
	mtx_lock(&_this->m);
#endif
	
	while (!cq_try_enqueue(&_this->mConsumeQueue, &data))
	{
		cnd_wait(&_this->cond, &_this->m);
	}
#ifndef COND_NO_MUTEX
	mtx_unlock(&_this->m);
#endif
	cnd_broadcast(&_this->cond);
	return true;
}

struct BufferData *b_getImpl(struct Buffer *_this, TBlockingQueue *queue)
{
	if (0 != queue)
	{
		struct BufferData *data = NULL;
#ifndef COND_NO_MUTEX
		mtx_lock(&_this->m);
#endif
		
		while (!cq_try_dequeue(queue, &data))
		{
			cnd_wait(&_this->cond, &_this->m);
		}
#ifndef COND_NO_MUTEX
		mtx_unlock(&_this->m);
#endif
		cnd_broadcast(&_this->cond);
		return data;
	}
	return 0;
}

bool b_putImpl(struct Buffer *_this, struct BufferData *data, TBlockingQueue *queue)
{
	if (0 != queue && 0 != data)
	{
#ifndef COND_NO_MUTEX
		mtx_lock(&_this->m);
#endif
		
		while (!cq_try_enqueue(queue, &data))
		{
			cnd_wait(&_this->cond, &_this->m);
		}
#ifndef COND_NO_MUTEX
		mtx_unlock(&_this->m);
#endif
		cnd_broadcast(&_this->cond);
		return true;
	}
	return false;
}
