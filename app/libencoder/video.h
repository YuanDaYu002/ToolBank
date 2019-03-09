#ifndef HAL_VIDEO_H
#define HAL_VIDEO_H

#include "typeport.h"


#ifdef __cplusplus
extern "C"
{
#endif


//摄像机所支持的调节功能掩码
#define CAMERA_CAPS_EXPOSURE    (1<<0)  //曝光调节
#define CAMERA_CAPS_WB          (1<<1)  //白平衡调节
#define CAMERA_CAPS_FOCUS       (1<<2)  //对焦调节
#define CAMERA_CAPS_IRIS        (1<<3)  //光圈调节
#define CAMERA_CAPS_DENOISE     (1<<4)  //降噪调节
#define CAMERA_CAPS_DRC         (1<<5)  //宽动态范围调节
#define CAMERA_CAPS_MIRROR_FLIP (1<<6)  //镜像和垂直翻转调节
#define CAMERA_CAPS_COLOR       (1<<7)  //图像亮度、对比度、饱和度等调整
#define CAMERA_CAPS_IRCUTTER    (1<<8)  //红外滤光片切换控制


//通道能力集描述结构体

typedef struct
{
    HLE_U32 width; /*输入图像宽度*/
    HLE_U32 height; /*输入图像高度*/
    HLE_U32 caps_mask; /*每bit代表一种调节功能，具体详见宏定义*/
    HLE_U32 osd_count; /*最大支持叠加的OSD数量*/
    HLE_U32 cover_count; /*最大支持叠加的屏蔽块数量*/
} VIDEOIN_CAP;

//图像颜色属性结构体

typedef struct
{
    HLE_U8 brightness; //亮度，0-100
    HLE_U8 contrast; //对比度，0-100
    HLE_U8 saturation; //饱和度，0-100
    HLE_U8 hue; //色调，0-100
} VIDEO_COLOR_ATTR;

typedef struct
{
    HLE_U32 enable; // 是否显示。0-不显示，其他参数忽略，1-显示
    HLE_U16 x; //x, y 坐标为相对REL_COORD_WIDTH X REL_COORD_HEIGHT坐标系大小
    HLE_U16 y;
    HLE_U16 width; //OSD点阵宽度，必须为8对其
    HLE_U16 height; //OSD点阵高度
    HLE_U32 fg_color; //前景色，16进制表示为0xAARRGGBB，A(Alpha)表示透明度。
    HLE_U32 bg_color; //背景色，同样是ARGB8888格式

    //需要叠加的OSD单色点阵数据。0表示该点无效，1表示该点有效。每个字节包含8个象素的数据，
    //最左边的象素在最高位，紧接的下一个字节对应右边8个象素，直到一行结束，接下来是下一行象素的点阵数据。
    HLE_U8 *raster;
} OSD_BITMAP_ATTR;

#define MAX_VIDEO_COVER_NUM 4

typedef struct
{
    HLE_U32 enable; //是否显示。0-不显示，其他参数忽略，1-显示
    HLE_U32 color; //被覆盖区域显示的颜色。16进制表示为0x00RRGGBB
    HLE_U16 left; //矩形区域坐标为相对REL_COORD_WIDTH X REL_COORD_HEIGHT坐标系大小
    HLE_U16 top;
    HLE_U16 right;
    HLE_U16 bottom;
} VIDEO_COVER_ATTR;

//曝光属性

typedef struct
{
    HLE_U8 mode; //0-自动曝光，1-手动曝光
    HLE_U8 a_gain; //手动模式下为模拟增益，自动曝光时为最大模拟增益，范围[1-8]
    HLE_U8 d_gain; //手动模式下为数字增益，自动曝光时为最大数字增益，范围[1-8]
    HLE_U8 reserved1; //预留
    HLE_U32 exposure_time; //手动模式下为曝光时间，自动模式下表示最大曝光时间，范围[100-40000]us
} CAMERA_EXPOSURE_ATTR;

//白平衡属性

typedef struct
{
    HLE_U8 mode; //0-自动平衡，1-手动平衡
    HLE_U8 r_gain; //手动模式下的白平衡R增益
    HLE_U8 b_gain; //手动模式下的白平衡B增益
    HLE_U8 reserved1[5]; //预留
} CAMERA_WB_ATTR;

//对焦属性

typedef struct
{
    HLE_U8 mode; //0-自动对焦，1-手动对焦
    HLE_U8 reserved1[3]; //预留
} CAMERA_FOCUS_ATTR;

//降噪属性

typedef struct
{
    HLE_U8 enable; //0-关闭，1-开启
    HLE_U8 level; //降噪级别，0-0xFF，值越大则对噪点的抑制越大
    HLE_U8 reserved1[2]; //预留
} CAMERA_DENOISE_ATTR;

//动态范围控制属性

typedef struct
{
    HLE_U8 enable; //0-关闭，1-开启
    HLE_U8 level; //强度级别，0-0xFF，值越大则暗区越亮，噪声也会随之增大
    HLE_U8 reserved1[2]; //预留
} CAMERA_DRC_ATTR;

typedef enum
{
    CAM_NONE,
    CAM_50HZ,
    CAM_60HZ,
} CAMERA_ANTIFLICK_MODE;

/*
    function:  videoin_get_chns
    description:  获取视频输入通道数
    args:
        无
    return:
        >=0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int videoin_get_chns(void);

/*
    function:  videoin_get_capability
    description:  获取视频输入通道能力信息
    args:
        VIDEOIN_CAP *vi_cap [out]，视频输入通道能力信息结构体指针
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int videoin_get_capability(VIDEOIN_CAP *vi_cap);

/*
    function:  videoin_set_osd
    description:  设置视频OSD
    args:
        int channel[in]，视频输入通道号
        int index[in]，OSD序号，序号0对应的是时间OSD。时间OSD点阵由底层内部产生，
                        OSD_BITMAP_ATTR中的width height raster参数被忽略
        OSD_BITMAP_ATTR *osd_attr[in]，OSD属性结构体指针
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int videoin_set_osd(int channel, int index, OSD_BITMAP_ATTR *osd_attr);

/*
    function:  videoin_set_cover
    description:  设置视频隐私屏蔽块
    args:
        int channel[in]，视频输入通道号
        int index[in]，屏蔽块号
        OSD_BITMAP_ATTR *cover_attr[in]，屏蔽块属性结构体指针
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int videoin_set_cover(int channel, int index, VIDEO_COVER_ATTR *cover_attr);

/*
    function:  camera_set_color
    description:  设置视频输入通道颜色属性（亮度、对比度、饱和度、色度）
    args:
        int channel[in]，视频输入通道号
        VIDEO_COLOR_ATTR *color[in]，视频输入通道颜色属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_color(int channel, VIDEO_COLOR_ATTR *color);

/*
    function:  camera_set_exposure_attr
    description:  设置摄像机曝光控制属性
    args:
        int channel[in]，视频输入通道号
        CAMERA_EXPOSURE_ATTR *attr[in]，曝光控制属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_exposure_attr(int channel, CAMERA_EXPOSURE_ATTR *attr);

/*
    function:  camera_set_exposure
    description:  设置摄像机白平衡控制属性
    args:
        int channel[in]，视频输入通道号
        CAMERA_WB_ATTR *attr[in]，白平衡控制属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_wb_attr(int channel, CAMERA_WB_ATTR *attr);

/*
    function:  camera_set_focus_attr
    description:  设置摄像机对焦控制属性
    args:
        int channel[in]，视频输入通道号
        CAMERA_FOCUS_ATTR *attr[in]，对焦控制属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_focus_attr(int channel, CAMERA_FOCUS_ATTR *attr);

/*
    function:  camera_set_iris_mode
    description:  设置摄像机光圈控制模式
    args:
        int channel[in]，视频输入通道号
        int mode[in]，光圈控制模式,0-自动光圈，1-手动光圈
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_iris_mode(int channel, int mode);

/*
    function:  camera_set_denoise_attr
    description:  设置摄像机降噪属性
    args:
        int channel[in]，视频输入通道号
        CAMERA_DENOISE_ATTR *attr[in]，降噪属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_denoise_attr(int channel, CAMERA_DENOISE_ATTR *attr);

/*
    function:  camera_set_drc_attr
    description:  设置摄像机动态范围控制属性
    args:
        int channel[in]，视频输入通道号
        CAMERA_DRC_ATTR *attr[in]，动态范围控属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_drc_attr(int channel, CAMERA_DRC_ATTR *attr);

/*
    function:  camera_set_mirror_flip
    description:  设置图像是否水平翻转和是否垂直翻转
    args:
           int channel[in]，视频输入通道号
        int mirror[in]，是否水平翻转 1:是； 0:否
        int flip[in]，是否垂直翻转 1:是； 0:否
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_mirror_flip(int channel, int mirror, int flip);

/*
    function:  camera_set_irc_mode
    description: 设置滤光片切换模式
    args:
           int channel[in]，视频输入通道号
        int mode[in],滤光片切换的方式 0:自动；1:open(Let IR in)；2:close(keep out of IR)
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int camera_set_irc_mode(int channel, int mode);

/*
    function:  camera_get_irc_status
    description: 获取IRC状态
    args:
                无
    return:
        0, 白天(红外光被挡在外面，红外灯关掉)
        1, 晚上(红外光透视进来，红外灯打开)
 */
int camera_get_irc_status(void);

/*
    function:  hal_correct_defect_pixel
    description: 进行坏点检测
    args:
        无
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int hal_correct_defect_pixel(void);

/*
    function:  hal_get_dpc_time
    description: 获取上次坏点检测的时间
    args:
        HLE_SYS_TIME *dpc_time[out]，上一次坏点检测的时间
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int hal_get_dpc_time(HLE_SYS_TIME *dpc_time);

#ifdef __cplusplus
}
#endif

#endif






