
#ifndef HAL_ENCODER_H
#define HAL_ENCODER_H

#include "typeport.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define MAX_STREAM_PER_CHN  4

//编码标准枚举

typedef enum
{
    VENC_STD_H264,
    VENC_STD_JEPG,
    VENC_STD_H265,

    VENC_STD_NR,
} E_VENC_STANDARD;

// 分辨率枚举

typedef enum
{
    IMAGE_SIZE_1920x1080,
    IMAGE_SIZE_960x544,
    IMAGE_SIZE_480x272,

    IMAGE_SIZE_NR
} E_IMAGE_SIZE;

typedef enum
{
    AENC_STD_G711A,
    AENC_STD_ADPCM,
    //AENC_STD_MP3,

    AENC_STD_NR,
} E_AENC_STANDARD;

typedef enum
{
    AUDIO_SR_8000, /* 8K samplerate*/
    AUDIO_SR_16000,
    AUDIO_SR_44100,

    AUDIO_SR_NR,
} E_AUDIO_SAMPLE_RATE;

typedef enum
{
    AUDIO_BW_16, /* 16bit width*/

    AUDIO_BW_NR,
} E_AUDIO_BIT_WIDTH;

typedef enum
{
    AUDIO_SM_MONO, /*mono*/

    AUDIO_SM_NR,
} E_AUDIO_SOUND_MODE;


//码流能力集描述结构体

typedef struct
{
    HLE_U32 enc_std_mask; /*所支持的编码标准，位图掩码: 每个bit代表一种编码标准，具体见E_VENC_STANDARD*/
    HLE_U32 img_size_mask; /*所支持的编码分辨率，位图掩码: 每个bit代表一种图像分辨率，具体见E_IMAGE_SIZE*/
    HLE_U32 max_framerate; /*编码支持的最大帧率*/
    HLE_U32 brc_mask; /*编码支持的码率控制方式，位图掩码：bit0-CBR, bit1-VBR，对应的位为1表示支持*/
    HLE_U32 max_bitrate; /*最大码率，单位Kbps*/
} STREAM_CAP;

//编码能力集描述结构体

typedef struct
{
    int stream_count; /*支持的码流数目，最大MAX_STREAM_PER_CHN个码流*/
    STREAM_CAP streams[MAX_STREAM_PER_CHN];
} ENC_CAPABILITY;


//编码属性信息结构体

typedef struct
{
    HLE_U8 enc_standard; /*编码标准，具体取值见E_VENC_STANDARD*/
    HLE_U8 img_size; /*图像大小，具体取值见E_IMAGE_SIZE定义*/
    HLE_U8 framerate; /*帧率设置，1-30*/
    HLE_U8 bitratectrl; /*码流类型，CBR为0，VBR为1*/
    HLE_U8 quality; /*画质设置[0, 4]，最清晰为0，最差为4*/
    HLE_U8 hasaudio; /*是否包含音频，0不包含、1为包含音频*/
    HLE_U16 bitrate; /*码流设置，单位Kbps*/
} ENC_STREAM_ATTR;


/*
码流帧格式:
    视频关键帧: FRAME_HDR + IFRAME_INFO + DATA
    视频非关键帧: FRAME_HDR + PFRAME_INFO + DATA
    音频帧: FRAME_HDR + AFRAME_INFO + DATA
关于帧类型和帧同步标志选取:
    前三个字节帧头同步码和H.264 NALU分割码相同，均为0x00 0x00 0x01
    第四个字节使用了H.264中不会使用到的0xF8-0xFF范围
 */


//音视频帧头标志

typedef struct
{
    HLE_U8 sync_code[3]; /*帧头同步码，固定为0x00,0x00,0x01*/
    HLE_U8 type; /*帧类型, 0xF8-视频关键帧，0xF9-视频非关键帧，0xFA-音频帧*/
} FRAME_HDR;

//关键帧描述信息

typedef struct
{
    HLE_U8 enc_std; //编码标准，具体见E_VENC_STANDARD
    HLE_U8 framerate; //帧率
    HLE_U16 reserved;
    HLE_U16 pic_width;
    HLE_U16 pic_height;
    HLE_SYS_TIME rtc_time; //当前帧时间戳，精确到秒，非关键帧时间戳需根据帧率来计算
    HLE_U32 length;
    HLE_U64 pts_msec; //毫秒级时间戳，一直累加，溢出后自动回绕
} IFRAME_INFO;

//非关键帧描述信息

typedef struct
{
    HLE_U64 pts_msec; //毫秒级时间戳，一直累加，溢出后自动回绕
    HLE_U32 length;
} PFRAME_INFO;

//音频帧描述信息

typedef struct
{
    HLE_U8 enc_type; //音频编码类型，具体见E_AENC_STANDARD
    HLE_U8 sample_rate; //采样频率，具体见E_AUDIO_SAMPLE_RATE
    HLE_U8 bit_width; //采样位宽，具体见E_AUDIO_BIT_WIDTH
    HLE_U8 sound_mode; //单声道还是立体声，具体见E_AUDIO_SOUND_MODE
    HLE_U32 length;
    HLE_U64 pts_msec;
} AFRAME_INFO;

//码流包描述结构体，包内有且只有一帧数据

typedef struct _tag_ENC_STREAM_PACK
{
    int channel; //通道号
    int stream_index; //码流编号
    int length; //包内有效数据长度
    HLE_U8 *data; //包缓冲首地址
} ENC_STREAM_PACK;

#define MAX_ROI_REGION_NUM  4

typedef struct
{
    HLE_U32 enable; //是否使能。0-不使能，其他参数忽略，1-使能
    HLE_S32 level; //编码roi区域的清晰度等级[0, 4], 0为最清晰
    HLE_U16 left; //矩形区域坐标为相对REL_COORD_WIDTH X REL_COORD_HEIGHT坐标系大小
    HLE_U16 top;
    HLE_U16 right;
    HLE_U16 bottom;
} ENC_ROI_ATTR;

//JPEG 编码描述结构体

typedef struct
{
    HLE_U32 img_size; //图像大小，具体取值见E_IMAGE_SIZE定义
    HLE_U32 level; //图片清晰等级[0, 4],0为最清晰
} ENC_JPEG_ATTR;

//JPEG 能力描述结构体

typedef struct
{
    HLE_U32 img_size_mask; //所支持的编码分辨率，位图掩码: 每个bit代表一种图像分辨率，具体见E_IMAGE_SIZE
} JPEG_CAPABILITY;

/*
    function:  get_image_size
    description:  根据分辨率枚举值获取图像大小
    args:
        E_IMAGE_SIZE image_size[in]，分辨率枚举值
        int *width[out]，图像大小的宽度
        int *height[out]，图像大小的高度
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int get_image_size(E_IMAGE_SIZE image_size, int *width, int *height);

/*
    function:  encoder_get_capability
    description:  获取编码能力接口
    args:
        int channel[in]，视频编码通道号
        ENC_CAPABILITY *enc_cap [out]，编码能力描述结构体
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_get_capability(int channel, ENC_CAPABILITY *enc_cap);

/*
    function:  encoder_get_optimized_config
    description:  获取编码器推荐属性接口，用于确认编码器在哪种编码属性下效果最好
    args:
        int channel[in]，通道号
        int stream_index[in]，码流索引，0为主码流，1为第二码流，2为第三码流
        ENC_STREAM_ATTR *enc_attr[in]，编码属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_get_optimized_config(int channel, int stream_index, ENC_STREAM_ATTR *enc_attr);

/*
    function:  encoder_get_h264_supportted_profile
    description:  获取设备支持的H.264编码profile类型
    args:
        int *profile_mask[out]，编码PROFILE bitmap，bit0: baseline; bit1:MP; bit2:HP, 对应的bit为1代表支持
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_get_h264_supportted_profile(int *profile_mask);

/*
    function:  encoder_set_h264_profile
    description:  设置H.264编码profile，对所有编码通道生效
    args:
        int profile[in]，编码PROFILE，0: baseline; 1:MP; 2:HP
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_set_h264_profile(int profile);

/*
    function:  encoder_get_roi_num
    description:  获取可配置roi 区域的个数
    args:
        无
    return:
        >=0, 可配置roi 区域的个数
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_get_roi_num(void);

/*
    function:  encoder_set_roi
    description:  设置ROI属性
    args:
        int channel[in]，视频输入通道号
        int roi_index[in]，roi 索引号
        ENC_ROI_ATTR *roi_attr[in]，ROI属性结构体指针
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_set_roi(int channel, int roi_index, ENC_ROI_ATTR *roi_attr);

/*
    function:  encoder_config
    description:  编码器属性配置接口，编码开始后使用此函数来改变编码属性
    args:
        int channel[in]，通道号
        int stream_index[in]，码流索引，0为主码流，1为第二码流，2为第三码流
        ENC_STREAM_ATTR *stream_attr[in]，编码属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_config(int channel, int stream_index, ENC_STREAM_ATTR *stream_attr);

int encoder_pause(void);
int encoder_resume(void);

/*
    function:  encoder_force_iframe
    description:  编码器强制I帧接口，收到该请求后编码器在尽可能短的时间内编出I帧
    args:
        int channel[in]，通道号
        int stream_index[in]，码流索引，0为主码流，1为第二码流，2为第三码流
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_force_iframe(int channel, int stream_index);

#define MAX_BLOCK_LEVEL     6       /*拥塞等级*/
/*
    function:  encoder_request_stream
    description: 请求码流接口，请求成功后可通过stream_get_packet来获取码流包
    args:
        int channel[in]，通道号
        int stream_index[in]，码流索引，0为主码流，1为第二码流，2为第三码流
        int auto_rc[in]，是否影响自动码率控制，1---影响，0---不影响
    return:
        >=0, 成功，申请到的码流编号
        -1, 失败
 */
int encoder_request_stream(int channel, int stream_index, int auto_rc);


/*
    function:  encoder_free_stream
    description:  释放码流接口，调用必须与encoder_request_stream一一对应
    args:
        int stream_id[in]，encoder_request_stream返回的码流编号
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_free_stream(int stream_id);

/*
    function:  encoder_if_have_audio
    description:  设置是否将Audio帧放入对应缓存队列
    args:
        int stream_id， 码流编号
        char have_audio，0：无audio     1:有Audio
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */

int encoder_if_have_audio(int stream_id,char have_audio);


/*
    function:  encoder_get_packet
    description:  获取编码数据包接口，包内有且只有一帧数据，具体格式见FRAME_HDR说明
    args:
        int stream_id[in]，encoder_request_stream返回的码流编号
    return:
        non-NULL, 成功，返回ENC_STREAM_PACK指针
        NULL, 失败
 */
ENC_STREAM_PACK *encoder_get_packet(int stream_id);



/*
    function:  encoder_release_packet
    description:  释放编码数据包接口，调用必须与encoder_get_packet一一对应
    args:
        ENC_STREAM_PACK *pack[in]，编码数据报描述结构体
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_release_packet(ENC_STREAM_PACK *pack);

/*
    function:  encoder_capability_jpeg
    description:  获取JPEG编码能力接口
    args:
        int channel[in]，JPEG编码通道号
        JPEG_CAPABILITY *enc_cap [out]，JPEG编码能力描述结构体
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_capability_jpeg(int channel, JPEG_CAPABILITY *jpeg_cap);

/*
    function:  encoder_config_jpeg
    description:  JPEG编码器属性配置接口，编码开始前使用此函数来配置属性
    args:
        int channel[in]，通道号
        ENC_JPEG_ATTR *jpeg_attr[in]，JPEG编码属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int encoder_config_jpeg(int channel, ENC_JPEG_ATTR *jpeg_attr);

/*
    function:  encoder_request_jpeg
    description: 请求JPEG 数据接口
    args:
        int channel[in]，通道号
        int *size[out]，获取的JPEG 数据的大小
    return:
        =NULL, 请求JPEG 失败
        !=NULL, 请求的JPEG数据的地址
 */
char *encoder_request_jpeg(int channel, int *size, int image_size);

/*
    function:  encoder_free_jpeg
    description: 释放JPEG 数据接口
    args:
        char *jpeg_data[int]，获取的JPEG 数据地址的指针
    return:
        无
 */
void encoder_free_jpeg(char *jpeg_data);

#ifdef __cplusplus
}
#endif

#endif





