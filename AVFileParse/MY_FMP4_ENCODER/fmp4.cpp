#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Box.h"
#include "typeport.h"


#define VIDEO_TRACK 1   //视频轨道
#define AUDIO_TRACK 2   //音频轨道


Fmp4TrackId AddVideoTrack()
{
	if(NULL == fmp4FileHandle)
		return -1;

	
	//暂不实现 ，默认一条视频轨道 ：VIDEO_TRACK
	//			     一条音频轨道 ：AUDIO_TRACK

}

typedef struct _trak_video_init_t
{
	unsigned int	width;
	unsigned int    height;
	unsigned int timescale; // timescale: 4 bytes    文件媒体在1秒时间内的刻度值，可以理解为1秒长度
	unsigned int duration;  // duration: 4 bytes  track的时间长度  duration/timescale = track的时长

	
}trak_video_init_t;

trak_video_t trakVideo = {0};
trak_video_t*	trak_video_init(trak_video_init_t*args)
{
	/*
	前期不好确定各个容器box的最终长度，所以在初始化时都只按照自身的结构体长度计算，
	待各个box数据都完成写入后再统一计算各个box的最终长度以及合成最终的FMP4文件。
	*/
	trakVideo.trakBox = trak_box_init(sizeof(trak_box));
	trakVideo.tkhdBox = tkhd_box_init(VIDEO_TRACK, args->duration,args->width,args->height);
	trakVideo.mdatBox = mdat_box_init(sizeof(mdat_box));
	trakVideo.mdhdBox = mdhd_box_init(args->timescale, args->duration);
	trakVideo.hdlrBox = hdlr_box_init();
	trakVideo.minfBox = minf_box_init(sizeof(minf_box));
	trakVideo.vmhdBox = vmhd_box_init();
	trakVideo.dinfBox = dinf_box_init(sizeof(dinf_box));
	trakVideo.drefBox = dref_box_init();
	trakVideo.urlBox  = url_box_init(); 
	trakVideo.stblBox = stbl_box_init(sizeof(stbl_box));
	trakVideo.stsdBox = stsd_box_init(VIDEO,args->width,args->height,0,0);
	//trakVideo.avc1Box; 	//归属在stsd_box中一起初始化了
	//trakVideo.avccBox;	//归属在stsd_box中一起初始化了
	//trakVideo.paspBox;	//暂未实现
	trakVideo.sttsBox = stts_box_init();
	trakVideo.stscBox = stsc_box_init();
	trakVideo.stszBox = stsz_box_init();
	trakVideo.stcoBox = stco_box_init();

	return &trakVideo;
	
}


typedef struct _trak_audio_init_t
{
	unsigned int timescale; // timescale: 4 bytes    文件媒体在1秒时间内的刻度值，可以理解为1秒长度
	unsigned int duration;  // duration: 4 bytes  track的时间长度  duration/timescale = track的时长
	
	unsigned char channelCount; //通道数 1 或 2
	unsigned short sampleRate;  //样本率
}trak_audio_init_t;

trak_audio_t trakAudio = {0};
trak_audio_t*	trak_audio_init(trak_audio_init_t*args)
{
	/*
	前期不好确定各个容器box的最终长度，所以在初始化时都只按照自身的结构体长度计算，
	待各个box数据都完成写入后再统一计算各个box的最终长度以及合成最终的FMP4文件。
	*/
	trakAudio.trakBox = trak_box_init(sizeof(trak_box));
	trakAudio.tkhdBox = tkhd_box_init(AUDIO_TRACK, args->duration,0,0);
	trakAudio.mdiaBox = mdia_box_init(sizeof(mdia_box));
	trakAudio.mdhdBox = mdhd_box_init(args->timescale,args->duration);
	trakAudio.hdlrBox = hdlr_box_init();
	trakAudio.minfBox = minf_box_init(sizeof(minf_box));
	trakAudio.smhdBox = smhd_box_init();
	trakAudio.dinfBox = dinf_box_init(sizeof(dinf_box));
	trakAudio.drefBox = dref_box_init();
	trakAudio.url_box = url_box_init();
	trakAudio.stblBox = stbl_box_init(sizeof(stbl_box));
	trakAudio.stsdBox = stsd_box_init(AUDIO,0,0, args->channelCount, args->sampleRate);
	//trakAudio.mp4aBox;	//归属在stsd_box中一起初始化了
	//trakAudio.esdsBox;	//归属在stsd_box中一起初始化了
	trakAudio.sttsBox = stts_box_init();
	trakAudio.stscBox = stsc_box_init();
	trakAudio.stszBox = stsz_box_init();
	trakAudio.stcoBox = stco_box_init();

	return &trakAudio;
	
}

traf_video_t trafVideo = {0};
traf_video_t* traf_video_init()
{
	trafVideo.trafBox = traf_box_init(sizeof(traf_box));
	trafVideo.tfhdBox = tfhd_box_init(VIDEO_TRACK);
	trafVideo.tfdtBox = tfdt_box_init(0);//传参： baseMediaDecodeTime 未知用途
	trafVideo.trunBox = trun_box_init();//未实现完

	return &trafVideo;
}

traf_audio_t trafAudio = {0};
traf_audio_t* traf_audio_init()
{
	trafAudio.trafBox = traf_box_init(sizeof(traf_box));
	trafAudio.tfhdBox = tfhd_box_init(AUDIO_TRACK);
	trafAudio.tfdtBox = tfdt_box_init(0);//传参： baseMediaDecodeTime 未知用途
	trafAudio.trunBox = trun_box_init();//未实现完

	return &trafAudio;
}



fmp4_file_box_t fmp4BOX = {0}; 
fmp4_file_box_t* fmp4_box_init()
{
	fmp4BOX.ftypBox = ftyp_box_init();
	fmp4BOX.moovBox = moov_box_init(sizeof(moov_box));
	fmp4BOX.mvhdBox = mvhd_box_init(1000,0);

	//初始化 video trak box
	trak_video_init_t args_video = {0};
	args_video.width  = 1920;
	args_video.height = 1080;
	args_video.timescale = 90000;
	args_video.duration = 0;
	fmp4BOX.trak_video = trak_video_init(&args_video);

	//初始化 audio trak box
	trak_audio_init_t args_audio = {0};
	args_audio.timescale = 8000;
	args_audio.duration = 0;
	args_audio.channelCount = 1;
	args_audio.sampleRate = 8000;
	fmp4BOX.trak_audio = trak_audio_init(&args_audio);
	fmp4BOX.mvexBox = mvex_box_init(0);
	fmp4BOX.trex_video = trex_box_init(VIDEO_TRACK);//里边涉及到sample duration等参数
	fmp4BOX.trex_audio = trex_box_init(AUDIO_TRACK);
	fmp4BOX.moofBox = moof_box_init(sizeof(moof_box));
	fmp4BOX.mfhdBox = mfhd_box_init();
	fmp4BOX.traf_video = traf_video_init();
	fmp4BOX.traf_audio = traf_audio_init();
	fmp4BOX.mdatBox = mdat_box_init(sizeof(mdat_box));
	
	return &fmp4BOX;

}



/*
功能： 	创建一个fmp4文件并初始化各个box,返回文件描述符
返回： 	成功 ： 文件描述符
		失败 ： -1;
*/
Fmp4FileId Fmp4_File_init(char * file_name)
{
	if(NULL == file_name)
		return -1;
	int file_handle = open(file_name, O_RDWR|O_CREAT, 0666);
	if(file_handle < 0)
	{
		ERROR_LOG("open fmp4 file failed!\n");
		return -1;
	}

	//box_init
		
	
}








