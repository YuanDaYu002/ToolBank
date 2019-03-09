#ifndef HAL_MOTION_DETECT_H
#define HAL_MOTION_DETECT_H

#include "typeport.h"


#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_MD_AREA_NUM     4

//移动侦测属性

typedef struct
{
    HLE_U8 enable; //1--enable, 0--disable
    HLE_U8 level; //[0, 4], 0 is most sensitive
    HLE_U8 reserved1[2];
    //HLE_RECT rect[MAX_MD_AREA_NUM];  /*REL_COORD_WIDTH坐标系*/
} MOTION_DETECT_ATTR;


/*
    function:  motion_detect_config
    description:  配置视频移动侦测属性接口，属性包括使能、灵敏度和检测区域
    args:
        int channel[in]，视频输入通道号
        MOTION_DETECT_ATTR *attr[in]，移动侦测属性
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int motion_detect_config(int chn, MOTION_DETECT_ATTR *attr);

/*
    function:  motion_detect_get_state
    description:  获取移动侦测结果
    args:
        int *state[out]，结果存储在state指向的存储空间中，值是各个通道检测状态的掩码。
                        低通道在低位，高通道在高位。移动置1，反之置0，不存在的通道置0。
    return:
        0, 成功
        <0, 失败，返回值为错误码，具体见错误码定义
 */
int motion_detect_get_state(int *state);
int motion_detect_write_cfg(void);
int motion_detect_init(void);
void motion_detect_exit(void);

#ifdef __cplusplus
}
#endif

#endif

