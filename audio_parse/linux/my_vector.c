

#include "my_vector.h"
#include "util.h"

#ifdef NO_INLINE
#define inline
#endif

struct myvector *vector_init2(struct myvector *_this, size_t _sizeofT, vf_destroy _fdestory, size_t n)
{
	_this->sizeofT = _sizeofT;
	_this->data_ = 0;
	_this->capacity_ = 0;
	_this->size_ = 0;
	_this->fdestory = _fdestory;
	vector_reserve(_this, n);
	return _this;
}

struct myvector *vector_init3(struct myvector *_this, size_t _sizeofT, vf_destroy _fdestory, size_t n)
{
	_this->sizeofT = _sizeofT;
	_this->capacity_ = n * _sizeofT;
	_this->size_ = 0;
	_this->fdestory = _fdestory;
	_this->data_ = (char *)malloc(_this->capacity_);
	memset(_this->data_, 0, _this->capacity_);
	return _this;
}

inline struct myvector *vector_init(struct myvector *_this, size_t _sizeofT)
{
	return vector_init2(_this, _sizeofT, NULL, VectorInitialSize);
}
inline bool     vector_empty(struct myvector *_this)      { return _this->size_==0; }
inline unsigned vector_size(struct myvector *_this)       { return _this->size_; }
inline unsigned vector_capacity(struct myvector *_this)   { return _this->capacity_; }

void vector_finalize(struct myvector *_this) {
	unsigned i;
	if(_this->fdestory != NULL)
	{
		for (i=0; i < _this->size_; ++i) {
			_this->fdestory(_this->data_ + i * _this->sizeofT);
		}
	}
	myfree(_this->data_);
	_this->data_ = 0;
	_this->capacity_ = 0;
	_this->size_ = 0;
}

void vector_clear(struct myvector *_this) {
	unsigned i;
	if(_this->fdestory != NULL)
	{
		for (i=0; i < _this->size_; ++i) {
			_this->fdestory(_this->data_ + i * _this->sizeofT);
		}
	}
	_this->size_ = 0;

	vector_reserve(_this, VectorInitialSize);
}

void vector_reserve(struct myvector *_this, unsigned n) {
	char *p;
	if ( n > _this->capacity_ ) {
		
		p = (char *)mymalloc(n * _this->sizeofT);
		memset(p, 0, n * _this->sizeofT);

		memcpy(p, _this->data_, _this->size_ * _this->sizeofT);

		myfree(_this->data_);
		_this->data_     = p;
		_this->capacity_ = n;
	}
}

void vector_reverse(struct myvector *_this, void *_swapBuf)
{
	int i, c;
	for(i = 0, c = _this->size_/2; i < c; i ++)
	{
		memcpy(_swapBuf, _this->data_ + i * _this->sizeofT, _this->sizeofT);
		memcpy(_this->data_ + i * _this->sizeofT, _this->data_ + (_this->size_ - 1 - i) * _this->sizeofT, _this->sizeofT);
		memcpy(_this->data_ + (_this->size_ - 1 - i) * _this->sizeofT, _swapBuf, _this->sizeofT);
	}
}

struct myvector *vector_splice(struct myvector *_this, size_t _start, size_t _end)
{
	size_t i;
	
	if (_start < 0)_start = 0;
	if (_end < _start)_end = _start;
	if(_end < _this->size_)
	{
		int outDel = 0;
		for (i = _end; i < _this->size_; i ++)
		{
			if(_this->fdestory)_this->fdestory(_this->data_+i * _this->sizeofT);
			outDel ++;
		}
		_this->size_ = _this->size_ - outDel;
	}
	if (_start > 0)
	{
		int innerDel = 0;
		for (i = 0; i < _start && i < _this->size_; i ++)
		{
			if(_this->fdestory)_this->fdestory(_this->data_+i * _this->sizeofT);
			innerDel ++;
		}
		if (innerDel > 0 && _this->size_ - innerDel > 0)
		{
			memmove(_this->data_, _this->data_ + _start * _this->sizeofT, (_this->size_ - innerDel) * _this->sizeofT);
		}
		_this->size_ = _this->size_ - innerDel;
	}
	return _this;
}

inline void vector_check_size(struct myvector *_this, unsigned _size)
{
	if (_size > _this->capacity_ ) {
		int newCapacity = _this->capacity_ * 2;
		if (newCapacity == 0)
		{
			newCapacity = VectorInitialSize;
		}
		if (newCapacity < _size)
		{
			newCapacity = _size;
		}
		vector_reserve(_this, newCapacity );
	}
}

void vector_push_back(struct myvector *_this, const void * t ) {
	vector_check_size(_this, _this->size_ + 1);
	asign(_this->data_+_this->size_ * _this->sizeofT, t, _this->sizeofT);
	assert(memcmp(_this->data_+_this->size_ * _this->sizeofT, t, _this->sizeofT) == 0);
	++_this->size_;
}

void vector_append_vector(struct myvector *_this, struct myvector *_that )
{
	vector_check_size(_this, _this->size_ + _that->size_);
	assert(_this->sizeofT == _that->sizeofT);
	mymemcpy(_this->data_+_this->size_ * _this->sizeofT, _that->data_, _that->size_ * _that->sizeofT);
	_this->size_ += _that->size_;
}

void vector_pop_back(struct myvector *_this)
{
	if(_this->size_ > 0)
	{
		_this->size_ = _this->size_ - 1;
	}
	else
	{
		throwError("my_array: pop_back: index out of range");
	}
}

void vector_pollFirst(struct myvector *_this)
{
	if(_this->size_ > 0)
	{
		_this->size_ = _this->size_ - 1;
		memmove(_this->data_, _this->data_ + 1 * _this->sizeofT, _this->size_ * _this->sizeofT);
	}
	else
	{
		throwError("my_array: pollFirst: index out of range");
	}
}

inline void vector_pollLast(struct myvector *_this)
{
	vector_pop_back(_this);
}

inline void *vector_at(struct myvector *_this, unsigned i)
{
	if (i>=_this->size_)
		throwError("my_array: operator[]: index out of range");
	return _this->data_+i * _this->sizeofT;
}

inline void *vector_first(struct myvector *_this)
{
	return vector_at(_this, 0);
}

inline void *vector_nativep(struct myvector *_this)
{
	return _this->data_;
}

inline void *vector_last(struct myvector *_this)
{
	return vector_at(_this, _this->size_ - 1);
}

inline void *vector_back(struct myvector *_this)
{
	if(_this->size_ > 0)
	{
		return vector_at(_this, _this->size_-1);
	}
	else
	{
		throwError("my_array: back: index out of range");
	}
}

void vector_erase(struct myvector *_this, size_t _pos)
{
	if(_pos >= 0 && _pos < _this->size_)
	{
		memmove(_this->data_ + _pos * _this->sizeofT, _this->data_ + (_pos + 1) * _this->sizeofT, (_this->size_ - _pos - 1) * _this->sizeofT);
		_this->size_ = _this->size_ - 1;
	}
	else
	{
		throwError("my_array: erase: index out of range");
	}
}

void vector_insert(struct myvector *_this, size_t _pos, const void * t)
{
	vector_check_size(_this, _this->size_ + 1);
	memmove(_this->data_ + (_pos + 1) * _this->sizeofT, _this->data_ + _pos * _this->sizeofT, (_this->size_ - _pos) * _this->sizeofT);
	memcpy(_this->data_+_pos * _this->sizeofT, t, _this->sizeofT);
	++_this->size_;
}

int vector_oinsert(struct myvector *_this, void *_obj, vf_comparator _comparator)
{
	int insertPos = 0;
	if (_this->size_ > 0)
	{
		insertPos = mybinarySearch_(_obj, _this->data_, _this->size_, _this->sizeofT, _comparator);
		if (insertPos < 0)
		{
			insertPos = -(insertPos+1);
		}		
	}
	vector_insert(_this, insertPos, _obj);
	return insertPos;
}

int vector_oindexOf(struct myvector *_this, void *_obj, vf_comparator _comparator)
{
	int insertPos = mybinarySearch_(_obj, _this->data_, _this->size_, _this->sizeofT, _comparator);
	return insertPos;
}

bool vector_oerase(struct myvector *_this, void *_obj, vf_comparator _comparator)
{
	int insertPos = mybinarySearch_(_obj, _this->data_, _this->size_, _this->sizeofT, _comparator);
	if (insertPos >= 0)
	{
		vector_erase(_this, insertPos);
		return true;
	}
	return false;
}

void vector_sort(struct myvector *_this, vf_comparator _comparator)
{
	qsort(_this->data_, _this->size_, _this->sizeofT, _comparator);
}

