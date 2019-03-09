/******************************************************************************
  Copyright (C), 2004-2050, Hisilicon Tech. Co., Ltd.
******************************************************************************
  File Name     : driver_hisi_lib_api.h
  Version       : Initial Draft
  Author        : Hisilicon WiFi software group
  Created       : 2017-01-06
  Last Modified :
  Description   : API for user calls
  Function List :
  History       :
  1.Date        : 2017-01-06
  Author        :
  Modification  : Created file
******************************************************************************/

#ifndef _DRIVER_HISI_LIB_API_H_
#define _DRIVER_HISI_LIB_API_H_




#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/


/*****************************************************************************
  2 基本数据类型定义
*****************************************************************************/
#define HISI_SUCC            0
#define HISI_EFAIL           1
#define HISI_EINVAL         22
#define HISI_NULL           (0L)

#define ETH_ALEN                 6
#define MAX_SSID_LEN             32
#define ETH_ADDR_LEN             6
#define HISI_RATE_INFO_FLAGS_MCS                    (1<<0)
#define HISI_RATE_INFO_FLAGS_VHT_MCS                (1<<1)
#define HISI_RATE_INFO_FLAGS_40_MHZ_WIDTH           (1<<2)
#define HISI_RATE_INFO_FLAGS_80_MHZ_WIDTH           (1<<3)
#define HISI_RATE_INFO_FLAGS_80P80_MHZ_WIDTH        (1<<4)
#define HISI_RATE_INFO_FLAGS_160_MHZ_WIDTH          (1<<5)
#define HISI_RATE_INFO_FLAGS_SHORT_GI               (1<<6)
#define HISI_RATE_INFO_FLAGS_60G                    (1<<7)


/*****************************************************************************
  4 枚举定义
*****************************************************************************/
typedef enum
{
    HISI_BOOL_FALSE   = 0,
    HISI_BOOL_TRUE    = 1,
    HISI_BOOL_BUTT
}hisi_bool_type_enum;


typedef enum
{
    HISI_BAND_WIDTH_40PLUS                 = 0,
    HISI_BAND_WIDTH_40MINUS                = 1,
    HISI_BAND_WIDTH_20M                    = 2,

    HISI_BAND_WIDTH_BUTT
}hisi_channel_bandwidth_enum;
typedef unsigned char hisi_channel_bandwidth_enum_uint8;

typedef enum
{
    HISI_MONITOR_SWITCH_OFF,
    HISI_MONITOR_SWITCH_MCAST_DATA,//上报组播(广播)数据包
    HISI_MONITOR_SWITCH_UCAST_DATA,//上报单播数据包
    HISI_MONITOR_SWITCH_MCAST_MANAGEMENT,//上报组播(广播)管理包
    HISI_MONITOR_SWITCH_UCAST_MANAGEMENT,//上报单播管理包
    HISI_MONITOR_SWITCH_BUTT
}hisi_monitor_switch_enum;
typedef unsigned char hisi_monitor_switch_enum_uint8;

typedef enum
{
    HISI_KEEPALIVE_OFF,
    HISI_KEEPALIVE_ON,
    HISI_KEEPALIVE_BUTT
}hisi_keepalive_switch_enum;
typedef unsigned char hisi_keepalive_switch_uint8;


typedef int (*hisi_upload_frame_cb)(void* frame, unsigned int len);


/*打印级别*/
typedef enum
{
    HISI_MSG_EXCESSIVE      =    0,
    HISI_MSG_MSGDUMP        =    1,
    HISI_MSG_DEBUG          =    2,
    HISI_MSG_INFO           =    3,
    HISI_MSG_WARNING        =    4,
    HISI_MSG_ERROR          =    5,
    HISI_MSG_OAM_BUTT
}e_hisi_msg_type_t;

typedef struct databk_addr_info  * (*get_databk_addr_info)(void);

typedef enum
{
    HISI_WOW_EVENT_ALL_CLEAR          = 0,          /* Clear all events */
    HISI_WOW_EVENT_MAGIC_PACKET       = 1<<0,       /* Wakeup on Magic Packet */
    HISI_WOW_EVENT_NETPATTERN_TCP     = 1<<1,       /* Wakeup on TCP NetPattern */
    HISI_WOW_EVENT_NETPATTERN_UDP     = 1<<2,       /* Wakeup on UDP NetPattern */
    HISI_WOW_EVENT_DISASSOC           = 1<<3,       /* 去关联/去认证，Wakeup on Disassociation/Deauth */
    HISI_WOW_EVENT_AUTH_RX            = 1<<4,       /* 对端关联请求，Wakeup on auth */
    HISI_WOW_EVENT_HOST_WAKEUP        = 1<<5,       /* Host wakeup */
    HISI_WOW_EVENT_TCP_UDP_KEEP_ALIVE = 1<<6,       /* Wakeup on TCP/UDP keep alive timeout */
    HISI_WOW_EVENT_OAM_LOG_WAKEUP     = 1<<7,       /* OAM LOG wakeup */
    HISI_WOW_EVENT_SSID_WAKEUP        = 1<<8,       /* SSID Scan wakeup */
}hisi_wow_event_enum;

typedef enum
{
    HISI_WOW_WKUP_REASON_TYPE_NULL               = 0,        /* None */
    HISI_WOW_WKUP_REASON_TYPE_MAGIC_PACKET       = 1,        /* Wakeup on Magic Packet */
    HISI_WOW_WKUP_REASON_TYPE_NETPATTERN_TCP     = 2,        /* Wakeup on TCP NetPattern */
    HISI_WOW_WKUP_REASON_TYPE_NETPATTERN_UDP     = 3,        /* Wakeup on UDP NetPattern */
    HISI_WOW_WKUP_REASON_TYPE_DISASSOC_RX        = 4,        /* 对端去关联/去认证，Wakeup on Disassociation/Deauth */
    HISI_WOW_WKUP_REASON_TYPE_DISASSOC_TX        = 5,        /* 对端去关联/去认证，Wakeup on Disassociation/Deauth */
    HISI_WOW_WKUP_REASON_TYPE_AUTH_RX            = 6,        /* 本端端关联请求，Wakeup on auth */
    HISI_WOW_WKUP_REASON_TYPE_TCP_UDP_KEEP_ALIVE = 7,        /* Wakeup on TCP/UDP keep alive timeout */
    HISI_WOW_WKUP_REASON_TYPE_HOST_WAKEUP        = 8,        /* Host wakeup */
    HISI_WOW_WKUP_REASON_TYPE_OAM_LOG            = 9,        /* OAM LOG wakeup */
    HISI_WOW_WKUP_REASON_TYPE_SSID_SCAN          = 10,       /* SSID Scan wakeup */
    HISI_WOW_WKUP_REASON_TYPE_BUT
}hisi_wow_wakeup_reason_type_enum;


/*****************************************************************************
  5 全局变量声明
*****************************************************************************/

/*****************************************************************************
  6 消息头定义
*****************************************************************************/

/*****************************************************************************
  7 消息定义
*****************************************************************************/



/*****************************************************************************
  8 STRUCT定义
*****************************************************************************/

typedef struct _hisi_rf_customize_stru
{
    int                             l_11b_scaling_value;
    int                             l_11g_u1_scaling_value;
    int                             l_11g_u2_scaling_value;
    int                             l_11n_20_u1_scaling_value;
    int                             l_11n_20_u2_scaling_value;
    int                             l_11n_40_u1_scaling_value;
    int                             l_11n_40_u2_scaling_value;
    int                             l_ban1_ref_value;
    int                             l_ban2_ref_value;
    int                             l_ban3_ref_value;
    int                             l_customize_enable;
    int                             l_disable_bw_40;
    int                             l_dtim_setting;
    int                             l_pm_switch;
} hisi_rf_customize_stru;

typedef struct _hisi_wpa_status_stru
{
    unsigned char                  auc_ssid[MAX_SSID_LEN];
    unsigned char                  auc_bssid[ETH_ADDR_LEN];
    unsigned char                  auc_rsv[2];
    unsigned int                   ul_status;

} hisi_wpa_status_stru;

typedef struct
{
    unsigned char                           uc_channel_num;
    hisi_channel_bandwidth_enum_uint8       uc_channel_bandwidth;
}hisi_channel_stru;

typedef struct _hisi_tcp_params_stru
{
    unsigned int        ul_sess_id;
    unsigned char       auc_dst_mac[6];  /* 目的MAC地址 */
    unsigned char       auc_resv[2];
    unsigned char       auc_src_ip[4];   /* 源IP地址 */
    unsigned char       auc_dst_ip[4];   /* 目的IP地址 */
    unsigned short      us_src_port;    /* 源端口号 */
    unsigned short      us_dst_port;    /* 目的端口号 */
    unsigned int        ul_seq_num;     /* 序列号 */
    unsigned int        ul_ack_num;     /* 确认号 */
    unsigned short      us_window;      /* TCP窗口大小 */
    unsigned short      us_retry_max_count;      /* 最大重传次数 */
    unsigned int        ul_interval_timer;       /* 心跳包发送周期 */
    unsigned int        ul_retry_interval_timer; /* 重传时心跳包发送周期 */
    unsigned int        ul_time_value;
    unsigned int        ul_time_echo;
    unsigned char      *puc_tcp_payload;
    unsigned int        ul_payload_len;
}hisi_tcp_params_stru;

struct databk_addr_info
{
    unsigned long                     databk_addr; /*flash addr, store data backup data*/
    unsigned int                      databk_length; /*data length, the length of the data backup data*/
    get_databk_addr_info              get_databk_info; /*get data backup info,include databk_addr and databk_length*/
};

struct station_info
{
    int    l_signal;/* 信号强度 */
    int    l_txrate;/* TX速率 */
};
/*****************************************************************************
  9 UNION定义
*****************************************************************************/

/*****************************************************************************
  10 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  11 函数声明
*****************************************************************************/

/*****************************************************************************
  11.1 睡眠唤醒相关对外暴露接口
*****************************************************************************/
/*****************************************************************************
 函 数 名  : hisi_wlan_suspend
 功能描述  : 强制睡眠 API接口
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月05日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern void hisi_wlan_suspend(void);

/*****************************************************************************
 函 数 名  : hisi_wlan_set_wow_event
 功能描述  : 设置强制睡眠功能开关接口
 输入参数  : unsigned int ul_event 事件开关值
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月05日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern void hisi_wlan_set_wow_event(unsigned int ul_event);

/*****************************************************************************
 函 数 名  : hisi_wlan_add_netpattern
 功能描述  : 强制睡眠netpattern唤醒报文格式的添加API接口
 输入参数  : unsigned int    ul_netpattern_index: netpattern 的索引, 0~3
             unsigned char  *puc_netpattern_data: netpattern 的内容
             unsigned int    ul_netpattern_len  : netpattern 的内容长度, 0~64
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月05日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern unsigned int hisi_wlan_add_netpattern(
                        unsigned int    ul_netpattern_index,
                        unsigned char  *puc_netpattern_data,
                        unsigned int    ul_netpattern_len
                        );

/*****************************************************************************
 函 数 名  : hisi_wlan_del_netpattern
 功能描述  : 强制睡眠netpattern唤醒报文格式的删除API接口
 输入参数  : unsigned int    ul_netpattern_index: netpattern 的索引, 0~3
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月05日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern unsigned int hisi_wlan_del_netpattern(unsigned int ul_netpattern_index);

/*****************************************************************************
 函 数 名  : hisi_wlan_wow_enable
 功能描述  : 保证wow相关参数能够顺利到达device,该函数一定要在所有下电前配置参数配置完成后调用
 输入参数  :
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 参数配置失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年12月06日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern unsigned int hisi_wlan_wow_enable(void);

/*****************************************************************************
 函 数 名  : hisi_wlan_set_wakeup_ssid
 功能描述  : 当AP异常或长期掉电时，主芯片侧鉴于功耗考虑，会下电；
             该场景下设置SSID，当wifi扫描到该SSID热点时，唤醒主芯片
 输入参数  : char *ssid: netpattern 的索引, 2~32
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年02月25日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_wakeup_ssid(char *ssid);

/*****************************************************************************
 函 数 名  : hisi_wlan_clear_wakeup_ssid
 功能描述  : 清除SSID，与hisi_wlan_set_wakeup_ssid配合使用
 输入参数  :
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年02月25日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_clear_wakeup_ssid(void);


/*****************************************************************************
 函 数 名  : hisi_wlan_get_wakeup_reason
 功能描述  : 强制睡眠唤醒原因的获取API接口
 输入参数  : unsigned int * pul_wakeup_reason[OUT]: 唤醒原因
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月05日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern unsigned int hisi_wlan_get_wakeup_reason(unsigned int * pul_wakeup_reason);
/*****************************************************************************
 函 数 名  : hisi_wlan_get_databk_addr_info
 功能描述  : 获取wifi驱动databk addr info
 输入参数  : 无
 输出参数  : 无
 返 回 值  : databk addr info
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年02月14日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern struct databk_addr_info * hisi_wlan_get_databk_addr_info(void);

/*****************************************************************************
  11.3 wifi相关对外暴露接口
*****************************************************************************/

/*****************************************************************************
 函 数 名  : hisi_wlan_wifi_init
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年12月19日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_wifi_init(struct netif **pst_wifi);

/*****************************************************************************
 函 数 名  : hisi_wlan_wifi_deinit
 功能描述  :
 输入参数  :
 输出参数  :
 返 回 值  :

 修改历史      :
  1.日    期   : 2016年12月19日
    作    者   : 
    修改内容   : 新生成函数
*****************************************************************************/
extern int hisi_wlan_wifi_deinit(void);
/*****************************************************************************
  11.4 hilink及smartconfig相关对外暴露接口
*****************************************************************************/
/*****************************************************************************
 函 数 名  : hisi_wlan_set_monitor
 功能描述  : 设置monitor模式开关API接口
 输入参数  : monitor_switch:开关参数
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_monitor(unsigned char monitor_switch, char rssi_level);
/*****************************************************************************
 函 数 名  : hisi_wlan_set_channel
 功能描述  : 设置信道的API接口
 输入参数  : 数据结构:hisi_channel_stru *s,仅限monitor模式使用
             主信道号加带宽； 带宽说明: 0->40M+  1->40M-  不支持20m带宽
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_channel(hisi_channel_stru *channel_info);

#ifndef WIN32
/*****************************************************************************
 函 数 名  : hisi_wlan_get_channel
 功能描述  : 获取信道
 输入参数  : 数据结构:hisi_channel_stru *s
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_get_channel(hisi_channel_stru *channel_info);


/*****************************************************************************
 函 数 名  : hisi_wlan_set_country
 功能描述  : 设置国家码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_country(unsigned char *puc_country);

/*****************************************************************************
 函 数 名  : hisi_wlan_get_country
 功能描述  : 获取国家码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern char * hisi_wlan_get_country(void);

/*****************************************************************************
 函 数 名  : hisi_wlan_rx_fcs_info
 功能描述  : 获取收包数
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_rx_fcs_info(int *pl_rx_succ_num);

/*****************************************************************************
 函 数 名  : hisi_wlan_set_always_tx
 功能描述  : 设置常发
 输入参数  : 参数为一个字符串数组，共计4个元素，每个元素空间大小设置为20个字节，模式为: 常发开关 常发模式 信道 速率
             例如: char ac_buffer[4][20] = {"1", "11b", "7", "11"};即在7信道上以11b 11m的速率常发
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_always_tx(char *pc_param);

/*****************************************************************************
 函 数 名  : hisi_wlan_set_pm_switch
 功能描述  : 设置低功耗开关
 输入参数  : 低功耗开关: 0 | 1(其他值无效)
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_pm_switch(unsigned int uc_pm_switch);


/*****************************************************************************
 函 数 名  : hisi_wlan_set_always_rx
 功能描述  : 设置常发
 输入参数  : 参数为一个字符串数组，共计3个成员，模式为: 常发开关 常发模式 信道
             例如: char ac_buffer[3][20] = {"1", "11b", "7"};即在7信道上以11b常收
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_always_rx(char *pc_param);
#endif
/*****************************************************************************
 函 数 名  : hisi_wlan_register_upload_frame_cb
 功能描述  : hilink或smartconfig上报数据帧的回调
 输入参数  : func回调处理函数的指针
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern unsigned int hisi_wlan_register_upload_frame_cb(hisi_upload_frame_cb func);

/*****************************************************************************
  11.5 TCP保活相关对外暴露接口
*****************************************************************************/
/*****************************************************************************
 函 数 名  : hisi_wlan_set_tcp_params
 功能描述  : 设置保活TCP链路参数的API接口
 输入参数  : tcp_params保活TCP参数的参数指针
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_tcp_params(hisi_tcp_params_stru *tcp_params);

/*****************************************************************************
 函 数 名  : hisi_wlan_set_keepalive_switch
 功能描述  : 设置保活TCP链路功能开关的API接口
 输入参数  : keepalive_switch开关值
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_set_keepalive_switch(unsigned char keepalive_switch, unsigned int keepalive_num);

/*****************************************************************************
 函 数 名  : hisi_wlan_get_macaddr
 功能描述  : 获取Mac地址
 输入参数  :
 输出参数  :
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern unsigned char* hisi_wlan_get_macaddr(void);

/*****************************************************************************
 函 数 名  : hisi_wlan_ip_notify
 功能描述  : 通知驱动IP地址变化
 输入参数  : ip,当前的IP值，mode获取或删除IP
 输出参数  : 无
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年01月16日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_ip_notify(unsigned int ip, unsigned int mode);


/*****************************************************************************
 函 数 名  : hisi_wlan_get_lose_tcpid
 功能描述  : 获取TCP断链后tcp断链ID
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 断链TCP链路ID的标记位,当前支持最大4个TCP连接,如下图所示
             每一个bit位代表相应的TCP链路是否断链1表示断链，0表示不断链
             | 31bit - 4bit | 3bit | 2bit | 1bit | 0bit |
             |    unused    | 标识 | 标识 | 标识 | 标识 |
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年02月20日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int hisi_wlan_get_lose_tcpid(void);


/*****************************************************************************
 函 数 名  : hisi_wlan_no_fs_config
 功能描述  : wifi驱动无文件系统时文件保存配置
 输入参数  : 保存文件的flash地址和长度
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
*****************************************************************************/
extern void hisi_wlan_no_fs_config(unsigned long ul_base_addr, unsigned int u_length);
/*****************************************************************************
 函 数 名  : hisi_wlan_get_station
 功能描述  : 获取station信息的API接口(只支持station模式)
 输入参数  :struct station_info *pst_sta
 输出参数  :
 返 回 值  : 0  : 成功
             非0: 失败
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年7月1日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
extern int  hisi_wlan_get_station(struct station_info *pst_sta);

/*****************************************************************************
 函 数 名  : hisi_wifi_debug_info
 功能描述  : 调用wifi_debug接口
 输入参数  : void

 输出参数  : 无
 返 回 值  :void
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2017年7月26日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/

extern void hisi_wifi_debug_info(void);


/*****************************************************************************
 驱动对外提供的OAM维测日志接口
*****************************************************************************/
#if (_HI113X_SW_VERSION == _HI113X_SW_DEBUG)
extern unsigned int oam_log_sdt_out(unsigned short      us_level,
                                    const signed char   *pc_func_name,
                                    const signed char         *pc_fmt,
                                    ...);

#define wpa_printf(level, fmt, ...) \
do{ \
    oam_log_sdt_out((unsigned short)level, (const signed char*)__func__, (const signed char*)fmt, ##__VA_ARGS__); \
    }while(0)

#define HISI_PRINT_INFO(fmt, ...) \
do{ \
    oam_log_sdt_out(HISI_MSG_INFO, (const signed char*)__func__, (const signed char*)fmt, ##__VA_ARGS__); \
    }while(0)

#define HISI_PRINT_WARNING(fmt, ...) \
do{ \
    oam_log_sdt_out(HISI_MSG_WARNING, (const signed char*)__func__, (const signed char*)fmt, ##__VA_ARGS__); \
    }while(0)

#define HISI_PRINT_ERROR(fmt, ...) \
do{ \
    oam_log_sdt_out(HISI_MSG_ERROR, (const signed char*)__func__, (const signed char*)fmt, ##__VA_ARGS__); \
    }while(0)

#elif (_HI113X_SW_VERSION == _HI113X_SW_RELEASE)
#define HISI_PRINT_INFO(fmt, arg...)
#define HISI_PRINT_WARNING(fmt, arg...)
#define HISI_PRINT_ERROR(fmt, arg...) \
do{ \
    printf(fmt, ##arg); \
    printf("\n"); \
    }while(0)

#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif    /*_DRIVER_HISI_LIB_API_H_*/
