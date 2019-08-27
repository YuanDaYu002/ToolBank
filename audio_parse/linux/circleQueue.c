#include <assert.h>
#include "circularQueue.h"
#include "memory.h"
#include "util.h"

#ifdef NO_INLINE
#define inline
#endif

struct CircularQueue *cq_init(struct CircularQueue *_this, int _sizeofT)
{
	return cq_init2(_this, _sizeofT, CircularQueue_initialSize);
}

struct CircularQueue *cq_init2(struct CircularQueue *_this, int _sizeofT, int _initSize)
{
	_this->m_iCapacity=_initSize;
	_this->m_iSize = 0;
	_this->m_iRear = -1;
	_this->m_iFront = -1;
	_this->sizeofT = _sizeofT;
	mtx_init(&_this->m, mtx_plain);
	_this->m_arrContainer = (char *)mymalloc(_this->m_iCapacity *_this->sizeofT);
	return _this;
}

void cq_finalize(struct CircularQueue *_this)
{	
	myfree(_this->m_arrContainer);
	_this->m_arrContainer = 0;
	mtx_destroy(&_this->m);
};

bool cq_peek2(struct CircularQueue *_this, void ** _elem)
{
	bool r = false;
	mtx_lock(&_this->m);

	if(_this->m_iRear != -1)
	{
		r = true;
		*_elem = _this->m_arrContainer+_this->m_iRear*_this->sizeofT;
	}
	mtx_unlock(&_this->m);
	return r;
}

bool cq_peekBotton1(struct CircularQueue *_this, void **_elem)
{
	bool r = false;
	mtx_lock(&_this->m);

	if(_this->m_iFront != -1)
	{
		r = true;
		*_elem = _this->m_arrContainer+_this->m_iFront*_this->sizeofT;
	}
	mtx_unlock(&_this->m);
	return r;
}

void *cq_peek(struct CircularQueue *_this)
{
	void *elem = NULL;
	mtx_lock(&_this->m);

	if(_this->m_iRear != -1)
	{
		elem = _this->m_arrContainer+_this->m_iRear*_this->sizeofT;
	}
	mtx_unlock(&_this->m);
	return elem;
}

void *cq_peekBotton(struct CircularQueue *_this)
{
	void *elem = NULL;
	mtx_lock(&_this->m);

	if(_this->m_iFront != -1)
	{
		elem = _this->m_arrContainer+_this->m_iFront*_this->sizeofT;
	}
	mtx_unlock(&_this->m);
	return elem;
}

void * cq_push_(struct CircularQueue *_this, void *elem)
{
	
	if(_this->m_iRear == -1)	
	{
		
		asign(_this->m_arrContainer+((++_this->m_iRear)*_this->sizeofT), elem, _this->sizeofT);
		assert(memcmp(_this->m_arrContainer+(_this->m_iRear*_this->sizeofT), elem, _this->sizeofT) == 0);
		_this->m_iFront = _this->m_iRear;		
		_this->m_iSize++;	
	}
	
	else if((_this->m_iFront < _this->m_iCapacity-1) && (_this->m_iFront+1 != _this->m_iRear) )
	{
		
		asign(_this->m_arrContainer+((++_this->m_iFront)*_this->sizeofT), elem, _this->sizeofT);
		assert(memcmp(_this->m_arrContainer+(_this->m_iFront*_this->sizeofT), elem, _this->sizeofT) == 0);
		_this->m_iSize++;	
	}
	
	else if(_this->m_iFront == _this->m_iCapacity-1 && _this->m_iRear != 0)	
	{
		_this->m_iFront = 0;
		
		asign(_this->m_arrContainer+(_this->m_iFront*_this->sizeofT), elem, _this->sizeofT);
		assert(memcmp(_this->m_arrContainer+(_this->m_iFront*_this->sizeofT), elem, _this->sizeofT) == 0);
		_this->m_iSize++;	
	}
	
	else
	{

		throwError("No space available");
	}
	return _this->m_arrContainer+_this->m_iFront*_this->sizeofT;
}

void * cq_push(struct CircularQueue *_this, void * elem)
{
	void * r;
	mtx_lock(&_this->m);

	r = cq_push_(_this, elem);
	mtx_unlock(&_this->m);
	return r;
}

void cq_pop_(struct CircularQueue *_this, void *elem)
{
	if(_this->m_iRear == -1)
		throwError("The queue is already empty");

	if (elem != NULL)
	{
		asign(elem, _this->m_arrContainer+_this->m_iRear*_this->sizeofT, _this->sizeofT);
		assert(memcmp(elem, _this->m_arrContainer+(_this->m_iRear*_this->sizeofT), _this->sizeofT) == 0);
		
	}
	
	if(_this->m_iRear == _this->m_iFront)	
		_this->m_iRear = _this->m_iFront = -1;
	else if(_this->m_iRear == _this->m_iCapacity -1)
		_this->m_iRear = 0;
	else 
		_this->m_iRear++;

	_this->m_iSize--;
	
}

void cq_pop(struct CircularQueue *_this)
{
	mtx_lock(&_this->m);

	cq_pop_(_this, NULL);
	mtx_unlock(&_this->m);
}

bool cq_try_push(struct CircularQueue *_this, void *_ele)
{
	bool r = false;
	mtx_lock(&_this->m);

	if (_this->m_iSize < _this->m_iCapacity)
	{
		cq_push_(_this, _ele);
		r = true;
	}
	mtx_unlock(&_this->m);
	return r;
}

bool cq_try_pop(struct CircularQueue *_this, void *_pelem)
{
	bool r = false;
	mtx_lock(&_this->m);

	if (_this->m_iSize > 0)
	{
		cq_pop_(_this, _pelem);
		r = true;
	}
	mtx_unlock(&_this->m);
	return r;
}

inline bool cq_empty(struct CircularQueue *_this){return (_this->m_iSize > 0)? true : false;};
inline void cq_clear(struct CircularQueue *_this){_this->m_iFront = _this->m_iRear = -1;_this->m_iSize = 0;};
inline int cq_size(struct CircularQueue *_this){return _this->m_iSize;};
inline int cq_capacity(struct CircularQueue *_this){return _this->m_iCapacity;};

inline bool cq_try_enqueue(struct CircularQueue *_this, void * element)
{
	return cq_try_push(_this, element);
}

inline void cq_enqueue(struct CircularQueue *_this, void * element)
{
	cq_push(_this, element);
}

inline bool cq_try_dequeue(struct CircularQueue *_this, void *_pelem)
{
	return cq_try_pop(_this, _pelem);
}

inline int cq_size_approx(struct CircularQueue *_this)
{
	return _this->m_iSize;
}

