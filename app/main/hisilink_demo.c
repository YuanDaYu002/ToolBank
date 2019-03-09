/******************************************************************************
  Copyright (C), 2004-2050, Hisilicon Tech. Co., Ltd.
******************************************************************************
  File Name     : hisilink_demo.c
  Version       : Initial Draft
  Author        : Hisilicon WiFi software group
  Created       : 2016-6-16
  Last Modified :
  Description   : hisilink sample code
  Function List :
  History       :
  1.Date        : 2016-6-16
  Author        :
  Modification  : Created file
******************************************************************************/
#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif
#endif
#include "hisilink_adapt.h"
#include "hostapd/hostapd_if.h"
#include "wpa_supplicant/wpa_supplicant.h"
#include "common/defs.h"
#include "los_task.h"
#include "sys/prctl.h"
#include <pthread.h>
#include "lwip/ip_addr.h"
#include "driver_hisi_lib_api.h"
#include "hisilink_lib.h"
#include "hisilink_ext.h"
#include <linux/completion.h>
#include "lwip/netif.h"
#include "asm/delay.h"
#include "lwip/dhcp.h"


#define CONFIG_NO_CONFIG_WRITE      1
        
#define HSL_PERIOD_CHANNEL 50     //切信道周期
#define HSL_PERIOD_TIMEOUT 70000 //hisilink超时时间

typedef enum
{
    HSL_STATUS_UNCREATE,
    HSL_STATUS_CREATE,
    HSL_STATUS_RECEIVE, //hisi_link处于接收组播阶段
    HSL_STATUS_CONNECT, //hisi_link处于关联阶段
    HSL_STATUS_BUTT
}hsl_status_enum;
typedef unsigned char hsl_status_enum_type ;

unsigned int         gul_hsl_taskid;
hsl_context_stru     st_context;
hsl_result_stru      gst_hsl_params;
hsl_status_enum_type guc_hsl_status   = 0;
extern unsigned char hsl_receive_flag;
/*****************************************************************************
                                  函数声明
*****************************************************************************/
int           hsl_demo_init(void);
int           hsl_demo_main(void);
void          hsl_demo_task_channel_change(void);
int           hsl_demo_connect(hsl_result_stru* pst_params);
int           hsl_demo_connect_prepare(void);
unsigned char hsl_demo_get_status(void);
int           hsl_demo_set_status(hsl_status_enum_type uc_type);
hsl_result_stru* hsl_demo_get_result(void);
int           hsl_demo_online(hsl_result_stru* pst_params);
void          wpa_connect_interface(struct wpa_assoc_request * pst_wpa_assoc_req);


/*****************************************************************************
 函 数 名  : hsl_demo_prepare
 功能描述  : hsl demo启动hostapd为接收组播做准备
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 成功返回HISI_SUCC,异常返回-HISI_EFAIL
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月25日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
int hsl_demo_prepare(void)
{
    int            l_ret;
    char           ac_ssid[33];
    unsigned char  auc_password[65];
    unsigned int   ul_ssid_len = 0;
    unsigned int   ul_pass_len = 0;
    unsigned char  auc_manufacturer_id[3] = {'0','0','3'};
    unsigned char  auc_device_id[3]       = {'0','0','1'};
    struct netif  *pst_netif;
    ip_addr_t      st_gw;
    ip_addr_t      st_ipaddr;
    ip_addr_t      st_netmask;
    struct hostapd_conf hapd_conf;

    /* 尝试删除wpa_supplicant和hostapd */
    l_ret = wpa_supplicant_stop();
    if (0 != l_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:wpa_supplicant stop fail",__func__,__LINE__);
    }
    pst_netif = netif_find("wlan0");
    if (NULL != pst_netif)
    {
        dhcps_stop(pst_netif);
    }
    l_ret = hostapd_stop();
    if (0 != l_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:hostapd stop fail",__func__,__LINE__);
    }

    /* 设置网口信息 */
    IP4_ADDR(&st_gw, 192, 168, 43, 1);
    IP4_ADDR(&st_ipaddr, 192, 168, 43, 1);
    IP4_ADDR(&st_netmask, 255, 255, 255, 0);
    netif_set_addr(pst_netif, &st_ipaddr, &st_netmask, &st_gw);

    memset(ac_ssid, 0, sizeof(char)*33);
    /* 获取启动AP需要的SSID及密码 */
    l_ret = hsl_get_ap_params(auc_manufacturer_id, auc_device_id, ac_ssid, &ul_ssid_len, auc_password, &ul_pass_len);
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:get ap params fail",__func__,__LINE__);
        return -HSL_FAIL;
    }

    /* 设置AP参数 */
    l_ret = hsl_memset(&hapd_conf, sizeof(struct hostapd_conf), 0, sizeof(struct hostapd_conf));
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:memset fail",__func__,__LINE__);
    }
    l_ret = hsl_memcpy(hapd_conf.ssid, MAX_ESSID_LEN + 1, ac_ssid, ul_ssid_len);
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:memcpy fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    l_ret = hsl_memcpy(hapd_conf.key, sizeof(hapd_conf.key), auc_password, ul_pass_len);
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:memcpy fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    l_ret = hsl_memcpy(hapd_conf.ht_capab, sizeof(hapd_conf.ht_capab), "[HT40+]", strlen("[HT40+]"));
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:memcpy fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    hapd_conf.channel_num           = 6;
    hapd_conf.wpa_key_mgmt          = WPA_KEY_MGMT_PSK;
    hapd_conf.wpa                   = 2;
    hapd_conf.authmode              = HOSTAPD_SECURITY_WPAPSK;
    hapd_conf.wpa_pairwise          = WPA_CIPHER_TKIP | WPA_CIPHER_CCMP;
    l_ret = hsl_memcpy(hapd_conf.driver, sizeof(hapd_conf.driver), "hisi", 5);
    if (HSL_SUCC != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:memcpy fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
#ifdef HISI_CONNETIVITY_PATCH
    hapd_conf.ignore_broadcast_ssid = 0;
#endif
    l_ret = hostapd_start(NULL, &hapd_conf);
    if (HSL_SUCC != l_ret)
    {
       HISI_PRINT_ERROR("HSL_start_hostapd:start failed[%d]",l_ret);
       return -HSL_FAIL;
    }
    return HSL_SUCC;
}

/*****************************************************************************
 函 数 名  : hsl_demo_init
 功能描述  : 初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 成功返回HISI_SUCC,异常返回-HISI_EFAIL
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月25日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
int hsl_demo_init(void)
{
    int l_ret;

    memset(&gst_hsl_params, 0, sizeof(hsl_result_stru));
    l_ret = hsl_os_init(&st_context);
    if (HSL_SUCC != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        return -HSL_FAIL;
    }
    l_ret = hisi_hsl_adapt_init();
    if (HISI_SUCC != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        HISI_PRINT_ERROR("%s[%d]:hisi_HSL_adapt_init fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    return HSL_SUCC;
}
/*****************************************************************************
 函 数 名  : hsl_demo_task_channel_change
 功能描述  : hsl demo的切信道线程处理函数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月25日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void hsl_demo_task_channel_change(void)
{
    int             l_ret;
    unsigned int    ul_ret;
    unsigned int    ul_time      = 0;
    unsigned int    ul_time_temp = 0;
    unsigned int    ul_wait_time = 0;
    unsigned char  *puc_macaddr;
    unsigned char   uc_status;
    struct netif   *pst_netif;

    ul_time_temp = hsl_get_time();
    ul_time      = ul_time_temp;
    uc_status    = hsl_demo_get_status();
    while((HSL_STATUS_RECEIVE == uc_status) && (HSL_PERIOD_TIMEOUT >= ul_wait_time))
    {
        if (RECEIVE_FLAG_ON == hsl_receive_flag)
        {
            ul_time_temp = hsl_get_time();
            if (HSL_PERIOD_CHANNEL < (ul_time_temp - ul_time))
            {
                ul_wait_time += (ul_time_temp - ul_time);
                ul_time = ul_time_temp;
                l_ret = hsl_get_lock_status();
                if (HSL_CHANNEL_UNLOCK == l_ret)
                {
                    l_ret = hisi_hsl_change_channel();
                    if (HISI_SUCC != l_ret)
                    {
                        HISI_PRINT_WARNING("%s[%d]:change channel fail",__func__,__LINE__);
                    }
                    hsl_os_reset();
                }
            }
        }
        l_ret = hisi_hsl_process_data();
        if (HSL_DATA_STATUS_FINISH == l_ret)
        {
            LOS_TaskLock();
            hsl_receive_flag = RECEIVE_FLAG_OFF;
            LOS_TaskUnlock();
            hsl_demo_set_status(HSL_STATUS_CONNECT);
            break;
        }
        msleep(1);
        uc_status = hsl_demo_get_status();
    }

    hisi_hsl_adapt_deinit();
    if (HSL_PERIOD_TIMEOUT >= ul_wait_time)
    {
        l_ret = hsl_memset(&gst_hsl_params, sizeof(hsl_result_stru), 0, sizeof(hsl_result_stru));
        if (HSL_SUCC != l_ret)
        {
            HISI_PRINT_ERROR("%s[%d]:memset fail",__func__,__LINE__);
        }
        l_ret = hsl_get_result(&gst_hsl_params);
        if (HSL_SUCC == l_ret)
        {
            puc_macaddr = hisi_wlan_get_macaddr();
            if (HSL_NULL != puc_macaddr)
            {
                if ((puc_macaddr[4] == gst_hsl_params.auc_flag[0]) && (puc_macaddr[5] == gst_hsl_params.auc_flag[1]))
                {
                    l_ret = hsl_demo_connect_prepare();
                    if (0 != l_ret)
                    {
                        HISI_PRINT_ERROR("hsl_demo_task_channel_change:connect prepare fail[%d]\n",l_ret);
                    }
                }
                else
                {
                    HISI_PRINT_ERROR("This device is not intended to be associated:%02x %02x\n",gst_hsl_params.auc_flag[0],gst_hsl_params.auc_flag[1]);
                }
            }
        }
    }
    else
    {
        HISI_PRINT_INFO("hsl timeout\n");

        /* 删除AP模式 */
        pst_netif = netif_find("wlan0");
        if (HSL_NULL != pst_netif)
        {
            dhcps_stop(pst_netif);
        }
        l_ret = hostapd_stop();
        if (0 != l_ret)
        {
            HISI_PRINT_ERROR("%s[%d]:stop hostapd fail",__func__,__LINE__);
        }
    }
    hsl_demo_set_status(HSL_STATUS_UNCREATE);
    ul_ret = LOS_TaskDelete(gul_hsl_taskid);
    if (0 != ul_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:delete task fail[%d]",__func__,__LINE__,ul_ret);
    }
}
extern struct completion  dhcp_complet;
/*****************************************************************************
 函 数 名  : hsl_demo_connect_prepare
 功能描述  : 切换到wpa准备关联
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
hsl_int32 hsl_demo_connect_prepare(void)
{
    int                      l_ret;
    unsigned int             ul_ret;
    hsl_result_stru         *pst_result;
    struct netif            *pst_netif;

    /* 删除AP模式 */
    pst_netif = netif_find("wlan0");
    if (HSL_NULL != pst_netif)
    {
        dhcps_stop(pst_netif);
    }
    l_ret = hostapd_stop();
    if (0 != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:stop hostapd fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    init_completion(&dhcp_complet);
#ifdef CONFIG_NO_CONFIG_WRITE
    l_ret = wpa_supplicant_start("wlan0", "hisi", HSL_NULL);
#else
    char *config = WPA_SUPPLICANT_CONFIG_PATH;
    l_ret = wpa_supplicant_start("wlan0", "hisi", (struct wpa_supplicant_conf*)config);
#endif
    if (0 != l_ret)
    {
        HISI_PRINT_ERROR("%s[%d]:start wpa_supplicant fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    ul_ret = wait_for_completion_timeout(&dhcp_complet, LOS_MS2Tick(40000));//40s超时
    if (0 == ul_ret)
    {
        HISI_PRINT_ERROR("hsl_demo_connect_prepare:cannot get ip\n");
        return -HSL_FAIL;
    }
    pst_result = hsl_demo_get_result();
    if (HSL_NULL != pst_result)
    {
        hsl_demo_online(pst_result);
    }
    return HSL_SUCC;
}
/*****************************************************************************
 函 数 名  : hsl_demo_connect
 功能描述  : 利用获取到的参数启动关联
 输入参数  : pst_params解码获取到的参数
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
int hsl_demo_connect(hsl_result_stru* pst_params)
{

    struct wpa_assoc_request wpa_assoc_req;
    if (HSL_NULL == pst_params)
    {
        HISI_PRINT_ERROR("%s[%d]:pst_params is null",__func__,__LINE__);
        return -HSL_ERR_NULL;
    }

    if (HSL_SUCC != hsl_memset(&wpa_assoc_req , sizeof(struct wpa_assoc_request), 0 ,sizeof(struct wpa_assoc_request)))
    {
        HISI_PRINT_ERROR("%s[%d]:memset fail",__func__,__LINE__);
    }

    wpa_assoc_req.hidden_ssid = 1;
    if (HSL_SUCC != hsl_memcpy(wpa_assoc_req.ssid, sizeof(wpa_assoc_req.ssid), pst_params->auc_ssid, pst_params->uc_ssid_len))
    {
        HISI_PRINT_ERROR("%s[%d]:memcpy fail",__func__,__LINE__);
        return -HSL_FAIL;
    }
    wpa_assoc_req.auth = pst_params->en_auth_mode;

    if (HSL_AUTH_TYPE_OPEN != wpa_assoc_req.auth)
    {
        if (HSL_SUCC != hsl_memcpy(wpa_assoc_req.key, sizeof(wpa_assoc_req.key), pst_params->auc_pwd, pst_params->uc_pwd_len))
        {
            HISI_PRINT_ERROR("%s[%d]:memcpy fail",__func__,__LINE__);
            return -HSL_FAIL;
        }
    }
    /* 开始创建连接 */
    wpa_connect_interface(&wpa_assoc_req);
    return HSL_SUCC;
}
/*****************************************************************************
 函 数 名  : hsl_demo_get_status
 功能描述  : 利用获取当前hisilink的状态
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
unsigned char hsl_demo_get_status(void)
{
    unsigned char uc_type;
    LOS_TaskLock();
    uc_type = guc_hsl_status;
    LOS_TaskUnlock();
    return uc_type;
}
/*****************************************************************************
 函 数 名  : hsl_demo_set_status
 功能描述  : 设置hisilink的状态
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
int hsl_demo_set_status(hsl_status_enum_type uc_type)
{
    LOS_TaskLock();
    guc_hsl_status = uc_type;
    LOS_TaskUnlock();
    return 0;
}
/*****************************************************************************
 函 数 名  : hsl_demo_get_result
 功能描述  : 获取解密结果指针
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
hsl_result_stru* hsl_demo_get_result(void)
{
    hsl_result_stru *pst_result;
    pst_result = &gst_hsl_params;
    return pst_result;
}

/*****************************************************************************
 函 数 名  : hsl_demo_online
 功能描述  : 成功获取IP后发送上线通知
 输入参数  :
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
int hsl_demo_online(hsl_result_stru* pst_params)
{
    if (HSL_NULL == pst_params)
    {
        HISI_PRINT_ERROR("%s[%d]:pst_params is null",__func__,__LINE__);
        return -HSL_ERR_NULL;
    }
    hisi_hsl_online_notice(pst_params);
    return HSL_SUCC;
}
extern int hilink_demo_get_status(void);
/*****************************************************************************
 函 数 名  : hsl_demo_main
 功能描述  : hsl demo程序总入口
 输入参数  : 无
 输出参数  : 无
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年11月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
hsl_int32 hsl_demo_main(void)
{
    unsigned int        ul_ret;
    int                 l_ret;
    unsigned char       uc_status;
    TSK_INIT_PARAM_S    st_hsl_task;

    uc_status = hsl_demo_get_status();
    if (HSL_STATUS_UNCREATE != uc_status)
    {
        HISI_PRINT_ERROR("hsl already start,cannot start again");
        return -HSL_FAIL;
    }
    hsl_demo_set_status(HSL_STATUS_RECEIVE);
    l_ret = hsl_demo_prepare();
    if (0 != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        HISI_PRINT_ERROR("%s[%d]:demo init fail",__func__,__LINE__);
        return -HSL_FAIL;
    }

   /* 创建切信道线程,线程结束后自动释放 */
    memset(&st_hsl_task, 0, sizeof(TSK_INIT_PARAM_S));
    st_hsl_task.pfnTaskEntry = (TSK_ENTRY_FUNC)hsl_demo_task_channel_change;

    st_hsl_task.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    st_hsl_task.pcName       = "hsl_thread";
    st_hsl_task.usTaskPrio   = 8;
    st_hsl_task.uwResved     = LOS_TASK_STATUS_DETACHED;
    ul_ret = LOS_TaskCreate(&gul_hsl_taskid, &st_hsl_task);
    if(0 != ul_ret)
    {
       hsl_demo_set_status(HSL_STATUS_UNCREATE);
       HISI_PRINT_ERROR("%s[%d]:create task fail[%d]",__func__,__LINE__,ul_ret);
       return -HSL_FAIL;
    }
    return HSL_SUCC;
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
