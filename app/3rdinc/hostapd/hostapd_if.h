/*----------------------------------------------------------------------------
 * Copyright (c) <2013-2015>, <Huawei Technologies Co., Ltd>
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * Notice of Export Control Law
 * ===============================================
 * Huawei Liteos may be subject to applicable export control laws and regulations, which might
 * include those applicable to Huawei Liteos of U.S. and the country in which you are located.
 * Import, export and usage of Huawei Liteos in any manner by you shall be in compliance with such
 * applicable export control laws and regulations.
 *---------------------------------------------------------------------------*/

#ifndef _HOSTAPD_IF_H_
#define _HOSTAPD_IF_H_

#include "common/defs.h"

#define MAX_ESSID_LEN 32

enum hostapd_security_mode {
    HOSTAPD_SECURITY_OPEN,
    HOSTAPD_SECURITY_WPAPSK,
    HOSTAPD_SECURITY_WPA2PSK,
    HOSTAPD_SECURITY_WPAPSK_WPA2PSK_MIX,
};

#define CONFIG_DRIVER_HI1131
#ifdef CONFIG_DRIVER_HI1131
enum hostapd_event
{
    HOSTAPD_EVT_ENABLED,
    HOSTAPD_EVT_DISABLED,
    HOSTAPD_EVT_CONNECTED,
    HOSTAPD_EVT_DISCONNECTED,
};

typedef void (*hostapd_event_cb)(enum hostapd_event event);
#endif


struct hostapd_conf {
    unsigned char  bssid[6];
    char ssid[MAX_ESSID_LEN + 1];
    unsigned char  channel_num;
    int wpa_key_mgmt;
    int wpa_pairwise;
    int authmode;
    unsigned char key[128];
    unsigned char macaddr_acl;
    int auth_algs;
    unsigned char  wep_idx;
    int wpa;
    char driver[16];
#ifdef CONFIG_DRIVER_HI1131
    /* ����ssid����ʹ�ã��������ļ�������Ч��ɾ�� */
    int ignore_broadcast_ssid;

    /*���ô���ʹ�ã��������ļ�������Ч��ɾ�� */
    char ht_capab[20];
#endif /*CONFIG_DRIVER_HI1131*/
};

int hostapd_start(char *ifname, struct hostapd_conf *hconf);

int hostapd_stop(void);
#ifdef CONFIG_DRIVER_HI1131
int hostapd_register_event_cb(hostapd_event_cb func);
#endif
#endif
