#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "typeport.h"
#include "fifo.h"

typedef struct {
	unsigned char *buf; /* store data */
	unsigned int size; /* cycle buffer size */
	unsigned int in; /* next write position in buffer */
	unsigned int out; /* next read position in buffer */
    pthread_mutex_t lck;
} CYCLE_BUFFER;

#define MIN(x, y) ((x) < (y) ? (x) : (y))

static int is_power_of_2(unsigned int n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

/*
	function:  fifo_malloc
	description:  申请环形缓冲区接口
	args:
		unsigned int size[in]，缓冲区大小，必须是2的N次方
	return:
		non-NULL, 成功，返回缓冲区句柄
		NULL, 失败
 */
FIFO_HANDLE fifo_malloc(unsigned int size)
{
	if (!is_power_of_2(size)) {
		ERROR_LOG("FIFO size must be power of 2\n");
		return NULL;
	}

	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) malloc(sizeof (CYCLE_BUFFER) + size);
	if (cir_buf == NULL)
		return NULL;

	cir_buf->size = size;
	cir_buf->in = 0;
	cir_buf->out = 0;
	cir_buf->buf = (unsigned char *)cir_buf + sizeof(CYCLE_BUFFER);
    pthread_mutex_init(&cir_buf->lck, NULL);

	return (FIFO_HANDLE) cir_buf;
}

/*
	function:  fifo_free
	description:  释放环形缓冲区接口
	args:
		FIFO_HANDLE fifo[in]，缓冲区句柄
	return:
		0, 成功
		-1, 失败
 */
int fifo_free(FIFO_HANDLE fifo, int lock)
{
	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) fifo;
	if (cir_buf == NULL)
		return -1;

    if (lock) {
        while (EBUSY == pthread_mutex_destroy(&cir_buf->lck)) {
            usleep(10 * 1000);
        }
    }
	free(cir_buf);
	return 0;
}

/*
	function:  fifo_reset
	description:  重置缓冲区读写指针接口
	args:
		FIFO_HANDLE fifo[in]，缓冲区句柄
	return:
		0, 成功
		-1, 失败
 */
int fifo_reset(FIFO_HANDLE fifo, int lock)
{
	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) fifo;
	if (cir_buf == NULL)
		return -1;

    if (lock) {
        pthread_mutex_lock(&cir_buf->lck);
    }
	cir_buf->in = 0;
	cir_buf->out = 0;
    if (lock) {
        pthread_mutex_unlock(&cir_buf->lck);
    }

	return 0;
}

/*
	function:  fifo_len
	description:  获取缓冲区中已存放数据长度接口
	args:
		FIFO_HANDLE fifo[in]，缓冲区句柄
	return:
		>0, 成功，返回缓冲区已存放的数据长度
		-1, 失败
 */
int fifo_len(FIFO_HANDLE fifo, int lock)
{
	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) fifo;
	if (cir_buf == NULL)
		return -1;

    if (lock) {
        pthread_mutex_lock(&cir_buf->lck);
        int r = cir_buf->in - cir_buf->out;
        pthread_mutex_unlock(&cir_buf->lck);
        return r;
    } else {
        return (int) (cir_buf->in - cir_buf->out);
    }
}

/*
	function:  fifo_avail
	description:  获取缓冲区剩余空间接口
	args:
		FIFO_HANDLE fifo[in]，缓冲区句柄
	return:
		>0, 成功，返回缓冲区剩余空间长度
		-1, 失败
 */
int fifo_avail(FIFO_HANDLE fifo, int lock)
{
	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) fifo;
	if (cir_buf == NULL)
		return -1;

    if (lock) {
        pthread_mutex_lock(&cir_buf->lck);
        int r = cir_buf->size - fifo_len(fifo, 0);
        pthread_mutex_unlock(&cir_buf->lck);
        return r;
    } else {
    	return (int) (cir_buf->size - fifo_len(fifo, 0));
    }
}

static unsigned int cirbuf_offset(CYCLE_BUFFER *cir_buf, unsigned int off)
{
	return off & (cir_buf->size - 1);
}

/*
	function:  fifo_in
	description:  数据放入缓冲区接口
	args:
		FIFO_HANDLE fifo[in]，缓冲区句柄
		void *from[in]，要放入的数据首地址
		unsigned int len[in]，要放入的数据长度
	return:
		>0, 成功，返回实际放入缓冲区的数据长度，可能小于@len
		-1, 失败
 */
int fifo_in(FIFO_HANDLE fifo, HLE_U8* from, unsigned int len, int lock)
{
	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) fifo;
	if (cir_buf == NULL)
		return -1;

    if (lock) {
        pthread_mutex_lock(&cir_buf->lck);
        
        unsigned int off = cirbuf_offset(cir_buf, cir_buf->in);

        /* first put the data starting from cir_buf->in to buffer end */
        unsigned int tail_len = MIN(len, cir_buf->size - off);
        memcpy(cir_buf->buf + off, from, tail_len);

        /* then put the rest (if any) at the beginning of the buffer */
        memcpy(cir_buf->buf, from + tail_len, len - tail_len);

        cir_buf->in += len;
        
        if (fifo_len(fifo, 0) > cir_buf->size) {
            ERROR_LOG("fifo overflow %d bytes\n", fifo_len(fifo, 0) - cir_buf->size);
            cir_buf->out = cir_buf->in - cir_buf->size;
        }
        
        pthread_mutex_unlock(&cir_buf->lck);
    } else {
            len = MIN(fifo_avail(fifo, 0), len);
            unsigned int off = cirbuf_offset(cir_buf, cir_buf->in);

            /* first put the data starting from cir_buf->in to buffer end */
            unsigned int tail_len = MIN(len, cir_buf->size - off);
            memcpy(cir_buf->buf + off, from, tail_len);

            /* then put the rest (if any) at the beginning of the buffer */
            memcpy(cir_buf->buf, from + tail_len, len - tail_len);

            cir_buf->in += len;
    }

	return len;
}

/*
	function:  fifo_out
	description:  从缓冲区中读取数据接口
	args:
		FIFO_HANDLE fifo[in]，缓冲区句柄
		void *to[in]，目的数据首地址
		unsigned int len[in]，要读取的数据长度
	return:
		>0, 成功，返回实际读取的数据长度，可能小于@len
		-1, 失败
 */
int fifo_out(FIFO_HANDLE fifo, HLE_U8* to, unsigned int len, int lock)
{
	CYCLE_BUFFER *cir_buf = (CYCLE_BUFFER *) fifo;
	if (cir_buf == NULL)
		return -1;

    if (lock) {
        pthread_mutex_lock(&cir_buf->lck);
    }

	len = MIN(fifo_len(fifo, 0), len);
	unsigned int off = cirbuf_offset(cir_buf, cir_buf->out);

	/* first get the data from cir_buf->out until the end of the buffer */
	unsigned int tail_len = MIN(len, cir_buf->size - off);
	memcpy(to, cir_buf->buf + off, tail_len);

	/* then get the rest (if any) from the beginning of the buffer */
	memcpy(to + tail_len, cir_buf->buf, len - tail_len);

	cir_buf->out += len;
        
    if (lock) {
        pthread_mutex_unlock(&cir_buf->lck);
    }

	return len;
}

