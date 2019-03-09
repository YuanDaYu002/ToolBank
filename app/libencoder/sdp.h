
#ifndef SDP_H
#define SDP_H

#include "encoder.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*
    function:  sdp_request_queue
    description:  从指定的编码通道中申请一路码流队列
    args:
        int enc_chn[in]，编码通道号
        int auto_rc[in]，是否影响自动码率控制，1---影响，0---不影响
    return:
        >=0  success，返回申请到的队列的ID
        <0  fail
 */
int sdp_request_queue(int enc_chn, int auto_rc);


/*
    function:  sdp_free_queue
    description:  释放一路码流队列
    args:
        int queue_id[in]，队列ID
    return:
        0  success
        <0  fail
 */
int sdp_free_queue(int queue_id);

/*
    function:  sdp_enqueue
    description:  码流包加入分发队列接口
    args:
        int enc_chn[in]，该包所属的编码通道
        ENC_STREAM_PACK *pack[in],  要加入的码流包
    return:
        0  success
        <0  fail
 */
int sdp_enqueue(int enc_chn, ENC_STREAM_PACK *pack);

/*
    function:  spm_dequeue
    description:  从分发队列取出码流包接口
    args:
        int queue_id[in], 码流分发队列ID
    return:
        non-NULL  success  指向所取出的码流包的指针
        NULL    fail
 */
ENC_STREAM_PACK *sdp_dequeue(int queue_id);

/*
    function:  sdp_init
    description:  SPD模块初始化接口
    args:
        int pack_count[in], SPM模块最大产出的pack数目
    return:
        0  success
        <0  fail
 */
int sdp_init(int pack_count);


void sdp_exit(void);

int sdp_max_prev_count(void);


#ifdef __cplusplus
}
#endif


#endif

