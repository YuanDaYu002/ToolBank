

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "hi_comm_video.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_vi.h"
#include "hi_comm_region.h"
#include "mpi_venc.h"
#include "mpi_aenc.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_region.h"

#include "hal.h"
#include "hal_def.h"
#include "video.h"
#include "encoder.h"
#include "spm.h"
#include "sdp.h"
#include "surface_scaler.h"
#include "ziku.h"
#include "watchdog.h"
#include "audio.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#define H264_PF_BL          0
#define H264_PF_MP          1
#define H264_PF_HP          2
#define H264_PF_SUPPORT     ((1 << H264_PF_BL) | (1 << H264_PF_MP) | (1 << H264_PF_HP))

#define H265_PF_MP   0
#define H265_PF_SUPPORT  (1 << H265_PF_MP)

#define BIT_CTRL_CBR        0
#define BIT_CTRL_VBR        1
#define BIT_CTRL_AVBR  2

#define ENC_STD_SUPPORT     ((1 << VENC_STD_H264) | (1 << VENC_STD_H265))
#define BIT_CTRL_SUPPORT    ((1 << BIT_CTRL_CBR) | (1 << BIT_CTRL_VBR) | (1 << BIT_CTRL_AVBR))

#define MAIN_STREAM_IMAGE_SIZE_SUPPORT   (1 << IMAGE_SIZE_1920x1080)

#define MINOR_STREAM_IMAGE_SIZE_SUPPORT  (1 << IMAGE_SIZE_960x544)

#define THIRD_STREAM_IMAGE_SIZE_SUPPORT  (1 << IMAGE_SIZE_480x272)

#define JPEG_IMAGE_SIZE_SUPPORT    (1 << IMAGE_SIZE_1920x1080)

#define MAX_BIT_RATE        (8*1024)



#define GET_ENC_CHN(viPort, index)         ((index) + (viPort)*STREAMS_PER_CHN)
#define ENC_GET_VI_CHN(encChn)         ((encChn)/STREAMS_PER_CHN)
#define ENC_GET_STREAN_INDEX(encChn)   ((encChn)%STREAMS_PER_CHN)
#define GET_OSD_HANDLE(encChn, osd_index)  ((osd_index) + (encChn)*VI_OSD_NUM + VI_COVER_NUM*VI_PORT_NUM)

#define GET_JPEG_ENC_CHN(chn)    (STREAMS_PER_CHN + (chn)*ENC_NUM_PER_CHN)

typedef enum
{
    ENCODE_VENC_MAIN = 0,
    ENCODE_VENC_MINOR,
    ENCODE_VENC_THIRD,
    ENCODE_VENC_JPEG,
} E_ENCODE_VENC_TYPE;

typedef enum
{
    ENC_STATE_IDLE,
    ENC_STATE_CONFIGED,
    ENC_STATE_RUNING,
} E_ENC_STATE;

typedef struct _tag_ENC_CHN_CONTEX
{
    int state;
    int enc_level;
    pthread_mutex_t lock;
    ENC_STREAM_ATTR enc_attr;
    ENC_STREAM_PACK *curr_pack;
} ENC_CHN_CONTEX;

typedef struct
{
    int running; //0:stop, 1:ending, 2:running
    pthread_t venc_pid;
    pthread_t Aenc_pid;
    pthread_t time_osd_pid;
    pthread_t rate_osd_pid;
    ENC_CHN_CONTEX encChn[ENC_STREAM_NUM];
} ENC_CONTEXT;

static int h264_profile = H264_PF_MP;

static ENC_CONTEXT enc_ctx;

static ENC_JPEG_ATTR jpeg_chn_attr;
static int jpeg_chn_state;
static pthread_mutex_t jpeg_enc_lock;

static ENC_STREAM_ATTR defEncStreamAttr[STREAMS_PER_CHN] = {
    {
        VENC_STD_H264,
        IMAGE_SIZE_1920x1080,
        15,
        BIT_CTRL_AVBR,
        1,
        1,
        64 * 20
    },
#if 0
    {
        VENC_STD_H264,
        IMAGE_SIZE_960x544,
        15,
        BIT_CTRL_AVBR,
        1,
        1,
        64 * 6
    },
#endif
    {
        VENC_STD_H264,
        IMAGE_SIZE_480x272,
        15,
        BIT_CTRL_AVBR,
        1,
        1,
        64 * 4
    },
};

int encoder_if_have_audio(int stream_id,char have_audio)
{
    if(stream_id > ENC_STREAM_NUM || stream_id < 0)
        return HLE_RET_EINVAL;

    enc_ctx.encChn[stream_id].enc_attr.hasaudio = have_audio;
    return HLE_RET_OK;
}

int get_image_size(E_IMAGE_SIZE image_size, int *width, int *height)
{
    switch (image_size) {
    case IMAGE_SIZE_1920x1080:
        *width = 1920;
        *height = 1080;
        break;
    case IMAGE_SIZE_960x544:
        *width = 960;
        *height = 544;
        break;
    case IMAGE_SIZE_480x272:
        *width = 480;
        *height = 272;
        break;
    default:
        return HLE_RET_ENOTSUPPORTED;
    }

    return HLE_RET_OK;
}

static void set_h264_venc_attr(int width, int height, VENC_CHN_ATTR_S *vencAttr)
{
    VENC_ATTR_H264_S *h264_attr = &(vencAttr->stVeAttr.stAttrH264e);

    vencAttr->stVeAttr.enType = PT_H264;
    h264_attr->u32MaxPicWidth = width;
    h264_attr->u32MaxPicHeight = height;
    h264_attr->u32PicWidth = width;
    h264_attr->u32PicHeight = height;
    h264_attr->u32BufSize = (CEILING_2_POWER(width, 64) * CEILING_2_POWER(height, 64) * 3) >> 1; //64对齐
    h264_attr->u32Profile = h264_profile;
    h264_attr->bByFrame = HI_TRUE;
}

static void set_h265_venc_attr(int width, int height, VENC_CHN_ATTR_S * vencAttr)
{
    VENC_ATTR_H265_S* h265Attr = &(vencAttr->stVeAttr.stAttrH265e);

    vencAttr->stVeAttr.enType = PT_H265;
    h265Attr->u32MaxPicWidth = width;
    h265Attr->u32MaxPicHeight = height;
    h265Attr->u32PicWidth = width;
    h265Attr->u32PicHeight = height;
    h265Attr->u32BufSize = (CEILING_2_POWER(width, 64) * CEILING_2_POWER(height, 64) * 3) >> 1; //64对齐
    h265Attr->u32Profile = H265_PF_MP;
    h265Attr->bByFrame = HI_TRUE;

}

int hal_get_max_frame_rate(void);

static void set_h264_cbr_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    VENC_ATTR_H264_CBR_S *h264_cbr = &(vencAttr->stRcAttr.stAttrH264Cbr);

    vencAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
    vencAttr->stRcAttr.pRcAttr = NULL;
    h264_cbr->u32Gop = stream_attr->framerate;
    h264_cbr->u32StatTime = 1;
    h264_cbr->u32SrcFrmRate = hal_get_max_frame_rate();
    h264_cbr->fr32DstFrmRate = stream_attr->framerate;
    h264_cbr->u32BitRate = stream_attr->bitrate * (MAX_BLOCK_LEVEL - enc_level) / MAX_BLOCK_LEVEL;
    h264_cbr->u32FluctuateLevel = 1;
}

#define VBR_QP_FACTOR 1

static void set_h264_vbr_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    VENC_ATTR_H264_VBR_S *h264_vbr = &(vencAttr->stRcAttr.stAttrH264Vbr);

    vencAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
    vencAttr->stRcAttr.pRcAttr = NULL;
    h264_vbr->u32Gop = stream_attr->framerate;
    h264_vbr->u32StatTime = 1;
    h264_vbr->u32SrcFrmRate = hal_get_max_frame_rate();
    h264_vbr->fr32DstFrmRate = stream_attr->framerate;
    h264_vbr->u32MinQp = 24;
    h264_vbr->u32MaxQp = (32 - 2 * VBR_QP_FACTOR) + stream_attr->quality * VBR_QP_FACTOR;
    h264_vbr->u32MinIQp = 26;
    h264_vbr->u32MaxBitRate = stream_attr->bitrate * (MAX_BLOCK_LEVEL - enc_level) / MAX_BLOCK_LEVEL;
}

static void set_h264_avbr_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    VENC_ATTR_H264_AVBR_S *h264AVbr = &(vencAttr->stRcAttr.stAttrH264AVbr);

    vencAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264AVBR;
    vencAttr->stRcAttr.pRcAttr = NULL;
    h264AVbr->u32Gop = stream_attr->framerate;
    h264AVbr->u32StatTime = 1;
    h264AVbr->u32SrcFrmRate = stream_attr->framerate;//hal_get_max_frame_rate();
    h264AVbr->fr32DstFrmRate = stream_attr->framerate;
    h264AVbr->u32MaxBitRate = stream_attr->bitrate * (MAX_BLOCK_LEVEL - enc_level) / MAX_BLOCK_LEVEL;
    printf("set_h264_avbr_attr: u32SrcFrmRate(%d) fr32DstFrmRate(%d) u32MaxBitRate(%d)\n",h264AVbr->u32SrcFrmRate,\
                                                                                          h264AVbr->fr32DstFrmRate,\
                                                                                          h264AVbr->u32MaxBitRate);
}

static void set_h265_cbr_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    VENC_ATTR_H265_CBR_S *h265_cbr = &(vencAttr->stRcAttr.stAttrH265Cbr);

    vencAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
    vencAttr->stRcAttr.pRcAttr = NULL;
    h265_cbr->u32Gop = stream_attr->framerate;
    h265_cbr->u32StatTime = 1;
    h265_cbr->u32SrcFrmRate = hal_get_max_frame_rate();
    h265_cbr->fr32DstFrmRate = stream_attr->framerate;
    h265_cbr->u32BitRate = stream_attr->bitrate * (MAX_BLOCK_LEVEL - enc_level) / MAX_BLOCK_LEVEL;
    h265_cbr->u32FluctuateLevel = 1;
}

#define VBR_QP_FACTOR 1

static void set_h265_vbr_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    VENC_ATTR_H265_VBR_S *h265_vbr = &(vencAttr->stRcAttr.stAttrH265Vbr);

    vencAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
    vencAttr->stRcAttr.pRcAttr = NULL;
    h265_vbr->u32Gop = stream_attr->framerate;
    h265_vbr->u32StatTime = 1;
    h265_vbr->u32SrcFrmRate = hal_get_max_frame_rate();
    h265_vbr->fr32DstFrmRate = stream_attr->framerate;
    h265_vbr->u32MinQp = 24;
    h265_vbr->u32MaxQp = (32 - 2 * VBR_QP_FACTOR) + stream_attr->quality * VBR_QP_FACTOR;
    h265_vbr->u32MinIQp = 26;
    h265_vbr->u32MaxBitRate = stream_attr->bitrate * (MAX_BLOCK_LEVEL - enc_level) / MAX_BLOCK_LEVEL;
}

static void set_h265_avbr_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    VENC_ATTR_H265_AVBR_S *h265AVbr = &(vencAttr->stRcAttr.stAttrH265AVbr);

    vencAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265AVBR;
    vencAttr->stRcAttr.pRcAttr = NULL;
    h265AVbr->u32Gop = stream_attr->framerate;
    h265AVbr->u32StatTime = 1;
    h265AVbr->u32SrcFrmRate = hal_get_max_frame_rate();
    h265AVbr->fr32DstFrmRate = stream_attr->framerate;
    //h265AVbr->u32MaxBitRate = stream_attr->bitrate * (MAX_BLOCK_LEVEL - enc_level) / MAX_BLOCK_LEVEL;
    h265AVbr->u32MaxBitRate = stream_attr->bitrate;
}

static void set_rcparam(int chn)
{
    VENC_RC_PARAM_S vrp;
    HI_S32 ret = HI_MPI_VENC_GetRcParam(chn, &vrp);
    if (ret) {
        ERROR_LOG("HI_MPI_VENC_GetRcParam fail: %#x!\n", ret);
        return;
    }

    vrp.u32RowQpDelta = 0;
    vrp.s32FirstFrameStartQp = 30; // 对于 AVBR 为[MinIQP, MaxIQp]和-1

    VENC_PARAM_H264_AVBR_S* pha = &(vrp.stParamH264AVbr);
    pha->s32ChangePos = 90; //[50, 100] default:90
    //pha->u32MinIprop = 1;
    //pha->u32MaxIprop = 100;
    //pha->s32MaxReEncodeTimes = 2;
    //pha->bQpMapEn = HI_FALSE;
    pha->s32MinStillPercent = 60; //[5, 100] default:25
    pha->u32MaxStillQP = 35;//40;//[MinIQp, MaxIQp]  default:35
 
    pha->u32MaxQp = 44; //[MinQp, 51] default:51
    pha->u32MinQp = 24; //[0, 51] default:16

    pha->u32MaxIQp = 40;//[MinIQp, 51] default:51
    pha->u32MinIQp = 20;//[0, 51] default:16

    ret = HI_MPI_VENC_SetRcParam(chn, &vrp);
    if (ret) {
        ERROR_LOG("HI_MPI_VENC_SetRcParam fail: %#x!\n", ret);
    }
}

static void set_venc_rc_attr(VENC_CHN_ATTR_S *vencAttr, ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    if (stream_attr->enc_standard == VENC_STD_H264) {
        if (stream_attr->bitratectrl == BIT_CTRL_CBR) {
            set_h264_cbr_attr(vencAttr, stream_attr, enc_level);
        }
        else if (stream_attr->bitratectrl == BIT_CTRL_VBR) {
            set_h264_vbr_attr(vencAttr, stream_attr, enc_level);
        }
        else {
            set_h264_avbr_attr(vencAttr, stream_attr, enc_level);
        }
    }
    else {
        if (stream_attr->bitratectrl == BIT_CTRL_CBR) {
            set_h265_cbr_attr(vencAttr, stream_attr, enc_level);
        }
        else if (stream_attr->bitratectrl == BIT_CTRL_VBR) {
            set_h265_vbr_attr(vencAttr, stream_attr, enc_level);
        }
        else {
            set_h265_avbr_attr(vencAttr, stream_attr, enc_level);
        }
    }
}

static void set_vencAttr(ENC_STREAM_ATTR *stream_attr, int enc_level,
                         int width, int height, VENC_CHN_ATTR_S *vencAttr)
{
    if (stream_attr->enc_standard == VENC_STD_H264) {
        set_h264_venc_attr(width, height, vencAttr);
    }
    else {
        set_h265_venc_attr(width, height, vencAttr);
    }
    set_venc_rc_attr(vencAttr, stream_attr, enc_level);
#if 0
    vencAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    vencAttr->stGopAttr.stNormalP.s32IPQpDelta = 3;
#else
    vencAttr->stGopAttr.enGopMode = VENC_GOPMODE_SMARTP; //编码智能 P 帧 GOP 类型。
    vencAttr->stGopAttr.stSmartP.u32BgInterval = 30; //长期参考帧的间隔,取值范围：大于等于 u32Gop，且必须是 u32Gop 的整数倍
    vencAttr->stGopAttr.stSmartP.s32BgQpDelta = 7;
    vencAttr->stGopAttr.stSmartP.s32ViQpDelta = 2;
    vencAttr->stRcAttr.stAttrH265AVbr.u32StatTime = 2;
#endif
}

static void set_jpeg_chn_attr(VENC_CHN_ATTR_S *vencAttr, int width, int height)
{
    VENC_ATTR_JPEG_S *jpeg_attr = &(vencAttr->stVeAttr.stAttrJpege);
    vencAttr->stVeAttr.enType = PT_JPEG;
    jpeg_attr->u32MaxPicWidth = width;
    jpeg_attr->u32MaxPicHeight = height;
    jpeg_attr->u32PicWidth = width;
    jpeg_attr->u32PicHeight = height;
    jpeg_attr->u32BufSize = width * height * 2;
    jpeg_attr->bByFrame = HI_TRUE;
    jpeg_attr->bSupportDCF = HI_FALSE;
}

int init_venc_stream(VENC_STREAM_S *stream, unsigned int pack_count)
{
    static int reqCnt = 0;
    if (stream->pstPack == NULL) {
        stream->pstPack = (VENC_PACK_S *) malloc(sizeof (VENC_PACK_S) * pack_count);
        if (stream->pstPack == NULL) {
            reqCnt = 0;
            ERROR_LOG("malloc venc_stream.pstPack failed\n");
            return HLE_RET_ERROR;
        }
        reqCnt = pack_count;
    }
    else if (reqCnt < pack_count) {
        free(stream->pstPack);
        stream->pstPack = (VENC_PACK_S *) malloc(sizeof (VENC_PACK_S) * pack_count);
        if (stream->pstPack == NULL) {
            reqCnt = 0;
            ERROR_LOG("malloc venc_stream.pstPack failed\n");
            return HLE_RET_ERROR;
        }
        reqCnt = pack_count;
    }
    stream->u32PackCount = pack_count;
    return HLE_RET_OK;
}

static int get_stream_data_len(VENC_STREAM_S *stream)
{
    int i;
    int ret_len = 0;

    for (i = 0; i < stream->u32PackCount; i++) {
        ret_len += stream->pstPack[i].u32Len;
    }
    return ret_len;
}

#if 0
// 获取当前时间戳

int get_cur_pts(void)
{
    HI_U64 cur_pts;

    int ret = HI_MPI_SYS_GetCurPts(&cur_pts);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_SYS_GetCurPts failed, ret = %#x\n", ret);
        return HLE_RET_ERROR;
    }
    //DEBUG_LOG("cur_pts ---- = %lld\n", cur_pts);
    time_t time_tmp = cur_pts / 1000000;
    DEBUG_LOG("cur_pts: %s", ctime(&time_tmp));

    return HLE_RET_OK;
}

// 初始化MPP 的时间戳基准

int set_base_pts(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    HLE_U64 pts_base_tmp = (HLE_U64) tv.tv_usec + (HLE_U64) tv.tv_sec * 1000000;
    //DEBUG_LOG("set sys pts  %lld\n", pts_base_tmp);
    int ret = HI_MPI_SYS_InitPtsBase(pts_base_tmp);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_SYS_GetCurPts failed, ret = %#x\n", ret);
        return HLE_RET_ERROR;
    }

    DEBUG_LOG("have init pts base already\n");
    return HLE_RET_OK;
}

//同步MPP 的时间戳

int sync_sys_pts(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    HLE_U64 pts_base_tmp = (HLE_U64) tv.tv_usec + (HLE_U64) tv.tv_sec * 1000000;
    int ret = HI_MPI_SYS_SyncPts(pts_base_tmp);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_SYS_SyncPts failed, ret = %#x\n", ret);
        return HLE_RET_ERROR;
    }

    DEBUG_LOG("have sync sys pts already\n");
    return HLE_RET_OK;
}
#endif

//return: 1 iframe, 0 non-iframe

int stream_is_iframe(VENC_STREAM_S *stream)
{
    int i = 0;
    for (i = 0; i < stream->u32PackCount; i++) {
        if (stream->pstPack[i].DataType.enH264EType == H264E_NALU_SPS
            || stream->pstPack[i].DataType.enH264EType == H264E_NALU_ISLICE
            || stream->pstPack[i].DataType.enH265EType == H265E_NALU_SPS
            || stream->pstPack[i].DataType.enH265EType == H265E_NALU_ISLICE)
            return 1;
    }

    return 0;
}

static ENC_STREAM_PACK *video_stream_to_pack(int encChn, VENC_STREAM_S *stream)
{
    unsigned char head_buf[sizeof (FRAME_HDR) + sizeof (IFRAME_INFO)];
    FRAME_HDR *frame_hdr = (FRAME_HDR *) head_buf;
    int head_len = sizeof (FRAME_HDR);
    int frame_data_len = get_stream_data_len(stream);
    //DEBUG_LOG("frame_data_len %d\n", frame_data_len);

    frame_hdr->sync_code[0] = 0x00;
    frame_hdr->sync_code[1] = 0x00;
    frame_hdr->sync_code[2] = 0x01;
    if (stream_is_iframe(stream)) {
        IFRAME_INFO *iframe_info = (IFRAME_INFO *) (head_buf + sizeof (FRAME_HDR));
        ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;

        iframe_info->enc_std = chn_ctx->enc_attr.enc_standard;
        iframe_info->framerate = chn_ctx->enc_attr.framerate;
        int width, height;
        int ret = get_image_size(chn_ctx->enc_attr.img_size, &width, &height);
        if (HLE_RET_OK != ret) {
            ERROR_LOG("get_image_size fail: %#x!\n", ret);
        }
        iframe_info->pic_width = width;
        iframe_info->pic_height = height;
        iframe_info->pts_msec = (stream->pstPack->u64PTS / 1000);
        HLE_U8 wday;
        hal_get_time(&iframe_info->rtc_time, 1, &wday);
        iframe_info->length = frame_data_len;

        frame_hdr->type = 0xF8;
        head_len += sizeof (IFRAME_INFO);
    }
    else {
        PFRAME_INFO *pframe_info = (PFRAME_INFO *) (head_buf + sizeof (FRAME_HDR));
        pframe_info->pts_msec =  (stream->pstPack->u64PTS / 1000);
        pframe_info->length = frame_data_len;

        frame_hdr->type = 0xF9;
        head_len += sizeof (PFRAME_INFO);
    }

ENC_STREAM_PACK *pack = spm_alloc(head_len + frame_data_len);
    if (pack == NULL)
        return NULL;

    pack->channel = ENC_GET_VI_CHN(encChn);
    pack->stream_index = ENC_GET_STREAN_INDEX(encChn);

    //copy head
    memcpy(pack->data, head_buf, head_len);
    pack->length = head_len;

    //copy stream
    int i;
    for (i = 0; i < stream->u32PackCount; i++) {
        memcpy(pack->data + pack->length, stream->pstPack[i].pu8Addr,
               stream->pstPack[i].u32Len);
        pack->length += stream->pstPack[i].u32Len;
    }

    return pack;
}

static ENC_STREAM_PACK *audio_stream_to_pack(int encChn, AUDIO_STREAM_S *stream)
{
    unsigned char head_buf[sizeof (FRAME_HDR) + sizeof (AFRAME_INFO)];
    FRAME_HDR *frame_hdr = (FRAME_HDR *) head_buf;
    AFRAME_INFO *aframe_info = (AFRAME_INFO *) (head_buf + sizeof (FRAME_HDR));
    frame_hdr->sync_code[0] = 0x00;
    frame_hdr->sync_code[1] = 0x00;
    frame_hdr->sync_code[2] = 0x01;
    frame_hdr->type = 0xfa;
    aframe_info->enc_type = AENC_STD_ADPCM;
    aframe_info->sample_rate = AUDIO_SR_16000;
    aframe_info->bit_width = AUDIO_BW_16;
    aframe_info->sound_mode = AUDIO_SM_MONO;
    aframe_info->length = stream->u32Len;
    aframe_info->pts_msec = (HLE_U32) (stream->u64TimeStamp / 1000);

    ENC_STREAM_PACK *pack = spm_alloc(sizeof (head_buf) + stream->u32Len);
    if (pack == NULL)
        return NULL;
    pack->channel = ENC_GET_VI_CHN(encChn);
    pack->stream_index = ENC_GET_STREAN_INDEX(encChn);

    //copy head
    memcpy(pack->data, head_buf, sizeof (head_buf));
    pack->length = sizeof (head_buf);

    //copy stream
    memcpy(pack->data + pack->length, stream->pStream, stream->u32Len);
    pack->length += stream->u32Len;

    return pack;
}

int add_to_talkback_fifo(void *data, int size);


void * venc_get_stream_proc(void *arg)
{
    DEBUG_LOG("pid = %d\n", getpid());
    prctl(PR_SET_NAME, "hal_stream", 0, 0, 0);
    
    /***venc fd init*******************/
    int i, ret, max_fd = 0;
    int venc_fd[ENC_STREAM_NUM];
    VENC_STREAM_S venc_stream;
    VENC_CHN_STAT_S stat;
    fd_set  video_read_fds;
    fd_set  old_fds;
    struct timeval time_out;
     
    for (i = 0; i < ENC_STREAM_NUM; i++) 
    {
        venc_fd[i] = HI_MPI_VENC_GetFd(i);
        if (venc_fd[i] < 0) 
        {
            ERROR_LOG("HI_MPI_VENC_GetFd failed, ret = %#x\n", venc_fd[i]);
            return NULL;
        }
        
        if (max_fd < venc_fd[i]) 
        {
            max_fd = venc_fd[i];
        }
    }

    while (enc_ctx.running) 
    {
        usleep(1000);
        #ifndef DBG_ONLY
        // TempDisable  watchdog_feed();
        #endif

        /*****获取 Video 编码流 , 放入相应缓冲队列 *******************/
    #if 1 
        time_out.tv_sec = 2;
        time_out.tv_usec = 0;
    
        for (i = 0; i < ENC_STREAM_NUM; i++) 
        {
            ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + i;
            
            FD_ZERO(&video_read_fds);
            FD_SET(venc_fd[i], &video_read_fds);
            
            if (chn_ctx->state == ENC_STATE_RUNING) 
            {

                ret = select(max_fd+1, &video_read_fds, NULL, NULL, &time_out);
                if (ret == -1) 
                {
                    if (errno == EINTR) 
                    {
                        continue;
                    }
                    else 
                    {
                        ERROR_LOG("video select failed\n");
                        break;
                    }
                }
                else if (ret == 0) 
                {
                    ERROR_LOG("video select time out, exit thread\n");
                    continue;
                }
                else
                {
                    if(FD_ISSET(venc_fd[i], &video_read_fds))
                    {
                        memset(&venc_stream, 0, sizeof(venc_stream));
                        ret = HI_MPI_VENC_Query(i, &stat);
                        if (HI_SUCCESS != ret) 
                        {
                            ERROR_LOG("HI_MPI_VENC_Query chn[%d] fail: %#x!\n", i, ret);
                            continue;
                        }

                        ret = init_venc_stream(&venc_stream, stat.u32CurPacks);
                        if (HLE_RET_OK != ret) 
                        {
                            return NULL;
                        }

                        ret = HI_MPI_VENC_GetStream(i, &venc_stream, 0);
                        if (HI_SUCCESS != ret) 
                        {
                            ERROR_LOG("HI_MPI_VENC_GetStream fail: %#x!\n", ret);
                            continue;
                        }
                        
                         pthread_mutex_lock(&chn_ctx->lock);
                        /* 申请pack并拷贝 stream 数据到pack */
                        ENC_STREAM_PACK *pack = video_stream_to_pack(i, &venc_stream);
                        pthread_mutex_unlock(&chn_ctx->lock);
                        
                        HI_MPI_VENC_ReleaseStream(i, &venc_stream);

                        //释放空间
                        free(venc_stream.pstPack);
                        venc_stream.pstPack = NULL;
                        
                        /*一定要在调用sdp_enqueue之前释放锁，否则有可能死锁*/
                        if (pack)
                            sdp_enqueue(i, pack);
                        
                    }
                    
                }
            }
                
        }
        
    #endif
        
    }

    DEBUG_LOG("venc_get_stream_proc exit!\n");

    if (venc_stream.pstPack != NULL)
    free(venc_stream.pstPack);

    return NULL;
}


extern int AI_to_AO_start;
/*海思aenc 进行g711编码*/
void * Aenc_get_stream_proc(void *arg)
{
    DEBUG_LOG("pid = %d\n", getpid());
    prctl(PR_SET_NAME, "hal_stream", 0, 0, 0);

    /***Aenc fd init*******************/
    int ret,i;
    AUDIO_STREAM_S stream;
    int max_fd1;
    int aenc_fd;
    fd_set audio_read_fds;
    struct timeval time_out;
    FD_ZERO(&audio_read_fds); 
    
    aenc_fd = HI_MPI_AENC_GetFd(TALK_AENC_CHN);
    if (aenc_fd > 0) 
    {
        FD_SET(aenc_fd, &audio_read_fds);
        if (max_fd1 < aenc_fd) 
        {
            max_fd1 = aenc_fd;
        }
    }

     while (enc_ctx.running) 
    {
        //
        while(0 == AI_to_AO_start)
        {
            sleep(1);
            DEBUG_LOG("Aenc Thread wait AI_to_AO start! \n");
        }
      
    #if 1 
        /*****获取 Audio 编码流 , 放入相应缓冲区 *******************/
        time_out.tv_sec = 1;
        time_out.tv_usec = 0;
        ret = select(max_fd1+1, &audio_read_fds, NULL, NULL, &time_out);
        if (ret == -1) 
        {
            if (errno == EINTR) 
            {
                ERROR_LOG("Aenc Thread errno == EINTR");
                continue;
            }
            else 
            {
                ERROR_LOG("Audio select failed\n");
                break;
            }
        }
        else if (ret == 0) 
        {
            ERROR_LOG("Audio select time out !\n");
            continue;
        }
        
        if (FD_ISSET(aenc_fd, &audio_read_fds)) 
        {
            /* get stream from aenc chn */
            ret = HI_MPI_AENC_GetStream(TALK_AENC_CHN, &stream, 0);
            if (ret != HI_SUCCESS) 
            {
                ERROR_LOG("HI_MPI_AENC_GetStream fail: %#x\n", ret);
                continue;
            }

            add_to_talkback_fifo(stream.pStream, stream.u32Len);

            for (i = 0; i < STREAMS_PER_CHN; i++) 
            {
                ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + i;
                pthread_mutex_lock(&chn_ctx->lock);
                if (chn_ctx->state == ENC_STATE_RUNING\
                    && chn_ctx->enc_attr.hasaudio) /* 是否把音频放进数据流里 */ 
                 {
                    /* 申请pack并拷贝 stream 数据到pack */
                    ENC_STREAM_PACK *pack = audio_stream_to_pack(i, &stream);
                    //debug
                    //printf("put one Audio pack into queue! stream_id(%d)\n",i);
                    
                    pthread_mutex_unlock(&chn_ctx->lock);
                    /*一定要在调用sdp_enqueue之前释放锁，否则有可能死锁*/
                    if (pack)
                        sdp_enqueue(i, pack);
                }
                else
                {
                    pthread_mutex_unlock(&chn_ctx->lock);
                }
                  
            }
            HI_MPI_AENC_ReleaseStream(TALK_AENC_CHN, &stream);
        }
        
   #endif     
        
    }

    DEBUG_LOG("Aenc_get_stream_proc exit!\n");
    return NULL;
}

/*AAC获取音频数据进行编码 (AAC编码/server 线程)*/
void * AAC_get_stream_proc(void *arg)
{
    pthread_detach(pthread_self()); 
    DEBUG_LOG("pid = %d\n", getpid());
    prctl(PR_SET_NAME, "hal_stream", 0, 0, 0);

    /***AAC encode fd init*******************/
    int ret,i;
    unsigned int  stream_len = 0;
    
    struct timeval time_out;

    AUDIO_STREAM_S stream;
    struct timeval in_time = {0};
    struct timeval out_time = {0};
    unsigned int use_time = 0; //用时 （ms）
    unsigned int frame_count = 0;//统计1s所产生的帧数

    //01 int AAC_fd = open("/jffs0/aac_out.aac", O_CREAT | O_WRONLY | O_TRUNC, 0664);

     while (enc_ctx.running) 
    {

        while(0 == AI_to_AO_start)
        {
            sleep(1);
            DEBUG_LOG("Aenc Thread wait AI_to_AO start! \n");
        }
      
        /*****获取 Audio 编码流 , 放入相应缓冲区 *******************/
            /* get one encoded AAC frame */
            gettimeofday(&in_time,NULL);
            ret = AAC_AENC_getFrame(&stream);
            if (ret < 0) 
            {
                ERROR_LOG("AAC_AENC_getFrame fail: %#x\n", ret);
                continue;
            }
            frame_count ++;
            gettimeofday(&out_time,NULL); //
            use_time += (out_time.tv_sec - in_time.tv_sec)*1000 + (out_time.tv_usec - in_time.tv_usec)/1000;
           // printf("get AAC frame use time (%d)ms \n",use_time);
           if(use_time >= 1000)//1s
           {
               // printf("1s AAC frame_count (%d)\n",frame_count);
                frame_count = 0;
                use_time = 0;
           }
          //02 write(AAC_fd,stream.pStream, stream.u32Len);

            add_to_talkback_fifo(&stream, ret);

            for (i = 0; i < STREAMS_PER_CHN; i++) 
            {
                ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + i;
                pthread_mutex_lock(&chn_ctx->lock);
                if (chn_ctx->state == ENC_STATE_RUNING\
                    && chn_ctx->enc_attr.hasaudio) /* 是否把音频放进数据流里 */ 
                 {
                    /* 申请pack并拷贝 stream 数据到pack */
                    ENC_STREAM_PACK *pack = audio_stream_to_pack(i, &stream);
                    //debug
                    //printf("put one Audio pack into queue! stream_id(%d)\n",i);
                    
                    pthread_mutex_unlock(&chn_ctx->lock);
                    /*一定要在调用sdp_enqueue之前释放锁，否则有可能死锁*/
                    if (pack)
                        sdp_enqueue(i, pack);
                }
                else
                {
                    pthread_mutex_unlock(&chn_ctx->lock);
                }
                  
            }
        
    }

   //03 close(AAC_fd);
    DEBUG_LOG("Aenc_get_stream_proc exit!\n");
    return NULL;
}



#if 0

void *enc_get_stream_proc(void *arg)
{
    DEBUG_LOG("pid = %d\n", getpid());
    prctl(PR_SET_NAME, "hal_stream", 0, 0, 0);

    /***venc fd init*******************/
    int i, ret, max_fd = 0;
    int venc_fd[ENC_STREAM_NUM];
    VENC_STREAM_S venc_stream;
    VENC_CHN_STAT_S stat;
    fd_set  video_read_fds;
    fd_set  old_fds;
    for (i = 0; i < ENC_STREAM_NUM; i++) 
    {
        venc_fd[i] = HI_MPI_VENC_GetFd(i);
        if (venc_fd[i] < 0) 
        {
            ERROR_LOG("HI_MPI_VENC_GetFd failed, ret = %#x\n", venc_fd[i]);
            return NULL;
        }
        
        if (max_fd < venc_fd[i]) 
        {
            max_fd = venc_fd[i];
        }
    }


    /***Aenc fd init*******************/
    AUDIO_STREAM_S stream;
    int max_fd1;
    int aenc_fd;
    fd_set audio_read_fds;
    FD_ZERO(&audio_read_fds); 
    aenc_fd = HI_MPI_AENC_GetFd(TALK_AENC_CHN);
    if (aenc_fd > 0) 
    {
        FD_SET(aenc_fd, &audio_read_fds);
        if (max_fd1 < aenc_fd) 
        {
            max_fd1 = aenc_fd;
        }
    }
    
     struct timeval time_out;
    
    /***获取音视频编码数据到缓冲队列****************************/
    while (enc_ctx.running) 
    {
        //usleep(1000);
   
    #ifndef DBG_ONLY
        // TempDisable  watchdog_feed();
    #endif

    /*****获取 Video 编码流 , 放入相应缓冲区队列*******************/
    #if 1 
        time_out.tv_sec = 1;
        time_out.tv_usec = 0;
    
        for (i = 0; i < ENC_STREAM_NUM; i++) 
        {
            ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + i;
            FD_ZERO(&video_read_fds);
            FD_SET(venc_fd[i], &video_read_fds);
            
            //pthread_mutex_lock(&chn_ctx->lock);
            if (chn_ctx->state == ENC_STATE_RUNING ) 
            {

                ret = select(max_fd+1, &video_read_fds, NULL, NULL, &time_out);
                if (ret == -1) 
                {
                    if (errno == EINTR) 
                    {
                        continue;
                    }
                    else 
                    {
                        ERROR_LOG("video select failed\n");
                        break;
                    }
                }
                else if (ret == 0) 
                {
                     ERROR_LOG("video select time out, i = %d\n",i);
                    continue;
                }
                else
                {
                    if(FD_ISSET(venc_fd[i], &video_read_fds))
                    {
                        memset(&venc_stream, 0, sizeof(venc_stream));
                        ret = HI_MPI_VENC_Query(i, &stat);
                        if (HI_SUCCESS != ret) 
                        {
                           // pthread_mutex_unlock(&chn_ctx->lock);
                            ERROR_LOG("HI_MPI_VENC_Query chn[%d] fail: %#x!\n", i, ret);
                            continue;
                        }

                        ret = init_venc_stream(&venc_stream, stat.u32CurPacks);
                        if (HLE_RET_OK != ret) 
                        {
                            //pthread_mutex_unlock(&chn_ctx->lock);
                            return NULL;
                        }

                        ret = HI_MPI_VENC_GetStream(i, &venc_stream, 0);
                        if (HI_SUCCESS != ret) 
                        {
                            //pthread_mutex_unlock(&chn_ctx->lock);
                            ERROR_LOG("HI_MPI_VENC_GetStream fail: %#x!\n", ret);
                            continue;
                        }

                        /* 申请pack并拷贝 stream 数据到pack */
                        pthread_mutex_lock(&chn_ctx->lock);
                        ENC_STREAM_PACK *pack = video_stream_to_pack(i, &venc_stream);

                        HI_MPI_VENC_ReleaseStream(i, &venc_stream);
                        pthread_mutex_unlock(&chn_ctx->lock);
                        /*一定要在调用sdp_enqueue之前释放锁，否则有可能死锁*/
                        if (pack)
                            sdp_enqueue(i, pack);
                    }
                }   
            }
            else
            {
               // pthread_mutex_unlock(&chn_ctx->lock);
            }
        }
    #endif


    #if 1
        /*****获取 Audio 编码流 , 放入相应缓冲区 *******************/
        time_out.tv_sec = 1;
        time_out.tv_usec = 0;
        ret = select(max_fd1+1, &audio_read_fds, NULL, NULL, &time_out);
        if (ret == -1) 
        {
            if (errno == EINTR) 
            {
                continue;
            }
            else 
            {
                ERROR_LOG("Audio select failed\n");
                break;
            }
        }
        else if (ret == 0) 
        {
            ERROR_LOG("Audio select time out, exit thread\n");
            continue;
        }
        
        if (FD_ISSET(aenc_fd, &audio_read_fds)) 
        {
            /* get stream from aenc chn */
            ret = HI_MPI_AENC_GetStream(TALK_AENC_CHN, &stream, 0);
            if (ret != HI_SUCCESS) 
            {
                ERROR_LOG("HI_MPI_AENC_GetStream fail: %#x\n", ret);
                continue;
            }

            add_to_talkback_fifo(stream.pStream, stream.u32Len);

            for (i = 0; i < STREAMS_PER_CHN; i++) 
            {
                ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + i;
                pthread_mutex_lock(&chn_ctx->lock);
                if (chn_ctx->state == ENC_STATE_RUNING\
                    && chn_ctx->enc_attr.hasaudio) /* 是否把音频放进数据流里 */ 
                 {
                    /* 申请pack并拷贝 stream 数据到pack */
                    ENC_STREAM_PACK *pack = audio_stream_to_pack(i, &stream);

                    pthread_mutex_unlock(&chn_ctx->lock);
                    /*一定要在调用sdp_enqueue之前释放锁，否则有可能死锁*/
                    if (pack)
                        sdp_enqueue(i, pack);
                }
                else
                {
                    pthread_mutex_unlock(&chn_ctx->lock);
                }
                  
            }
            HI_MPI_AENC_ReleaseStream(TALK_AENC_CHN, &stream);
        }
   #endif     

    }
    
    if (venc_stream.pstPack != NULL)
        free(venc_stream.pstPack);

    return NULL;
}


#endif

#define CHAR_PIXEL_WIDTH        8
#define CHAR_PIXEL_HEIGHT       16

#define TIME_OSD_STR_LEN        20
#define TIME_OSD_PIXEL_WIDTH    (CHAR_PIXEL_WIDTH*(TIME_OSD_STR_LEN-1))
#define TIME_OSD_PIXEL_HEIGHT   CHAR_PIXEL_HEIGHT
#define TIME_OSD_PIXEL_LEN      (TIME_OSD_PIXEL_WIDTH*TIME_OSD_PIXEL_HEIGHT)

#define RATE_OSD_STR_LEN       16
#define RATE_OSD_PIXEL_WIDTH   (CHAR_PIXEL_WIDTH*(RATE_OSD_STR_LEN-1))
#define RATE_OSD_PIXEL_HEIGHT  CHAR_PIXEL_HEIGHT
#define RATE_OSD_PIXEL_LEN     (RATE_OSD_PIXEL_WIDTH*RATE_OSD_PIXEL_HEIGHT)

static pthread_mutex_t osd_lock[VI_PORT_NUM];
static OSD_BITMAP_ATTR osd_attrs[VI_PORT_NUM][VI_OSD_NUM] = {
    {
        {0}
    }
};
static HLE_U8 time_osd_matrix[TIME_OSD_PIXEL_LEN / 8];
static HLE_U8 frame_osd_matrix[RATE_OSD_PIXEL_LEN / 8];

static pthread_mutex_t roi_lock[VI_PORT_NUM];
static ENC_ROI_ATTR roi_attrs[VI_PORT_NUM][ROI_REGION_NUM];

static int copy_osd_attr(int osd_index, OSD_BITMAP_ATTR *dest, OSD_BITMAP_ATTR *src)
{
    dest->enable = src->enable;
    dest->x = src->x;
    dest->y = src->y;
    dest->fg_color = src->fg_color;
    dest->bg_color = src->bg_color;

    if (osd_index == TIME_OSD_INDEX) {
        dest->width = TIME_OSD_PIXEL_WIDTH;
        dest->height = TIME_OSD_PIXEL_HEIGHT;
    }
    else if (osd_index == RATE_OSD_INDEX) {
        dest->width = RATE_OSD_PIXEL_WIDTH;
        dest->height = RATE_OSD_PIXEL_HEIGHT;
    }
    else {
        int src_size = (src->width / 8) * src->height;
        if (dest->raster != NULL) {
            int dest_size = (dest->width / 8) * dest->height;
            if (src_size != dest_size) {
                free(dest->raster);
                dest->raster = malloc(src_size);
            }
        }
        else
            dest->raster = malloc(src_size);

        if (dest->raster == NULL) {
            ERROR_LOG("malloc text raster failed!\n");
            return -1;
        }
        dest->width = src->width;
        dest->height = src->height;
        memcpy(dest->raster, src->raster, src_size);
    }

    return 0;
}

static HLE_S32 date_string_format = DDF_YYYYMMDD;

void encoder_set_date_format(E_DATE_DISPLAY_FMT date_fmt)
{
    date_string_format = date_fmt;
}

void get_date_string(char *time_str)
{
    time_t time_cur;
    time(&time_cur);

    struct tm time_date;
    localtime_r(&time_cur, &time_date);

    HLE_S32 year = 1900 + time_date.tm_year;
    HLE_S32 mon = 1 + time_date.tm_mon;
    HLE_S32 mday = time_date.tm_mday;
    HLE_S32 hour = time_date.tm_hour;
    HLE_S32 min = time_date.tm_min;
    HLE_S32 sec = time_date.tm_sec;

    switch (date_string_format) {
    case DDF_YYYYMMDD:
    {
        snprintf(time_str, TIME_OSD_STR_LEN, "%04d-%02d-%02d %02d:%02d:%02d",
                 year, mon, mday, hour, min, sec);
        break;
    }
    case DDF_MMDDYYYY:
    {
        snprintf(time_str, TIME_OSD_STR_LEN, "%02d/%02d/%04d %02d:%02d:%02d",
                 mon, mday, year, hour, min, sec);
        break;
    }
    case DDF_DDMMYYYY:
    {
        snprintf(time_str, TIME_OSD_STR_LEN, "%02d/%02d/%04d %02d:%02d:%02d",
                 mday, mon, year, hour, min, sec);
        break;
    }
    default:
        DEBUG_LOG("Unsupport date string format\n");
        break;
    }
}

#define RC_PATH    "/proc/umap/rc"
#define VENC_PATH   "/proc/umap/venc"
#define MODE    "r"

#if 0

int get_bitrate(int encChn, int *bitrate)
{
    int ret = HLE_RET_ERROR;
    FILE *fp = fopen(RC_PATH, MODE);
    if (!fp) {
        ERROR_LOG("open %s error\n", RC_PATH);
        return ret;
    }

    int flag = 1;
    char line[128];
    while (fgets(line, sizeof (line), fp)) {
        char c;
        if (flag) {
            if (sscanf(line, "%*[-] BASE PARAMS %c", &c) == 1) {
                flag = 0;
            }
            continue;
        }
        int chn = -1;

        if (sscanf(line, "%d %*d %*d %*d %*s %*d %*s %d", &chn, bitrate) != 2) {
            continue;
        }

        if (chn != encChn) {
            continue;
        }
        else {
            ret = HLE_RET_OK;
            break;
        }
    }

    fclose(fp);
    return ret;
}

int get_real_frame_rate(int encChn, int *frame_rate)
{
    int ret = HLE_RET_ERROR;
    FILE *fp = fopen(VENC_PATH, MODE);
    if (!fp) {
        ERROR_LOG("fopen %s error\n", VENC_PATH);
        return ret;
    }

    int flag = 1;
    char line[128];
    while (fgets(line, sizeof (line), fp)) {
        char c;
        if (flag) {
            if (sscanf(line, "%*[-] VENC STREAM STATE %c", &c) == 1) {
                flag = 0;
            }
            continue;
        }
        int chn = -1;
        if (sscanf(line, "%d %*d %*d %*d %*d %*d %*d %*d %d ", &chn, frame_rate) != 2)
            continue;

        if (chn != encChn)
            continue;
        else {
            ret = HLE_RET_OK;
            break;
        }
    }

    fclose(fp);
    return ret;
}
#endif

int get_real_framerate_bitrate(int encChn, int* frmrate, int* bitrate)
{
    int ret = HLE_RET_ERROR;
    FILE *fp = fopen(RC_PATH, MODE);
    if (!fp) {
        ERROR_LOG("open %s error\n", RC_PATH);
        return ret;
    }

    int flag = 1;
    char line[128];
    while (fgets(line, sizeof (line), fp)) {
        char c;
        if (flag) {
            if (sscanf(line, "%*[-]RUN INFO%c", &c) == 1) {
                flag = 0;
            }
            continue;
        }
        int chn = -1;

        if (sscanf(line, "%d %d %d", &chn, bitrate, frmrate) != 3) {
            continue;
        }

        if (chn != encChn) {
            continue;
        }
        else {
            ret = HLE_RET_OK;
            break;
        }
    }

    fclose(fp);
    return ret;
}

void get_frame_str(int channel, int stream_index, char *frame_str)
{
    int encChn = GET_ENC_CHN(channel, stream_index);

    int frm_rate;
    int bit_rate;
    get_real_framerate_bitrate(encChn, &frm_rate, &bit_rate);

    snprintf(frame_str, RATE_OSD_STR_LEN, "%2dfps,%5dkbps", frm_rate, bit_rate);
    frame_str[RATE_OSD_STR_LEN - 1] = 0;
}

static int get_encoder_size(int encChn, int *width, int *height)
{
    if (enc_ctx.encChn[encChn].state != ENC_STATE_RUNING)
        return HLE_RET_ERROR;

    return get_image_size(enc_ctx.encChn[encChn].enc_attr.img_size, width, height);
}

#if 0
int needWdr = 0;

static void stat_luma(HI_U8* pY, VIDEO_FRAME_INFO_S* vfi)
{
    int sum[1920 / 64][1088 / 64]; //1920/64=30, 1088/64=17
    memset(sum, 0, sizeof (sum));
    int x, y, sx, sy;
    for (x = 0; x < vfi->stVFrame.u32Width; x++) {
        for (y = 0; y < vfi->stVFrame.u32Height; y++) {
            sx = x / 64;
            sy = y / 64;
            sum[sx][sy] += *pY++;
        }
    }

    int lvl[256 / 16];
    memset(lvl, 0, sizeof (lvl));
    for (sx = 0; sx < 1920 / 64; sx++) {
        for (sy = 0; sy < 1088 / 64; sy++) {
            lvl[sum[sx][sy] / (16 * 64 * 64)]++;
        }
    }

    if ((lvl[0] + lvl[1] + lvl[2]) > 200) {
        needWdr++;
        if (needWdr > 10) needWdr = 10;
    }
    else {
        needWdr--;
        if (needWdr < 0) needWdr = 0;
    }

    /*
                    for (x = 0; x < 256 / 16; ++x) {
                                    printf("%d ", lvl[x]);
                    }
                    printf("--needWdr=%d\n", needWdr);
    //*/
}
#endif

#if 0

static int need_reverse_osd(OSD_BITMAP_ATTR *osd_attr, VIDEO_FRAME_INFO_S* vfi, int idx, HI_U8* pY)
{
    if (pY) {
        //printf("----------------------\npY=%p\n", pY);
        //printf("%d, %d, %d, %d, %d, %d, %d\n%d, %d, %d, %d\n", vfi->stVFrame.u32Width, vfi->stVFrame.u32Height, vfi->stVFrame.u32Field, vfi->stVFrame.enPixelFormat, 
        //  vfi->stVFrame.enVideoFormat, vfi->stVFrame.enCompressMode, vfi->stVFrame.u32Stride[0], vfi->stVFrame.s16OffsetTop, 
        //  vfi->stVFrame.s16OffsetBottom, vfi->stVFrame.s16OffsetLeft, vfi->stVFrame.s16OffsetRight);
        int enc_w = 1920;
        int enc_h = 1080;
        //get_encoder_size(VPSS_CHN_ENC0, &enc_w, &enc_h);
        //int osd_w = (osd_attr->width * enc_w / 480) & (~1);
        //int osd_h = (osd_attr->height * enc_h / 270) & (~1);
        int osd_w = (osd_attr->width * 4) & (~1);
        int osd_h = (osd_attr->height * 4) & (~1);
        int x = osd_attr->x * enc_w / REL_COORD_WIDTH;
        int y = osd_attr->y * enc_h / REL_COORD_HEIGHT;
        if (x + osd_w > enc_w) {
            x = enc_w - osd_w;
            if (x < 0) x = 0;
        }
        if (y + osd_h > enc_h) {
            y = enc_h - osd_h;
            if (y < 0) y = 0;
        }
        int vix, viy;
        vix = (idx % (osd_attr->width / 8)) * 8 * 4;
        viy = 0;
        int i, j;
        HI_U32 luma = 0;
        for (i = 0; i < 8 * 4; i++) {
            for (j = 0; j < 16 * 4; j++) {
                HI_U8* p = pY + (y + viy + vfi->stVFrame.s16OffsetTop + j) * vfi->stVFrame.u32Stride[0] + (x + vix + vfi->stVFrame.s16OffsetLeft + i);
                //printf("p=%p\n", p);
                luma += *p;
            }
        }
        if (luma > 160 * 16 * 4 * 8 * 4) return 1;
    }

    return 0;
}
#endif

#define RGB24_TO_RGB15(c) (HLE_U16)((((((c)>>16)&0xFF)>>3)<<10) | (((((c)>>8)&0xff)>>3)<<5) | (((c)&0XFF)>>3))

/*点阵转BMP位图*/
static void draw_bmp(OSD_BITMAP_ATTR *osd_attr, HLE_U16 *bitmap)
{
    HLE_U32 i, j;
    int matrix_len = (osd_attr->width / 8) * osd_attr->height;

    HLE_U8 *matrix = osd_attr->raster;
    HLE_U16 fg_color = RGB24_TO_RGB15(osd_attr->fg_color) | 0x8000;
    HLE_U16 bg_color = RGB24_TO_RGB15(osd_attr->bg_color);
#if 0
    HLE_U16 rv_color = (~RGB24_TO_RGB15(osd_attr->fg_color)) | 0x8000;
    VIDEO_FRAME_INFO_S vfi;
    HI_S32 ret = -1;
    ret = HI_MPI_VI_GetFrame(VI_PHY_CHN, &vfi, 10);
    HI_U8* pY = NULL;
    int msz = vfi.stVFrame.u32Stride[0] * vfi.stVFrame.u32Height;
    if (HI_SUCCESS == ret) {
        pY = (HI_U8*) HI_MPI_SYS_Mmap(vfi.stVFrame.u32PhyAddr[0], msz);
        //stat_luma(pY, &vfi);
    }
#endif
    for (i = 0; i < matrix_len; i++) {
        //HLE_U16 clr = need_reverse_osd(osd_attr, &vfi, i, pY) ? rv_color : fg_color;
        for (j = 0; j < 8; j++) {
            if (*matrix & (0x01 << (7 - j))) {
                *(bitmap++) = fg_color;
            }
            else {
                *(bitmap++) = bg_color;
            }
        }

        matrix++;
    }
#if 0
    if (pY) {
        HI_MPI_SYS_Munmap(pY, msz);
    }
    if (!ret) {
        HI_MPI_VI_ReleaseFrame(VI_PHY_CHN, &vfi);
    }
#endif
}

static int get_jpeg_encoder_size(int *width, int *height)
{
    return get_image_size(jpeg_chn_attr.img_size, width, height);
}

//创建OSD区域

static int create_osd_region(RGN_HANDLE osd_handle, int width, int height)
{
    RGN_ATTR_S rgn_attr;

    rgn_attr.enType = OVERLAY_RGN;
    rgn_attr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
    rgn_attr.unAttr.stOverlay.stSize.u32Width = width;
    rgn_attr.unAttr.stOverlay.stSize.u32Height = height;
    rgn_attr.unAttr.stOverlay.u32BgColor = 0;
    rgn_attr.unAttr.stOverlay.u32CanvasNum = 2;
    int ret = HI_MPI_RGN_Create(osd_handle, &rgn_attr);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_RGN_Create (%d) fail: %#x!\n", osd_handle, ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

//把位图设置到区域上

static int set_osd_bitmap(RGN_HANDLE osd_handle, int width, int height, void *bmp)
{
    BITMAP_S st_bitmap;

    st_bitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
    st_bitmap.u32Width = width;
    st_bitmap.u32Height = height;
    st_bitmap.pData = bmp;
    int ret = HI_MPI_RGN_SetBitMap(osd_handle, &st_bitmap);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_RGN_SetBitMap fail: %#x!\n", ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

static int osd_region_attach(int encChn, int osd_index, int enc_w, int enc_h,
                             int osd_w, int osd_h, OSD_BITMAP_ATTR *osd_attr)
{
    //DEBUG_LOG("osd_region_attach encChn %d, osd_index %d\n", encChn, osd_index);
    int x = osd_attr->x * enc_w / REL_COORD_WIDTH;
    int y = osd_attr->y * enc_h / REL_COORD_HEIGHT;

    //如果超出视频范围的话，调整起始坐标，但是必须保证起始坐标非负
    if (x + osd_w > enc_w) {
        x = enc_w - osd_w;
        if (x < 0)
            x = 0;
    }
    if (y + osd_h > enc_h) {
        y = enc_h - osd_h;
        if (y < 0)
            y = 0;
    }

    MPP_CHN_S st_chn;
    st_chn.enModId = HI_ID_VENC;
    st_chn.s32DevId = 0;
    st_chn.s32ChnId = encChn;
    RGN_CHN_ATTR_S osd_chn_attr = {0};
    osd_chn_attr.bShow = osd_attr->enable;
    osd_chn_attr.enType = OVERLAY_RGN;
    osd_chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = x & (~3);
    osd_chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = y & (~3);
    osd_chn_attr.unChnAttr.stOverlayChn.u32FgAlpha = osd_attr->fg_color >> 25;
    osd_chn_attr.unChnAttr.stOverlayChn.u32BgAlpha = osd_attr->bg_color >> 25;
    osd_chn_attr.unChnAttr.stOverlayChn.u32Layer = osd_index;
    osd_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    osd_chn_attr.unChnAttr.stOverlayChn.stQpInfo.s32Qp = 0;
    osd_chn_attr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;
    osd_chn_attr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    osd_chn_attr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
    osd_chn_attr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = MORETHAN_LUM_THRESH;
    osd_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    osd_chn_attr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;

    RGN_HANDLE osd_handle = GET_OSD_HANDLE(encChn, osd_index);
    int ret = HI_MPI_RGN_AttachToChn(osd_handle, &st_chn, &osd_chn_attr);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_RGN_AttachToChn fail: %#x!\n", ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

static int change_osd_position(int encChn, RGN_HANDLE osd_handle, int enc_w,
                               int enc_h, int osd_w, int osd_h, OSD_BITMAP_ATTR *osd_attr)
{
    MPP_CHN_S st_chn;
    RGN_CHN_ATTR_S osd_chn_attr;
    st_chn.enModId = HI_ID_VENC;
    st_chn.s32DevId = 0;
    st_chn.s32ChnId = encChn;
    int ret = HI_MPI_RGN_GetDisplayAttr(osd_handle, &st_chn, &osd_chn_attr);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_RGN_GetDisplayAttr encChn(%d) fail: %#x!\n",
                  encChn, ret);
        return HLE_RET_ERROR;
    }

    int x = osd_attr->x * enc_w / REL_COORD_WIDTH;
    int y = osd_attr->y * enc_h / REL_COORD_HEIGHT;

    //如果超出视频范围的话，调整起始坐标，但是必须保证起始坐标非负
    if (x + osd_w > enc_w) {
        x = enc_w - osd_w;
        if (x < 0)
            x = 0;
    }
    if (y + osd_h > enc_h) {
        y = enc_h - osd_h;
        if (y < 0)
            y = 0;
    }

    osd_chn_attr.unChnAttr.stOverlayChn.stPoint.s32X = x & (~3);
    osd_chn_attr.unChnAttr.stOverlayChn.stPoint.s32Y = y & (~3);
    ret = HI_MPI_RGN_SetDisplayAttr(osd_handle, &st_chn, &osd_chn_attr);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_RGN_GetDisplayAttr encChn(%d) fail: %#x!\n",
                  encChn, ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}


//配置单个OSD区域，enc_w和enc_h为所在编码通道编码图像的宽高

static int config_single_osd(int encChn, int osd_index, int enc_w, int enc_h,
                             OSD_BITMAP_ATTR *osd_attr, HLE_SURFACE *org_sfc)
{
    RGN_HANDLE osd_handle = GET_OSD_HANDLE(encChn, osd_index);
    RGN_ATTR_S rgn_attr;

    //DEBUG_LOG("encChn %d, osd_index %d, enc_w %d, enc_h %d\n", encChn, osd_index, enc_w, enc_h);

    int osd_w;
    int osd_h;
    int stream_idx = ENC_GET_STREAN_INDEX(encChn);
    if (stream_idx >= 2) {
        osd_w = osd_attr->width & (~1);
        osd_h = osd_attr->height & (~1);
    }
    else {
        //汉字字库大小为16X16，适合作为480X270的OSD，其他大小需要缩放
        //根据编码图像高宽计算出OSD高宽
        osd_w = (osd_attr->width * enc_w / 480) & (~1);
        osd_h = (osd_attr->height * enc_h / 270) & (~1);
    }

    if (HI_MPI_RGN_GetAttr(osd_handle, &rgn_attr) != HI_SUCCESS) {
        //获取属性失败则创建
        if (create_osd_region(osd_handle, osd_w, osd_h) != HLE_RET_OK)
            return HLE_RET_ERROR;

        HLE_SURFACE new_sfc;
        new_sfc.u32Width = osd_w;
        new_sfc.u32Height = osd_h;
        char *new_bmp = create_surface(&new_sfc);
        if (new_bmp == NULL) {
            ERROR_LOG("malloc new_bmp failed!\n");
            return HLE_RET_ENORESOURCE;
        }
        scale_surface(&new_sfc, org_sfc);

        if (set_osd_bitmap(osd_handle, osd_w, osd_h, new_bmp) != HLE_RET_OK) {
            destroy_surface(&new_sfc, new_bmp);
            return HLE_RET_ERROR;
        }

        osd_region_attach(encChn, osd_index, enc_w, enc_h, osd_w, osd_h, osd_attr);
        destroy_surface(&new_sfc, new_bmp);
    }
    else if (rgn_attr.unAttr.stOverlay.stSize.u32Width != osd_w
         || rgn_attr.unAttr.stOverlay.stSize.u32Height != osd_h) {
        //大小不符合要求则先销毁再创建
        HI_MPI_RGN_Destroy(osd_handle);
        if (create_osd_region(osd_handle, osd_w, osd_h) != HLE_RET_OK)
            return HLE_RET_ERROR;

        HLE_SURFACE new_sfc;
        new_sfc.u32Width = osd_w;
        new_sfc.u32Height = osd_h;
        char *new_bmp = create_surface(&new_sfc);
        if (new_bmp == NULL) {
            ERROR_LOG("malloc new_bmp failed!\n");
            return HLE_RET_ENORESOURCE;
        }
        scale_surface(&new_sfc, org_sfc);

        if (set_osd_bitmap(osd_handle, osd_w, osd_h, new_bmp) != HLE_RET_OK) {
            destroy_surface(&new_sfc, new_bmp);
            return HLE_RET_ERROR;
        }

        osd_region_attach(encChn, osd_index, enc_w, enc_h, osd_w, osd_h, osd_attr);
        destroy_surface(&new_sfc, new_bmp);
    }
    else {
        HLE_SURFACE new_sfc;
        new_sfc.u32Width = osd_w;
        new_sfc.u32Height = osd_h;
        char *new_bmp = create_surface(&new_sfc);
        if (new_bmp == NULL) {
            ERROR_LOG("malloc new_bmp failed!\n");
            return HLE_RET_ENORESOURCE;
        }
        scale_surface(&new_sfc, org_sfc);

        set_osd_bitmap(osd_handle, osd_w, osd_h, new_bmp);
        change_osd_position(encChn, osd_handle, enc_w, enc_h, osd_w, osd_h, osd_attr);
        destroy_surface(&new_sfc, new_bmp);
    }

    return HLE_RET_OK;
}

static int create_single_osd_jpeg(int encChn, int osd_index,
                                  OSD_BITMAP_ATTR *osd_attr, HLE_SURFACE *org_sfc)
{
    RGN_HANDLE osd_handle = GET_OSD_HANDLE(encChn, osd_index);

    int enc_w, enc_h;
    get_jpeg_encoder_size(&enc_w, &enc_h);

    int osd_w, osd_h;
    osd_w = (osd_attr->width * enc_w / 480) & (~1);
    osd_h = (osd_attr->height * enc_h / 270) & (~1);

    if (create_osd_region(osd_handle, osd_w, osd_h) != HLE_RET_OK)
        return HLE_RET_ERROR;

    HLE_SURFACE new_sfc;
    new_sfc.u32Width = osd_w;
    new_sfc.u32Height = osd_h;
    char *new_bmp = create_surface(&new_sfc);
    if (new_bmp == NULL) {
        ERROR_LOG("malloc new_bmp failed!\n");
        return HLE_RET_ENORESOURCE;
    }
    scale_surface(&new_sfc, org_sfc);

    if (set_osd_bitmap(osd_handle, osd_w, osd_h, new_bmp) != HLE_RET_OK) {
        destroy_surface(&new_sfc, new_bmp);
        return HLE_RET_ERROR;
    }

    osd_region_attach(encChn, osd_index, enc_w, enc_h, osd_w, osd_h, osd_attr);
    destroy_surface(&new_sfc, new_bmp);

    return HLE_RET_OK;
}

static void* time_osd_proc(void *arg)
{
    DEBUG_LOG("pid = %d\n", getpid());
    prctl(PR_SET_NAME, "hal_timeosd", 0, 0, 0);

    char *org_bmp;
    HLE_SURFACE org_sfc;

    org_sfc.u32Width = TIME_OSD_PIXEL_WIDTH;
    org_sfc.u32Height = TIME_OSD_PIXEL_HEIGHT;
    org_bmp = create_surface(&org_sfc);
    if (org_bmp == NULL) {
        ERROR_LOG("create_surface org_bmp failed!\n");
        return NULL;
    }
    sleep(2);
    time_t time_cur = 0;
    time_t time_old = 0;
    while (enc_ctx.running) {
        time(&time_cur); /*取得当前时间,判断是否需要更新TIME_OSD*/
        if (time_cur == time_old)
            goto continue_sleep;

        //struct timeval tv1, tv2;
        //gettimeofday(&tv1, NULL);
        time_old = time_cur;

        if (enc_ctx.running == 0x197521) {
            continue;
        }

        HLE_U8 time_str[TIME_OSD_STR_LEN];
        get_date_string((char *) time_str);
        str2matrix(time_str, time_osd_matrix);

        int i, j;
        for (i = 0; i < VI_PORT_NUM; i++) {
            pthread_mutex_lock(osd_lock + i);
            OSD_BITMAP_ATTR *osd_attr = &osd_attrs[i][TIME_OSD_INDEX];
            if (osd_attr->enable == 0) {
                pthread_mutex_unlock(osd_lock + i);
                continue;
            }

            osd_attr->raster = time_osd_matrix;
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);
            for (j = 0; j < STREAMS_PER_CHN; j++) {
                int enc_w, enc_h;
                int encChn = GET_ENC_CHN(i, j);
                if (get_encoder_size(encChn, &enc_w, &enc_h) != HLE_RET_OK)
                    continue;

                config_single_osd(encChn, 0, enc_w, enc_h, osd_attr, &org_sfc);
            }
            pthread_mutex_unlock(osd_lock + i);
        }
        //gettimeofday(&tv2, NULL);
        //int used = (tv2.tv_sec - tv1.tv_sec)*1000*1000 + tv2.tv_usec - tv1.tv_usec;
        //DEBUG_LOG("used time %d\n", used);

continue_sleep:
        usleep(100 * 1000);
    }

    destroy_surface(&org_sfc, org_bmp);
    return NULL;
}

static void* rate_osd_proc(void *arg)
{
    DEBUG_LOG("pid = %d\n", getpid());
    prctl(PR_SET_NAME, "hal_rateosd", 0, 0, 0);

    char *org_bmp;
    HLE_SURFACE org_sfc;

    org_sfc.u32Width = RATE_OSD_PIXEL_WIDTH;
    org_sfc.u32Height = RATE_OSD_PIXEL_HEIGHT;
    org_bmp = create_surface(&org_sfc);
    if (org_bmp == NULL) {
        ERROR_LOG("create_surface org_bmp failed!\n");
        return NULL;
    }
    sleep(2);

    while (enc_ctx.running) {
        if (enc_ctx.running == 0x197521) {
            sleep(1);
            continue;
        }

        int i, j;

        for (i = 0; i < VI_PORT_NUM; i++) {
            pthread_mutex_lock(osd_lock + i);
            OSD_BITMAP_ATTR *osd_attr = &osd_attrs[i][RATE_OSD_INDEX];
            if (osd_attr->enable == 0) {
                pthread_mutex_unlock(osd_lock + i);
                continue;
            }

            for (j = 0; j < STREAMS_PER_CHN; j++) {
                int enc_w, enc_h;
                int encChn = GET_ENC_CHN(i, j);

                if (get_encoder_size(encChn, &enc_w, &enc_h) != HLE_RET_OK)
                    continue;

                HLE_U8 frame_str[RATE_OSD_STR_LEN];
                get_frame_str(i, j, (char *) frame_str);
                str2matrix(frame_str, frame_osd_matrix);

                osd_attr->raster = frame_osd_matrix;
                draw_bmp(osd_attr, (HLE_U16 *) org_bmp);

                config_single_osd(encChn, RATE_OSD_INDEX, enc_w, enc_h, osd_attr, &org_sfc);
            }
            pthread_mutex_unlock(osd_lock + i);
        }

        usleep(1000 * 1000);
    }

    destroy_surface(&org_sfc, org_bmp);
    return NULL;
}

static int config_osd_region(int channel, int osd_index, OSD_BITMAP_ATTR *osd_attr)
{
    char *org_bmp;
    HLE_SURFACE org_sfc;

    org_sfc.u32Width = osd_attr->width;
    org_sfc.u32Height = osd_attr->height;
    org_bmp = create_surface(&org_sfc);
    if (org_bmp == NULL) {
        ERROR_LOG("create_surface org_bmp failed!\n");
        return -1;
    }

    if (osd_index == TIME_OSD_INDEX) {
        HLE_U8 time_str[TIME_OSD_STR_LEN];
        get_date_string((char *) time_str);
        str2matrix(time_str, time_osd_matrix);
        osd_attr->raster = time_osd_matrix;
        draw_bmp(osd_attr, (HLE_U16 *) org_bmp);

    }
    else if (osd_index != RATE_OSD_INDEX)
        draw_bmp(osd_attr, (HLE_U16 *) org_bmp);

    int i;
    for (i = 0; i < STREAMS_PER_CHN; i++) {
        int enc_w, enc_h;
        int encChn = GET_ENC_CHN(channel, i);
        if (get_encoder_size(encChn, &enc_w, &enc_h) != HLE_RET_OK)
            continue;

        if (osd_index == RATE_OSD_INDEX) {
            HLE_U8 frame_str[RATE_OSD_STR_LEN];
            get_frame_str(channel, i, (char *) frame_str);
            str2matrix(frame_str, frame_osd_matrix);
            osd_attr->raster = frame_osd_matrix;
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);
        }

        config_single_osd(encChn, osd_index, enc_w, enc_h, osd_attr, &org_sfc);
    }

    destroy_surface(&org_sfc, org_bmp);
    return HLE_RET_OK;
}

static int venc_update_osd(int channel, int stream_index, int enc_w, int enc_h)
{
    int i;
    int encChn = GET_ENC_CHN(channel, stream_index);
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

    for (i = 0; i < VI_OSD_NUM; i++) {
        OSD_BITMAP_ATTR *osd_attr = &osd_attrs[channel][i];
        DEBUG_LOG("channel %d, stream_index %d, osd_index %d, osd_attr->enable %d\n",
                  channel, stream_index, i, osd_attr->enable);
        if (osd_attr->enable == 0)
            continue;

        HLE_SURFACE org_sfc;
        org_sfc.u32Width = osd_attr->width;
        org_sfc.u32Height = osd_attr->height;
        char *org_bmp = create_surface(&org_sfc);
        if (org_bmp == NULL) {
            ERROR_LOG("create_surface org_bmp failed!\n");
            return HLE_RET_ENORESOURCE;
        }
        if (i == TIME_OSD_INDEX) {
            HLE_U8 time_str[TIME_OSD_STR_LEN];
            get_date_string((char *) time_str);
            str2matrix(time_str, time_osd_matrix);
            osd_attr->raster = time_osd_matrix;
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);
        }
        else if (i == RATE_OSD_INDEX) {
            HLE_U8 frame_str[RATE_OSD_STR_LEN];
            get_frame_str(channel, stream_index, (char *) frame_str);
            str2matrix(frame_str, frame_osd_matrix);
            osd_attr->raster = frame_osd_matrix;
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);
        }
        else
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);

        config_single_osd(encChn, i, enc_w, enc_h, osd_attr, &org_sfc);
        destroy_surface(&org_sfc, org_bmp);
    }

    gettimeofday(&tv2, NULL);
    int used = (tv2.tv_sec - tv1.tv_sec)*1000 * 1000 + tv2.tv_usec - tv1.tv_usec;
    DEBUG_LOG("used time %d\n", used);

    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

static int destroy_osd_jpeg(int channel, OSD_BITMAP_ATTR *osd_attrs_jpeg)
{
    int encChn = GET_JPEG_ENC_CHN(channel);
    MPP_CHN_S mpp_chn;
    mpp_chn.enModId = HI_ID_VENC;
    mpp_chn.s32DevId = 0;
    mpp_chn.s32ChnId = encChn;

    int i;
    for (i = 0; i < VI_OSD_NUM; i++) {
        OSD_BITMAP_ATTR *osd_attr = osd_attrs_jpeg + i;
        if ((osd_attr->enable == 0) || (i == RATE_OSD_INDEX))
            continue;

        RGN_HANDLE osd_handle = GET_OSD_HANDLE(encChn, i);
        int ret;
        ret = HI_MPI_RGN_DetachFromChn(osd_handle, &mpp_chn);
        if (HI_SUCCESS != ret) {
            ERROR_LOG("HI_MPI_RGN_DetachFrmChn(%d, %d)fail: %#x!\n", encChn, i, ret);
            return HLE_RET_ERROR;
        }

        ret = HI_MPI_RGN_Destroy(osd_handle);
        if (HLE_RET_OK != ret) {
            ERROR_LOG("HI_MPI_RGN_Destroy fail: %#x \n", ret);
            return HLE_RET_ERROR;
        }
    }

    return HLE_RET_OK;
}

static int create_osd_jpeg(int channel, OSD_BITMAP_ATTR *osd_attrs_jpeg)
{
    int i;
    int encChn = GET_JPEG_ENC_CHN(channel);

    for (i = 0; i < VI_OSD_NUM; i++) {
        OSD_BITMAP_ATTR *osd_attr = osd_attrs_jpeg + i;
        if (osd_attr->enable == 0)
            continue;

        HLE_SURFACE org_sfc;
        org_sfc.u32Width = osd_attr->width;
        org_sfc.u32Height = osd_attr->height;
        char *org_bmp = create_surface(&org_sfc);
        if (org_bmp == NULL) {
            ERROR_LOG("create_surface org_bmp failed!\n");
            return HLE_RET_ENORESOURCE;
        }
        if (i == TIME_OSD_INDEX) {
            HLE_U8 time_str[TIME_OSD_STR_LEN];
            get_date_string((char *) time_str);
            str2matrix(time_str, time_osd_matrix);
            osd_attr->raster = time_osd_matrix;
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);
        }
        else if (i == RATE_OSD_INDEX)
            continue;
        else
            draw_bmp(osd_attr, (HLE_U16 *) org_bmp);

        create_single_osd_jpeg(encChn, i, osd_attr, &org_sfc);
        destroy_surface(&org_sfc, org_bmp);
    }

    return HLE_RET_OK;
}

int encoder_set_osd(int channel, int osd_index, OSD_BITMAP_ATTR *osd_attr)
{
    OSD_BITMAP_ATTR *old_attr = &osd_attrs[channel][osd_index];
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
    pthread_mutex_lock(osd_lock + channel);
    if (osd_attr->enable == 0) {
        if (old_attr->enable == 0) //Nothing to do
        {
            pthread_mutex_unlock(osd_lock + channel);
            return HLE_RET_OK;
        }

        //销毁和该通道相关联的所有REGION
        int i;
        for (i = 0; i < STREAMS_PER_CHN; i++) {
            int encChn = GET_ENC_CHN(channel, i);
            RGN_HANDLE osd_handle = GET_OSD_HANDLE(encChn, osd_index);
            HI_MPI_RGN_Destroy(osd_handle);
        }

        old_attr->enable = 0;
        pthread_mutex_unlock(osd_lock + channel);
        return HLE_RET_OK;
    }

    copy_osd_attr(osd_index, old_attr, osd_attr);
    config_osd_region(channel, osd_index, old_attr);
    pthread_mutex_unlock(osd_lock + channel);
    gettimeofday(&tv2, NULL);
    int used = (tv2.tv_sec - tv1.tv_sec)*1000 * 1000 + tv2.tv_usec - tv1.tv_usec;
    DEBUG_LOG("used time %d\n", used);

    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

int encoder_get_roi_num(void)
{
    return ROI_REGION_NUM;
}

static int config_single_roi(int encChn, int roi_index, int enc_w, int enc_h,
                             ENC_ROI_ATTR *roi_attr)
{
    VENC_ROI_CFG_S roi_cfg;
    int ret;
    ret = HI_MPI_VENC_GetRoiCfg(encChn, roi_index, &roi_cfg);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_VENC_GetRoiCfg failed %#x\n", ret);
        return HLE_RET_ERROR;
    }

    roi_cfg.bAbsQp = HI_FALSE; //采用 相对Qp 的设置
    roi_cfg.bEnable = roi_attr->enable;
    roi_cfg.s32Qp = -10 + 5 * roi_attr->level; //5为一个等级，-10为最清晰
    roi_cfg.u32Index = roi_index;
    roi_cfg.stRect.s32X = (roi_attr->left * enc_w / REL_COORD_WIDTH) & (~15);
    roi_cfg.stRect.s32Y = (roi_attr->top * enc_h / REL_COORD_HEIGHT) & (~15);
    roi_cfg.stRect.u32Width =
            ((roi_attr->right - roi_attr->left) * enc_w / REL_COORD_WIDTH) & (~15);
    if (roi_cfg.stRect.u32Width < 16)
        roi_cfg.stRect.u32Width = 16;
    roi_cfg.stRect.u32Height =
            ((roi_attr->bottom - roi_attr->top) * enc_h / REL_COORD_HEIGHT) & (~15);
    if (roi_cfg.stRect.u32Height < 16)
        roi_cfg.stRect.u32Height = 16;
    ret = HI_MPI_VENC_SetRoiCfg(encChn, &roi_cfg);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_VENC_SetRoiCfg failed %#x\n", ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

static int config_roi_region(int channel, int roi_index, ENC_ROI_ATTR *roi_attr)
{
    int i;
    for (i = 0; i < STREAMS_PER_CHN; i++) {
        int encChn = GET_ENC_CHN(channel, i);
        int enc_w, enc_h;
        if (get_encoder_size(encChn, &enc_w, &enc_h) != HLE_RET_OK)
            continue;

        config_single_roi(encChn, roi_index, enc_w, enc_h, roi_attr);
    }
    return HLE_RET_OK;
}

static int venc_update_roi(int channel, int stream_index, int enc_w, int enc_h)
{
    int i;
    int encChn = GET_ENC_CHN(channel, stream_index);

    for (i = 0; i < ROI_REGION_NUM; i++) {
        ENC_ROI_ATTR *roi_attr = &roi_attrs[channel][i];
        config_single_roi(encChn, i, enc_w, enc_h, roi_attr);
    }

    return HLE_RET_OK;
}

int encoder_set_roi(int channel, int roi_index, ENC_ROI_ATTR *roi_attr)
{
    if (channel < 0 || channel >= VI_PORT_NUM ||
        roi_index < 0 || roi_index >= ROI_REGION_NUM || roi_attr == NULL)
        return HLE_RET_EINVAL;

    ENC_ROI_ATTR *old_attr = &roi_attrs[channel][roi_index];
    if (memcmp(roi_attr, old_attr, sizeof (ENC_ROI_ATTR)) != 0) {
        pthread_mutex_lock(roi_lock + channel);
        memcpy(old_attr, roi_attr, sizeof (ENC_ROI_ATTR));
        config_roi_region(channel, roi_index, old_attr);
        pthread_mutex_unlock(roi_lock + channel);
    }

    return HLE_RET_OK;
}

//int vpss_set_chn_size(int chn, int width, int height);

static int __encoder_start(int channel, int stream_index,
                           ENC_STREAM_ATTR *stream_attr, int enc_level)
{
    HLE_S32 ret;
    VENC_CHN encChn = GET_ENC_CHN(channel, stream_index);

//TempDisable
    
    //Create Venc Channel
    int width, height;
    ret = get_image_size(stream_attr->img_size, &width, &height);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("get_image_size fail: %#x!\n", ret);
        return HLE_RET_ERROR;
    }

    VENC_CHN_ATTR_S vencAttr;
    set_vencAttr(stream_attr, enc_level, width, height, &vencAttr);

    printf("\n create encChn = %d \n",encChn);
    ret = HI_MPI_VENC_CreateChn(encChn, &vencAttr);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_VENC_CreateChn (%d) fail: %#x!\n", encChn, ret);
        ERROR_LOG("bitrate %d, framerate %d, img_size %d\n",
                  stream_attr->bitrate, stream_attr->framerate, stream_attr->img_size);
        return HLE_RET_ERROR;
    }
    set_rcparam(encChn);

    //vpss_set_chn_size(encChn + VPSS_CHN_ENC1, width, height);
    //venc_update_osd(channel, stream_index, width, height);
    //venc_update_roi(channel, stream_index, width, height);

    //Start Recv Venc Pictures
    ret = HI_MPI_VENC_StartRecvPic(encChn);
    if (HI_SUCCESS != ret) {
        HI_MPI_VENC_DestroyChn(encChn);
        ERROR_LOG("HI_MPI_VENC_StartRecvPic (%d) fail: %#x!\n", encChn, ret);
        return HLE_RET_ERROR;
    }

    DEBUG_LOG("encoder (%d, %d) start success!\n", channel, stream_index);
    return HLE_RET_OK;
}

static int __encoder_start_jpeg(int channel, ENC_JPEG_ATTR *jpeg_attr)
{
    int ret;

    //Create JPEG Venc Group
    VENC_CHN encChn = GET_JPEG_ENC_CHN(channel);

    MPP_CHN_S src, dst;
    src.enModId = HI_ID_VPSS;
    src.s32DevId = VPSS_GRP_ID;
    src.s32ChnId = VPSS_CHN_JPEG;
    dst.enModId = HI_ID_VENC;
    dst.s32DevId = 0;
    dst.s32ChnId = encChn;
    ret = HI_MPI_SYS_Bind(&src, &dst);
    //DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
                  src.enModId, src.s32DevId, src.s32ChnId,
                  dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
        return HLE_RET_ERROR;
    }

    //Create JPEG Venc Channel
    int width, height;
    ret = get_image_size(jpeg_attr->img_size, &width, &height);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("get_image_size fail: %#x!\n", ret);
        HI_MPI_SYS_UnBind(&src, &dst);
        return HLE_RET_ERROR;
    }

    VENC_CHN_ATTR_S vencAttr;
    set_jpeg_chn_attr(&vencAttr, width, height);
    ret = HI_MPI_VENC_CreateChn(encChn, &vencAttr);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_VENC_CreateChn (%d) fail: %#x!\n", encChn, ret);
        HI_MPI_SYS_UnBind(&src, &dst);
        return HLE_RET_ERROR;
    }

    /*改变图像质量*/
    VENC_PARAM_JPEG_S para_jpeg;
    ret = HI_MPI_VENC_GetJpegParam(encChn, &para_jpeg);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_VENC_GetJpegParam (%d) fail: %#x\n", encChn, ret);
        HI_MPI_VENC_DestroyChn(encChn);
        HI_MPI_SYS_UnBind(&src, &dst);
        return HLE_RET_ERROR;
    }

    //u32Qfactor越大[1::99]，得到的图像质量会更好，编码压缩率更低
    para_jpeg.u32Qfactor = 90 - jpeg_attr->level * 10;
    ret = HI_MPI_VENC_SetJpegParam(encChn, &para_jpeg);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_VENC_GetJpegParam (%d) fail: %#x\n", encChn, ret);
        HI_MPI_VENC_DestroyChn(encChn);
        HI_MPI_SYS_UnBind(&src, &dst);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

static int __encoder_stop(int channel, int stream_index)
{
    HI_S32 ret;

    HLE_S32 encChn = GET_ENC_CHN(channel, stream_index);
    ret = HI_MPI_VENC_StopRecvPic(encChn);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_VENC_StopRecvPic (%d) fail: %#x!\n", encChn, ret);
    }

    ret = HI_MPI_VENC_DestroyChn(encChn);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_VENC_DestroyChn (%d) fail: %#x!\n", encChn, ret);
        return HLE_RET_ERROR;
    }

    DEBUG_LOG("encoder (%d, %d) stop success!\n", channel, stream_index);
    return HLE_RET_OK;
}

static int __encoder_stop_jpeg(int channel)
{
    int encChn = GET_JPEG_ENC_CHN(channel);

    int ret;
    ret = HI_MPI_VENC_DestroyChn(encChn);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_VENC_DestroyChn (%d) fail: %#x!\n", encChn, ret);
        return HLE_RET_ERROR;
    }

    MPP_CHN_S dst;
    dst.enModId = HI_ID_VENC;
    dst.s32DevId = 0;
    dst.s32ChnId = encChn;
    ret = HI_MPI_SYS_UnBind(NULL, &dst);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_SYS_UnBind fail: %#x!\n", ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

int encoder_pause(void)
{
    int i;
    for (i = 0; i < ENC_STREAM_NUM; ++i) {
        if (enc_ctx.encChn[i].state == ENC_STATE_RUNING) {
            HI_MPI_VENC_StopRecvPic(i);
        }
    }
    enc_ctx.running = 0x197521;
}

int encoder_resume(void)
{
    int i;
    for (i = 0; i < ENC_STREAM_NUM; ++i) {
        if (enc_ctx.encChn[i].state == ENC_STATE_RUNING) {
            HI_MPI_VENC_StartRecvPic(i);
        }
    }
    enc_ctx.running = 1;
}

int encoder_start(int channel, int stream_index)
{
    if (channel < 0 || channel >= VI_PORT_NUM
        || stream_index < 0 || stream_index >= STREAMS_PER_CHN) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    int encChn = GET_ENC_CHN(channel, stream_index);
    ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

    pthread_mutex_lock(&chn_ctx->lock);
    pthread_mutex_lock(roi_lock + channel);
    pthread_mutex_lock(osd_lock + channel);
    if (chn_ctx->state == ENC_STATE_IDLE) {
        pthread_mutex_unlock(osd_lock + channel);
        pthread_mutex_unlock(roi_lock + channel);
        pthread_mutex_unlock(&chn_ctx->lock);
        return HLE_RET_ERROR;
    }
    else if (chn_ctx->state == ENC_STATE_CONFIGED) {
        int ret;
        ENC_STREAM_ATTR *stream_attr = &chn_ctx->enc_attr;
        chn_ctx->enc_level = 0;
        ret = __encoder_start(channel, stream_index, stream_attr, chn_ctx->enc_level);
        if (ret != HLE_RET_OK) {
            pthread_mutex_unlock(osd_lock + channel);
            pthread_mutex_unlock(roi_lock + channel);
            pthread_mutex_unlock(&chn_ctx->lock);
            return HLE_RET_ERROR;
        }
        chn_ctx->state = ENC_STATE_RUNING;
    }

    pthread_mutex_unlock(osd_lock + channel);
    pthread_mutex_unlock(roi_lock + channel);
    pthread_mutex_unlock(&chn_ctx->lock);

    gettimeofday(&tv2, NULL);
    int used = (tv2.tv_sec - tv1.tv_sec)*1000 * 1000 + tv2.tv_usec - tv1.tv_usec;
    DEBUG_LOG("used time %d\n", used);
    return HLE_RET_OK;
}

int encoder_stop(int channel, int stream_index)
{
    if (channel < 0 || channel >= VI_PORT_NUM
        || stream_index < 0 || stream_index >= STREAMS_PER_CHN) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    int ret = HLE_RET_OK;
    int encChn = GET_ENC_CHN(channel, stream_index);
    ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;

    pthread_mutex_lock(&chn_ctx->lock);
    pthread_mutex_lock(roi_lock + channel);
    pthread_mutex_lock(osd_lock + channel);
    if (chn_ctx->state == ENC_STATE_RUNING) {
        ret = __encoder_stop(channel, stream_index);
        chn_ctx->state = ENC_STATE_CONFIGED;
    }
    pthread_mutex_unlock(osd_lock + channel);
    pthread_mutex_unlock(roi_lock + channel);
    pthread_mutex_unlock(&chn_ctx->lock);

    return ret;
}

int encoder_config(int channel, int stream_index, ENC_STREAM_ATTR *stream_attr)
{
    if (channel < 0 || channel >= VI_PORT_NUM || stream_index < 0
        || stream_index >= STREAMS_PER_CHN || stream_attr == NULL) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    int ret;
    int encChn = GET_ENC_CHN(channel, stream_index);
    ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;
    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

    pthread_mutex_lock(&chn_ctx->lock);
    pthread_mutex_lock(roi_lock + channel);
    pthread_mutex_lock(osd_lock + channel);
    if (chn_ctx->state != ENC_STATE_RUNING) {
        //没有调用encoder_start，直接更新编码参数，返回成功
        chn_ctx->enc_attr = *stream_attr;
        chn_ctx->state = ENC_STATE_CONFIGED;
        pthread_mutex_unlock(osd_lock + channel);
        pthread_mutex_unlock(roi_lock + channel);
        pthread_mutex_unlock(&chn_ctx->lock);
        return HLE_RET_OK;
    }

    if (chn_ctx->enc_attr.img_size != stream_attr->img_size
        || chn_ctx->enc_attr.enc_standard != stream_attr->enc_standard) {
        /*编码通道静态属性发生改变，需销毁了再创建编码通道*/
        __encoder_stop(channel, stream_index);
        chn_ctx->enc_attr = *stream_attr;
        ret = __encoder_start(channel, stream_index, stream_attr, chn_ctx->enc_level);
        if (ret != 0)
            chn_ctx->state = ENC_STATE_CONFIGED;
        pthread_mutex_unlock(osd_lock + channel);
        pthread_mutex_unlock(roi_lock + channel);
        pthread_mutex_unlock(&chn_ctx->lock);

        gettimeofday(&tv2, NULL);
        int used = (tv2.tv_sec - tv1.tv_sec)*1000 * 1000 + tv2.tv_usec - tv1.tv_usec;
        DEBUG_LOG("used time %d\n", used);
        return ret;
    }

    /*改变动态属性*/
    VENC_CHN_ATTR_S vencAttr;
    HI_MPI_VENC_GetChnAttr(encChn, &vencAttr);
#if 0
    if (stream_attr->bitratectrl == BIT_CTRL_CBR)
        set_h264_cbr_attr(&vencAttr, stream_attr, chn_ctx->enc_level);
    else if (stream_attr->bitratectrl == BIT_CTRL_VBR)
        set_h264_vbr_attr(&vencAttr, stream_attr, chn_ctx->enc_level);
    else
        set_h264_avbr_attr(&vencAttr, stream_attr, chn_ctx->enc_level);
#endif
    set_venc_rc_attr(&vencAttr, stream_attr, chn_ctx->enc_level);
    ret = HI_MPI_VENC_SetChnAttr(encChn, &vencAttr);
    if (ret != HI_SUCCESS) {
        pthread_mutex_unlock(osd_lock + channel);
        pthread_mutex_unlock(roi_lock + channel);
        pthread_mutex_unlock(&chn_ctx->lock);
        ERROR_LOG("HI_MPI_VENC_SetChnAttr (%d) fail: %#x\n", encChn, ret);
        return HLE_RET_ERROR;
    }

    chn_ctx->enc_attr = *stream_attr;
    pthread_mutex_unlock(osd_lock + channel);
    pthread_mutex_unlock(roi_lock + channel);
    pthread_mutex_unlock(&chn_ctx->lock);

    gettimeofday(&tv2, NULL);
    int used = (tv2.tv_sec - tv1.tv_sec)*1000 * 1000 + tv2.tv_usec - tv1.tv_usec;
    DEBUG_LOG("used time %d\n", used);
    return HLE_RET_OK;
}

int encoder_force_iframe(int channel, int stream_index)
{
    if (channel < 0 || channel >= VI_PORT_NUM || stream_index < 0
        || stream_index >= STREAMS_PER_CHN) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    int encChn = GET_ENC_CHN(channel, stream_index);
    ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;

    pthread_mutex_lock(&chn_ctx->lock);
    if (chn_ctx->state != ENC_STATE_RUNING) {
        pthread_mutex_unlock(&chn_ctx->lock);
        return HLE_RET_ENOTINIT;
    }

    int ret = HI_MPI_VENC_RequestIDR(encChn, HI_TRUE);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_VENC_RequestIDR (%d) fail: %#x\n", encChn, ret);
        pthread_mutex_unlock(&chn_ctx->lock);
        return HLE_RET_ERROR;
    }

    DEBUG_LOG("encoder_force_iframe (%d, %d) success\n", channel, stream_index);
    pthread_mutex_unlock(&chn_ctx->lock);
    return HLE_RET_OK;
}

int encoder_set_enc_level(int channel, int stream_index, int level)
{
    if (channel < 0 || channel >= VI_PORT_NUM || stream_index < 0
        || stream_index >= STREAMS_PER_CHN || level < 0 || level >= MAX_BLOCK_LEVEL) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    int encChn = GET_ENC_CHN(channel, stream_index);
    ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;

    pthread_mutex_lock(&chn_ctx->lock);
    if (chn_ctx->state != ENC_STATE_RUNING) {
        pthread_mutex_unlock(&chn_ctx->lock);
        return HLE_RET_ENOTINIT;
    }

    if (chn_ctx->enc_level != level) {
        chn_ctx->enc_level = level;

        /*让level生效*/
        VENC_CHN_ATTR_S vencAttr;
        HI_MPI_VENC_GetChnAttr(encChn, &vencAttr);
#if 0
        if (chn_ctx->enc_attr.bitratectrl == BIT_CTRL_CBR)
            set_h264_cbr_attr(&vencAttr, &chn_ctx->enc_attr, chn_ctx->enc_level);
        else
            set_h264_vbr_attr(&vencAttr, &chn_ctx->enc_attr, chn_ctx->enc_level);
#endif
        set_venc_rc_attr(&vencAttr, &chn_ctx->enc_attr, chn_ctx->enc_level);
        int ret = HI_MPI_VENC_SetChnAttr(encChn, &vencAttr);
        if (ret != HI_SUCCESS) {
            pthread_mutex_unlock(&chn_ctx->lock);
            ERROR_LOG("HI_MPI_VENC_SetChnAttr (%d) fail: %#x\n", encChn, ret);
            return HLE_RET_ERROR;
        }

        DEBUG_LOG("encoder_set_enc_level chn[%d], index[%d], level[%d], success\n",
                  channel, stream_index, level);
    }
    pthread_mutex_unlock(&chn_ctx->lock);
    return HLE_RET_OK;
}

int encoder_request_stream(int channel, int stream_index, int auto_rc)
{
    if (channel < 0 || channel >= VI_PORT_NUM || stream_index < 0
        || stream_index >= STREAMS_PER_CHN) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    int encChn = GET_ENC_CHN(channel, stream_index);
    return sdp_request_queue(encChn, auto_rc);
}

int encoder_free_stream(int stream_id)
{
    return sdp_free_queue(stream_id);
}

ENC_STREAM_PACK *encoder_get_packet(int stream_id)
{
    return sdp_dequeue(stream_id);
}

int encoder_release_packet(ENC_STREAM_PACK *pack)
{
    return spm_dec_pack_ref(pack);
}

int encoder_get_capability(int channel, ENC_CAPABILITY *enc_cap)
{
    if (channel < 0 || channel >= VI_PORT_NUM || enc_cap == NULL) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_ERROR;
    }

    enc_cap->stream_count = 3; //每个通道支持3路码流
    /* 主码流通道的编码能力描述 */
    enc_cap->streams[0].enc_std_mask = ENC_STD_SUPPORT;
    enc_cap->streams[0].img_size_mask = MAIN_STREAM_IMAGE_SIZE_SUPPORT;
    enc_cap->streams[0].max_framerate = 15;
    enc_cap->streams[0].brc_mask = BIT_CTRL_SUPPORT;
    enc_cap->streams[0].max_bitrate = MAX_BIT_RATE;

    /* 次码流通道的编码能力描述 */
    enc_cap->streams[1].enc_std_mask = ENC_STD_SUPPORT;
    enc_cap->streams[1].img_size_mask = MINOR_STREAM_IMAGE_SIZE_SUPPORT;
    enc_cap->streams[1].max_framerate = 15;
    enc_cap->streams[1].brc_mask = BIT_CTRL_SUPPORT;
    enc_cap->streams[1].max_bitrate = MAX_BIT_RATE >> 2;

    /* 第三码流通道的编码能力描述 */
    enc_cap->streams[2].enc_std_mask = ENC_STD_SUPPORT;
    enc_cap->streams[2].img_size_mask = THIRD_STREAM_IMAGE_SIZE_SUPPORT;
    enc_cap->streams[2].max_framerate = 15;
    enc_cap->streams[2].brc_mask = BIT_CTRL_SUPPORT;
    enc_cap->streams[2].max_bitrate = MAX_BIT_RATE >> 3;

    return HLE_RET_OK;
}

int encoder_get_optimized_config(int channel, int stream_index, ENC_STREAM_ATTR *enc_attr)
{
    if ((stream_index >= STREAMS_PER_CHN) || (stream_index < 0) ||
        (channel < 0) || (channel >= VI_PORT_NUM) || (NULL == enc_attr)) {
        ERROR_LOG("encoder_get_optimized_config invalid para!\n");
        return HLE_RET_EINVAL;
    }

    memcpy(enc_attr, &defEncStreamAttr[stream_index], sizeof (*enc_attr));

    return HLE_RET_OK;
}

int encoder_get_h264_supportted_profile(int *profile_mask)
{
    *profile_mask = H264_PF_SUPPORT;
    return HLE_RET_OK;
}

int encoder_get_h265_supportted_profile(int* profile_mask)
{
    *profile_mask = H265_PF_SUPPORT;
    return HLE_RET_OK;
}

int encoder_set_h264_profile(int profile)
{
    if (profile < H264_PF_BL || profile > H264_PF_HP)
        return HLE_RET_EINVAL;

    if ((H264_PF_SUPPORT & (1 << profile)) == 0)
        return HLE_RET_ENOTSUPPORTED;

    h264_profile = profile;

    int i, j;
    for (i = 0; i < VI_PORT_NUM; i++) {
        for (j = 0; j < STREAMS_PER_CHN; j++) {
            int encChn = GET_ENC_CHN(i, j);
            ENC_CHN_CONTEX *chn_ctx = enc_ctx.encChn + encChn;

            pthread_mutex_lock(&chn_ctx->lock);
            pthread_mutex_lock(roi_lock + i);
            pthread_mutex_lock(osd_lock + i);
            if (chn_ctx->state == ENC_STATE_RUNING) {
                /*编码通道静态属性发生改变，需销毁了再创建编码通道*/
                __encoder_stop(i, j);
                int ret = __encoder_start(i, j, &chn_ctx->enc_attr, chn_ctx->enc_level);
                if (ret != 0)
                    chn_ctx->state = ENC_STATE_CONFIGED;
            }
            pthread_mutex_unlock(osd_lock + i);
            pthread_mutex_unlock(roi_lock + i);
            pthread_mutex_unlock(&chn_ctx->lock);
        }
    }

    return HLE_RET_OK;
}

int encoder_capability_jpeg(int channel, JPEG_CAPABILITY *jpeg_cap)
{
    if (channel < 0 || channel >= VI_PORT_NUM || jpeg_cap == NULL) {
        ERROR_LOG("jpeg_encoder_capability invalid para!\n");
        return HLE_RET_ERROR;
    }

    jpeg_cap->img_size_mask = JPEG_IMAGE_SIZE_SUPPORT;

    return HLE_RET_OK;
}

int encoder_config_jpeg(int channel, ENC_JPEG_ATTR *jpeg_attr)
{
    if (channel < 0 || channel >= VI_PORT_NUM || jpeg_attr == NULL) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    pthread_mutex_lock(&jpeg_enc_lock);

    jpeg_chn_attr = *jpeg_attr;
    jpeg_chn_state = ENC_STATE_CONFIGED;

    pthread_mutex_unlock(&jpeg_enc_lock);

    return HLE_RET_OK;
}


#if 0

static char* gen_time_string()
{
    static char buf[20];
    memset(buf, 0, 20);
    time_t cur;
    struct tm dt;
    time(&cur);
    localtime_r(&cur, &dt);
    snprintf(buf, sizeof (buf) - 1, "%04d%02d%02d_%02d%02d%02d",
             dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday,
             dt.tm_hour, dt.tm_min, dt.tm_sec);
    buf[sizeof (buf) - 1] = 0;
    return buf;
}

#define SNAP_SAVE_PATH "/hle/tf"

int save_jpeg_to_file(char *data, int len)
{
    FILE *index_fp = NULL;
    char buf[64];
    memset(buf, 0, 64);
    sprintf(buf, "%s/%s.jpg", SNAP_SAVE_PATH, gen_time_string());
    index_fp = fopen(buf, "w");
    if (NULL == index_fp) {
        ERROR_LOG("open SNAP_SAVE_PATH fail!\n");
        return -1;
    }

    fwrite(data, len, 1, index_fp);
    fclose(index_fp);

    return 0;
}
#endif

char *encoder_request_jpeg(int channel, int *size, int image_size)
{
    ENC_JPEG_ATTR jpeg_attr;
    jpeg_attr.img_size = image_size; //IMAGE_SIZE_1920x1080;
    jpeg_attr.level = 4;
    encoder_config_jpeg(0, &jpeg_attr);

    if (channel < 0 || channel >= VI_PORT_NUM || size == NULL) {
        ERROR_LOG("invalid para!\n");
        return NULL;
    }

    if (jpeg_chn_state == ENC_STATE_IDLE) {
        ERROR_LOG("Config jpeg encoder first!\n");
        return NULL;
    }

    VENC_CHN encChn = GET_JPEG_ENC_CHN(channel);
    pthread_mutex_lock(&jpeg_enc_lock);
    int ret;
    char *jpeg_data = NULL;

    //Start JPEG Recv Venc Pictures
    ret = __encoder_start_jpeg(channel, &jpeg_chn_attr);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("__encoder_start_jpeg (%d) failed \n", encChn);
        pthread_mutex_unlock(&jpeg_enc_lock);
        return NULL;
    }

    pthread_mutex_lock(osd_lock + channel);
    OSD_BITMAP_ATTR osd_attrs_jpeg[VI_PORT_NUM][VI_OSD_NUM];
    memcpy((char *) osd_attrs_jpeg, (char *) osd_attrs, sizeof (osd_attrs));
    ret = create_osd_jpeg(channel, osd_attrs_jpeg[channel]);
    if (HLE_RET_OK != ret) {
        pthread_mutex_unlock(osd_lock + channel);
        goto STOP_ENCODER_JPEG;
    }
    pthread_mutex_unlock(osd_lock + channel);

    ret = HI_MPI_VENC_StartRecvPic(encChn);
    if (HI_SUCCESS != ret) {
        ERROR_LOG("HI_MPI_VENC_StartRecvPic (%d) fail: %#x!\n", encChn, ret);
        goto DESTROY_OSD_JPEG;
    }

    fd_set read_fds;
    FD_ZERO(&read_fds);
    int venc_fd = HI_MPI_VENC_GetFd(encChn);
    if (venc_fd < 0) {
        ERROR_LOG("HI_MPI_VENC_GetFd failed, ret = %#x\n", venc_fd);
        goto DESTROY_OSD_JPEG;
    }
    FD_SET(venc_fd, &read_fds);

    struct timeval time_out;
    time_out.tv_sec = 2;
    time_out.tv_usec = 0;
    ret = select(venc_fd + 1, &read_fds, NULL, NULL, &time_out);
    if (ret == -1) {
        ERROR_LOG("select failed\n");
        goto DESTROY_OSD_JPEG;
    }
    else if (ret == 0) {
        ERROR_LOG("select time out, exit thread\n");
        goto STOP_RECE_PIC;
    }

    if (FD_ISSET(venc_fd, &read_fds)) {
        VENC_CHN_STAT_S stat;
        ret = HI_MPI_VENC_Query(encChn, &stat);
        if (HI_SUCCESS != ret) {
            ERROR_LOG("HI_MPI_VENC_Query chn[%d] fail: %#x!\n", encChn, ret);
            goto DESTROY_OSD_JPEG;
        }

        VENC_STREAM_S venc_stream = {0};
        venc_stream.pstPack = (VENC_PACK_S*) malloc(sizeof (VENC_PACK_S) * stat.u32CurPacks);
        venc_stream.u32PackCount = stat.u32CurPacks;
        ret = HI_MPI_VENC_GetStream(encChn, &venc_stream, HI_FALSE);
        if (HI_SUCCESS != ret) {
            ERROR_LOG("HI_MPI_VENC_GetStream fail: %#x!\n", ret);
            free(venc_stream.pstPack);
            goto DESTROY_OSD_JPEG;
        }

        int frame_data_len = get_stream_data_len(&venc_stream);
        jpeg_data = (char *) malloc(frame_data_len);
        if (NULL == jpeg_data) {
            ERROR_LOG("malloc jpeg data failed !\n");
            free(venc_stream.pstPack);
            goto DESTROY_OSD_JPEG;
        }

        //copy stream
        int i;
        char *jpeg_data_tmp = jpeg_data;
        for (i = 0; i < venc_stream.u32PackCount; i++) {
            memcpy(jpeg_data_tmp, venc_stream.pstPack[i].pu8Addr,
                   venc_stream.pstPack[i].u32Len);
            jpeg_data_tmp += venc_stream.pstPack[i].u32Len;
        }

        HI_MPI_VENC_ReleaseStream(encChn, &venc_stream);
        *size = frame_data_len;
        free(venc_stream.pstPack);
    }

STOP_RECE_PIC:

    ret = HI_MPI_VENC_StopRecvPic(encChn);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_VENC_StopRecvPic fail: %#x!\n", ret);
        free(jpeg_data);
        jpeg_data = NULL;
        goto DESTROY_OSD_JPEG;
    }

DESTROY_OSD_JPEG:
    destroy_osd_jpeg(channel, osd_attrs_jpeg[channel]);
STOP_ENCODER_JPEG:
    __encoder_stop_jpeg(channel);

    pthread_mutex_unlock(&jpeg_enc_lock);

    return jpeg_data;
}

void encoder_free_jpeg(char *jpeg_data)
{
    free(jpeg_data);
}

//pack_count:预分配的码流包个数
int hal_encoder_init(HLE_S32 pack_count)
{
    int i, j;
    HLE_S32 ret;

    spm_init(pack_count); //pack count may should asigned by app layer
    sdp_init(pack_count);

    //TempDisable zk_init();
    scaler_init();

    if (enc_ctx.running == 1) {
        return HLE_RET_OK;
    }

    for (i = 0; i < VI_PORT_NUM; i++) {
        for (j = 0; j < STREAMS_PER_CHN; j++) {
            //创建编码通道组，并绑定到VI physical channel
            
            int encChn = GET_ENC_CHN(i, j);

            MPP_CHN_S src, dst;
            src.enModId = HI_ID_VPSS;
            src.s32DevId = VPSS_GRP_ID;
            src.s32ChnId = VPSS_CHN_ENC0 + j;
            dst.enModId = HI_ID_VENC;
            dst.s32DevId = 0;
            dst.s32ChnId = encChn;
            ret = HI_MPI_SYS_Bind(&src, &dst);
            DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
            if (HI_SUCCESS != ret) {
                ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
                          src.enModId, src.s32DevId, src.s32ChnId,
                          dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
                return HLE_RET_ERROR;
            }

            enc_ctx.encChn[encChn].state = ENC_STATE_IDLE;
            pthread_mutex_init(&enc_ctx.encChn[encChn].lock, NULL);
        }

        jpeg_chn_state = ENC_STATE_IDLE;
        pthread_mutex_init(&jpeg_enc_lock, NULL);

        pthread_mutex_init(osd_lock + i, NULL);
        pthread_mutex_init(roi_lock + i, NULL);
    }

    enc_ctx.running = 1;
   
  /*
    //视频音频采集用同一线程
    if (pthread_create(&enc_ctx.venc_pid, 0, enc_get_stream_proc, NULL)) 
    {
        enc_ctx.running = 0;
        return HLE_RET_ERROR;
    }    
  */
 #if 1 //视频音频采集用两个线程
     if (pthread_create(&enc_ctx.venc_pid, 0, venc_get_stream_proc, NULL)) 
    {
        enc_ctx.running = 0;
        ERROR_LOG("pthread_create venc_get_stream_proc failed!\n");
        return HLE_RET_ERROR;
    }

    #if USE_HISI_AENC
        if (pthread_create(&enc_ctx.Aenc_pid, 0, Aenc_get_stream_proc, NULL)) 
        {
            enc_ctx.running = 0;
            ERROR_LOG("pthread_create Aenc_get_stream_proc failed!\n");
            return HLE_RET_ERROR;
        }
    #else
        #if 1
         if (pthread_create(&enc_ctx.Aenc_pid, 0, AAC_get_stream_proc, NULL)) 
        {
            enc_ctx.running = 0;
            ERROR_LOG("pthread_create AAC_get_stream_proc failed!\n");
            return HLE_RET_ERROR;
        }
        #endif
    #endif
    

    
  #endif

#if 0
    pthread_create(&enc_ctx.rate_osd_pid, 0, rate_osd_proc, NULL);
    pthread_create(&enc_ctx.time_osd_pid, 0, time_osd_proc, NULL);
#endif
    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

void hal_encoder_exit(void)
{
    if (enc_ctx.running == 0)
        return;

    enc_ctx.running = 0;
    pthread_join(enc_ctx.venc_pid, NULL);
    pthread_join(enc_ctx.Aenc_pid, NULL);
    pthread_join(enc_ctx.time_osd_pid, NULL);
    pthread_join(enc_ctx.rate_osd_pid, NULL);


    int i, j;
    for (i = 0; i < VI_PORT_NUM; i++) {
        for (j = 0; j < STREAMS_PER_CHN; j++) {
            int encChn = GET_ENC_CHN(i, j);
            pthread_mutex_destroy(&enc_ctx.encChn[encChn].lock);
        }

        pthread_mutex_destroy(&jpeg_enc_lock);

        pthread_mutex_destroy(osd_lock + i);
        pthread_mutex_destroy(roi_lock + i);
    }
}

















