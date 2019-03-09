#include "wifi_hi1131s.h"

#include "los_event.h"
#include "los_printf.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "eth_drv.h"
#include "arch/perf.h"
#include "wpa_supplicant/wpa_supplicant.h"
#include "hostapd/hostapd_if.h"
#include "mmc/host.h"
#include "gpio.h"
#include "hisilink_lib.h"
#include "hilink_link.h"
#include "driver_hisi_lib_api.h"
#include "linux/completion.h"


typedef enum
{
    HSL_STATUS_UNCREATE,
    HSL_STATUS_CREATE,
    HSL_STATUS_RECEIVE,
    HSL_STATUS_CONNECT,
    HSL_STATUS_BUTT
}hsl_status_enum;

typedef enum
{
    HILINK_STATUS_UNCREATE,
    HILINK_STATUS_RECEIVE, //hilink处于接收组播阶段
    HILINK_STATUS_CONNECT, //hilink处于关联阶段
    HILINK_STATUS_BUTT
}hilink_status_enum;

typedef struct {
    unsigned int wlan_irq;

    unsigned int wifi_data_intr_gpio_group;
    unsigned int wifi_data_intr_gpio_offset;

    unsigned int dev_wak_host_gpio_group;
    unsigned int dev_wak_host_gpio_offset;

    unsigned int host_wak_dev_gpio_group;
    unsigned int host_wak_dev_gpio_offset;

    void (*wifi_power_set)(unsigned char);
    void (*wifi_rst_set)(unsigned char);
    void (*wifi_sdio_detect)(void);
    void (*host_pow_off)(void);
}BOARD_INFO;


#define DHCP_CHECK_CNT                                  30
#define DHCP_CHECK_TIME                                 1000
#define DHCP_IP_SET                                     0
#define DHCP_IP_DEL                                     1

#define WLAN_FILE_STORE_MIN_SIZE                        (0)
#define WLAN_FILE_STORE_MID_SIZE                        (0x30000)
#define WLAN_FILE_STORE_MAX_SIZE                        (0x70000)
#define WLAN_FILE_STORE_BASEADDR                        (0x750000)

#define WIFI_IRQ                                        NUM_HAL_INTERRUPT_GPIO
#define WIFI_SDIO_INDEX                                 (1)

#define REG_MUXCTRL_SDIO0_CLK_MAP                       (IO_MUX_REG_BASE + 0x7c)
#define REG_MUXCTRL_SDIO0_DETECT_MAP                    (IO_MUX_REG_BASE + 0x68)
#define REG_MUXCTRL_SDIO0_CDATA1_MAP                    (IO_MUX_REG_BASE + 0x74)
#define REG_MUXCTRL_SDIO0_CDATA0_MAP                    (IO_MUX_REG_BASE + 0x70)
#define REG_MUXCTRL_SDIO0_CDATA3_MAP                    (IO_MUX_REG_BASE + 0x80)
#define REG_MUXCTRL_SDIO0_CCMD_MAP                      (IO_MUX_REG_BASE + 0x78)
#define REG_MUXCTRL_SDIO0_CDATA2_MAP                    (IO_MUX_REG_BASE + 0x84)

#define REG_MUXCTRL_SDIO1_CLK_MAP                       (IO_MUX_REG_BASE + 0x0b8)
#define REG_MUXCTRL_SDIO1_DETECT_MAP                    (IO_MUX_REG_BASE + 0x0a8)
#define REG_MUXCTRL_SDIO1_CDATA1_MAP                    (IO_MUX_REG_BASE + 0x0aC)
#define REG_MUXCTRL_SDIO1_CDATA0_MAP                    (IO_MUX_REG_BASE + 0x0b0)
#define REG_MUXCTRL_SDIO1_CDATA3_MAP                    (IO_MUX_REG_BASE + 0x0b4)
#define REG_MUXCTRL_SDIO1_CCMD_MAP                      (IO_MUX_REG_BASE + 0x0a0)
#define REG_MUXCTRL_SDIO1_CDATA2_MAP                    (IO_MUX_REG_BASE + 0x098)

#define WIFI_PWR_ON_GPIO_GROUP                          (1)
#define WIFI_PWR_ON_GPIO_OFFSET                         (0)

#define WIFI_RST_GPIO_GROUP                             (1)
#define WIFI_RST_GPIO_OFFSET                            (6)

#define WIFI_DATA_INTR_GPIO_GROUP                       (5)
#define WIFI_DATA_INTR_GPIO_OFFSET                      (7)

#define HOST_WAK_DEV_GPIO_GROUP                         (1)
#define HOST_WAK_DEV_GPIO_OFFSET                        (2)

//#define WIFI_WAK_FLAG_GPIO_GROUP                        (3)
//#define WIFI_WAK_FLAG_GPIO_OFFSET                       (1)

#define DEV_WAK_HOST_GPIO_GROUP                         (1)
#define DEV_WAK_HOST_GPIO_OFFSET                        (5)

#define REG_MUXCTRL_WIFI_DATA_INTR_GPIO_MAP             (IO_MUX_REG_BASE + 0x0a8)
#define REG_MUXCTRL_HOST_WAK_DEV_GPIO_MAP               (IO_MUX_REG_BASE + 0x048)
//#define REG_MUXCTRL_WIFI_WAK_FLAG_GPIO_MAP              (IO_MUX_REG_BASE + 0x090)
#define REG_MUXCTRL_DEV_WAK_HOST_GPIO_MAP               (IO_MUX_REG_BASE + 0x054)
#define REG_MUXCTRL_WIFI_PWR_ON_GPIO_MAP                (IO_MUX_REG_BASE + 0x040)
#define REG_MUXCTRL_WIFI_RST_GPIO_MAP                   (IO_MUX_REG_BASE + 0x058)


#define WPA_SUPPLICANT_CONFIG_PATH  "/jffs0/etc/hisi_wifi/wifi/wpa_supplicant.conf"


extern hisi_rf_customize_stru g_st_rf_customize;
extern void mcu_uart_proc(void);
extern void hisi_wifi_shell_cmd_register(void);
extern BOARD_INFO * get_board_info(void);
extern void wlan_resume_state_set(unsigned int ul_state);
extern void HI_HAL_MCUHOST_WiFi_Clr_Flag(void);
extern void HI_HAL_MCUHOST_WiFi_Power_Set(unsigned char val);
extern void HI_HAL_MCUHOST_WiFi_Rst_Set(unsigned char val);
extern unsigned char hsl_demo_get_status(void);
extern hsl_result_stru* hsl_demo_get_result(void);
extern unsigned char hilink_demo_get_status(void);
extern hsl_result_stru* hilink_demo_get_result(void);
extern void hisi_reset_addr(void);
extern int hilink_demo_online(hilink_s_result* pst_result);
extern int hsl_demo_online(hsl_result_stru* pst_params);
extern void start_dhcps(void);

struct timer_list   hisi_dhcp_timer;
unsigned int        check_ip_loop = 0;
struct netif       *pwifi = NULL;


void hi_rf_customize_init(void)
{
    /*11b scaling 功率，共4字节，低字节到高字节对应1m、2m、5.5m、11m，值越大，11b模式发射功率越高*/
    g_st_rf_customize.l_11b_scaling_value              = 0x9c9c9c9c;
    /*11g scaling 功率，共4字节，低字节到高字节对应6m、9m、12m、18m，值越大，11g模式发射功率越高*/
    g_st_rf_customize.l_11g_u1_scaling_value           = 0x5b5b5b5b;
    /*11g scaling 功率，共4字节，低字节到高字节对应24m、36m、48m、54m，值越大，11g模式发射功率越高*/
    g_st_rf_customize.l_11g_u2_scaling_value           = 0x665b5b5b;
    /*11n 20m scaling 功率，共4字节，低字节到高字节对应mcs4、mcs5、mcs6、mcs7，值越大，11n模式发射功率越高*/
    g_st_rf_customize.l_11n_20_u1_scaling_value        = 0x57575757;
    /*11n 20m scaling 功率，共4字节，低字节到高字节对应mcs0、mcs1、mcs2、mcs3，值越大，11n模式发射功率越高*/
    g_st_rf_customize.l_11n_20_u2_scaling_value        = 0x5c5c5757;
    /*11n 40m scaling 功率，共4字节，低字节到高字节对应mcs4、mcs5、mcs6、mcs7，值越大，11n模式发射功率越高*/
    g_st_rf_customize.l_11n_40_u1_scaling_value        = 0x5a5a5a5a;
    /*11n 40m scaling 功率，共4字节，低字节到高字节对应mcs0、mcs1、mcs2、mcs3，值越大，11n模式发射功率越高*/
    g_st_rf_customize.l_11n_40_u2_scaling_value        = 0x5d5d5a5a;

    /*1-4信道upc功率调整，值越大，该信道功率越高*/
    g_st_rf_customize.l_ban1_ref_value                  = 50;
    /*5-9信道upc功率调整，值越大，该信道功率越高*/
    g_st_rf_customize.l_ban2_ref_value                  = 49;
    /*10-13信道upc功率调整，值越大，该信道功率越高*/
    g_st_rf_customize.l_ban3_ref_value                  = 50;

    /*40M带宽bypass，置1禁用40M带宽，写0使用40M带宽*/
    g_st_rf_customize.l_disable_bw_40                   = 1;
    /*低功耗开关，写1打开低功耗，写0关闭低功耗*/
    g_st_rf_customize.l_pm_switch                       = 0;
    /*dtim设置, 范围1-10*/
    g_st_rf_customize.l_dtim_setting                    = 1;


    /*如需定制化，请置1，否则请置0*/
    g_st_rf_customize.l_customize_enable                = 1;

}

void hi_wifi_no_mcu_gpio_init(void)
{
    gpio_dir_config(WIFI_PWR_ON_GPIO_GROUP, WIFI_PWR_ON_GPIO_OFFSET, GPIO_DIR_OUT);
    gpio_dir_config(WIFI_RST_GPIO_GROUP, WIFI_RST_GPIO_OFFSET, GPIO_DIR_OUT);
}

void hi_wifi_power_set(unsigned char val)
{
    gpio_write(WIFI_PWR_ON_GPIO_GROUP, WIFI_PWR_ON_GPIO_OFFSET, val);
    //HI_HAL_MCUHOST_WiFi_Power_Set(val);
}
void hi_wifi_rst_set(unsigned char val)
{
    gpio_write(WIFI_RST_GPIO_GROUP, WIFI_RST_GPIO_OFFSET, val);
    //HI_HAL_MCUHOST_WiFi_Rst_Set(val);
}

void hi_wifi_register_init(void)
{
    unsigned int value = 0;

    /*配置管脚复用*/
    if (0 == WIFI_SDIO_INDEX)
    {
        value = 0x0001;
        writel(value, REG_MUXCTRL_SDIO0_CLK_MAP);
        //writel(value, REG_MUXCTRL_SDIO0_DETECT_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA1_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA0_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA3_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CCMD_MAP);
        writel(value, REG_MUXCTRL_SDIO0_CDATA2_MAP);
    }
    else if (1 == WIFI_SDIO_INDEX)
    {
        value = 0X0002;
        writel(value, REG_MUXCTRL_SDIO1_CLK_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA1_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA0_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA3_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CCMD_MAP);
        writel(value, REG_MUXCTRL_SDIO1_CDATA2_MAP);
    }

    value = 0x0;
    writel(value, REG_MUXCTRL_WIFI_DATA_INTR_GPIO_MAP);
    writel(value, REG_MUXCTRL_HOST_WAK_DEV_GPIO_MAP);
    //writel(value, REG_MUXCTRL_WIFI_WAK_FLAG_GPIO_MAP);
    writel(value, REG_MUXCTRL_DEV_WAK_HOST_GPIO_MAP);
    writel(value, REG_MUXCTRL_WIFI_RST_GPIO_MAP);
    writel(value, REG_MUXCTRL_WIFI_PWR_ON_GPIO_MAP);

    gpio_dir_config(HOST_WAK_DEV_GPIO_GROUP, HOST_WAK_DEV_GPIO_OFFSET, 1);
    gpio_write(HOST_WAK_DEV_GPIO_GROUP, HOST_WAK_DEV_GPIO_OFFSET, 0);

    hi_wifi_power_set(0);
    msleep(10);
    hi_wifi_power_set(1);
    hi_wifi_rst_set(1);
}

void himci_wifi_sdio_detect_trigger(void)
{
    unsigned int reg_value = 0;

    struct mmc_host *mmc = NULL;
    mmc = get_mmc_host(WIFI_SDIO_INDEX);
    struct himci_host *host = (struct himci_host *)mmc->priv;

    if (0 == WIFI_SDIO_INDEX)
    {
        writel(0x0001, REG_MUXCTRL_SDIO0_DETECT_MAP);
    }
    else if (1 == WIFI_SDIO_INDEX)
    {
        himci_mmc_rescan(WIFI_SDIO_INDEX);
    }

    hi_mci_pad_ctrl_cfg(host, SIGNAL_VOLT_1V8);
}

void hi_host_pow_off(void)
{
    //HI_HAL_MCUHOST_PowreOff_Request();
}

void hi_wifi_board_info_register(void)
{
    BOARD_INFO *wlan_board_info = get_board_info();
    if (NULL == wlan_board_info)
    {
        dprintf("wifi_board_info is NULL!\n");
        return;
    }

    wlan_board_info->wlan_irq = WIFI_IRQ;

    wlan_board_info->wifi_data_intr_gpio_group = WIFI_DATA_INTR_GPIO_GROUP;
    wlan_board_info->wifi_data_intr_gpio_offset = WIFI_DATA_INTR_GPIO_OFFSET;

    wlan_board_info->dev_wak_host_gpio_group = DEV_WAK_HOST_GPIO_GROUP;
    wlan_board_info->dev_wak_host_gpio_offset = DEV_WAK_HOST_GPIO_OFFSET;

    wlan_board_info->host_wak_dev_gpio_group = HOST_WAK_DEV_GPIO_GROUP;
    wlan_board_info->host_wak_dev_gpio_offset = HOST_WAK_DEV_GPIO_OFFSET;

    wlan_board_info->wifi_power_set = hi_wifi_power_set;
    wlan_board_info->wifi_rst_set = hi_wifi_rst_set;
    wlan_board_info->wifi_sdio_detect = himci_wifi_sdio_detect_trigger;
    wlan_board_info->host_pow_off = hi_host_pow_off;
}

void hi_wifi_check_wakeup_flag(void)
{
    int wak_flag = 0;
    //gpio_dir_config(WIFI_WAK_FLAG_GPIO_GROUP, WIFI_WAK_FLAG_GPIO_OFFSET, 0);
    //wak_flag = gpio_read(WIFI_WAK_FLAG_GPIO_GROUP, WIFI_WAK_FLAG_GPIO_OFFSET);
    //HI_HAL_MCUHOST_WiFi_Clr_Flag();
    wlan_resume_state_set(wak_flag);
}

struct databk_addr_info *hi_wifi_databk_info_cb(void)
{
    /* 暂时写死，由应用侧补齐 */
    struct databk_addr_info *wlan_databk_addr_info = NULL;
    wlan_databk_addr_info = malloc(sizeof(struct databk_addr_info));
    if (NULL == wlan_databk_addr_info)
    {
        dprintf("hi_wifi_databk_info_cb:wlan_databk_addr_info is NULL!\n");
        return NULL;
    }
    memset(wlan_databk_addr_info, 0, sizeof(struct databk_addr_info));
    wlan_databk_addr_info->databk_addr = 0x7d0000;
    wlan_databk_addr_info->databk_length = 0x20000;
    wlan_databk_addr_info->get_databk_info = NULL;
    return wlan_databk_addr_info;
}

void hi_wifi_register_backup_addr(void)
{
    struct databk_addr_info *wlan_databk_addr_info = hisi_wlan_get_databk_addr_info();
    if (NULL == wlan_databk_addr_info)
    {
        dprintf("hi_wifi_register_backup_addr:wlan_databk_addr_info is NULL!\n");
        return;
    }
    /* 暂时写死，由应用侧补齐 */
    wlan_databk_addr_info->databk_addr = 0x7d0000;
    wlan_databk_addr_info->databk_length = 0x10000;
    wlan_databk_addr_info->get_databk_info = hi_wifi_databk_info_cb;
}

void hi_wifi_no_fs_init(void)
{
    hisi_wlan_no_fs_config(WLAN_FILE_STORE_BASEADDR, WLAN_FILE_STORE_MIN_SIZE);
}

void hi_wifi_pre_proc(void)
{
    hi_wifi_no_mcu_gpio_init();
    hi_wifi_register_init();
    hi_wifi_board_info_register();
    //hi_wifi_check_wakeup_flag();
    hi_wifi_register_backup_addr();
    hi_wifi_no_fs_init();
    hi_rf_customize_init();
}

struct completion  dhcp_complet;
struct completion  wpa_ready_complet;

void hi_check_dhcp_success(unsigned long para)
{
    para = para;
    int             ret    = 0;
    unsigned int     ipaddr = 0;

    ret = dhcp_is_bound(pwifi);
    if (0 == ret)
    {
        /* IP获取成功后通知wifi驱动 */
        dprintf("\n\n DHCP SUCC\n\n");
        hisi_wlan_ip_notify(ipaddr, DHCP_IP_SET);
        if (HSL_STATUS_UNCREATE != hsl_demo_get_status())
        {
            complete(&dhcp_complet);
        }
        del_timer(&hisi_dhcp_timer);
        return;
    }

    if (check_ip_loop++ > DHCP_CHECK_CNT)
    {
        /* IP获取失败执行去关联 */
        dprintf("\n\n DHCP FAILED\n\n");
        wpa_cli_disconnect();
        del_timer(&hisi_dhcp_timer);
        return;
    }

    /* 重启查询定时器 */
    add_timer(&hisi_dhcp_timer);
    return;
}

unsigned int test_get_time(void)
{
    hsl_uint32 ul_time = 0;
    struct timeval tv;

    gettimeofday(&tv, HSL_NULL);
    ul_time = tv.tv_usec / 1000 + tv.tv_sec * 1000;
    return ul_time;
}

extern int hisi_get_psk_demo(void);
extern int hisi_set_psk_demo(void);

extern int hsl_demo_connect(hsl_result_stru* pst_params);
extern int hilink_demo_connect(hilink_s_result* pst_result);
void hisi_wifi_event_cb(enum wpa_event event)
{
    unsigned char    uc_status;
    unsigned int     ipaddr = 0;
    hsl_result_stru *pst_hsl_result;
    hilink_s_result *pst_hilink_result;
    printf("wifi_event_cb,event:%d\n",event);

    if(pwifi == 0)
        return ;

    switch(event) {
        case WPA_EVT_SCAN_RESULTS:
            printf("Scan results available\n");
        break;
        case WPA_EVT_CONNECTED:
            printf("WiFi: Connected\n");
            /* 启动dhcp获取IP */
            netifapi_dhcp_stop(pwifi);
            netifapi_dhcp_start(pwifi);

            /* 查询IP是否获取 */
            check_ip_loop = 0;
            init_timer(&hisi_dhcp_timer);
            hisi_dhcp_timer.expires = LOS_MS2Tick(DHCP_CHECK_TIME);
            hisi_dhcp_timer.function = hi_check_dhcp_success;
            add_timer(&hisi_dhcp_timer);
            msleep(500);
            break;
        case WPA_EVT_DISCONNECTED:
            printf("WiFi: disconnect\n");
            netifapi_dhcp_stop(pwifi);
            hisi_reset_addr();
            hisi_wlan_ip_notify(ipaddr, DHCP_IP_DEL);
            break;
        case WPA_EVT_ADDIFACE:
            //wpa_supplicant创建成功
            complete(&wpa_ready_complet);
            /* 判断是否为hisi_link创建的WPA成功 */
            uc_status = hsl_demo_get_status();
            printf("hsl_status=%d\n",uc_status);
            if (HSL_STATUS_CONNECT == uc_status)
            {
                pst_hsl_result = hsl_demo_get_result();
                if (NULL != pst_hsl_result)
                {
                    hsl_demo_connect(pst_hsl_result);
                    break;
                }
            }
            break;
        case WPA_EVT_WRONGKEY:
            printf("maybe key is wrong\n");
            break;
        default:
            break;
    }
}

void hisi_reset_addr(void)
{
    ip_addr_t        st_gw;
    ip_addr_t        st_ipaddr;
    ip_addr_t        st_netmask;
    struct netif    *pst_lwip_netif;

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

extern int hsl_demo_init(void);
extern int hilink_demo_init(void);
void hisi_wifi_hostapd_event_cb(enum hostapd_event event)
{
    unsigned char uc_status;

    printf("hostapd_event=%d\n",event);
    switch(event)
    {
        case HOSTAPD_EVT_ENABLED:
            //hostapd创建成功
            start_dhcps();
            /* 判断是否起hisilink */
            uc_status = hsl_demo_get_status();
            printf("hsl_status=%d\n",uc_status);
            if (HSL_STATUS_RECEIVE == uc_status)
            {
                hsl_demo_init();
            }
            break;
        case HOSTAPD_EVT_DISABLED:
            //hostapd删除成功
            hisi_reset_addr();
            break;
        case HOSTAPD_EVT_CONNECTED:
            //用户关联成功
            break;
        case HOSTAPD_EVT_DISCONNECTED:
            //用户去关联
            break;
        default:
            break;
    }
}

void hi1131s_init(void)
{
    hi_wifi_pre_proc();

    dprintf("porc fs init ...\n");
    proc_fs_init();

    wpa_register_event_cb(hisi_wifi_event_cb);
    hostapd_register_event_cb(hisi_wifi_hostapd_event_cb);
    int ret = hisi_wlan_wifi_init(&pwifi);
    if (0 != ret) {
        dprintf("fail to start hi1131 wifi\n");
        if(pwifi != NULL)
            hisi_wlan_wifi_deinit();
    }
}