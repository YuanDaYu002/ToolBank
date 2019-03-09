

#ifndef HAL_DEF_H
#define HAL_DEF_H



#ifdef __cplusplus
extern "C"
{
#endif


/*这个文件定义和硬件相关的宏*/

/*****************************GLOBAL*****************************/


/*****************************VIDEO*****************************/
#define VI_PORT_NUM         1

#define VI_DEV_ID           0
#define VI_PHY_CHN          0
//#define VI_CHN_VDA          1
//#define VI_PHYCHN_NUM   1

#define VPSS_GRP_ID         0

#define VPSS_CHN_ENC0       0
#define VPSS_CHN_ENC1       1
#define VPSS_CHN_ENC2       2
#define VPSS_CHN_JPEG       3
#define VPSS_CHN_MD         4
//#define VPSS_CHN_OD         5

#if 0
#define VPSS_GRP_VDA 1
#define VPSS_CHN_MD  0
#define VPSS_CHN_OD  1
#endif

#define VI_COVER_NUM    4
#define VI_OSD_NUM      3

#define TIME_OSD_INDEX  0
#define USER_OSD_INDEX 1
#define RATE_OSD_INDEX 2

/*****************************AUDIO*****************************/
//#define AI_CHN_NUM      1

#define AI_DEVICE_ID    0
#define TALK_AI_CHN     0
#define TALK_AENC_CHN   0

#define AO_DEVICE_ID    0
#define TALK_AO_CHN     0
#define TALK_ADEC_CHN   0

#if USE_HISI_AENC
#define AUDIO_PTNUMPERFRM       160
#else
#define AUDIO_PTNUMPERFRM       480 //160  320  480
#endif

/******************************ENC*******************************/
#define STREAMS_PER_CHN     2   //每个通道的码流数目
#define ENC_STREAM_NUM      (VI_PORT_NUM*STREAMS_PER_CHN)    //设备编码通道数

#define JPEGS_PER_CHN       1

#define ENC_NUM_PER_CHN     (JPEGS_PER_CHN + STREAMS_PER_CHN)

#define AI_AENC_CHN         1   //音频编码通道数

#define NTSC_MAX_FRAME_RATE         30
#define PAL_MAX_FRAME_RATE          25

#define ROI_REGION_NUM              4

/***************************ALARM*******************************/
#define ALARMIN_CHN_NUM             1
#define ALARMOUT_CHN_NUM            1


/***************************DETECT*******************************/
#define VDA_CHN_ID_MD  1
#define VDA_CHN_ID_OD  0


/***************************SERIAL*******************************/
#define MAX_SERIAL_PORT_NUM  1

#define SERIAL_PORT_FOR_485  0

//相对坐标系宽高定义
#define REL_COORD_WIDTH         (8192)      //X坐标范围[0,8192)
#define REL_COORD_HEIGHT        (8192)      //Y坐标范围[0,8192)


#ifdef __cplusplus
}
#endif


#endif

