

 
#ifndef _FMP4_H
#define _FMP4_H
#include "Box.h"


#define OUT  //标记为输出参数
#define IN	 //标记为输入参数


/**************************************************************************
生成的fmp4文件直接保存到文件/内存
1.jffs2 文件系统在flash使用率比较高时写文件速度很慢
2.会造成NorFlash多次擦写，影响寿命
3.在liteOS下尝试建立ramfs文件系统，但不稳定，写大文件照样延迟大，
	并且还会报内存不足，华为官方不推荐使用。

*************************************************************************/	
typedef enum 
{
	SAVE_IN_MEMORY = 1,  //保存到内存
	SAVE_IN_FILE = 2	//保存到文件
}file_mode_e;


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
	char * file_name; //最终存储文件的名字
}file_mode_t;
/********************************************************
采用"内存存储模式" 请初始化 buf_mode ，将file_mode置为NULL
采用"文件存储模式" 请初始化 file_mode，将buf_mode置为NULL
注意：至少初始化一个，两个都初始化默认采用 buf_mode
********************************************************/
typedef struct _fmp4_out_info_t
{
	buf_mode_t 		buf_mode; 		//"内存存储模式"
	file_mode_t		file_mode;		//"文件存储模式"
}fmp4_out_info_t;



/*================START=====================================================
以下结构用来记录fmp4文件各个box距文件开头的位置偏移
==========================================================================*/
//fmp4文件中 video trak 分支各box距文件开头的位置偏移
typedef struct _trak_video_lable_t
{
	lve2 unsigned int trakBox_offset;
		lve3 unsigned int tkhdBox_offset;
		lve3 unsigned int mdiaBox_offset;
			lve4 unsigned int mdhdBox_offset;
			lve4 unsigned int hdlrBox_offset;
			lve4 unsigned int minfBox_offset;
				lve5 unsigned int vmhdBox_offset;
				lve5 unsigned int dinfBox_offset;
					lve6 unsigned int drefBox_offset;
						lve7 unsigned int urlBox_offset; 
				lve5 unsigned int stblBox_offset;
					lve6 unsigned int stsdBox_offset;
						//lve7 unsigned int avc1Box_offset; 		
							//lve8 unsigned int avccBox_offset;	
							//lve8 unsigned int paspBox_offset;	//暂未实现
					lve6 unsigned int sttsBox_offset;
					lve6 unsigned int stscBox_offset;
					lve6 unsigned int stszBox_offset;
					lve6 unsigned int stcoBox_offset;
}trak_video_lable_t;

//fmp4文件中 audio trak 分支各box距文件开头的位置偏移
typedef struct _trak_audio_lable_t
{
	lve2 unsigned int trakBox_offset;
		lve3 unsigned int tkhdBox_offset;
		lve3 unsigned int mdiaBox_offset;
			lve4 unsigned int mdhdBox_offset;
			lve4 unsigned int hdlrBox_offset;
			lve4 unsigned int minfBox_offset;
				lve5 unsigned int smhdBox_offset;
				lve5 unsigned int dinfBox_offset;
					lve6 unsigned int drefBox_offset;
						lve7 unsigned int url_box_offset;
				lve5 unsigned int stblBox_offset;
					lve6 unsigned int stsdBox_offset;
						//lve7 unsigned int mp4aBox_offset;		//归属在stsd_box中一起初始化了
							//lve8 unsigned int esdsBox_offset;	//归属在stsd_box中一起初始化了
					lve6 unsigned int sttsBox_offset;
					lve6 unsigned int stscBox_offset;
					lve6 unsigned int stszBox_offset;
					lve6 unsigned int stcoBox_offset;
					

}trak_audio_lable_t;

//fmp4文件中 video traf 分支各box距文件开头的位置偏移
typedef struct _traf_video_lable_t
{
	lve2 unsigned int trafBox_offset;
		lve3 unsigned int tfhdBox_offset;
		lve3 unsigned int tfdtBox_offset;
		lve3 unsigned int trunBox_offset;
		
}traf_video_lable_t;

//fmp4文件中 audio traf 分支各box距文件开头的位置偏移
typedef struct _traf_audio_lable_t
{
	lve2 unsigned int trafBox_offset;
		lve3 unsigned int tfhdBox_offset;
		lve3 unsigned int tfdtBox_offset;
		lve3 unsigned int trunBox_offset;
}traf_audio_lable_t;

//fmp4文件各个box距文件开头的位置偏移
typedef struct _fmp4_file_lable_t
{
	lve1 unsigned int ftypBox_offset;
	lve1 unsigned int moovBox_offset;
		lve2 unsigned int mvhdBox_offset;
		lve2 trak_video_lable_t trak_video_offset;
		lve2 trak_audio_lable_t trak_audio_offset;
		lve2 unsigned int mvexBox_offset;
			lve3 unsigned int trex_video_offset;
			lve3 unsigned int trex_audio_offset;
		//lve2 unsigned int udtaBox_offset;
	lve1 unsigned int moofBox_offset;
		lve2 unsigned int mfhdBox_offset;
		lve2 traf_video_lable_t traf_video_offset;
		lve2 traf_audio_lable_t traf_audio_offset;
	lve1 unsigned int mdatBox_offset;
	lve1 unsigned int mfraBox_offset;
		lve2 unsigned int tfra_video_offset;
		lve2 unsigned int tfra_audio_offset;
		lve2 unsigned int mfroBox_offset;

}fmp4_file_lable_t;
fmp4_file_lable_t fmp4_file_lable = {0};

#define is_equal(a,b,stream) 	do{\
	if(a != b)\
	{\
		ERROR_LOG("fwrite file failed!\n");\
		fclose(stream);\
		return -1;\
	}\
}while(0)

/*共用了之前的接口，当out_mode == SAVE_IN_MEMORY 时，stream参数无效*/
#define fwrite_box(ptr,size,nmemb,stream,ret) 	do{\
	if(NULL == ptr)\
		return -1;\
	if(out_mode == SAVE_IN_FILE )\
	{\
		ret = fwrite (ptr,size,nmemb,stream );\
		is_equal(ret, (size*nmemb),stream);\
	}\
	else\
	{\
		if(out_info.buf_mode.w_offset + size*nmemb > out_info.buf_mode.buf_size)\
		{\
			ERROR_LOG("over write! fmp4 out put file memory is pool!\n");\
			return -1;\
		}\
		memcpy(out_info.buf_mode.buf_start + out_info.buf_mode.w_offset,(unsigned char*)ptr,size*nmemb);\
		out_info.buf_mode.w_offset += size*nmemb;\
		ret = size*nmemb;\
	}\
	free(ptr);\
	ptr = NULL;\
}while(0)

//===============END=================================================


typedef struct _trak_video_init_t
{
	unsigned short	width;
	unsigned short    height;
	unsigned int timescale; // timescale: 4 bytes    文件媒体在1秒时间内的刻度值，可以理解为1秒长度
	unsigned int duration;  // duration: 4 bytes  track的时间长度  duration/timescale = track的时长

	
}trak_video_init_t;

typedef struct _trak_audio_init_t
{
	unsigned int timescale; // timescale: 4 bytes    文件媒体在1秒时间内的刻度值，可以理解为1秒长度
	unsigned int duration;  // duration: 4 bytes  track的时间长度  duration/timescale = track的时长
	
	unsigned short sampleRate;  //样本率
	unsigned char channelCount; //通道数 1 或 2
	unsigned char reserved;
}trak_audio_init_t;




/*===================================================================
音视频混合相关部分
n*(moof + mdat)模式
1.将传入的原始数据构造成mdat box(真实数据) 
2.并生成对应的 moofbox(描述信息)
===================================================================*/

//定义1s大小的音视频帧缓空间
/*
44100HZ AAC数据采样   1024采样点为1帧， 16位宽下 能容纳1s的容量
(1024*2)*(44100/1024) = 88200 Bytes
*/
//#define REMUX_AUDIO_BUF_SIZE  (88200)

//16KHZ采样率下： 16000HZ   *2Bytes = 32000Bytes
#define REMUX_AUDIO_BUF_SIZE  (34000)


//无法确定1s的h264数据有多大，给个大概值  30s大概3M    。。。。1S大概3*1024KB/30 = 102.4KB
#define REMUX_VIDEO_BUF_SIZE  (1024*300) 

/*audio trun box里边允许容纳的最大samples（一帧一个sample）数，
该值不能小于传入audio数据的帧率，音视频混合器 remux 是按照1S的数据量来打包的
aac：44100HZ/1024 = 44帧/s（向上取整）
g711: 8000HZ/160 = 50 帧/s
*/
#define TRUN_AUDIO_MAX_SAMPLES (60)

/*video trun box里边允许容纳的最大samples（一帧一个sample）数，
该值不能小于传入video数据的帧率，音视频混合器 remux 是按照1S的数据量来打包的
H264: MAX ： 30帧/s
*/
#define TRUN_VIDEO_MAX_SAMPLES (40)

//最大能产生的moof+mdat结构个数，超过该值会出错（一秒一个，60代表能录制60S）。
#define MAX_MOOF_MDAT_NUM (60)



//写入mdat box中的每个sample的描述信息（放在trun box中）
typedef struct _sample_V_info_t  //video
{
	char* 				sample_pos;		//该sample在buf中的位置
	unsigned int		sample_len;		//该sample的长度
	trun_V_sample_t   	trun_sample; 	//trun box 的sample数组元素信息
}sample_V_info_t;

typedef struct _sample_A_info_t  //audio
{
	char* 				sample_pos;		//该sample在buf中的位置
	unsigned int		sample_len;		//该sample的长度
	trun_A_sample_t   	trun_sample; 	//trun box 的sample数组元素信息
}sample_A_info_t;


/*
混合器，video 缓冲结构
*/
typedef struct _buf_remux_video_t
{
	unsigned char*	remux_video_buf;	//buf的首地址
	unsigned char*	write_pos;			//写指针的位置
	unsigned char*	read_pos;			//读指针位置
	unsigned int 	frame_count;		//buf 中已经存储的帧数量
	unsigned int 	frame_rate; 		//外部传入视频数据的原本帧率
	sample_V_info_t sample_info[TRUN_VIDEO_MAX_SAMPLES]; //buffer 中 video sample（帧）的信息数组指针
	unsigned int write_index;	//sample_info数组的即将要写的下标
	unsigned char read_index;	//sample_info数组的即将要读的下标
	unsigned char need_remux;	//buf已经缓冲好1S的samples,需要将完整的 moof+mdat box 写入到文件
	unsigned char reserved[2];
	//以下参数直到释放前不需要复位
	tfra_entry_info_t entry_info[MAX_MOOF_MDAT_NUM];
	unsigned int entry_info_num;

	pthread_mutex_t mut;		//读写锁
//	pthread_cond_t remux_ready;
}buf_remux_video_t;
buf_remux_video_t buf_remux_video = {0};

/*
混合器，audio 缓冲结构
*/
typedef struct _buf_remux_audio_t
{
	char*	remux_audio_buf;	//buf的首地址
	char*	write_pos;			//写指针的位置
	char*	read_pos;			//读指针位置
	int 	frame_count;		//buf 中已经存储的帧数量
	int 	frame_rate; 		//外部传入视频数据的原本帧率
	sample_A_info_t sample_info[TRUN_AUDIO_MAX_SAMPLES]; //buffer 中 audio sample（帧）的信息数组
	unsigned char write_index;	//sample_info数组的即将要写的下标
	unsigned char read_index;	//sample_info数组的即将要读的下标
	unsigned char need_remux;	//buf已经缓冲好1S的samples,需要将完整的 moof+mdat box 写入到文件
	unsigned char reserved;
	//以下参数直到释放前不需要复位
	tfra_entry_info_t entry_info[MAX_MOOF_MDAT_NUM];
	unsigned int entry_info_num;
	pthread_mutex_t mut;		//读写锁
//	pthread_cond_t remux_ready;
	
}buf_remux_audio_t;
buf_remux_audio_t buf_remux_audio = {0};


int remux_init(unsigned int Vframe_rate,unsigned int Aframe_rate);
int sps_pps_parameter_set(void);


 #endif


