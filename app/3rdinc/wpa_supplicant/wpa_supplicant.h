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

#ifndef _WPA_SUPPLICANT_H_
#define _WPA_SUPPLICANT_H_

struct wpa_supplicant_conf {
    // TODO
};

enum wpa_security_mode {
    WPA_SECURITY_OPEN,
    WPA_SECURITY_WEP,
    WPA_SECURITY_WPAPSK,
    WPA_SECURITY_WPA2PSK,
    WPA_SECURITY_WPAPSK_WPA2PSK_MIX,
};

enum wpa_event {
    WPA_EVT_SCAN_RESULTS,
    WPA_EVT_CONNECTED,
    WPA_EVT_DISCONNECTED,
    WPA_EVT_ADDIFACE,
    WPA_EVT_WRONGKEY
};

struct wpa_assoc_request {
    char ssid[33];
    enum wpa_security_mode auth;
    char key[128];
    unsigned int hidden_ssid;
};

struct wpa_ap_info {
    char ssid[32*4+1];
    char bssid[18];
    unsigned int channel;
    enum wpa_security_mode auth;
    int rssi;
};

typedef void (*wpa_event_cb)(enum wpa_event event);

extern int wpa_supplicant_start(char *ifname, char *driver, struct wpa_supplicant_conf *conf);

extern int wpa_supplicant_stop(void);

extern int wpa_register_event_cb(wpa_event_cb func);

extern int wpa_cli_scan(void);

extern int wpa_cli_scan_results(struct wpa_ap_info *results, unsigned int *num);

extern int wpa_cli_connect(struct wpa_assoc_request *assoc);

extern int wpa_cli_disconnect(void);

extern int wpa_cli_wps_pbc(char *bssid);

extern int wpa_cli_wps_pin(char *pin, char *bssid);

extern int wpa_supplicant_get_wps_ssid_key(char *ssid, unsigned int *ssid_len, char *key, unsigned int *key_len);

#endif
