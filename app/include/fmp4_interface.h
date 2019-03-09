


#ifndef _FMP4_INTERFACE_H
#define _FMP4_INTERFACE_H
#include <stdio.h>


//"内存存储模式"描述信息
typedef struct _buf_mode_t
{
	unsigned char	*buf_start; //最终存储内存的起始地址
	unsigned int 	buf_size;
	unsigned int 	w_offset;	//写指针的偏移量，最终值就是实际文件的长度
}buf_mode_t;

//"文件存储模式"描述信息
typedef struct _file_mode_t
{
	char * file_name; //最终存储文件的名字（liteos需要绝对路径，否则创建文件会失败）
}file_mode_t;
/********************************************************
采用"内存存储模式" 请初始化 buf_mode ，将file_mode置为空
采用"文件存储模式" 请初始化 file_mode，将buf_mode置为空
注意：至少初始化一个，两个都初始化默认采用 buf_mode
********************************************************/
typedef struct _fmp4_out_info_t
{
	buf_mode_t 		buf_mode; 		//"内存存储模式"
	file_mode_t		file_mode;		//"文件存储模式"
}fmp4_out_info_t;

/***STEP 1********************************************************************************
功能：fmp4编码初始化
参数：out_info ： 要生成的 fmp4 文件存储模式描述信息
					
	  Vframe_rate: 转入视频的原始帧率
	  Aframe_rate：传入音频的原始帧率
	  audio_sampling_rate: 传入音频数据的原始采样率
返回值：成功 ： 0  失败：-1
*******************************************************************************************/
int Fmp4_encode_init(fmp4_out_info_t * out_info,unsigned int Vframe_rate,unsigned int Aframe_rate,unsigned short audio_sampling_rate);


/***STEP 2********************************************************************************
功能：接收 sps/pps NALU 包，设置 avcC box参数，
参数：naluData：nalu包数据首地址 
	  naluSize：nalu的长度
返回值：成功：0 失败 -1;
注意：该接口内部会对naluData自动偏移5个字节长度
*******************************************************************************************/
//int sps_pps_parameter_set(unsigned char* naluData, int naluSize);



/***n*(STEP3-1)*****************************************************************************
功能：放入一帧	video        frame 进行fmp4编码
参数：<video_frame> ：video frame 的首地址
	  <frame_length>：video frame 帧长
	  <frame_rate>  : video frame 的帧率
	  <time_scale>  : video frame 的时间戳(ms)
返回值：成功:0
		失败：-1
*******************************************************************************************/
int Fmp4VEncode(void *video_frame,unsigned int frame_length,unsigned int frame_rate,unsigned long long time_scale);

/***n*(STEP3-2)*****************************************************************************
功能：放入一帧	audio        frame 进行fmp4编码
参数：<audio_frame> ：audio frame 的首地址
	  <audio_length>：audio frame 帧长
	  <frame_rate>  : audio frame 的帧率
	  <time_scale>  : audio frame 的时间戳(ms)
返回值：成功:0
		失败：-1
*******************************************************************************************/
int Fmp4AEncode(void * audio_frame, unsigned int frame_length, unsigned int frame_rate,unsigned long long time_scale);

/***STEP5**********************************************************************************
功能：fmp4编码退出
参数：无
返回值：成功:0
		失败：不会失败
*******************************************************************************************/
int Fmp4_encode_exit(void);


#endif





