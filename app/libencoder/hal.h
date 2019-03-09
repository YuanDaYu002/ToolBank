
#ifndef HLE_HAL_H
#define HLE_HAL_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "hi_comm_video.h"

#include "typeport.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*
    读指定大小的数据，如果出错则返回-1。
    如果读到文件末尾，返回的大小有可能小于指定的大小
     */
    ssize_t hal_readn(int fd, void *vptr, size_t n);

    /*
    写指定大小的数据，如果出错则返回-1。
     */
    ssize_t hal_writen(int fd, const void *vptr, size_t n);



    /*
        function:  hal_init
        description:  硬件抽象层初始化函数
        args:
            pack_count 编码缓冲帧数
        return:
            0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_init(HLE_S32 pack_count, VIDEO_STD_E video_std);
    void hal_exit(void);

    /*
        function:  hal_set_capture_size
        description:  设置SENSOR捕获视频大小，LS1301项目专用接口
        args:
            int size 视频大小，IMAGE_SIZE_1280X960或IMAGE_SIZE_HD
        return:
            0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_set_capture_size(int size);


    /*
        function:  hal_get_time
        description:  获取系统时间
        args:
            HLE_SYS_TIME *sys_time[out]
        return:
            0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_get_time(HLE_SYS_TIME *sys_time, int utc, HLE_U8* wday);

    /*
        function:  hal_set_time
        description:  设置系统时间，并将时间设置到RTC，计算编码时间戳偏移
        args:
            HLE_SYS_TIME *sys_time[in]
        return:
            0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_set_time(HLE_SYS_TIME *sys_time, int utc);

    /*
        function:  set_date_string_format
        description:  设置日期字符串的格式
        args:
            E_DATE_DISPLAY_FMT date_fmt[in]
        return:
               无
     */
    void hal_set_date_format(E_DATE_DISPLAY_FMT date_fmt);

    /*
        function:  hal_get_defaultkey_status
        description:  获取到恢复出厂状态按键的状态
        args:
            无
        return:
           1，按键按下
           0，按键弹起
     */
    int hal_get_defaultkey_status(void);

#if 0
    /*
        function:  hal_get_project_name
        description:  获取项目名称
        args:
            无
        return:
            项目名称字符串
     */
    const char* hal_get_project_name(void);
#endif

    /*
        function:  hal_get_max_preview_count
        description:  获取设备最多能够同时远程预览的路数，具体路数和设备性能有关
        args:
            无
        return:
            设备最多能够同时远程预览的路数
     */
    int hal_get_max_preview_count(void);


    /*
        function:  hal_get_unique_id
        description: 得到加密芯片的 unique_id
        args:
            unsigned char *chip_id[out]
            unsigned int *chip_id_len
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义

     */
    int hal_get_unique_id(unsigned char *unique_id, unsigned int *unique_id_len);

    /*
        function:  hal_write_serial_no
        description:  往加密芯片里面写入设备 SN
        args:
            unsigned char *sn_data[in]
            unsigned int sn_len[in]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_write_serial_no(unsigned char *sn_data, unsigned int sn_len);

    /*
        function:  hal_read_serial_no
        description:  从加密芯片里面读出设备 SN
        args:
            unsigned char *sn_data[out]
            unsigned int sn_len[in]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_read_serial_no(unsigned char *sn_data, unsigned int sn_len);

    /*
        function:  hal_write_mac_addr
        description:  往加密芯片里面写入设备 MAC 地址
        args:
            unsigned char *mac_data[in]
            unsigned int mac_len[in]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_write_mac_addr(unsigned char *mac_data, unsigned int mac_len);

    /*
        function:  hal_read_mac_addr
        description:  从加密芯片里面读出设备 MAC 地址
        args:
            unsigned char *mac_data[out]
            unsigned int mac_len[in]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_read_mac_addr(unsigned char *mac_data, unsigned int mac_len);

    /*
        function:  hal_write_encrypt_key
        description:  往加密芯片里面写入密钥
        args:
            unsigned char *encrypt_key_data[in]
            unsigned int encrypt_key_len[in]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_write_encrypt_key(unsigned char *encrypt_key_data, unsigned int encrypt_key_len);

    /*
        function:  hal_read_encrypt_key
        description:  从加密芯片里面读出密钥
        args:
            unsigned char *encrypt_key_data[out]
            unsigned int encrypt_key_len[in]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_read_encrypt_key(unsigned char *encrypt_key_data, unsigned int encrypt_key_len);

    /*
        function:  hal_get_temperature
        description:  读取温度值
        args:
            int *val[out]
        return:
            =0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_get_temperature(int *val);

    /*
        function:  hal_reboot
        description:  重启
        args:
            无
        return:
            0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_reboot(void);

    /*
        function:  hal_shutdown
        description:  关机
        args:
            无
        return:
            0, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int hal_shutdown(void);


    /*
        function:  get_one_JPEG_frame
        description:  获取一个JPEG图像，media server 请求JPEG图像时回调该函数
        args:
            frame_addr：返回 获取到的帧首地址
            length：返回 帧长度
        return:
            HLE_RET_OK, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    HLE_S32 get_one_JPEG_frame(const void*frame_addr,HLE_S32 length);

    /*
        function:  get_one_encoded_frame
        description:  获取一个编码帧，media server 请求编码帧时回调该函数
        args:
            stream_id(传入) :码流编号
            have_audio(传入):是否有音频帧
            pack_addr（返回）:帧所在的包首地址
            frame_addr（返回）：获取到的帧首地址，用于发送完成后释放数据包
            length（返回）： 帧长度
        return:
            HLE_RET_OK, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */

    HLE_S32 get_one_encoded_frame(HLE_S32 stream_id,HLE_S8 have_audio,void**pack_addr, void**frame_addr,HLE_S32* frame_length);

    /*
        function:  media_server_module_init
        description:  媒体服务程序初始化
        args:
            无
        return:
            HLE_RET_OK, 成功
            <0, 失败，返回值为错误码，具体见错误码定义
     */
    int media_server_module_init(void);

#ifdef __cplusplus
}
#endif

#endif




