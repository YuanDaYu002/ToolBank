#ifndef __FFMPEG_H__
#define __FFMPEG_H__

#include "info.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#include "libavdevice/avdevice.h"  //摄像头所用
#include "libavfilter/avfilter.h"
#include "libavutil/error.h"
#include "libavutil/mathematics.h"  
#include "libavutil/time.h"  
#include "libavutil/fifo.h"
#include "libavutil/audio_fifo.h"   //这里是做分片时候重采样编码音频用的
#include "inttypes.h"
#include "stdint.h"
};

#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")

#define AUDIO_ID            0    //packet 中的ID ，如果先加入音频 pocket 则音频是 0  视频是1，否则相反(影响add_out_stream顺序)
#define VIDEO_ID            1

//#define INPUTURL   "../in_stream/22.ts"
#define INPUTURL   "../in_stream/avier1.mp4"    
//#define INPUTURL   "../in_stream/father.avi"    //这个有问题？没有音频
//#define INPUTURL   "../in_stream/ceshi.avi" 

//m3u8 param
#define OUTPUT_PREFIX       "ZWG_TEST"              //切割文件的前缀
#define M3U8_FILE_NAME      "ZWG_TEST.m3u8"         //生成的m3u8文件名
#define URL_PREFIX          "../out_stream/"        //生成目录
#define NUM_SEGMENTS        50                      //在磁盘上一共最多存储多少个分片
#define SEGMENT_DURATION    10                      //每一片切割多少秒
extern unsigned int m_output_index;                 //生成的切片文件顺序编号(第几个文件)
extern char m_output_file_name[256];                //输入的要切片的文件


extern int nRet;                                                              //状态标志
extern AVFormatContext* icodec;												  //输入流context
extern AVFormatContext* ocodec ;                                              //输出流context
extern char szError[256];                                                     //错误字符串
extern AVStream* ovideo_st;
extern AVStream* oaudio_st;              
extern int video_stream_idx;
extern int audio_stream_idx;
extern AVCodec *audio_codec;
extern AVCodec *video_codec;   
extern AVBitStreamFilterContext * vbsf_aac_adtstoasc;                         //aac->adts to asc过滤器
static struct SwsContext * img_convert_ctx_video = NULL;
static int sws_flags = SWS_BICUBIC; //差值算法,双三次
extern AVBitStreamFilterContext * vbsf_h264_toannexb;
extern int IsAACCodes;

int init_demux(char * Filename,AVFormatContext ** iframe_c);
int init_mux();
int uinit_demux();
int uinit_mux();
//for mux
AVStream * add_out_stream(AVFormatContext* output_format_context,AVMediaType codec_type_t); 

//具体的切片程序
void slice_up();
//填写m3u8文件
int write_index_file(const unsigned int first_segment, const unsigned int last_segment, const int end, const unsigned int actual_segment_durations[]);


#endif