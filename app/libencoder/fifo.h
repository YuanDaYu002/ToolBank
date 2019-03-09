#ifndef HLE_FIFO_H
#define HLE_FIFO_H


#include "typeport.h"


#ifdef __cplusplus
extern "C"
{
#endif


typedef void *FIFO_HANDLE;

/*
    function:  fifo_malloc
    description:  申请环形缓冲区接口
    args:
        unsigned int size[in]，缓冲区大小，必须是2的N次方
    return:
        non-NULL, 成功，返回缓冲区句柄
        NULL, 失败
 */
FIFO_HANDLE fifo_malloc(unsigned int size);

/*
    function:  fifo_free
    description:  释放环形缓冲区接口
    args:
        FIFO_HANDLE fifo[in]，缓冲区句柄
    return:
        0, 成功
        -1, 失败
 */
int fifo_free(FIFO_HANDLE fifo, int lock);

/*
    function:  fifo_reset
    description:  重置缓冲区读写指针接口
    args:
        FIFO_HANDLE fifo[in]，缓冲区句柄
    return:
        0, 成功
        -1, 失败
 */
int fifo_reset(FIFO_HANDLE fifo, int lock);

/*
    function:  fifo_len
    description:  获取缓冲区中已存放数据长度接口
    args:
        FIFO_HANDLE fifo[in]，缓冲区句柄
    return:
        >0, 成功，返回缓冲区已存放的数据长度
        -1, 失败
 */
int fifo_len(FIFO_HANDLE fifo, int lock);


/*
    function:  fifo_avail
    description:  获取缓冲区剩余空间接口
    args:
        FIFO_HANDLE fifo[in]，缓冲区句柄
    return:
        >0, 成功，返回缓冲区剩余空间长度
        -1, 失败
 */
int fifo_avail(FIFO_HANDLE fifo, int lock);

/*
    function:  fifo_in
    description:  数据放入缓冲区接口
    args:
        FIFO_HANDLE fifo[in]，缓冲区句柄
        HLE_U8 *from[in]，要放入的数据首地址
        unsigned int len[in]，要放入的数据长度
    return:
        >0, 成功，返回实际放入缓冲区的数据长度，可能小于@len
        -1, 失败
 */
int fifo_in(FIFO_HANDLE fifo, HLE_U8* from, unsigned int len, int lock);

/*
    function:  fifo_out
    description:  从缓冲区中读取数据接口
    args:
        FIFO_HANDLE fifo[in]，缓冲区句柄
        HLE_U8 *to[in]，目的数据首地址
        unsigned int len[in]，要读取的数据长度
    return:
        >0, 成功，返回实际读取的数据长度，可能小于@len
        -1, 失败
 */
int fifo_out(FIFO_HANDLE fifo, HLE_U8* to, unsigned int len, int lock);


#ifdef __cplusplus
}
#endif

#endif

