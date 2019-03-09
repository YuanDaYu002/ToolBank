/******************************************************************************
  Copyright (C), 2004-2050, Hisilicon Tech. Co., Ltd.
******************************************************************************
  File Name     : aplink_demo.c
  Version       : Initial Draft
  Author        : Hisilicon WiFi software group
  Created       : 2016-6-16
  Last Modified :
  Description   : aplink sample code
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
#include "driver_hisi_lib_api.h"
#include "hisilink_lib.h"
#include "lwip/sockets.h"
#include "hisilink_ext.h"
#include "asm/delay.h"

#define APLINK_PERIOD_TIMEOUT 70000 //aplink��ʱʱ��

typedef enum
{
    HSL_STATUS_UNCREATE,
    HSL_STATUS_CREATE,
    HSL_STATUS_RECEIVE,
    HSL_STATUS_CONNECT,
    HSL_STATUS_BUTT
}hsl_status_enum;
typedef unsigned char hsl_status_enum_type ;

hsl_result_stru* g_pst_aplink_result;
unsigned int     gul_aplink_taskid = 0;
unsigned int     gul_aplink_timeout_taskid = 0;
unsigned int     gul_aplink_task_flag = 0;
int              gl_aplink_socketfd;

extern hsl_result_stru gst_hsl_params;
/*****************************************************************************
                                  ��������
*****************************************************************************/
extern unsigned char hsl_demo_get_status(void);
extern int hsl_demo_set_status(hsl_status_enum_type uc_type);
extern int hsl_demo_connect_prepare(void);
extern int hsl_demo_prepare(void);

/***************************************************************************
�� �� ��  : aplink_demo_get_result
��������  : ��ȡAP��Ϣ���
�������  : ��
�������  : pst_result����ó��Ľ��
�� �� ֵ  : �ɹ�����0���쳣����-1
���ú���  :
��������  :

�޸���ʷ      :
 1.��    ��   : 2016��11��12��
   ��    ��   : 
   �޸�����   : �����ɺ���
***************************************************************************/
int aplink_demo_get_result(unsigned char* puc_data, hsl_result_stru* pst_result)
{
    unsigned char      uc_ssid_len;
    unsigned char      uc_pwd_len;
    unsigned int       ul_index = 0;

    if ((NULL == puc_data) || (NULL == pst_result))
    {
        HISI_PRINT_ERROR("aplink_get_result:puc_data=%x,pst_result=%x",puc_data,pst_result);
        return -1;
    }

    /* ��ʼ�� */
    memset(pst_result, 0, sizeof(hsl_result_stru));
    /**************************************************************************************/
    /*SSID�ĳ���|PWD�ĳ���|SSID������|PWD������|���ܷ�ʽ|���ͷ�ʽ|�ֻ���IP|�ֻ��˶˿ں�|�豸���ӱ�ʶ|*/
    /*   1�ֽ�  |  1�ֽ�  | �ɱ䳤�� | �ɱ䳤��| 4bits  | 4bits  | 4�ֽ�  |    2�ֽ�   |    2�ֽ�   |*/
    /**************************************************************************************/
    /* ǰ�����ֽڵ����ݷֱ��ʾssid��pwd�ĳ��� */
    uc_ssid_len = puc_data[0];
    uc_pwd_len  = puc_data[1];
    if ((32 < uc_ssid_len) || (128 < uc_pwd_len))
    {
        HISI_PRINT_ERROR("aplink_get_result:ssid_len[%d],pwd_len[%d]",uc_ssid_len, uc_pwd_len);
        return -1;
    }
    ul_index += 2;
    pst_result->uc_ssid_len = uc_ssid_len;
    pst_result->uc_pwd_len  = uc_pwd_len;
    /* ��ȡSSID */
    memcpy(pst_result->auc_ssid, &puc_data[ul_index],uc_ssid_len);
    ul_index += uc_ssid_len;
    /* ��ȡPWD */
    memcpy(pst_result->auc_pwd, &puc_data[ ul_index], uc_pwd_len);
    ul_index += uc_pwd_len;
    /* ��ȡ���ܷ�ʽ */
    pst_result->en_auth_mode = (puc_data[ul_index] & 0xf0) >> 4;
    /*��ȡ����֪ͨ���ķ��ͷ�ʽ*/
    pst_result->en_online_type = puc_data[ul_index++] & 0x0f;
    /* ��ȡ�ֻ���IP��ַ */
    memcpy(pst_result->auc_ip, &puc_data[ul_index], IP_ADDR_LEN);
    ul_index += IP_ADDR_LEN;
    /* ��ȡ�˿ں� */
    pst_result->us_port = (puc_data[ul_index] << 8) + puc_data[ul_index + 1];
    ul_index += 2;
    /* ��ȡ���ӱ�ʶ */
    pst_result->auc_flag[0] = puc_data[ul_index++];
    pst_result->auc_flag[1] = puc_data[ul_index++];
    HISI_PRINT_INFO("ssid:%s\n",pst_result->auc_ssid);
    HISI_PRINT_INFO("auth_mode:%d\n",pst_result->en_auth_mode);
    HISI_PRINT_INFO("phone ip:%d.%d.%d.%d\n",pst_result->auc_ip[0], pst_result->auc_ip[1], \
                pst_result->auc_ip[2], pst_result->auc_ip[3]);
    HISI_PRINT_INFO("port:%d\n",pst_result->us_port);
    HISI_PRINT_INFO("flag:%02x %02x\n",pst_result->auc_flag[0], pst_result->auc_flag[1]);
    memcpy(g_pst_aplink_result, pst_result, sizeof(hsl_result_stru));
    return 0;
}

/***************************************************************************
�� �� ��  : aplink_demo_tcpserver
��������  : ��ͳAPģʽ�µ�TCP���������ڽ����ֻ����͵�AP��Ϣ
�������  :
�������  : ��
�� �� ֵ  : �쳣����-1����������0
���ú���  :
��������  :

�޸���ʷ      :
 1.��    ��   : 2016��11��12��
   ��    ��   : 
   �޸�����   : �����ɺ���
***************************************************************************/
int aplink_demo_tcpserver(void)
{
    int                    sockfd;
    struct sockaddr_in     serveraddr;
    struct sockaddr_in     clientaddr;
    unsigned int           ul_len;
    unsigned char          auc_buf[256];
    hsl_result_stru        st_result;
    unsigned int           ul_ret;
    int                    l_ret;
    unsigned char         *puc_macaddr;

    gl_aplink_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > gl_aplink_socketfd)
    {
        HISI_PRINT_ERROR("%s[%d]:tcp create socket fail",__func__,__LINE__);
        return -1;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&clientaddr, 0, sizeof(clientaddr));
    serveraddr.sin_family      = AF_INET;
    serveraddr.sin_port        = htons(5000);//�������˵Ķ˿ں�
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (0 != bind(gl_aplink_socketfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)))
    {
        HISI_PRINT_ERROR("%s[%d]:tcp bind fail",__func__,__LINE__);
        return -1;
    }
    if (0 != listen(gl_aplink_socketfd, 10))//10����TCP����������ܹ�����10��TCP���Ӽ��������10���ͻ��˵�����
    {
        HISI_PRINT_ERROR("%s[%d]:tcp listen fail",__func__,__LINE__);
        return -1;
    }
    ul_len =  sizeof(struct sockaddr_in);
    sockfd = accept(gl_aplink_socketfd, (struct sockaddr *)&clientaddr, &ul_len);
    if (0 > sockfd)
    {
        HISI_PRINT_ERROR("%s[%d]:tcp connect fail",__func__,__LINE__);
        return -1;
    }

    memset(auc_buf, 0, sizeof(auc_buf));
    recv(sockfd, auc_buf, sizeof(auc_buf), 0);
    /* ��ȡ��� */
        if(0 == aplink_demo_get_result(auc_buf, &st_result))
    {
        LOS_TaskLock();
        memcpy(g_pst_aplink_result, &st_result, sizeof(hsl_result_stru));
        LOS_TaskUnlock();

        /* ��hsl��״̬����ΪCONNECT��֤��������˳�� */
        hsl_demo_set_status(HSL_STATUS_CONNECT);
        puc_macaddr = hisi_wlan_get_macaddr();
        if (HSL_NULL != puc_macaddr)
        {
            if ((puc_macaddr[4] == g_pst_aplink_result->auc_flag[0]) && (puc_macaddr[5] == g_pst_aplink_result->auc_flag[1]))
            {
                l_ret = hsl_demo_connect_prepare();
                if (0 != l_ret)
                {
                    HISI_PRINT_ERROR("aplink_demo_tcpserver:connect prepare fail[%d]\n",l_ret);
                }
            }
            else
            {
                HISI_PRINT_ERROR("This device is not intended to be associated:%02x %02x\n",g_pst_aplink_result->auc_flag[0],g_pst_aplink_result->auc_flag[1]);
            }
        }
    }

    close(gl_aplink_socketfd);
    close(sockfd);
    gul_aplink_task_flag = 0;
    hsl_demo_set_status(HSL_STATUS_UNCREATE);
    /* ɾ��aplink TCP�������߳� */
    ul_ret = LOS_TaskDelete(gul_aplink_taskid);
    if (0 != ul_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:delete task fail[%d]",__func__,__LINE__,ul_ret);
    }
    return 0;
}

/***************************************************************************
�� �� ��  : aplink_demo_timeout_task
��������  : ��ͳAPģʽ�µĳ�ʱ��ʱ���߳�
�������  :
�������  : ��
�� �� ֵ  : �쳣����-1����������0
���ú���  :
��������  :

�޸���ʷ      :
 1.��    ��   : 2017��3��29��
   ��    ��   : 
   �޸�����   : �����ɺ���
***************************************************************************/
int aplink_demo_timeout_task(void)
{
    unsigned int    ul_wait_time = 0;
    unsigned int    ul_start_time;
    unsigned int    ul_current_time;
    unsigned int    ul_ret;

    ul_start_time = hsl_get_time();
    ul_current_time = hsl_get_time();
    while ((0 != gul_aplink_task_flag) && (APLINK_PERIOD_TIMEOUT > ul_wait_time))
    {
        msleep(50);
        ul_current_time = hsl_get_time();
        ul_wait_time = ul_current_time - ul_start_time;
    }

    if (0 != gul_aplink_task_flag)
    {
        printf("timeout,delete aplink task!\n");
        /* ��ʱɾ���߳� */
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        close(gl_aplink_socketfd);
        /* ɾ��aplink TCP�������߳� */
        ul_ret = LOS_TaskDelete(gul_aplink_taskid);
        if (0 != ul_ret)
        {
            HISI_PRINT_WARNING("%s[%d]:delete task fail[%d]",__func__,__LINE__,ul_ret);
        }
    }

    /* ɾ�������߳� */
    ul_ret = LOS_TaskDelete(gul_aplink_timeout_taskid);
    if (0 != ul_ret)
    {
        HISI_PRINT_WARNING("%s[%d]:delete task fail[%d]",__func__,__LINE__,ul_ret);
    }
    return 0;
}

extern int hilink_demo_get_status(void);
/*****************************************************************************
 �� �� ��  : hsl_demo_main
 ��������  : hsl demo���������
 �������  : ��
 �������  : ��
 �� �� ֵ  :
 ���ú���  :
 ��������  :

 �޸���ʷ      :
  1.��    ��   : 2016��11��29��
    ��    ��   : 
    �޸�����   : �����ɺ���

*****************************************************************************/
int aplink_demo_main(void)
{
    unsigned int      ul_ret;
    int               l_ret;
    unsigned char     uc_status;
    TSK_INIT_PARAM_S  st_aplink_task;
    TSK_INIT_PARAM_S  st_aplink_timeout_task;

    uc_status = hsl_demo_get_status();
    if (HSL_STATUS_UNCREATE != uc_status)
    {
        HISI_PRINT_ERROR("aplink already start,cannot start again");
        return -HSL_FAIL;
    }
    /* ��ͳAPģʽ��ȡ�Ľ����hsl��ȡ�Ľ���洢��ͬһλ�� */
    g_pst_aplink_result    = &gst_hsl_params;
    hsl_demo_set_status(HSL_STATUS_CREATE);
    l_ret = hsl_demo_prepare();
    if (0 != l_ret)
    {
        hsl_demo_set_status(HSL_STATUS_UNCREATE);
        HISI_PRINT_ERROR("%s[%d]:demo init fail",__func__,__LINE__);
        return -HSL_FAIL;
    }

    /* ����aplink��TCP�������߳� */
    memset(&st_aplink_task, 0, sizeof(TSK_INIT_PARAM_S));
    st_aplink_task.pfnTaskEntry = (TSK_ENTRY_FUNC)aplink_demo_tcpserver;

    st_aplink_task.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    st_aplink_task.pcName       = "aplink_thread";
    st_aplink_task.usTaskPrio   = 8;
    st_aplink_task.uwResved     = LOS_TASK_STATUS_DETACHED;
    ul_ret = LOS_TaskCreate(&gul_aplink_taskid, &st_aplink_task);
    if(0 != ul_ret)
    {
       hsl_demo_set_status(HSL_STATUS_UNCREATE);
       HISI_PRINT_ERROR("%s[%d]:create task fail[%d]",__func__,__LINE__,ul_ret);
       return -HSL_FAIL;
    }
    gul_aplink_task_flag = 1;

    /* ����aplink�ĳ�ʱ�߳� */
    memset(&st_aplink_timeout_task, 0, sizeof(TSK_INIT_PARAM_S));
    st_aplink_timeout_task.pfnTaskEntry = (TSK_ENTRY_FUNC)aplink_demo_timeout_task;

    st_aplink_timeout_task.uwStackSize  = LOSCFG_BASE_CORE_TSK_DEFAULT_STACK_SIZE;
    st_aplink_timeout_task.pcName       = "aplink_timeout_task";
    st_aplink_timeout_task.usTaskPrio   = 8;
    st_aplink_timeout_task.uwResved     = LOS_TASK_STATUS_DETACHED;
    ul_ret = LOS_TaskCreate(&gul_aplink_timeout_taskid, &st_aplink_timeout_task);
    if(0 != ul_ret)
    {
       hsl_demo_set_status(HSL_STATUS_UNCREATE);
       close(gl_aplink_socketfd);
       /* ɾ��aplink TCP�������߳� */
       ul_ret = LOS_TaskDelete(gul_aplink_taskid);
       if (0 != ul_ret)
       {
           HISI_PRINT_WARNING("%s[%d]:delete task fail[%d]",__func__,__LINE__,ul_ret);
       }
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
