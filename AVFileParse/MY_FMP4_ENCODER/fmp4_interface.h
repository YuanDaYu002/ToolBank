


#ifndef _FMP4_INTERFACE_H
#define _FMP4_INTERFACE_H
#include <stdio.h>

/***STEP 1********************************************************************************
功能：fmp4编码初始化
参数：file_name ： 要生成的 fmp4 文件名（liteos需要绝对路径，否则创建文件会失败）
	  Vframe_rate: 转入视频的原始帧率
	  Aframe_rate：传入音频的原始帧率
返回值：文件描述符
*******************************************************************************************/
FILE* Fmp4_encode_init(char * file_name,unsigned int Vframe_rate,unsigned int Aframe_rate);




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
返回值：成功:0
		失败：-1
*******************************************************************************************/
int Fmp4VEncode(void *video_frame,unsigned int frame_length,unsigned int frame_rate);

/***n*(STEP3-2)*****************************************************************************
功能：放入一帧	audio        frame 进行fmp4编码
参数：<audio_frame> ：audio frame 的首地址
	  <audio_length>：audio frame 帧长
	  <frame_rate>  : audio frame 的帧率
返回值：成功:0
		失败：-1
*******************************************************************************************/
int Fmp4AEncode(void * audio_frame, unsigned int frame_length, unsigned int frame_rate);

/***STEP5**********************************************************************************
功能：fmp4编码退出
参数：无
返回值：成功:0
		失败：不会失败
*******************************************************************************************/
int Fmp4_encode_exit(void);


#endif





