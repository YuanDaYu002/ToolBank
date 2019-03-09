/*********************************************************************************
  *FileName: media_server_interface.h
  *Create Date: 2018/09/05
  *Description: 本文件为设备media server 和设备端app软件交互的接口定义。 
  *Others:  
  *History:  
**********************************************************************************/
#ifndef MEDIA_SERVER_INTERFACE_H
#define MEDIA_SERVER_INTERFACE_H

#include <sys/types.h>
//#include <sys/ipc.h>
//#include <sys/msg.h>

#include "typeport.h"

#ifdef __cplusplus
extern "C" {
#endif


enum P2P_media_server_status
{
	offline = -1,		//离线状态（失去与服务器连接）
	sleeping = 0,		//睡眠状态（无客户端连接）
	wakeup = 1,			//唤醒状态（无客户端连接）
	connected,			//客户端已连接状态
	livingstream,		//实时流传输状态
	upgrade,			//升级状态
	
};



enum med_ser_alarm_file_type
{
	JPEG_file = 0,
	H264_video_file = 1,
	H265_video_file = 2,
	g711_audio_file = 3,
	H264_mix_g711_file = 4,
	H265_mix_g711_file = 5,
};

enum med_ser_device_type
{
	HI3516C_V300_IPC = 0,
	HI3516E_V100_IPC = 1,
};

typedef struct _med_ser_alarm_file_t
{
    	HLE_S8  file_type;			//告警文件类型，赋值参考enum media_server_alarm_file_type
    	HLE_S8  alarm_level;			//告警等级，待定
   		HLE_S8  is_finished;			//是否已经记录完成（完整的文件）
    	HLE_S8  reserved;				//字节对齐，占位
    	HLE_S32   file_size;			//告警文件大小
    	HLE_S8  create_date[32];		//文件创建日期，格式：2018/09/05
    	HLE_S8  create_time[32];		//文件创建时间，格式：00:00-24:00, 24小时制
    	HLE_S8  finish_time[32];		//文件记录结束时间，格式：00:00-24:00, 24小时制
    	HLE_S8  file_path[96];		//文件路径

} med_ser_alarm_file_t;


typedef struct _med_ser_alarm_push_condition_t 
{
	//待定,告警信息，告警发生的位置、时长、面积比、速度等信息
	//以及告警发生时要不要降低码率，减小录制的告警文件大小、码率多长时间恢复等
        HLE_S8   if_push_to_app;	//是否推送到手机APP，1：推送  0：不推送
	med_ser_alarm_file_t push_file;

}med_ser_alarm_push_condition_t;

typedef struct _device_id_t
{
	HLE_S8 	mac[6];
	HLE_S8 	reserve[2];
	HLE_S8	deviceid[16];

}device_id_t;


typedef struct _med_ser_init_info_t
{
	HLE_U32 	device_type;				//设备类型,参考enum med_ser_device_type;
	HLE_U32 	device_capacity;			//设备能力位，设备支持的功能属性；
	HLE_U32		device_video_channel_num;	//设备的视频通道数；
	HLE_U32 	use_wireless_network;		//设备是无线的还是有线，1：无线，0：有线
	const HLE_S8 		*device_id;					//设备id；
	const HLE_S8 		*device_name;				//设备名；
	const HLE_S8 		*config_dir;				//配置文件路径；
	const HLE_S8 		*uboot_version;
	const HLE_S8 		*kernel_version;
	const HLE_S8 		*rootfs_version;
	const HLE_S8 		*app_version;
	const HLE_S8 		*network_interface;			//网卡名称 wlan0, ra0, eth0 ...；
	const HLE_S8 		*high_resolution;			//主码流(高清)的分辨率,格式:1920*1080;
	const HLE_S8 		*secondary_resolution;		//次码流(普清)的分辨率,格式:1280*720;
	const HLE_S8 		*low_resolution;			//低码流(低清)的分辨率,格式:640*480;

	key_t msg_queue_key;				//消息队列键值（设备端app和media server 之间进行通信,
										//media_server_init会初始化）。需不需要这个东西呢？？？
	
	/*服务器返回时间戳校时，回调函数*/
	void (*timing_callback)(time_t timestamp);

	/*设备上线会回调该函数，告知设备软件设备已上线*/
	void (*device_is_online_callback)(HLE_S32 status);
	
	/*告警的推送策略设置接口,服务器下发的告警策略，media server 会调用该接口进行设置
 	  @condition:策略
 	  @condition_count:策略的条数
 	*/
	void (*set_push_condition_callback)(med_ser_alarm_push_condition_t *condition,HLE_S32 condition_count);
	

	/*
  	  临时设置或者恢复码流的帧率和码率
 	  @type: 1为临时设置，0为恢复默认
 	  @stream_type: 码流类型，2是HD，1是MD，0是LD
 	  @frame_rate: 当type为1时，表示码流帧率
 	  @bit_rate: 当type为1时，表示码流码率，单位Kbit		
	*/
	void (*set_stream_temporary_callback)(HLE_S32 type, HLE_S32 stream_type, HLE_S32 frame_rate, HLE_S32 bit_rate);	
	
	/*
 	  临时设置或者恢复码流的gop
  	  @type: 1为临时设置，0为恢复默认
 	  @gop: I帧间隔,1~10S
 	*/
	void (*on_set_stream_gop_callback)(HLE_S32 type, HLE_S32 gop);

	/*
	 一系列获取帧返回的接口
	 @channel: 通道, 0:1920*1080P , 1:1280*720P
	 @frame_addr:帧地址
	 @length: 帧长度 
	*/
	HLE_S32 (*get_one_YUV_frame_callback )(HLE_S32 stream_id,const void*frame_addr,HLE_S32 length);
	HLE_S32 (*get_one_H264_frame_callback)(HLE_S32 stream_id,const void*frame_addr,HLE_S32 length);
	HLE_S32 (*get_one_H265_frame_callback)(HLE_S32 stream_id,const void*frame_addr,HLE_S32 length);
	HLE_S32 (*get_one_JPEG_frame_callback)(const void*frame_addr,HLE_S32 length);
	HLE_S32 (*get_one_g711_frame_callback)(const void*frame_addr,HLE_S32 length);
	HLE_S32 (*get_one_g726_frame_callback)(const void*frame_addr,HLE_S32 length);
	HLE_S32 (*get_one_PCM_frame_callback )(const void*frame_addr,HLE_S32 length); 

	/*
	 获取一个编码帧返回（实时流传输）。
	 	考虑编码后的缓存buffer，如果视频帧和audio帧是在一个大buffer中均匀混合的，
	 	media server就不区分是音频帧还是视频帧,在传输实时流也不需要去区分。
	 @stream_id（传入）: 编码流编号, 0:1920*1088P , 1:960*544P, 2:480*272
	 @have_audio（传入）：是否需要audio帧
	 @pack_addr（返回）：编码帧所在的包的首地址，用于发送完成后释放数据包
	 @frame_addr（返回）:编码帧地址
	 @frame_length: 编码帧长度 
	*/
	HLE_S32 (*get_one_encoded_frame_callback)(HLE_S32 stream_id,HLE_S8 have_audio, void **pack_addr, void**frame_addr,HLE_S32* frame_length);
	
	/*
		编码帧（包）的引用计数减1回调接口，加1在get_one_encoded_frame_callback函数里边执行
		如果减1后帧引用计数为0,则帧缓存会被释放
	*/
	HLE_S32 (*dec_frame_ref_callback)(void*pack_addr);


	HLE_S32 (*set_device_id_callback)(device_id_t*data,HLE_S32 length);
	
	//强制I帧回调函数
	HLE_S32 (*encoder_force_iframe)(HLE_S32 channel, HLE_S32 stream_index);

}med_ser_init_info_t;


/*********************************************************************
 *函数名：media_server_init
 *功能：初始化媒体服务器
 *参数：
 *返回值：
 *	成功：HLE_RET_OK
 *	失败：HLE_RET_ERROR
 * ******************************************************************/
HLE_S32 media_server_init(med_ser_init_info_t* info, HLE_U32 info_len);


/*********************************************************************
 *函数名：media_server_start_thread
 *功能：启动媒体服务器（线程）
 *参数：
 *返回值：
 *	成功：HLE_RET_OK
 *	失败：错误码

 * ******************************************************************/
 HLE_S32 media_server_start_thread(void);


/*********************************************************************
 *函数名：media_server_get_server_timestamp
 *功能：获取服务器上的时间，用于设备校时
 *参数：无
 *返回值：
 *	成功：HLE_RET_OK
 *	失败：HLE_RET_ERROR（设备没有连接上服务器）
 * ******************************************************************/
//HLE_S32 media_server_get_server_timestamp(void);


/*********************************************************************
 *函数名：media_server_if_upgrading
 *功能：判断是否在升级
 *参数：无
 *返回值：
 *	在升级状态：media_server_upgrading
 *	不在升级状态：media_server_success
 * ******************************************************************/
HLE_S32 media_server_if_upgrading(void);


char* http_dowload_file(int argc, char  *argv[]);
char* http_post(char *host_url);

int dowload_upadte_file(int argc,char**argv);
#ifdef __cplusplus
}
#endif


#endif
 











