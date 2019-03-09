
#ifndef SPM_H
#define SPM_H

#include "encoder.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*
    function:  spm_alloc
    description:  空闲码流包请求接口
    args:
    return:
        non-NULL  success  指向申请到的空闲包的指针
        NULL    fail
 */
ENC_STREAM_PACK *spm_alloc(unsigned int length);


/*
    function:  spm_inc_pack_ref
    description:  增加包引用计数接口(+1)
    args:
        ENC_STREAM_PACK *pack  码流包
    return:
        0  success
        -1  fail
 */
int spm_inc_pack_ref(ENC_STREAM_PACK *pack);


//debug
int spm_pack_print_ref(ENC_STREAM_PACK *pack);

/*
    function:  spm_dec_pack_ref
    description:  减少包引用计数接口(-1)，减完后如果引用计数为0则释放该包
    args:
        ENC_STREAM_PACK *pack  码流包
    return:
        0  success
        -1  fail
 */
int spm_dec_pack_ref(ENC_STREAM_PACK *pack);


/*
    function:  spm_init
    description:  码流包管理模块初始化接口
    args:
        int pack_count  预分配的码流包个数

    return:
        0  success
        -1  fail
 */
int spm_init(int pack_count);


/*
    function:  spm_exit
    description:  码流包管理模块去初始化接口
    args:

    return:
 */
void spm_exit(void);


#ifdef __cplusplus
}
#endif

#endif

