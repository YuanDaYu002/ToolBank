#ifndef CIRCULARQUEUE_H
#define CIRCULARQUEUE_H

#pragma once

#include "base.h"
#include "thread.h"

#define CircularQueue_initialSize 4
struct CircularQueue 
{
	int m_iCapacity;
	int m_iSize;
	int sizeofT;
	char * m_arrContainer;
	int m_iRear, m_iFront;
	mtx_t m;
};

struct CircularQueue *cq_init(struct CircularQueue *_this, int _sizeofT);
struct CircularQueue *cq_init2(struct CircularQueue *_this, int _sizeofT, int _initSize);
void cq_finalize(struct CircularQueue *_this);

void * cq_push_(struct CircularQueue *_this, void *_ele);
void cq_pop_(struct CircularQueue *_this, void *elem);
void * cq_push(struct CircularQueue *_this, void * elem);	
void cq_pop(struct CircularQueue *_this);					
bool cq_try_push(struct CircularQueue *_this, void *_ele);
bool cq_try_pop(struct CircularQueue *_this, void *_pelem);

bool cq_empty(struct CircularQueue *_this);
void cq_clear(struct CircularQueue *_this);
int cq_size(struct CircularQueue *_this);
int cq_capacity(struct CircularQueue *_this);
bool cq_try_enqueue(struct CircularQueue *_this, void * element);
void cq_enqueue(struct CircularQueue *_this, void * element);

bool cq_try_dequeue(struct CircularQueue *_this, void *_pelem);
int cq_size_approx(struct CircularQueue *_this);

bool cq_peek2(struct CircularQueue *_this, void ** _elem);

bool cq_peekBotton1(struct CircularQueue *_this, void ** _elem);

void *cq_peek(struct CircularQueue *_this);

void *cq_peekBotton(struct CircularQueue *_this);

#endif

