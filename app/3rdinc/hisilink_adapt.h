/******************************************************************************
  Copyright (C), 2004-2050, Hisilicon Tech. Co., Ltd.
******************************************************************************
  File Name     : hisilink_adapt.h
  Version       : Initial Draft
  Author        : Hisilicon WiFi software group
  Created       : 2016-06-06
  Last Modified :
  Description   : the API of hisilink for user calls
  Function List :
  History       :
  1.Date        : 2016-06-06
  Author        :
  Modification  : Created file
******************************************************************************/
#ifndef __HISILINK_ADAPT_H__
#define __HISILINK_ADAPT_H__
#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif
#endif
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "hisilink_lib.h"
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HSL_CHANNEL_NUM     18
#define HSL_RSSI_LEVEL      -100 //hsl处理组播包的RSSI下限值
typedef void (*hsl_connect_cb)(hsl_result_stru*);
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    RECEIVE_FLAG_OFF,
    RECEIVE_FLAG_ON,
    RECEIVE_FLAG_BUTT
}recevie_flag_enum;
/*****************************************************************************
  4 全局变量声明
*****************************************************************************/

/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct
{
    hsl_uint32                ul_taskid1;       //HSL config非传统连接处理线程的ID
    hsl_uint32                ul_taskid2;       //传统连接方式处理线程ID
    hsl_uint16                us_timerout_id;   //HSL config超时定时器
    hsl_uint16                us_timer_id;     //周期切信道的定时器ID
    hsl_uint8                 en_status;       //HSL config当前处的状态
    hsl_uint8                 en_connect;      //当前连接状态
    hsl_uint8                 auc_res[2];
    hsl_uint32                ul_mux_lock;     //保护锁
}hisi_hsl_status_stru;

typedef struct hsl_datalist
{
    struct hsl_datalist *next;
    void *data;
    unsigned int length;
}hsl_data_list_stru;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
hsl_int32 hisi_hsl_adapt_init(void);
hsl_int32 hisi_hsl_adapt_deinit(void);
hsl_int32 hisi_hsl_change_channel(void);
hsl_int32 hisi_hsl_process_data(void);
hsl_int32 hisi_hsl_online_notice(hsl_result_stru *pst_result);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif //__HSLCONFIG_ADAPT_H__

