
#ifndef MY_VECTOR_H
#define MY_VECTOR_H

#include <string.h> 
#include <stdlib.h>
#include <assert.h>
#include "base.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vf_destroy)(void*);
typedef int (*vf_comparator)(const void *, const void *);

#define VectorInitialSize 4
struct myvector {
	unsigned sizeofT;
	char* data_;            
	unsigned capacity_;  
	unsigned size_;      
	vf_destroy fdestory;
}; 

struct myvector *vector_init2(struct myvector *_this, size_t _sizeofT, vf_destroy _fdestory, size_t n);
struct myvector *vector_init(struct myvector *_this, size_t _sizeofT);

struct myvector *vector_init3(struct myvector *_this, size_t _sizeofT, vf_destroy _fdestory, size_t n);
void vector_finalize(struct myvector *_this);
void vector_clear(struct myvector *_this);
bool     vector_empty(struct myvector *_this);
unsigned vector_size(struct myvector *_this);
unsigned vector_capacity(struct myvector *_this);

void vector_reserve(struct myvector *_this, unsigned n);
void vector_reverse(struct myvector *_this, void *_swapBuf);

struct myvector * vector_splice(struct myvector *_this, size_t _start, size_t _end);
void vector_push_back(struct myvector *_this, const void * t );
void vector_append_vector(struct myvector *_this, struct myvector *_that );
void vector_pop_back(struct myvector *_this);
void vector_pollFirst(struct myvector *_this);
void vector_pollLast(struct myvector *_this);
void vector_erase(struct myvector *_this, size_t _pos);
void vector_insert(struct myvector *_this, size_t _pos, const void * t);
void *vector_at(struct myvector *_this, unsigned i);
void *vector_first(struct myvector *_this);
void *vector_nativep(struct myvector *_this);
void *vector_last(struct myvector *_this);
void *vector_back(struct myvector *_this);

int vector_oinsert(struct myvector *_this, void *_obj, vf_comparator _comparator);

bool vector_oerase(struct myvector *_this, void *_obj, vf_comparator _comparator);

int vector_oindexOf(struct myvector *_this, void *_obj, vf_comparator _comparator);

void vector_sort(struct myvector *_this, vf_comparator _comparator);

#ifdef __cplusplus
} 
#endif

#endif

