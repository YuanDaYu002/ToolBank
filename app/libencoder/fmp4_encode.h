#ifndef _FMP4_ENCODER_H
#define _FMP4_ENCODER_H
#include <pthread.h>


typedef struct _fmp4_input_buffer_t
{
	unsigned int 	stream_id;		//码流id号，主线程会依据该id号存放对应通道的编码帧
	unsigned char*	frame_Buffer;	//缓存帧的首地址
	unsigned int  	frame_len;		//当前帧的长度
	unsigned char	frame_type;		//帧类型
	volatile unsigned char 	enable;			//开始接收编码帧数据（使能标志）0：暂停，1：开始
	unsigned char	reserved[2];
	pthread_mutex_t mut;	
	pthread_cond_t cond_ready;
	
}fmp4_input_buffer_t;




#endif



