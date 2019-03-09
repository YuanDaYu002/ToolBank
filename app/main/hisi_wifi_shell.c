/******************************************************************************
  Copyright (C), 2004-2050, Hisilicon Tech. Co., Ltd.
******************************************************************************
  File Name     : hisi_wifi_shell.c
  Version       : Initial Draft
  Author        : Hisilicon WiFi software group
  Created       : 2016-3-22
  Last Modified :
  Description   : shell cmd
  Function List :
  History       :
  1.Date        : 2016-3-22
  Author        :
  Modification  : Created file
******************************************************************************/


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/

//#include "wifi_shell_cmd_register.h"
//#include "plat_firmware.h"
//#include "plat_pm.h"
//#include "gpio.h"
//#include "plat_pm_wlan.h"
#include "wpa_supplicant/wpa_supplicant.h"
//#include "oam_misc.h"
#include "linux/kernel.h"
#include "shell.h"
#include "driver_hisi_lib_api.h"
#include "los_event.h"
#include "los_task.h"
#include "stdio.h"
#include "lwip/netif.h"
#include "common/defs.h"
#include "hostapd/hostapd_if.h"
#include "driver_hisi_lib_api.h"


//#include "oal_types.h"
//#include "misc_ext_if.h"

//#include "data_process.h"

//#include "hmac_wow.h"

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
unsigned char  sg_wifi_init_module = HISI_BOOL_FALSE;
unsigned char  sg_hwal_init_module = HISI_BOOL_FALSE;
unsigned char  sg_pm_init_module   = HISI_BOOL_FALSE;
unsigned char  sg_wifi_power_on    = HISI_BOOL_FALSE;

unsigned char  sg_wifi_start       = HISI_BOOL_FALSE; /*Flag:wifi init*/


extern unsigned int             g_ul_wlan_resume_state;
extern unsigned int             g_ul_wlan_reusme_wifi_mode;

//#ifdef _PRE_WLAN_FEATURE_DATA_BACKUP
extern EVENT_CB_S          g_st_backup_event;
//#endif

#ifdef CONFIG_NO_CONFIG_WRITE
    /* 无文件系统，通过全局变量配置参数 */
    struct     hostapd_conf     gst_hapd_conf = {0};
#else
    /* 采用文件系统配置参数 */
    #define HOSTAPD_CONF_PATH           "/jffs0/etc/hisi_wifi/wifi/hostapd.conf"
    #define WPA_SUPPLICANT_CONFIG_PATH  "/jffs0/etc/hisi_wifi/wifi/wpa_supplicant.conf"
#endif
#define SCAN_AP_LIMIT            64

typedef unsigned char   hisi_printf_level_enum_uint8;
extern unsigned int wlan_pm_suspend(void);
extern unsigned int wlan_pm_resume(void);
extern void         wlan_pm_dump_host_info(void);
extern unsigned int wlan_pm_timeout_slp(void);
extern unsigned int g_ul_wlan_resume_wifi_init_flag;
extern void         oam_file_print(char* pc_file_name);

/*****************************************************************************
  3 函数实现
*****************************************************************************/
extern int wlan_power_on(void);
extern int wlan_power_off(void);
extern int hilink_demo_set_status(unsigned char uc_type);
extern int hsl_demo_set_status(unsigned char uc_type);
#ifndef WIN32
extern int wal_update_macaddr_to_netdev_and_lwip(unsigned char *puc_macaddr);
extern int use_wpa_supplicant;
extern int use_hostapd;


extern int           hwal_ioctl_set_pm_on(char *puc_ifname, void *p_buf);
extern int hisi_wlan_set_country(unsigned char *puc_country);
extern char *hisi_wlan_get_country(void);
extern int hisi_wlan_set_always_tx(char *pc_param);
extern int hisi_wlan_set_always_rx(char *pc_param);
extern int hisi_wlan_rx_fcs_info(int *pl_rx_succ_num);
extern int hisi_wlan_get_channel(hisi_channel_stru *channel_info);
extern int wal_set_wlan_pm_state(unsigned int ul_pm_state);
extern int hisi_wlan_set_pm_switch(unsigned int ul_pm_switch);
extern int hisi_wlan_get_station(struct station_info *pst_sta);
extern void stop_dhcps(void);
extern void stop_dhcp(void);



#define ALTX_PARA_NUM 4
#define ALTX_PARA_LEN 20
#define ALRX_PARA_NUM 3
#define ALRX_PARA_LEN 20

/*****************************************************************************
 函 数 名  :hisi_cmd_set_always_tx
 功能描述  :设置国家码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_set_always_tx(int argc, unsigned char *argv[])
{
    unsigned int    ul_index;
    char            ac_buffer[ALTX_PARA_NUM][ALTX_PARA_LEN] = {0};
    int             l_ret;

    if (argc != ALTX_PARA_NUM)
    {
        HISI_PRINT_ERROR("arg nums not right\n");
        HISI_PRINT_ERROR("FAIL!\n");
        return;
    }

    for (ul_index = 0; ul_index < ALTX_PARA_NUM; ul_index++)
    {
        strcpy(ac_buffer[ul_index], argv[ul_index]);
    }

    l_ret = hisi_wlan_set_always_tx((char *)ac_buffer);
    if (l_ret)
    {
        HISI_PRINT_ERROR("tx err code is %d\n", l_ret);
        HISI_PRINT_ERROR("FAIL!\n");
    }
    else
    {
        HISI_PRINT_ERROR("OK!\n");
    }
    return;

}

/*****************************************************************************
 函 数 名  :hisi_cmd_set_always_rx
 功能描述  :设置国家码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_set_always_rx(int argc, unsigned char *argv[])
{
    unsigned int    ul_index;
    char            ac_buffer[ALRX_PARA_NUM][ALRX_PARA_LEN] = {0};
    int             l_ret;

    if (argc != ALRX_PARA_NUM)
    {
        HISI_PRINT_ERROR("arg nums not right\n");
        HISI_PRINT_ERROR("FAIL!\n");

        return;
    }

    for (ul_index = 0; ul_index < ALRX_PARA_NUM; ul_index++)
    {
        strcpy(ac_buffer[ul_index], argv[ul_index]);
    }

    l_ret = hisi_wlan_set_always_rx((char *)ac_buffer);
    if (l_ret)
    {
        HISI_PRINT_ERROR("rx err code is %d\n", l_ret);
        HISI_PRINT_ERROR("FAIL!\n");
    }
    else
    {
        HISI_PRINT_ERROR("OK!\n");
    }

    return ;
}


/*****************************************************************************
 函 数 名  :hisi_cmd_rx_info
 功能描述  :查看常收收包数
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_rx_info(void)
{
    int l_rx_succ_num = -1;
    if (HISI_SUCC != hisi_wlan_rx_fcs_info(&l_rx_succ_num))
    {
         HISI_PRINT_ERROR("fx_info faile");
    }

    HISI_PRINT_ERROR("succ num [%d]", l_rx_succ_num);
    return;
}

/*****************************************************************************
 函 数 名  :hisi_set_country
 功能描述  :设置国家码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_set_country(int argc, unsigned char *argv[])
{
    unsigned char *p_uc_country;

    p_uc_country = argv[0];
    if (NULL == p_uc_country)
    {
        HISI_PRINT_ERROR("hisi_cmd_set_country::point is NULL");
        return;
    }

    if (HISI_SUCC != hisi_wlan_set_country(p_uc_country))
    {
        HISI_PRINT_ERROR("hisi_cmd_set_country::set country faile!");
        return;
    }

    HISI_PRINT_ERROR("hisi_cmd_set_country::set country succ!");
    return;
}



/*****************************************************************************
 函 数 名  :hisi_set_country
 功能描述  :获取国家码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_get_country(void)
{
    char *pc_country = hisi_wlan_get_country();
    if (NULL == pc_country)
    {
       HISI_PRINT_ERROR("hisi_cmd_get_country::get country faile!");
       return;
    }

    HISI_PRINT_ERROR("country is %c%c", pc_country[0], pc_country[1]);
    return;
}

/*****************************************************************************
 函 数 名  :hisi_cmd_get_channel
 功能描述  :获取信道
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_get_channel(void)
{
    hisi_channel_stru channel_info;
    if (HISI_SUCC != hisi_wlan_get_channel(&channel_info))
    {
        HISI_PRINT_ERROR("get_channel_fail");
        return;
    }

    HISI_PRINT_ERROR("channel[%d], bandwith[%d]", channel_info.uc_channel_num, channel_info.uc_channel_bandwidth);

    return;
}



/*****************************************************************************
 函 数 名  :hisi_cmd_set_pm_switch
 功能描述  :设置低功耗开关
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年01月19日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_cmd_set_pm_switch(int argc, unsigned char *argv[])
{
    unsigned int pm_switch_value;
    pm_switch_value =  (int)strtol(argv[0], NULL, 16);

    /*获取低功耗开关状态*/
    if ((0 != pm_switch_value) && (1 != pm_switch_value))
    {
        HISI_PRINT_ERROR("parama is error, please check!");
        return ;
    }

    /*下发低功耗状态*/
    if (HISI_SUCC != hisi_wlan_set_pm_switch(pm_switch_value))
    {
        HISI_PRINT_ERROR("pm switch set faile");
        return;
    }

    HISI_PRINT_ERROR("pm switch set succ");
    return;
}
/*****************************************************************************
 函 数 名  :hisi_cmd_get_station
 功能描述  :获取station信息
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/

void hisi_cmd_get_station(void)
{
    struct station_info st_sta ;

    /*获取station信息*/
    if (HISI_SUCC != hisi_wlan_get_station(&st_sta))
    {
        HISI_PRINT_ERROR("get station info fail!");
        return;
    }

    HISI_PRINT_ERROR("get station info succ!");
    HISI_PRINT_ERROR("rssi = %d,tx rate = %d/10 Mbps\n",st_sta.l_signal,st_sta.l_txrate);

    return;
}

#endif
extern void hardware_test_shell_cmd_register(void);
extern int  hi1102_host_main_init(void);
extern struct netif       *pwifi;
/*****************************************************************************
 函 数 名  : cmd_wifi_init_module
 功能描述  : 初始化wifi模块
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_void
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月22日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_wifi_init_module(void)
{
    HISI_PRINT_INFO("init wifi module.");
    if (HISI_BOOL_FALSE == sg_wifi_init_module)
    {
        if (HISI_SUCC != hi1102_host_main_init())
        {
            HISI_PRINT_ERROR("wifi init module fail.");
            return;
        }
        pwifi = netif_find("wlan0");
        if (NULL == pwifi)
        {
            printf("\n\n\n pwifi is null\n\n");
        }
        /* hardware shell cmd register */
        hardware_test_shell_cmd_register();

        sg_wifi_init_module = HISI_BOOL_TRUE;
    }
    else
    {
        HISI_PRINT_INFO("wifi module already init.\n");
    }
    return;
}

const char *cmd_start_hapd_Help =
"\nUsage:\n"
"\tstart_hapd <ssid_name> <security_mode> <encryption> <security_key>\n"
"\tchannel    : Channel number\n"
"\tessid      : Name of access point\n"
"\thiddenssid : properties of ssid hidden\n"
"\tsecurity   : [none] [wep-open] [wep-shared] [wpa] [wpa2] [wpa+wpa2]\n"
"\tencryption : [tkip] [aes] [tkip+aes]-For WPA/WPA2 and key index for WEP\n"
"\tkey        : Passphrase / WEP Key\n"
"\nExample:\n"
"\tstart_hapd 9 softap 0 none\t(Connect with AP 'softap' in open authentication)\n"
"\tstart_hapd 9 softap 1 wep_open 0 wepkey\t(Connect with AP 'softap' in wep open mode with key index 0 and key value of 'wpakey')\n"
"\tstart_hapd 9 softap 2 wpa2 tkip wepkey\t(Connect with AP 'softap' in WPA2 mode in tkip encryption with key value of 'wpakey')\n";

#ifdef CONFIG_NO_CONFIG_WRITE
extern int use_hostapd;
/*****************************************************************************
 函 数 名  : cmd_start_hapd
 功能描述  : 无文件系统，通过shell命令启动hostapd
 输入参数  : oal_int32 argc
             oal_int8 *argv[]
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_void
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月22日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_start_hapd(int argc, unsigned char *argv[])
{
    ip_addr_t           st_gw;
    ip_addr_t           st_ipaddr;
    ip_addr_t           st_netmask;
    struct netif       *pst_lwip_netif;
    unsigned char      *puc_encipher_mode;
    /* 正常启动时恢复hisi_link状态 */
    hsl_demo_set_status(0);
    memset(&gst_hapd_conf, 0, sizeof(struct hostapd_conf));

    IP4_ADDR(&st_gw, 192, 168, 43, 1);
    IP4_ADDR(&st_ipaddr, 192, 168, 43, 1);
    IP4_ADDR(&st_netmask, 255, 255, 255, 0);

    pst_lwip_netif = netif_find("wlan0");
    if (HISI_NULL == pst_lwip_netif)
    {
        HISI_PRINT_ERROR("cmd_start_hapd::Null param of netdev");
        return;
    }

    if (1 == use_hostapd)
    {
        HISI_PRINT_INFO("cmd_start_hapd::hostapd is allready started!");
        return;
    }
    memcpy(gst_hapd_conf.driver, "hisi", 5);
    gst_hapd_conf.channel_num = atoi(argv[0]);
    memcpy(gst_hapd_conf.ssid, argv[1], 32);
    gst_hapd_conf.ignore_broadcast_ssid = atoi(argv[2]);
    memcpy(gst_hapd_conf.ht_capab, argv[3], strlen(argv[3]));

    puc_encipher_mode = argv[4];

    if (!strcmp(puc_encipher_mode, "none"))
    {
        gst_hapd_conf.authmode = HOSTAPD_SECURITY_OPEN;
    }
    else if (!strcmp(puc_encipher_mode, "wpa"))
    {
        gst_hapd_conf.authmode = HOSTAPD_SECURITY_WPAPSK;
    }
    else if (!strcmp(puc_encipher_mode, "wpa2"))
    {
        gst_hapd_conf.authmode = HOSTAPD_SECURITY_WPA2PSK;
    }
    else if (!strcmp(puc_encipher_mode, "wpa+wpa2"))
    {
        gst_hapd_conf.authmode = HOSTAPD_SECURITY_WPAPSK_WPA2PSK_MIX;
    }
    else
    {
        HISI_PRINT_WARNING("Invalid security");
        HISI_PRINT_WARNING("%s", cmd_start_hapd_Help);
        return;
    }

    if (gst_hapd_conf.authmode != HOSTAPD_SECURITY_OPEN)
    {
        if (7 != argc)
        {
            HISI_PRINT_WARNING("Invalid argc");
            HISI_PRINT_WARNING("%s", cmd_start_hapd_Help);
            return;
        }
        strcpy((char *)gst_hapd_conf.key, argv[6]);
    }

    if (HOSTAPD_SECURITY_WPAPSK == gst_hapd_conf.authmode ||
        HOSTAPD_SECURITY_WPA2PSK == gst_hapd_conf.authmode ||
        HOSTAPD_SECURITY_WPAPSK_WPA2PSK_MIX == gst_hapd_conf.authmode)
    {
        puc_encipher_mode = argv[5];
        if (!strcmp(puc_encipher_mode, "tkip"))
        {
            gst_hapd_conf.wpa_pairwise = WPA_CIPHER_TKIP;
        }
        else if (!strcmp(puc_encipher_mode, "aes"))
        {
            gst_hapd_conf.wpa_pairwise = WPA_CIPHER_CCMP;
        }
        else if (!strcmp(puc_encipher_mode, "tkip+aes"))
        {
            gst_hapd_conf.wpa_pairwise = WPA_CIPHER_TKIP | WPA_CIPHER_CCMP;
        }
        else
        {
            HISI_PRINT_WARNING("Invalid encryption");
            HISI_PRINT_WARNING("%s", cmd_start_hapd_Help);
            return;
        }
    }

    /* 重新设置netif的网关和mac地址，防止STA切AP时，没有还原 */
    netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);

    if (HISI_SUCC != hostapd_start(HISI_NULL, &gst_hapd_conf))
    {
        HISI_PRINT_ERROR("hostapd start failed");
        return;
    }

    HISI_PRINT_INFO("hostapd start succ");

    use_hostapd = 1;
    return;
}

#else

/*****************************************************************************
 函 数 名  : cmd_start_hapd
 功能描述  : 通过文件系统启动hostapd
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : oal_void
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年7月26日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_start_hapd(void)
{
    ip_addr_t        st_gw;
    ip_addr_t        st_ipaddr;
    ip_addr_t        st_netmask;
    struct netif    *pst_lwip_netif;

    IP4_ADDR(&st_gw, 192, 168, 43, 1);
    IP4_ADDR(&st_ipaddr, 192, 168, 43, 1);
    IP4_ADDR(&st_netmask, 255, 255, 255, 0);

    pst_lwip_netif = netif_find("wlan0");
    if (HISI_NULL == pst_lwip_netif)
    {
        HISI_PRINT_ERROR("cmd_start_hapd::Null param of netdev");
        return;
    }

    /* 重新设置netif的网关和mac地址，防止STA切AP时，没有还原 */
    netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);

    if (HISI_SUCC != hostapd_start(HOSTAPD_CONF_PATH, NULL))
    {
        HISI_PRINT_ERROR("hostapd start failed");
        return;
    }

    HISI_PRINT_INFO("hostapd start succ");

    return;
}
#endif

/*****************************************************************************
 函 数 名  : cmd_stop_hapd
 功能描述  : 启动hostapd命令
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : OAL_STATIC oal_void
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年5月5日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_stop_hapd(void)
{
    if (HISI_SUCC != hostapd_stop())
    {
        HISI_PRINT_WARNING("hostapd stop failed");
        return;
    }
    stop_dhcps();
}

/*****************************************************************************
 函 数 名  : cmd_wpa_start
 功能描述  : 启动wpa_supplicant
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/


void cmd_wpa_start(int argc, char *argv[])
{
    int l_ret = 0;

    /* 正常启动时恢复hisi_link状态 */
    hsl_demo_set_status(0);
#ifdef CONFIG_NO_CONFIG_WRITE
    l_ret = wpa_supplicant_start("wlan0", "hisi", NULL);
#else
    char * config = WPA_SUPPLICANT_CONFIG_PATH;
    l_ret = wpa_supplicant_start("wlan0", "hisi", (struct wpa_supplicant_conf *)config);
#endif

    if (l_ret != HISI_SUCC)
    {
        HISI_PRINT_ERROR("cmd_wpa_start fail.");
        return;
    }
}
/*****************************************************************************
 函 数 名  : cmd_wpa_stop
 功能描述  : 停止wpa_supplicant
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/

void cmd_wpa_stop(int argc, char *argv[])
{
    ip_addr_t        st_gw;
    ip_addr_t        st_ipaddr;
    ip_addr_t        st_netmask;
    struct netif    *pst_lwip_netif;

    int l_ret = 0;
    l_ret = wpa_supplicant_stop();
    if (l_ret != HISI_SUCC)
    {
        HISI_PRINT_WARNING("cmd_wpa_stop fail.");
        return;
    }

    stop_dhcp();

    IP4_ADDR(&st_gw, 0, 0, 0, 0);
    IP4_ADDR(&st_ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&st_netmask, 0, 0, 0, 0);

    pst_lwip_netif = netif_find("wlan0");
    if (HISI_NULL == pst_lwip_netif)
    {
        HISI_PRINT_ERROR("cmd_start_hapd::Null param of netdev");
        return;
    }

    /* wpa_stop后，重新设置netif的网关和mac地址 */
    netif_set_addr(pst_lwip_netif, &st_ipaddr, &st_netmask, &st_gw);
}
/*****************************************************************************
 函 数 名  : cmd_wpa_scan
 功能描述  : 发起关联
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/

void cmd_wpa_scan(int argc, char *argv[])
{
    int l_ret = 0;

    l_ret = wpa_cli_scan();
    if (l_ret != 0)
    {
        HISI_PRINT_WARNING("cmd_wpa_scan fail.");
    }
}

void cmd_wpa_channel_scan(int argc, char *argv[])
{
    int l_ret = 0;
    int channel;

    channel = atoi(argv[0]);
    l_ret = wpa_cli_channel_scan(channel);
    if (l_ret != 0)
    {
        HISI_PRINT_WARNING("cmd_wpa_channel_scan fail.");
    }
}

/*****************************************************************************
 函 数 名  : cmd_wpa_scan_results
 功能描述  : 获取扫描结果
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_wpa_scan_results(int argc, char *argv[])
{
    struct wpa_ap_info *pst_results = HISI_NULL;
    unsigned int   num = SCAN_AP_LIMIT ;
    unsigned int   ul_loop;

    pst_results = malloc(sizeof(struct wpa_ap_info) * SCAN_AP_LIMIT);

    if(HISI_NULL == pst_results)
    {
          printf("cmd_wpa_scan_results, OAL_PTR_NULL == pst_results\n");
          return;
    }

    memset(pst_results, 0, (sizeof(struct wpa_ap_info) * SCAN_AP_LIMIT));
    wpa_cli_scan_results(pst_results,&num);

    for (ul_loop = 0; (ul_loop < num) && (ul_loop < SCAN_AP_LIMIT); ul_loop++)
    {
        printf("<------------------------>");
        HISI_PRINT_WARNING("<------------------------>");
        printf("ssid:%s\n",pst_results[ul_loop].ssid);
        HISI_PRINT_WARNING("ssid:%s",pst_results[ul_loop].ssid);
        printf("bssid:%s\n",pst_results[ul_loop].bssid);
        HISI_PRINT_WARNING("bssid:%s",pst_results[ul_loop].bssid);
        printf("channel:%d\n",pst_results[ul_loop].channel);
        HISI_PRINT_WARNING("channel:%d",pst_results[ul_loop].channel);
        printf("rssi:%d\n",pst_results[ul_loop].rssi/100);
        HISI_PRINT_WARNING("rssi:%d",pst_results[ul_loop].rssi/100);
        switch(pst_results[ul_loop].auth)
        {
            case WPA_SECURITY_OPEN:
            printf("auth type: open\n");
            HISI_PRINT_WARNING("auth type: open");
            break;
            case WPA_SECURITY_WEP:
            printf("auth type: wep\n");
            HISI_PRINT_WARNING("auth type: wep");
            break;
            case WPA_SECURITY_WPAPSK:
            printf("auth type: wpa\n");
            HISI_PRINT_WARNING("auth type: wpa");
            break;
            case WPA_SECURITY_WPA2PSK:
            printf("auth type: wpa2\n");
            HISI_PRINT_WARNING("auth type: wpa2");
            break;
            case WPA_SECURITY_WPAPSK_WPA2PSK_MIX:
            printf("auth type: wpa+wpa2\n");
            HISI_PRINT_WARNING("auth type: wpa+wpa2");
            break;
            default:
            printf("auth type error\n");
            HISI_PRINT_WARNING("auth type error");
            break;
        }
    }

    free(pst_results);
    pst_results = HISI_NULL;
}

/*****************************************************************************
 函 数 名  : wpa_connect_interface
 功能描述  : 发起关联
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/

void wpa_connect_interface(struct wpa_assoc_request * pst_wpa_assoc_req)
{
    wpa_cli_connect(pst_wpa_assoc_req);

    HISI_PRINT_INFO("Authentication:%d",pst_wpa_assoc_req->auth);
}

/*****************************************************************************
 函 数 名  : cmd_wpa_connect
 功能描述  : 发起关联
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/

void cmd_wpa_connect(int argc, char *argv[])
{
    struct wpa_assoc_request wpa_assoc_req;
    unsigned char auth_type[32],key[64];
    unsigned char err;

    if (argc < 2)
    {
        HISI_PRINT_ERROR("wpa_connect: parameter is not valid");
        return;
    }

    memset(&wpa_assoc_req , 0 ,sizeof(struct wpa_assoc_request));

    //get hidden_ssid
    wpa_assoc_req.hidden_ssid=atoi(argv[0]);

    //get ssid
    if (strlen(argv[1]) >= sizeof(wpa_assoc_req.ssid))
    {
        HISI_PRINT_ERROR("wpa_connect: parameter 1 length is not valid");
        return;
    }
    strcpy(wpa_assoc_req.ssid,argv[1]);
    HISI_PRINT_ERROR("wpa_connect: ssid: %s",wpa_assoc_req.ssid);

    //get auth_type
    if (strlen(argv[2]) >= sizeof(auth_type))
    {
        HISI_PRINT_ERROR("wpa_connect: parameter 2 length is not valid");
        return;
    }
    strcpy(auth_type,argv[2]);
    if (!strcmp(auth_type, "open"))
    {
        wpa_assoc_req.auth = WPA_SECURITY_OPEN;
    }
    else if (!strcmp(auth_type, "wep"))
    {
        wpa_assoc_req.auth = WPA_SECURITY_WEP;
    }
    else if (!strcmp(auth_type, "wpa"))
    {
        wpa_assoc_req.auth = WPA_SECURITY_WPAPSK;
    }
    else if (!strcmp(auth_type, "wpa2"))
    {
        wpa_assoc_req.auth = WPA_SECURITY_WPA2PSK;
    }
    else if (!strcmp(auth_type, "wpa+wpa2"))
    {
        wpa_assoc_req.auth = WPA_SECURITY_WPAPSK_WPA2PSK_MIX;
    }
    else
    {
        HISI_PRINT_ERROR("Error! Unknwon Authentication Type: %s",auth_type);
        return;
    }

    HISI_PRINT_ERROR("wpa_connect: Authentication Type = %d",wpa_assoc_req.auth);

    //get key
    if (argc >= 4)
    {
        if (strlen(argv[3]) >= sizeof(wpa_assoc_req.key))
        {
            HISI_PRINT_ERROR("wpa_connect: parameter 3 length is not valid");
            return;
        }
        strcpy(wpa_assoc_req.key, argv[3]);

        HISI_PRINT_ERROR("wpa_connect: Key = %s",wpa_assoc_req.key);

    }

    wpa_connect_interface(&wpa_assoc_req);
}
/*****************************************************************************
 函 数 名  : cmd_wpa_disconnect
 功能描述  : 发起去关联
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_wpa_disconnect(int argc, char *argv[])
{
    int l_ret = 0;

    l_ret = wpa_cli_disconnect();
    if (l_ret != 0)
    {
        HISI_PRINT_ERROR("cmd_wpa_disconnect fail.");
    }
}
#if 0
/*****************************************************************************
 函 数 名  : cmd_wpa_get_status
 功能描述  : 获取关联状态
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年6月29日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void cmd_wpa_get_status(int argc, char *argv[])
{
    hisi_wpa_status_stru st_wpa_status;
    FILE          *fp = HISI_NULL;
    unsigned char  ucflag = 0;
    unsigned char *uctmp_path = HISI_NULL;

    if ((1 == argc) && (!strncmp(">", argv[0], strlen(">"))))
    {
        ucflag |= BIT0;
        uctmp_path = WIFI_WPA_GET_STATUS;
    }

    if ((2 == argc) && (!strncmp(">", argv[0], strlen(">"))))
    {
        ucflag |= BIT1;
        uctmp_path = argv[1];
    }

    if (ucflag)
    {
        fp = freopen(uctmp_path, "w", stdout);
        if (HISI_NULL == fp)
        {
            printf("cmd_wpa_get_status, open file failed \n");
            HISI_PRINT("cmd_wpa_get_status, open file failed \n");
        }
    }

    memset(&st_wpa_status , 0 ,sizeof(hisi_wpa_status_stru));

    if (HISI_SUCC == wpa_cli_wpa_get_status(&st_wpa_status))
    {
        if (WPA_COMPLETED == st_wpa_status.ul_status)
        {
            printf("wpa_status:connect ssid:%s \n",st_wpa_status.auc_ssid);
            HISI_PRINT("wpa_status:connect ssid:%s \n",st_wpa_status.auc_ssid);
            printf("The mac address is %2x:%2x:%2x:%2x:%2x:%2x\n",st_wpa_status.auc_bssid[0],st_wpa_status.auc_bssid[1],st_wpa_status.auc_bssid[2],
                        st_wpa_status.auc_bssid[3],st_wpa_status.auc_bssid[4],st_wpa_status.auc_bssid[5]);
            HISI_PRINT("The mac address is %2x:%2x:%2x:%2x:%2x:%2x\n",st_wpa_status.auc_bssid[0],st_wpa_status.auc_bssid[1],st_wpa_status.auc_bssid[2],
                        st_wpa_status.auc_bssid[3],st_wpa_status.auc_bssid[4],st_wpa_status.auc_bssid[5]);
        }
        else
        {
            printf("wpa_status:disconnect!\n");
            HISI_PRINT("wpa_status:disconnect!\n");
        }
    }
    else
    {
        printf("cmd_wpa_get_status fail.\n");
        HISI_PRINT("cmd_wpa_get_status fail.\n");
    }

    if (ucflag)
    {
        fp = freopen(CONSOLE, "w", stdout);
        if (NULL == fp)
        {
            printf("cmd_wpa_get_status, open CONSOLE failed\n");
            HISI_PRINT("cmd_wpa_get_status, open CONSOLE failed\n");
        }
    }

}
#endif
//extern unsigned int g_ul_wlan_resume_state = 0;
//extern unsigned int g_ul_wlan_reusme_wifi_mode = 0;

/*****************************************************************************
 函 数 名  : hisi_get_resume_wifi_mode
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
#if 0
int hisi_get_resume_wifi_mode()
{
   return g_ul_wlan_reusme_wifi_mode;
}

int hisi_set_resume_wifi_mode(int mode)
{
   if(mode >0 && mode < 3)
   {
       g_ul_wlan_reusme_wifi_mode = mode;
   }
   else
   {
       dprintf("hisi_set_resume_wifi_mode fail,mode:%d",mode);
       return -1;
   }

   return 0;
}

int hisi_get_resume_wifi_state()
{
   return g_ul_wlan_resume_state;
}

int hisi_set_resume_wifi_state(int state)
{
   if(state ==0 || state == 1)
   {
       g_ul_wlan_resume_state = state;
   }
   else
   {
       dprintf("hisi_set_resume_wifi_mode fail,state:%d",state);
       return -1;
   }

   return 0;
}
#endif
#if 0
/*****************************************************************************
 函 数 名  : hisi_wifi_resume_process
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
void hisi_wifi_resume_process(void)
{
    printf("{wifi_init, wifi_mode = %d}\n", g_ul_wlan_reusme_wifi_mode);

    if (1 == g_ul_wlan_reusme_wifi_mode)
    {
        cmd_wifi_init_module();
        printf("cmd_wifi_init_module:: end!\n");
        msleep(3000);
        //cmd_start_hapd();
    }
    else if (2 == g_ul_wlan_reusme_wifi_mode)
    {
#ifdef _PRE_WLAN_FEATURE_DATA_BACKUP
        LOS_EventInit(&g_st_backup_event);
#endif

#ifdef CONFIG_NO_CONFIG_WRITE
        //无文件系统
        wpa_supplicant_start("wlan0", "hisi", NULL);
#else
        //文件系统
        wpa_supplicant_start("wlan0", "hisi", WPA_SUPPLICANT_CONFIG_PATH);
#endif

        /*TODO ,move the data bakup in wpa_supplicant to hisi_data_backup_recovery_process*/
#ifdef _PRE_WLAN_FEATURE_DATA_BACKUP
        hisi_data_backup_recovery_process(TYPE_DATA_WIFI_STA);
#endif
        printf("=======cmd_wpa_start:: success!======\n");
    }
    else
    {
        //nothing
    }
}
#endif
extern void wifi_debug_shell_cmd_register(void);

extern int hilink_demo_main(void);
extern int hisi_hilink_online_notice(void);
extern int keepalive_demo_main(int argc, char *argv[]);
extern int keepalive_demo_set_switch(int argc, char *argv[]);
extern int keepalive_demo_get_lose_tcp_id(void);
extern int hsl_demo_main(void);
extern void hi1131_online(void);
extern void hi1131_video(void);
extern int aplink_demo_main(void);
extern int hisi_wlan_set_wakeup_ssid(char *ssid);
extern int hisi_wlan_clear_wakeup_ssid(void);
extern int hisi_wlan_set_wakeup_ssid_ex(char *ssid,unsigned char channel_num,unsigned short pno_period);
/*****************************************************************************
 函 数 名  :hisi_wlan_shell_set_wakeup_ssid
 功能描述  :设置ssid
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年02月27日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_wlan_shell_set_wakeup_ssid(int argc, unsigned char *argv[])
{
    unsigned int       ret;
    unsigned short     us_pno_period   = 0;
    unsigned char      uc_channel_num  = 0;
    if((argc !=1 && argc != 3) || HISI_NULL == argv[0] || strlen(argv[0]) > MAX_SSID_LEN)
    {
        HISI_PRINT_ERROR("param is invalid,argc:%d,argv[0]== NULL:%d!",argc,(argv[0]== NULL));
        return;
    }

    HISI_PRINT_INFO("hisi_wlan_shell_set_wakeup_ssid ssid:%s \r\n",argv[0]);
    if (argc == 1)
    {
        ret = hisi_wlan_set_wakeup_ssid(argv[0]);
        if(HISI_SUCC == ret)
        {
            HISI_PRINT_INFO("succ to  set ssid");
        }
        else
        {
            HISI_PRINT_ERROR("fail to  set ssid,ret:%d!",ret);
        }
    }
    if (argc == 3)
    {
        uc_channel_num  = atoi(argv[1]);
        us_pno_period   = atoi(argv[2]);
        ret = hisi_wlan_set_wakeup_ssid_ex(argv[0],uc_channel_num,us_pno_period);
        if(HISI_SUCC == ret)
        {
            HISI_PRINT_INFO("succ to  set ssid");
        }
        else
        {
            HISI_PRINT_ERROR("fail to  set ssid,ret:%d!",ret);
        }
    }
    return;
}

/*****************************************************************************
 函 数 名  :hisi_get_wps_ssid_key
 功能描述  :获取WPS关联后的SSID和密码
 输入参数  :
 输出参数  :
 返 回 值  :
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   :2017年11月16日
    作    者   :
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_get_wps_ssid_key(void)
{
    char         ac_ssid[33] = {0};
    char         ac_key[65]  = {0};
    unsigned int ul_ssid_len = 0;
    unsigned int ul_key_len  = 0;
    wpa_supplicant_get_wps_ssid_key(ac_ssid,&ul_ssid_len,ac_key,&ul_key_len);
    if (0 != ul_key_len)
    {
        printf("wps:ssid=%s,ssid_len=%d,key=%s,key_len=%d\n",ac_ssid,ul_ssid_len,ac_key,ul_key_len);
    }
    else
    {
        printf("wps:ssid=%s,ssid_len=%d\n",ac_ssid,ul_ssid_len);
    }
}

#ifdef _PRE_WLAN_FEATURE_QUICK_START
extern int hisi_set_psk_demo(void);
void hisi_disable_quick_shell(int argc, char *argv[])
{
    int l_disable_flag;
    l_disable_flag = atoi(argv[0]);
    hisi_disable_quick_start(l_disable_flag);
    return;
}

void hisi_cmd_set_qstart_ssid(int argc, char *argv[])
{
    int     l_ret = -1;
    if (NULL == argv)
    {
        printf("hisi_cmd_set_qstart_ssid::error param\n");
        return;
    }

    l_ret = hisi_set_quick_start_ssid(argv[0], strlen(argv[0]));
    if (0 != l_ret)
    {
        printf("hisi_cmd_set_qstart_ssid::set ssid fail\n");
        return;
    }
}
#endif

/*****************************************************************************
 函 数 名  : hisi_wifi_shell_cmd_register
 功能描述  : 注册wifi shell命令
 输入参数  : oal_void
 输出参数  : 无
 返 回 值  : oal_void
 调用函数  :
 被调函数  :

 修改历史      :
  1.日    期   : 2016年3月22日
    作    者   : 
    修改内容   : 新生成函数

*****************************************************************************/
void hisi_wifi_shell_cmd_register(void)
{
    wifi_debug_shell_cmd_register();
    osCmdReg(CMD_TYPE_EX, "always_tx",  0, (CMD_CBK_FUNC)hisi_cmd_set_always_tx);
    osCmdReg(CMD_TYPE_EX, "always_rx",  0, (CMD_CBK_FUNC)hisi_cmd_set_always_rx);
    osCmdReg(CMD_TYPE_EX, "rx_info",    0, (CMD_CBK_FUNC)hisi_cmd_rx_info);
    osCmdReg(CMD_TYPE_EX, "set_country",  0, (CMD_CBK_FUNC)hisi_cmd_set_country);
    osCmdReg(CMD_TYPE_EX, "get_country",  0, (CMD_CBK_FUNC)hisi_cmd_get_country);
    osCmdReg(CMD_TYPE_EX, "get_channel",  0, (CMD_CBK_FUNC)hisi_cmd_get_channel);
    osCmdReg(CMD_TYPE_EX, "get_station",    0, (CMD_CBK_FUNC)hisi_cmd_get_station);
    osCmdReg(CMD_TYPE_EX, "set_pm_switch",  0, (CMD_CBK_FUNC)hisi_cmd_set_pm_switch);
    osCmdReg(CMD_TYPE_EX, "start_hapd",     0, (CMD_CBK_FUNC)cmd_start_hapd);
    osCmdReg(CMD_TYPE_EX, "stop_hapd",      0, (CMD_CBK_FUNC)cmd_stop_hapd);
    osCmdReg(CMD_TYPE_EX, "wpa_start",      0, (CMD_CBK_FUNC)cmd_wpa_start);
    osCmdReg(CMD_TYPE_EX, "wpa_stop",       0, (CMD_CBK_FUNC)cmd_wpa_stop);
    osCmdReg(CMD_TYPE_EX, "wpa_scan",       0, (CMD_CBK_FUNC)cmd_wpa_scan);
    osCmdReg(CMD_TYPE_EX, "wpa_scan_result",0, (CMD_CBK_FUNC)cmd_wpa_scan_results);
    osCmdReg(CMD_TYPE_EX, "wpa_connect",    5, (CMD_CBK_FUNC)cmd_wpa_connect);
    osCmdReg(CMD_TYPE_EX, "wpa_disconnect", 0, (CMD_CBK_FUNC)cmd_wpa_disconnect);
    //osCmdReg(CMD_TYPE_EX, "hilink", 0, (CMD_CBK_FUNC)hilink_demo_main);
    osCmdReg(CMD_TYPE_EX, "tcp_build", 2, (CMD_CBK_FUNC)keepalive_demo_main);
    osCmdReg(CMD_TYPE_EX, "tcp_switch", 0, (CMD_CBK_FUNC)keepalive_demo_set_switch);
    osCmdReg(CMD_TYPE_EX, "tcp_lose_id", 0, (CMD_CBK_FUNC)keepalive_demo_get_lose_tcp_id);
    osCmdReg(CMD_TYPE_EX, "hisilink", 0, (CMD_CBK_FUNC)hsl_demo_main);
    osCmdReg(CMD_TYPE_EX, "wow_set_ssid", 0, (CMD_CBK_FUNC)hisi_wlan_shell_set_wakeup_ssid);
    osCmdReg(CMD_TYPE_EX, "wow_clear_ssid", 0, (CMD_CBK_FUNC)hisi_wlan_clear_wakeup_ssid);
    osCmdReg(CMD_TYPE_EX, "aplink", 0, (CMD_CBK_FUNC)aplink_demo_main);
    osCmdReg(CMD_TYPE_EX, "wps_ssid_key", 0, (CMD_CBK_FUNC)hisi_get_wps_ssid_key);
    osCmdReg(CMD_TYPE_EX, "wpa_chl_scan",       0, (CMD_CBK_FUNC)cmd_wpa_channel_scan);
#ifdef _PRE_WLAN_FEATURE_QUICK_START
    osCmdReg(CMD_TYPE_EX, "enable_qstart",  0, (CMD_CBK_FUNC)hisi_enable_quick_start);
    osCmdReg(CMD_TYPE_EX, "disable_qstart", 1, (CMD_CBK_FUNC)hisi_disable_quick_shell);
    osCmdReg(CMD_TYPE_EX, "qstart_set_ssid", 1, (CMD_CBK_FUNC)hisi_cmd_set_qstart_ssid);
#if (_HI113X_SW_VERSION == _HI113X_SW_DEBUG)
    osCmdReg(CMD_TYPE_EX, "set_psk",        0, (CMD_CBK_FUNC)hisi_set_psk_demo);
#endif
#endif
#if 0 //sdk中没有这两个命令
    osCmdReg(CMD_TYPE_EX, "online", 0, (CMD_CBK_FUNC)hi1131_online);
    osCmdReg(CMD_TYPE_EX, "video", 0, (CMD_CBK_FUNC)hi1131_video);
#endif
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

