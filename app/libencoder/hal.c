



#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
//#include <sys/reboot.h>
#include <sys/time.h>

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "hi_comm_video.h"

#include "typeport.h"
#include "hal.h"
#include "encoder.h"
//#include "misc.h"
#include "watchdog.h"
#include "hal_def.h"
#include "video.h"
//#include "record.h"
#include "sdp.h"
#include "audio.h"
#include "motion_detect.h"
#include "media_server_interface.h"
#include "hi_comm_aio.h"
#include "fmp4_encode.h"
#include "itfEncoder.h"






#if 0

const char * hal_get_project_name(void)
{
    return HAL_PROJECT_NAME;
}
#endif

int hal_get_max_preview_count(void)
{
    return sdp_max_prev_count();
}

static void mpp_sys_exit(void)
{
    HI_MPI_SYS_Exit();
    HI_MPI_VB_Exit();
}

static int calc_pic_vbblk_size(int width, int height, int align)
{
    if (16 != align && 32 != align && 64 != align) {
        return -1;
    }


    return (CEILING_2_POWER(width, align) * CEILING_2_POWER(height, align) * 3 / 2);
}

static int mpp_sys_init(void)
{
#define SYS_ALIGN_WIDTH  16

    HLE_S32 ret;

    mpp_sys_exit();

    VB_CONF_S vb_conf; /* vb config define */
    memset(&vb_conf, 0, sizeof (VB_CONF_S));
    vb_conf.u32MaxPoolCnt = VB_MAX_POOLS;
    vb_conf.astCommPool[0].u32BlkSize = calc_pic_vbblk_size(1920, 1088, SYS_ALIGN_WIDTH);
    //vb_conf.astCommPool[0].u32BlkCnt = 6;
    vb_conf.astCommPool[0].u32BlkCnt = 5;
    //vb_conf.astCommPool[1].u32BlkSize = calc_pic_vbblk_size(960, 544, SYS_ALIGN_WIDTH);
    //vb_conf.astCommPool[1].u32BlkCnt = 6;
    vb_conf.astCommPool[1].u32BlkSize = calc_pic_vbblk_size(480, 272, SYS_ALIGN_WIDTH);
    //vb_conf.astCommPool[1].u32BlkCnt = 10;
    vb_conf.astCommPool[1].u32BlkCnt = 6;
    ret = HI_MPI_VB_SetConf(&vb_conf);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("HI_MPI_VB_SetConf fail: %#x.\n", ret);
        return HLE_RET_ERROR;
    }

    ret = HI_MPI_VB_Init();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("HI_MPI_VB_Init fail: %#x.\n", ret);
        return HLE_RET_ERROR;
    }

    MPP_SYS_CONF_S sys_conf;
    sys_conf.u32AlignWidth = SYS_ALIGN_WIDTH;
    ret = HI_MPI_SYS_SetConf(&sys_conf);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("HI_MPI_SYS_SetConf fail: %#x.\n", ret);
        return HLE_RET_ERROR;
    }

    ret = HI_MPI_SYS_Init();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("HI_MPI_SYS_Init fail: %#x.\n", ret);
        return HLE_RET_ERROR;
    }

    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

static int max_frame_rate;

int hal_set_video_std(VIDEO_STD_E video_std)
{
    if (!((video_std == VIDEO_STD_NTSC) || (video_std == VIDEO_STD_PAL))) {
        ERROR_LOG("invalid para!\n");
        return HLE_RET_EINVAL;
    }

    max_frame_rate =
            (video_std == VIDEO_STD_NTSC) ? NTSC_MAX_FRAME_RATE : PAL_MAX_FRAME_RATE;

    return HLE_RET_OK;
}

int hal_set_capture_size(int size)
{
    return HLE_RET_ENOTSUPPORTED;
}

int hal_get_max_frame_rate(void)
{
    return max_frame_rate;
}

int rtc_init(void);
int hal_video_init(void);
int hal_encoder_init(HLE_S32 pack_count);
int hal_encoder_exit(void);
int hal_video_exit(void);
int rtc_exit(void);

int hal_init(HLE_S32 pack_count, VIDEO_STD_E video_std)
{

    if (HLE_RET_OK != hal_set_video_std(video_std))
        goto mode_set_failed;

    if (HLE_RET_OK != rtc_init())
        ERROR_LOG("RTC init failed\n");

    if (HLE_RET_OK != mpp_sys_init())
        goto mpp_init_failed;

    if (HLE_RET_OK != hal_video_init())
        goto video_init_failed;

    if (HLE_RET_OK != hal_audio_init())
        goto audio_init_failed;

    if (HLE_RET_OK != hal_encoder_init(pack_count))
        goto encoder_init_failed;

    if (HLE_RET_OK != motion_detect_init())
        goto motion_init_failed;

    //if (HLE_RET_OK != blind_detect_init())
    //  goto blind_init_failed;

    //if (HLE_RET_OK != misc_init())
    //    ERROR_LOG("misc init failed\n");

    DEBUG_LOG("success\n");
    return HLE_RET_OK;

    //blind_init_failed:
    //    blind_detect_exit();
motion_init_failed:
    motion_detect_exit();
audio_init_failed:
    hal_audio_exit();
encoder_init_failed:
    hal_encoder_exit();
video_init_failed:
    hal_video_exit();
mpp_init_failed :
    mpp_sys_exit();
mode_set_failed:
    return HLE_RET_ERROR;
}

void hal_exit(void)
{
    //misc_exit();
    //blind_detect_exit();
    motion_detect_exit();
    hal_encoder_exit();
    hal_audio_exit();
    hal_video_exit();
    mpp_sys_exit();
    rtc_exit();
}

int hal_get_time(HLE_SYS_TIME *sys_time, int utc, HLE_U8* wday)
{
    time_t time_cur;
    time(&time_cur);
    struct tm cur_sys_time;
    if (utc) {
        gmtime_r(&time_cur, &cur_sys_time);
    }
    else {
        localtime_r(&time_cur, &cur_sys_time);
    }

    sys_time->tm_year = cur_sys_time.tm_year + 1900;
    sys_time->tm_mon = cur_sys_time.tm_mon + 1;
    sys_time->tm_mday = cur_sys_time.tm_mday;
    sys_time->tm_msec = ((cur_sys_time.tm_hour) * 3600 +
            (cur_sys_time.tm_min) * 60 + cur_sys_time.tm_sec)*1000;
    *wday = cur_sys_time.tm_wday;

    return HLE_RET_OK;
}

int hal_set_time(HLE_SYS_TIME *sys_time, int utc)
{
    int get_rtc_time(struct tm *tm_time);
    int set_rtc_time(struct tm *tm_time);
    
    if (sys_time->tm_year < 1970 || sys_time->tm_year > 2037
        || sys_time->tm_mon < 1 || sys_time->tm_mon > 12
        || sys_time->tm_mday < 1 || sys_time->tm_mday > 31
        || sys_time->tm_msec > 60 * 60 * 24 * 1000)
        return HLE_RET_EINVAL;

    struct tm tm_time = {0};
    tm_time.tm_year = sys_time->tm_year - 1900;
    tm_time.tm_mon = sys_time->tm_mon - 1;
    tm_time.tm_mday = sys_time->tm_mday;
    tm_time.tm_hour = (sys_time->tm_msec / 1000) / 3600;
    tm_time.tm_min = ((sys_time->tm_msec / 1000) % 3600) / 60;
    tm_time.tm_sec = (sys_time->tm_msec / 1000) % 60;

    if (utc) {
        // set rtc clock
        struct tm rtc;
        int ret = get_rtc_time(&rtc);
        if ((HLE_RET_OK != ret) || (mktime(&tm_time) != mktime(&rtc))) {
            set_rtc_time(&tm_time);
            DEBUG_LOG("Set RTC time : %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                      tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }

        //设置系统时间
        struct timeval tv = {0};
        char* oldtz = getenv("TZ");
        //setenv("TZ", "UTC0", 1);
        tv.tv_sec = mktime(&tm_time);
        if (oldtz) {
            //setenv("TZ", oldtz, 1);
        }
        else {
            //unsetenv("TZ");
        }
        time_t curr_time = time(NULL);
        if (tv.tv_sec != curr_time) {
            //只有当需要设置的时间和当前时间之间有1秒以上的偏差才做真正的校时动作
            settimeofday(&tv, NULL);
            DEBUG_LOG("Set Sys time :%s", ctime(&tv.tv_sec));
        }
    }
    else {
        // set rtc clock
        struct timeval tv = {0};
        tv.tv_sec = mktime(&tm_time);
        gmtime_r(&tv.tv_sec, &tm_time);
        struct tm rtc;
        int ret = get_rtc_time(&rtc);
        if ((HLE_RET_OK != ret) || (mktime(&tm_time) != mktime(&rtc))) {
            set_rtc_time(&tm_time);
            DEBUG_LOG("Set RTC time : %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                      tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                      tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }

        //设置系统时间
        time_t curr_time = time(NULL);
        if (tv.tv_sec != curr_time) {
            //只有当需要设置的时间和当前时间之间有1秒以上的偏差才做真正的校时动作
            settimeofday(&tv, NULL);
            DEBUG_LOG("Set Sys time :%s", ctime(&tv.tv_sec));
        }
    }

    return HLE_RET_OK;
}

void hal_set_date_format(E_DATE_DISPLAY_FMT date_fmt)
{
    void encoder_set_date_format(E_DATE_DISPLAY_FMT date_fmt);
    
    if ((date_fmt != DDF_YYYYMMDD) && (date_fmt != DDF_MMDDYYYY) &&
        (date_fmt != DDF_DDMMYYYY)) {
        ERROR_LOG("date_string_format unsupport\n");
        return;
    }
    encoder_set_date_format(date_fmt);
}

int hal_get_defaultkey_status(void)
{
    //return detect_defaultkey_status();
    return 0;
}

#if 0
#define EEPROM_PAGE_FOR_SN     0
#define EEPROM_PAGE_FOR_MAC    1
#define EEPROM_PAGE_FOR_KEY    2

int hal_get_unique_id(unsigned char *unique_id, unsigned int *unique_id_len)
{
    return get_encrypt_chip_id(unique_id, unique_id_len);
}

int hal_write_serial_no(unsigned char *sn_data, unsigned int sn_len)
{
    return eeprom_write_page(EEPROM_PAGE_FOR_SN, sn_data, sn_len);
}

int hal_read_serial_no(unsigned char *sn_data, unsigned int sn_len)
{
    return eeprom_read_page(EEPROM_PAGE_FOR_SN, sn_data, sn_len);
}

int hal_write_mac_addr(unsigned char *mac_data, unsigned int mac_len)
{
    return eeprom_write_page(EEPROM_PAGE_FOR_MAC, mac_data, mac_len);
}

int hal_read_mac_addr(unsigned char *mac_data, unsigned int mac_len)
{
    return eeprom_read_page(EEPROM_PAGE_FOR_MAC, mac_data, mac_len);
}

int hal_write_encrypt_key(unsigned char *encrypt_key_data, unsigned int encrypt_key_len)
{
    return eeprom_write_page(EEPROM_PAGE_FOR_KEY, encrypt_key_data, encrypt_key_len);
}

int hal_read_encrypt_key(unsigned char *encrypt_key_data, unsigned int encrypt_key_len)
{
    return eeprom_read_page(EEPROM_PAGE_FOR_KEY, encrypt_key_data, encrypt_key_len);
}

int hal_get_temperature(int *val)
{
    *val = 0;
    return HLE_RET_ERROR;
}

void __suicide();

int hal_reboot()
{
    //do hal cleanup stuff

    __suicide();
    sleep(2);
    reboot(RB_AUTOBOOT); //2秒后如果看门狗还没有把系统复位，就使用reboot进行软重启
    return 0;
}

int hal_shutdown()
{
    //do hal cleanup stuff

    watchdog_disable();
    return reboot(RB_HALT_SYSTEM);
}
#endif

/*
功能：
	读指定大小的数据，如果出错则返回-1。
	如果读到文件末尾，返回的大小有可能小于指定的大小
参数：
	@fd : 文件描述符
	@vptr ：数据缓冲位置
	@n : 需要读的字节大小
返回：
	成功：读到的字节数
	失败：-1;
 */
ssize_t hal_readn(int fd, void *vptr, size_t n)
{
    ssize_t left;
    ssize_t read_len;
    char *ptr;

    ptr = vptr;
    left = n;
    while (left > 0) {
        read_len = read(fd, ptr, left);
        if ((read_len) < 0) {
            if (errno == EINTR)
                read_len = 0; /* and call read() again */
            else
                return -1; /* error */
        }
        else if (read_len == 0)
            break;
        left -= read_len;
        ptr += read_len;
    }
    return (n - left);
}

/*
功能：写指定大小的数据，如果出错则返回-1。
参数：
	@fd ： 文件描述符
	@vptr ：源数据缓存区位置
	@n: 要写入数据的大小
返回：
	成功 ：写入的字节数
	失败 ： -1
 */
ssize_t hal_writen(int fd, const void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0; /* and call write() again */
            else
                return (-1); /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return (n);
}

HLE_S32 get_one_JPEG_frame(const void*frame_addr,HLE_S32 length)
{
    frame_addr = encoder_request_jpeg(0, &length, IMAGE_SIZE_1920x1080);
    if(NULL == frame_addr)
        return HLE_RET_ERROR;
    else
        return HLE_RET_OK;
        
}


int client_stream_id = -1; //后边最好定义一个能直返回正在请求流编号的接口替换
HLE_S32 get_one_encoded_frame(HLE_S32 stream_id,HLE_S8 have_audio, void **pack_addr, void**frame_addr,HLE_S32* frame_length)
{

    if(stream_id > ENC_STREAM_NUM || stream_id < 0)
        return HLE_RET_EINVAL;

    client_stream_id = stream_id;
    int skip_len = 0;

    encoder_if_have_audio(stream_id,have_audio);

    ENC_STREAM_PACK *pack = encoder_get_packet(stream_id);
    if(NULL == pack)
    {
        return HLE_RET_ERROR;
    }
    
    
    *pack_addr = pack;
    *frame_addr = pack->data;
    *frame_length = pack->length;
    //printf("get pack pack_addr(%p)  frame_addr(%p)  frame_length(%d) bytes\n",*pack_addr,*frame_addr,*frame_length);


    return HLE_RET_OK;
}



/*
    media server 回调函数，编码帧（包）的引用计数减1
*/
extern int spm_dec_pack_ref(ENC_STREAM_PACK *pack);
int dec_frame_ref(void *pack_addr)
{

    if(NULL == pack_addr)
        return HLE_RET_EINVAL;
    
    //DEBUG_LOG("dec_frame_ref free pack addr = %p\n\n",pack_addr);

    spm_dec_pack_ref((ENC_STREAM_PACK *)pack_addr);
    
    return HLE_RET_OK;
}

int media_server_module_init(void)
{
    med_ser_init_info_t med_ser_init_info;
    memset(&med_ser_init_info,0,sizeof(med_ser_init_info));
    
    med_ser_init_info.get_one_encoded_frame_callback =get_one_encoded_frame;
    med_ser_init_info.dec_frame_ref_callback = dec_frame_ref;
    med_ser_init_info.encoder_force_iframe = encoder_force_iframe;
    med_ser_init_info.get_one_JPEG_frame_callback = get_one_JPEG_frame;
    med_ser_init_info.use_wireless_network = 1;
    //其余的项暂时不初始化，以后需要再加
    
    int ret =  media_server_init(&med_ser_init_info, sizeof(med_ser_init_info_t));
    if(HLE_RET_OK == ret)
        return HLE_RET_OK;
    else
        return HLE_RET_ERROR;
}




#if 1
extern HLE_S32 connected_client_num; 
//#define USE_AI_AENC_BIND  1   // 打开该宏 ：AI绑定到AENC          注释该宏：AI直接输出到AO  
extern int AI_to_AO_start;
int app_main(int argc, char *argv[])
{
    int ret;


    ret = hal_init(200, VIDEO_STD_NTSC);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("hal_init fail!\n");
        return -1;
    }
    

    #if 0

    ret = media_server_module_init();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("media_server_module_init fail!\n");
        return -1;
    }
    
    ret = media_server_start_thread();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("media_server_start_thread fail!\n");
        return -1;
    }
    #endif

    
    int i, snap;
    int id[STREAMS_PER_CHN];
    int fd[STREAMS_PER_CHN];
    int aud;



    for (i = 0; i < STREAMS_PER_CHN; ++i) 
    {
        ENC_STREAM_ATTR attr;
        encoder_get_optimized_config(0, i, &attr);
        encoder_config(0, i, &attr);

        
        //获取queue_id
        id[i] = encoder_request_stream(0, i, 1);
        
        
        char buf[50];
        snprintf(buf, sizeof(buf), "/jffs0/test%d.h264", i);
       // fd[i] = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0664);

    }

    


    OSD_BITMAP_ATTR osdAttr;
    osdAttr.enable = 1;
    osdAttr.x = 100;
    osdAttr.y = 100;
    osdAttr.width = 8 * 20;
    osdAttr.height = 16;
    osdAttr.fg_color = 0xffffffff;
    osdAttr.bg_color = 0;
    osdAttr.raster = NULL;
    videoin_set_osd(0, TIME_OSD_INDEX, &osdAttr);
    osdAttr.x = 100;
    osdAttr.y = 600;
    osdAttr.width = 8 * 20;
    osdAttr.height = 16;
    videoin_set_osd(0, RATE_OSD_INDEX, &osdAttr);

    ENC_JPEG_ATTR jpgAttr;
    jpgAttr.img_size = IMAGE_SIZE_1920x1080;
    jpgAttr.level = 0;
    encoder_config_jpeg(0, &jpgAttr);

    
    #if 0
    
    play_tip("/jffs0/001.pcm",NULL,NULL);
    #else
    AI_to_AO_start = 1;//开始AI直接输出到AO模式
    #endif
    
    snap = 0;
    int cycle_num = 0;
    int skip_len = 0;

    #if 0
    pthread_t threadID_Idle;
    HLE_S32 err = pthread_create(&threadID_Idle, NULL, &fmp4_record, NULL);
    if (0 != err) 
    {
        ERROR_LOG("create media_server_start failed!\n");
        return HLE_RET_ERROR;
    }   
    DEBUG_LOG("fmp4_record start success!\n");
    #endif


    for (;;) 
    {
         for (i = 0; i < STREAMS_PER_CHN; ++i) 
        {
            
     
            if(connected_client_num >0)//依据queue_id号来决定哪些编码队列不做处理
            {
                //流编号和客户端正在拉取的一致，这里就不做处理，直接continue
                if(i == client_stream_id)
                {
                    cycle_num++;
                    if(10 == cycle_num)
                    {
                        printf("client num(%d) app_main... \n",connected_client_num);
                        cycle_num = 0;

                            
                    }
                    printf("continue...\n");
                    continue;
                }
                    
            }
            
                
            ENC_STREAM_PACK *pack = encoder_get_packet(id[i]);
            FRAME_HDR *header = (FRAME_HDR *) pack->data;
            if (header->sync_code[0] != 0 || header->sync_code[1] != 0\
            || header->sync_code[2] != 1) 
            {
                printf("------BUG------\n");
                continue;
            }

            //0xF8-视频关键帧
            if (header->type == 0xF8) {
                skip_len = sizeof (FRAME_HDR) + sizeof (IFRAME_INFO);
               // write(fd[i], pack->data + skip_len, pack->length - skip_len);
            }
            //0xF9-视频非关键帧
            else if (header->type == 0xF9) {
                skip_len = sizeof (FRAME_HDR) + sizeof (PFRAME_INFO);
                //write(fd[i], pack->data + skip_len, pack->length - skip_len);
            }
            
            //0xFA-音频帧
            else if (header->type == 0xFA)
            {
                 skip_len = sizeof (FRAME_HDR) + sizeof (AFRAME_INFO);
                //写音频文件放到了开始音频编码的线程了
            }
            else 
            {
                printf("------BUG------\n");
                continue;
            }

            
        
            encoder_release_packet(pack);
            //spm_pack_print_ref(pack);
    
        }
     
  
        #if 0
            ++snap;
            if ((snap % 30) == 0) 
            {
                int size;
                char * jpg = encoder_request_jpeg(0, &size, IMAGE_SIZE_1920x1080);
                if (jpg) 
                {
                    #if 0
                    char buf[50];
                    snprintf(buf, sizeof(buf), "/jffs0/snap%d.jpg", snap);
                    int fd = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                    write(fd, jpg, size);
                    close(fd);
                    #endif
                    
                    encoder_free_jpeg(jpg);
                }
            }
        #endif

    }

  
    for (i = 0; i < STREAMS_PER_CHN; ++i) {
        encoder_free_stream(id[i]);
        close(fd[i]);
    }
 
    
    hal_exit();
    

    
    return 0;
}
//simple test, disable it if not needed, don't remove it
#else //原来调试通过的代码
int app_main(int argc, char *argv[])
{
    int ret = hal_init(200, VIDEO_STD_NTSC);
    if (HLE_RET_OK != ret) {
        ERROR_LOG("hal_init fail!\n");
        return -1;
    }
    
    int i, cnt, snap;
    int id[STREAMS_PER_CHN];
    int fd[STREAMS_PER_CHN];
    int aud;

    for (i = 0; i < STREAMS_PER_CHN; ++i) {
      //  if(1 == i)continue;

        ENC_STREAM_ATTR attr;
        encoder_get_optimized_config(0, i, &attr);
        encoder_config(0, i, &attr);
        id[i] = encoder_request_stream(0, i, 1);

        char buf[50];
        snprintf(buf, sizeof(buf), "/jffs0/test%d.h264", i);
        fd[i] = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0664);

    }


    aud = open("rawaudio", O_CREAT | O_WRONLY | O_TRUNC, 0664);

    OSD_BITMAP_ATTR osdAttr;
    osdAttr.enable = 1;
    osdAttr.x = 100;
    osdAttr.y = 100;
    osdAttr.width = 8 * 20;
    osdAttr.height = 16;
    osdAttr.fg_color = 0xffffffff;
    osdAttr.bg_color = 0;
    osdAttr.raster = NULL;
    videoin_set_osd(0, TIME_OSD_INDEX, &osdAttr);
    osdAttr.x = 100;
    osdAttr.y = 600;
    osdAttr.width = 8 * 20;
    osdAttr.height = 16;
    videoin_set_osd(0, RATE_OSD_INDEX, &osdAttr);

    ENC_JPEG_ATTR jpgAttr;
    jpgAttr.img_size = IMAGE_SIZE_1920x1080;
    jpgAttr.level = 0;
    encoder_config_jpeg(0, &jpgAttr);

    snap = 0;
    for (cnt = 0; cnt < 15 * 60; ++cnt) {
        for (i = 0; i < STREAMS_PER_CHN; ++i) {
            ENC_STREAM_PACK *pack = encoder_get_packet(id[i]);
            FRAME_HDR *header = (FRAME_HDR *) pack->data;
            if (header->sync_code[0] != 0 || header->sync_code[1] != 0
                || header->sync_code[2] != 1) {
                printf("------BUG------\n");
                continue;
            }

            //0xF8-视频关键帧
            if (header->type == 0xF8) {
                int skip_len = sizeof (FRAME_HDR) + sizeof (IFRAME_INFO);
                write(fd[i], pack->data + skip_len, pack->length - skip_len);
            }
            //0xF9-视频非关键帧
            else if (header->type == 0xF9) {
                int skip_len = sizeof (FRAME_HDR) + sizeof (PFRAME_INFO);
                write(fd[i], pack->data + skip_len, pack->length - skip_len);
            }
            //0xFA-音频帧
            else if (header->type == 0xFA) {
                int skip_len = sizeof (FRAME_HDR) + sizeof (AFRAME_INFO);
                write(aud, pack->data + skip_len, pack->length - skip_len);
            }
            else {
                printf("------BUG------\n");
            }
            encoder_release_packet(pack);
        }

#if 1
        ++snap;
        if ((snap % 30) == 0) {
            int size;
            char * jpg = encoder_request_jpeg(0, &size, IMAGE_SIZE_1920x1080);
            if (jpg) {
                char buf[50];
                snprintf(buf, sizeof(buf), "snap%d.jpg", snap);
                int fd = open(buf, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                write(fd, jpg, size);
                close(fd);
                encoder_free_jpeg(jpg);
            }
        }
#endif

    }

    for (i = 0; i < STREAMS_PER_CHN; ++i) {
        encoder_free_stream(id[i]);
        close(fd[i]);
    }
    hal_exit();
    return 0;
}

#endif






























