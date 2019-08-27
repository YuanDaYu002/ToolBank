#ifndef BUFFER_H
#define BUFFER_H

#include <assert.h>
#include "thread.h"
#include "base.h"
#include "memory.h"

#include "circularQueue.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TBlockingQueue struct CircularQueue

struct BufferData
{
	char *mData;
	float *floatData;
	int mFilledSize, mMaxBufferSize;
	int floatBufferSize, floatFilledSize, floatOverlapSize;
	int debugIdx;
};

struct BufferData *bd_init(struct BufferData *_this, int maxBufferSize);
void bd_finalize(struct BufferData * _this);
struct BufferData *bd_getNullBuffer();
char *bd_getData(struct BufferData * _this);
float *bd_getFloatData(struct BufferData * _this);
bool bd_isNULL(struct BufferData * _this);
void bd_reset(struct BufferData * _this);
int bd_getMaxBufferSize(struct BufferData * _this);
void bd_setFilledSize(struct BufferData * _this, int size);
int bd_getFilledSize(struct BufferData * _this);
int bd_getFloatFilledSize(struct BufferData * _this);
void bd_setFloatFilledSize(struct BufferData * _this, int _filledSize);
int bd_getFloatBufferSize(struct BufferData * _this);
int bd_getFloatOverlapSize(struct BufferData * _this);
void bd_setFloatOverlapSize(struct BufferData * _this, int _floatOverlapSize);

struct Buffer
{
	int mBufferCount;
	int mBufferSize;

	TBlockingQueue mProducerQueue;
	TBlockingQueue mConsumeQueue;

	mtx_t m;
	cnd_t cond;
};

struct Buffer *b_init(struct Buffer *_this, int bufferCount, int bufferSize);
struct Buffer *b_init2(struct Buffer *_this, int bufferCount, int bufferSize, bool _createData);
void b_finalize(struct Buffer *_this);
void b_reset(struct Buffer *_this);
int b_getEmptyCount(struct Buffer *_this);
int b_getFullCount(struct Buffer *_this);
struct BufferData *b_getEmpty(struct Buffer *_this);
struct BufferData *b_tryGetEmpty(struct Buffer *_this);
bool b_putEmpty(struct Buffer *_this, struct BufferData *data);
struct BufferData *b_getFull(struct Buffer *_this);
bool b_putFull(struct Buffer *_this, struct BufferData *data);
struct BufferData *b_getImpl(struct Buffer *_this, TBlockingQueue *queue);
bool b_putImpl(struct Buffer *_this, struct BufferData *data, TBlockingQueue *queue);

struct BufferSource
{
	struct BufferData *(*getBuffer)(struct BufferSource *__this);
	struct BufferData *(*tryGetBuffer)(struct BufferSource *__this);
	void (*freeBuffer)(struct BufferSource *__this, struct BufferData *data);
};

struct DataSource
{
	void (*freeData)(struct DataSource *__this, struct BufferData *buffer);
	struct BufferData *(*getData)(struct DataSource *__this);
};

#ifdef __cplusplus
}
#endif

#endif

