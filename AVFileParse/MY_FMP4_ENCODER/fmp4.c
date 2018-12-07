#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>



#include "typeport.h"
#include "fmp4.h"
#include "fmp4_interface.h"


FILE* file_handle = NULL;   //fmp4文件描述符


Fmp4TrackId AddVideoTrack()
{


	
	//暂不实现 ，默认一条视频轨道 ：VIDEO_TRACK
	//			     一条音频轨道 ：AUDIO_TRACK

}



trak_video_t trakVideo = {0};
trak_video_t*	trak_video_init(trak_video_init_t*args)
{
	/*
	前期不好确定各个容器box的最终长度，所以在初始化时都只按照自身的结构体长度计算，
	待各个box数据都完成写入后再统一计算各个box的最终长度以及合成最终的FMP4文件。
	*/
	trakVideo.trakBox = trak_box_init(sizeof(trak_box));
	trakVideo.tkhdBox = tkhd_box_init(VIDEO_TRACK, args->duration,args->width,args->height);
	trakVideo.mdiaBox = mdia_box_init(sizeof(mdia_box));
	trakVideo.mdhdBox = mdhd_box_init(args->timescale, args->duration);
	trakVideo.hdlrBox = hdlr_box_init();
	trakVideo.minfBox = minf_box_init(sizeof(minf_box));
	trakVideo.vmhdBox = vmhd_box_init();
	trakVideo.dinfBox = dinf_box_init(sizeof(dinf_box));
	trakVideo.drefBox = dref_box_init();
	//trakVideo.urlBox  = url_box_init(); //不需要
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
	//trakAudio.url_box = url_box_init();//不需要
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
注意：初始化时，各个容器 box的长度信息都默认为无子box时的长度
*/
FILE* Fmp4_encode_init(char * file_name,unsigned int Vframe_rate,unsigned int Aframe_rate)
{
	if(NULL == file_name)
		return NULL;
	file_handle = fopen(file_name, "wb+");
	if(NULL == file_handle )
	{
		ERROR_LOG("open fmp4 file failed!\n");
		return NULL;
	}

	//box_init
	fmp4_file_box_t* box =  fmp4_box_init();

	int ret = 0;
	int curr_offset = 0;
	
	//write ftyp
	fwrite_box(box->ftypBox,1,box->ftypBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.ftypBox_offset = curr_offset;
	
	//write moov
	fwrite_box(box->moovBox,1,box->moovBox->header.size,file_handle,ret);
	curr_offset +=ret;
	fmp4_file_lable.moovBox_offset = curr_offset;
	

	//write mvhd
	fwrite_box(box->mvhdBox,1,box->mvhdBox->header.size,file_handle,ret);
	curr_offset +=ret;
	fmp4_file_lable.mvhdBox_offset = curr_offset;


#if HAVE_VIDEO
	//write trak_video
	//trak
	fwrite_box(box->trak_video->trakBox,1,box->trak_video->trakBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.trakBox_offset = curr_offset;

	//tkhd
	fwrite_box(box->trak_video->tkhdBox,1,box->trak_video->tkhdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.tkhdBox_offset = curr_offset;

	//mdia
	fwrite_box(box->trak_video->mdiaBox,1,box->trak_video->mdiaBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.mdiaBox_offset = curr_offset;


	//mdhd
	fwrite_box(box->trak_video->mdhdBox,1,box->trak_video->mdhdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.mdhdBox_offset = curr_offset;

	//hdlr
	fwrite_box(box->trak_video->hdlrBox,1,box->trak_video->hdlrBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.hdlrBox_offset = curr_offset;

	//minf
	fwrite_box(box->trak_video->minfBox,1,box->trak_video->minfBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.minfBox_offset = curr_offset;

	//vmhd
	fwrite_box(box->trak_video->vmhdBox,1,box->trak_video->vmhdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.vmhdBox_offset = curr_offset;

	//dinf
	fwrite_box(box->trak_video->dinfBox,1,box->trak_video->dinfBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.dinfBox_offset = curr_offset;

	//dref
	fwrite_box(box->trak_video->drefBox,1,box->trak_video->drefBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.drefBox_offset = curr_offset;

	//url
	fwrite_box(box->trak_video->urlBox,1,box->trak_video->urlBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.urlBox_offset = curr_offset;

	//stbl
	fwrite_box(box->trak_video->stblBox,1,box->trak_video->stblBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stblBox_offset = curr_offset;

	//stsd
	fwrite_box(box->trak_video->stsdBox,1,box->trak_video->stsdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stsdBox_offset = curr_offset;

	//stts
	fwrite_box(box->trak_video->sttsBox,1,box->trak_video->sttsBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.sttsBox_offset = curr_offset;

	//stsc
	fwrite_box(box->trak_video->stscBox,1,box->trak_video->stscBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stscBox_offset = curr_offset;

	//stsz
	fwrite_box(box->trak_video->stszBox,1,box->trak_video->stszBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stszBox_offset = curr_offset;

	//stco
	fwrite_box(box->trak_video->stcoBox,1,box->trak_video->stszBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stszBox_offset = curr_offset;

#endif

#if HAVE_AUDIO
	//write_trak_audio
	//trak
	fwrite_box(box->trak_audio->trakBox,1,box->trak_audio->trakBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.trakBox_offset = curr_offset;

	//tkhd
	fwrite_box(box->trak_audio->tkhdBox,1,box->trak_audio->tkhdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.tkhdBox_offset = curr_offset;

	//mdia
	fwrite_box(box->trak_audio->mdiaBox,1,box->trak_audio->mdiaBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.mdiaBox_offset = curr_offset;

	//mdhd
	fwrite_box(box->trak_audio->mdhdBox,1,box->trak_audio->mdhdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.mdhdBox_offset = curr_offset;

	//hdlr
	fwrite_box(box->trak_audio->hdlrBox,1,box->trak_audio->hdlrBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.hdlrBox_offset = curr_offset;

	//minf
	fwrite_box(box->trak_audio->minfBox,1,box->trak_audio->minfBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.minfBox_offset = curr_offset;

	//smhd
	fwrite_box(box->trak_audio->smhdBox,1,box->trak_audio->smhdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.smhdBox_offset = curr_offset;

	//dinf
	fwrite_box(box->trak_audio->dinfBox,1,box->trak_audio->dinfBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.dinfBox_offset = curr_offset;

	//dref
	fwrite_box(box->trak_audio->drefBox,1,box->trak_audio->drefBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.drefBox_offset = curr_offset;

	//url
	fwrite_box(box->trak_audio->url_box,1,box->trak_audio->url_box->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.url_box_offset = curr_offset;

	//stbl
	fwrite_box(box->trak_audio->stblBox,1,box->trak_audio->stblBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stblBox_offset = curr_offset;

	//stsd
	fwrite_box(box->trak_audio->stsdBox,1,box->trak_audio->stsdBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stsdBox_offset = curr_offset;

	//stts
	fwrite_box(box->trak_audio->sttsBox,1,box->trak_audio->sttsBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.sttsBox_offset = curr_offset;

	//stsc
	fwrite_box(box->trak_audio->stscBox,1,box->trak_audio->stscBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stscBox_offset = curr_offset;

	//stsz
	fwrite_box(box->trak_audio->stszBox,1,box->trak_audio->stszBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stszBox_offset = curr_offset;

	//stco
	fwrite_box(box->trak_audio->stcoBox,1,box->trak_audio->stcoBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stcoBox_offset = curr_offset;
	
#endif

	//mvex
	fwrite_box(box->mvexBox,1,box->mvexBox->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.mvexBox_offset = curr_offset;

#if HAVE_VIDEO
	//trex_video
	fwrite_box(box->trex_video,1,box->trex_video->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trex_video_offset = curr_offset;
#endif

#if HAVE_AUDIO
	//trex_audio
	fwrite_box(box->trex_audio,1,box->trex_audio->header.size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trex_audio_offset = curr_offset;
#endif

	/*随后就是 n*(moof + mdat)结构，将由“音视频混合相关业务”部分来写
	  包括更新已经写入文件的 box (trak、trex)
	*/

	ret = remux_init(Vframe_rate,Aframe_rate);// video/audio混合器 初始化
	if(ret < 0)
	{
		ERROR_LOG("remux init faied!\n");
		fclose(file_handle);
		return NULL;
	}

	DEBUG_LOG("fmp4 file init success!\n");
	return file_handle;
	
	
}



/*===remux===================================================================
音视频混合相关业务
n*(moof + mdat)模式
封装：只要 remuxVideo 或者 remuxAideo 缓存满1S数据，就进行封装box，写入文件的操作。
1.将传入的原始数据封装成mdat box(包含真实数据) 
2.依据传入的原始数据生成对应的    samples的描述信息(moof box)
3.将缓存好的原始数据及描述信息封装成moof box 和 mdat box 填充到fmp4文件
===========================================================================*/

/*
int video_sample_flags()
{
   	char isLeading =  0;
    char dependsOn = keyframe ? 2 : 1;
    char isDependedOn = keyframe ? 1 : 0;
    char hasRedundancy = 0;
    char isNonSync = keyframe ? 0 : 1;
	
}
*/

/*
	frame_rate:是外部传入视频数据的原有帧率
	返回值：失败：-1  		 成功：0
*/
static unsigned char remux_run = 0;	//remux 运行标志（1：继续运行 0：退出）
static unsigned char remux_write_last_ok = 0;	//	remux完成最后一轮数据的写入操作（即将进行释放操作）
static unsigned char remux_v_can_write = 0; // buf_remux_video 中video的数据是否已经缓存够1s,可以封box写入文件
static unsigned char remux_A_can_write = 0; // buf_remux_audio 中audio的数据是否已经缓存够1s,可以封box写入文件
int	remuxVideo(void *video_frame,unsigned int frame_length,unsigned int frame_rate)
{
	if(NULL == video_frame)
	{
		ERROR_LOG("Illegal parameter!\n");
		return -1;
	}
	if(NULL == buf_remux_video.remux_video_buf)
	{
		ERROR_LOG("remux video not init!\n");
		return -1;
	}
	
	 //将该帧放到暂存区，最大只存1S的数据
	if(buf_remux_video.frame_count < buf_remux_video.frame_rate)//缓存区的帧数不够1S，继续将帧数据放入缓存区
	{
		if(buf_remux_video.write_pos + frame_length > buf_remux_video.remux_video_buf + REMUX_VIDEO_BUF_SIZE)
		{
			//缓存大小不够，退出。
			ERROR_LOG("remux_video_buf is not enough to story one mdat box data !\n");
			free(buf_remux_video.remux_video_buf);
			return -1;
		}
		pthread_mutex_lock(&buf_remux_video.mut);
			memcpy(buf_remux_video.write_pos , video_frame , frame_length);
			buf_remux_video.write_pos += frame_length;
			buf_remux_video.frame_count ++;
			buf_remux_video.sample_info[buf_remux_video.write_index].sample_pos = buf_remux_video.write_pos;
			buf_remux_video.sample_info[buf_remux_video.write_index].sample_len = frame_length;
			
			//保存sample的信息，   用来更新moof 里边 video traf 下相关 box 的信息(主要是 trun box)
			buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_duration = 1000/frame_rate;
			buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_size = frame_length;
			buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_flags = 0;
			//buf_remux_video.sample_info[buf_remux_video.curr_index].trun_sample.sample_composition_time_offset = ;
			buf_remux_video.write_index ++;
			if(buf_remux_video.write_index > TRUN_VIDEO_MAX_SAMPLES)
			{
				ERROR_LOG("buf_remux_video.sample_info overflow !\n");
				return -1;
			}
			
		pthread_mutex_unlock(&buf_remux_video.mut);

	}
	
	/*
	1.此处不能用 else 衔接，因 上衣个if 出来时可能 buf_remux_video.frame_count 恰好 = buf_remux_video.frame_rate，
		用 else 会直接跳过如下写入操作，等下一帧进来时才会执行如下操作，但是这个“下一帧”会被丢掉
	2.还需要确保video 部分的box要写在 audio box的前边，不然会出问题。
	*/
	if(buf_remux_video.frame_count >= buf_remux_video.frame_rate) //存够了1S的数据,就更新 fmp4BOX.traf_video 分支 
	{
		remux_v_can_write = 1;
		while(remux_v_can_write)//等待写入操作成功
		{
			usleep(1000);
		}

		//指针归位，buf又循环重写 
		pthread_mutex_lock(&buf_remux_video.mut);		
		//pthread_cond_wait(&buf_remux_video.remux_ready, &buf_remux_video.mut);
		buf_remux_video.write_pos = buf_remux_video.remux_video_buf;
		buf_remux_video.read_pos = buf_remux_video.remux_video_buf;
		buf_remux_video.frame_count = 0;
		buf_remux_video.write_index = 0;
		pthread_mutex_unlock(&buf_remux_video.mut);
	}

	return 0;
	
}

/*
	frame_rate:是外部传入音频数据的原有帧率
	返回值：失败：-1  		 成功：0
*/
int	remuxAudio(void *audio_frame,unsigned int frame_length,unsigned int frame_rate)
{
	if(NULL == audio_frame)
	{
		ERROR_LOG("Illegal parameter!\n");
		return -1;
	}
	if(NULL == buf_remux_audio.remux_audio_buf)
	{
		ERROR_LOG("remux audio not init!\n");
		return -1;
	}

	 //将该帧放到暂存区
	if(buf_remux_audio.frame_count < buf_remux_audio.frame_rate)//缓存区的帧数不够1S，继续将帧数据放入缓存区
	{
		if(buf_remux_audio.write_pos + frame_length > buf_remux_audio.remux_audio_buf + REMUX_VIDEO_BUF_SIZE)
		{
			//缓存大小不够，退出。
			ERROR_LOG("remux_audio_buf is not enough to story one mdat box data !\n");
			free(buf_remux_audio.remux_audio_buf);
			return -1;
		}
		pthread_mutex_lock(&buf_remux_audio.mut);
		memcpy(buf_remux_audio.write_pos , audio_frame , frame_length);
		buf_remux_audio.write_pos += frame_length;
		buf_remux_audio.frame_count ++;

		buf_remux_audio.sample_info[buf_remux_audio.write_index].sample_pos = buf_remux_audio.write_pos;
		buf_remux_audio.sample_info[buf_remux_audio.write_index].sample_len = frame_length;
		
		//保存sample的信息，   用来更新moof 里边 video traf 下相关 box 的信息(主要是 trun box)
		buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_duration = 1000/frame_rate;
		buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_size = frame_length;
		buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_flags = 0;
		//buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_composition_time_offset = ;
		buf_remux_audio.write_index ++;
		if(buf_remux_audio.write_index > TRUN_VIDEO_MAX_SAMPLES)
		{
			ERROR_LOG("buf_remux_video.sample_info overflow !\n");
			return -1;
		}
		
		pthread_mutex_unlock(&buf_remux_audio.mut);
		
	}
	else //存够了1S的数据，开始写入到 mdat box  
	{	
		remux_A_can_write = 1;
		while(remux_A_can_write)//等待写入操作成功
		{
			usleep(1000);
		}

		//指针归位，buf又循环重写 
		pthread_mutex_lock(&buf_remux_audio.mut);
		//pthread_cond_wait(&buf_remux_audio.remux_ready, &buf_remux_audio.mut);
		buf_remux_audio.write_pos = buf_remux_audio.remux_audio_buf;
		buf_remux_audio.read_pos = buf_remux_audio.remux_audio_buf;
		buf_remux_audio.frame_count = 0;
		buf_remux_audio.write_index = 0;	
		pthread_mutex_unlock(&buf_remux_audio.mut);
	}

	return 0;
	
}


/*	
	音视频混合线程主要功能函数
	将获得的1S时间的音视频结合，生成 mdat box，写入fmp4文件
*/
void * remuxVideoAudio(void *args)
{
	pthread_detach(pthread_self());	
	
	if(NULL == buf_remux_video.remux_video_buf||\
	   NULL == buf_remux_audio.remux_audio_buf)
	{
		ERROR_LOG("remux not init!\n");
		pthread_exit(0);
	}

	do
	{
		/*
		!remux_run: 外部要求fmp4编码结束的情况，此时需要将remux buf 中剩余的数据写入到文件当中去才能够释放;
		remux_v_can_write||remux_A_can_write: remuxVideo 或者 remuxAideo 缓存满1S数据，就进行封装box写入文件的操作,
		*/
		if(!remux_run||remux_v_can_write || remux_A_can_write)
		{

			//===视频部分 moof + mdat box 写入文件==========================================================================
			//---修正 moof   box下video相关 box的长度信息---------------------------------------------------------------------
		#if HAVE_VIDEO
			pthread_mutex_lock(&buf_remux_video.mut);
			//更新 trun box 的长度,需要加上samples的描述信息长度
			int i = 0;
			for(i = 0 ; i < buf_remux_video.write_index; i++)
			{
				fmp4BOX.traf_video->trunBox->header.size += buf_remux_video.sample_info[i].trun_sample.sample_size;
			}
			
			//更新 traf box长度
			fmp4BOX.traf_video->trafBox->header.size += fmp4BOX.traf_video->tfhdBox->header.size\
														  + fmp4BOX.traf_video->tfdtBox->header.size\
														  + fmp4BOX.traf_video->trunBox->header.size;
			
			//更新 moof box长度
			fmp4BOX.moofBox->header.size +=  fmp4BOX.mfhdBox->header.size\
											+ fmp4BOX.traf_video->trafBox->header.size;/*其中，audio traf box中不需要再加一次*/

			//更新 mdat box长度
			fmp4BOX.mdatBox->header.size += buf_remux_video.write_pos - buf_remux_video.remux_video_buf;

			//转化成大端格式：
			fmp4BOX.traf_video->trunBox->header.size = htonl(fmp4BOX.traf_video->trunBox->header.size);
			fmp4BOX.traf_video->trafBox->header.size = htonl(fmp4BOX.traf_video->trafBox->header.size);
			//以下两个后边的音频还需要继续更新长度，这里先不要转化成大端模式
			//fmp4BOX.moofBox->header.size = htonl(fmp4BOX.moofBox->header.size);
			//fmp4BOX.mdatBox->header.size = htonl(fmp4BOX.mdatBox->header.size);

			
					
			//-----将视频部分的 traf + mdat box   	 写入到文件--------------------------------------------------------------------------------
			//moof
			fmp4_file_lable.moofBox_offset = ftell(file_handle);
			fwrite(fmp4BOX.moofBox,1,sizeof(moof_box),file_handle);   //audio traf box 中不需要再写一次

			//mfhd
			fmp4_file_lable.mfhdBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.mfhdBox,1,sizeof(mfhd_box),file_handle); //audio traf box 中不需要再写一次

			//traf
			fmp4_file_lable.traf_video_offset.trafBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->trafBox ,1,sizeof(traf_box),file_handle);

			//tfhd
			fmp4_file_lable.traf_video_offset.tfhdBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->tfhdBox ,1,sizeof(tfhd_box),file_handle);

			//tfdt
			fmp4_file_lable.traf_video_offset.tfdtBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->tfdtBox ,1,sizeof(tfdt_box),file_handle);

			//trun
			fmp4_file_lable.traf_video_offset.trunBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->trunBox ,1,sizeof(trun_box),file_handle);
			//trun--->samples info
			
			for(i = 0;i < buf_remux_video.write_index;i++)
			{
				fwrite(&buf_remux_video.sample_info[i].trun_sample ,1,sizeof(trun_sample_t),file_handle);
			}
			
			pthread_mutex_unlock(&buf_remux_video.mut);
		#endif
  
			//===音频部分 moof + mdat box 写入文件==========================================================================
			//---修正 moof   box下 audio 相关 box的长度信息---------------------------------------------------------------------
		#if HAVE_AUDIO
			pthread_mutex_lock(&buf_remux_audio.mut);
			//更新 trun box 的长度,需要加上samples的描述信息长度
			
			for(i = 0 ; i < buf_remux_audio.write_index; i++)
			{
				fmp4BOX.traf_audio->trunBox->header.size  += buf_remux_audio.sample_info[i].trun_sample.sample_size;
			}
			
			//更新 traf box长度
			fmp4BOX.traf_audio->trafBox->header.size += fmp4BOX.traf_audio->tfhdBox->header.size\
														  + fmp4BOX.traf_audio->tfdtBox->header.size\
														  + fmp4BOX.traf_audio->trunBox->header.size;
			
			//更新 moof box长度
			fmp4BOX.moofBox->header.size += fmp4BOX.traf_audio->trafBox->header.size;

			//更新 mdat box长度
			fmp4BOX.mdatBox->header.size += buf_remux_audio.write_pos - buf_remux_audio.remux_audio_buf;

			//转化成大端格式：
			fmp4BOX.traf_video->trunBox->header.size = htonl(fmp4BOX.traf_video->trunBox->header.size);
			fmp4BOX.traf_video->trafBox->header.size = htonl(fmp4BOX.traf_video->trafBox->header.size);
			fmp4BOX.moofBox->header.size = htonl(fmp4BOX.moofBox->header.size);
			fmp4BOX.mdatBox->header.size = htonl(fmp4BOX.mdatBox->header.size);
					
			//-----将音频部分的 traf       box 写入到文件--------------------------------------------------------------------------------
			//moof
			fmp4_file_lable.moofBox_offset = ftell(file_handle);
			fwrite(fmp4BOX.moofBox,1,sizeof(moof_box),file_handle);   //audio traf box 中不需要再写一次

			//mfhd
			fmp4_file_lable.mfhdBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.mfhdBox,1,sizeof(mfhd_box),file_handle); //audio traf box 中不需要再写一次

			//traf
			fmp4_file_lable.traf_video_offset.trafBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->trafBox ,1,sizeof(traf_box),file_handle);

			//tfhd
			fmp4_file_lable.traf_video_offset.tfhdBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->tfhdBox ,1,sizeof(tfhd_box),file_handle);

			//tfdt
			fmp4_file_lable.traf_video_offset.tfdtBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->tfdtBox ,1,sizeof(tfdt_box),file_handle);

			//trun
			fmp4_file_lable.traf_video_offset.trunBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.traf_video->trunBox ,1,sizeof(trun_box),file_handle);
			//trun--->samples info
			
			for(i = 0;i < buf_remux_video.write_index;i++)
			{
				fwrite(&buf_remux_video.sample_info[i].trun_sample ,1,sizeof(trun_sample_t),file_handle);
			}
			pthread_mutex_unlock(&buf_remux_audio.mut);
		#endif
		
			//===mdat box 部分写入文件======================================================================================
			//视频 samples data部分
		#if HAVE_VIDEO
			pthread_mutex_lock(&buf_remux_video.mut);		
			fmp4_file_lable.mdatBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.mdatBox ,1,sizeof(mdat_box),file_handle); 
			//mdat--->samples data
			fwrite(buf_remux_video.remux_video_buf ,1,buf_remux_video.write_pos-buf_remux_video.remux_video_buf,file_handle);
			pthread_mutex_unlock(&buf_remux_video.mut);
		#endif
			
		//音频 samples data  部分
		#if HAVE_AUDIO
			pthread_mutex_lock(&buf_remux_audio.mut);
			fmp4_file_lable.mdatBox_offset = ftell(file_handle);	
			fwrite(fmp4BOX.mdatBox ,1,sizeof(mdat_box),file_handle); 
			//mdat--->samples data
			fwrite(buf_remux_video.remux_video_buf ,1,buf_remux_video.write_pos-buf_remux_video.remux_video_buf,file_handle);
			pthread_mutex_unlock(&buf_remux_audio.mut);
		#endif

			//通知主线程，缓冲区复位，缓冲新数据
			remux_v_can_write = 0;
			remux_A_can_write = 0;

		/*	
		#if HAVE_VIDEO
			pthread_mutex_lock(&buf_remux_video.mut);
			pthread_cond_signal(&buf_remux_video.remux_ready);
			pthread_mutex_unlock(&buf_remux_video.mut);
		#endif

		#if HAVE_AUDIO
			pthread_mutex_lock(&buf_remux_audio.mut);
			pthread_cond_signal(&buf_remux_audio.remux_ready);
			pthread_mutex_unlock(&buf_remux_audio.mut);
		#endif
		*/

		}
		else
		{
			usleep(1000);
		}

	}
	while(remux_run);

	remux_write_last_ok = 1; 

	pthread_exit(0);
	
}


/*
	参数：
		Vframe_rate：传入视频的原本帧率
		Aframe_rate: 传入音频的原本帧率
	返回值：成功：0 失败：-1
*/
int remux_init(unsigned int Vframe_rate,unsigned int Aframe_rate)
{
	
	//===初始化 remux video部分的buf========================================
	buf_remux_video.remux_video_buf = (char *)malloc(REMUX_VIDEO_BUF_SIZE);
	if(NULL == buf_remux_video.remux_video_buf)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
	memset(buf_remux_video.remux_video_buf ,0,REMUX_VIDEO_BUF_SIZE);

	buf_remux_video.write_pos = buf_remux_video.remux_video_buf;
	buf_remux_video.read_pos = buf_remux_video.remux_video_buf;	
	buf_remux_video.frame_count = 0;
	buf_remux_video.frame_rate = Vframe_rate; 
	memset(buf_remux_video.sample_info,0,TRUN_VIDEO_MAX_SAMPLES);
	buf_remux_video.write_index = 0;
	buf_remux_video.read_index = 0;
	pthread_mutex_init(&buf_remux_video.mut,NULL);
	//buf_remux_video.cond_ready = PTHREAD_COND_INITIALIZER;



	DEBUG_LOG("video frame rate(%d) ",buf_remux_video.frame_rate);

	//===初始化 remux audio 部分的buf=======================================
	buf_remux_audio.remux_audio_buf = (char*)malloc(REMUX_AUDIO_BUF_SIZE);
	if(NULL == buf_remux_audio.remux_audio_buf)
	{
		ERROR_LOG("malloc failed!\n");
		free(buf_remux_video.remux_video_buf);
		return -1;
	}
	memset(buf_remux_audio.remux_audio_buf ,0,REMUX_AUDIO_BUF_SIZE);
	buf_remux_audio.write_pos = buf_remux_audio.remux_audio_buf;
	buf_remux_audio.read_pos = buf_remux_audio.remux_audio_buf;
	buf_remux_audio.frame_count = 0;
	buf_remux_audio.frame_rate = Aframe_rate;
	memset(buf_remux_audio.sample_info,0,TRUN_AUDIO_MAX_SAMPLES);
	buf_remux_audio.write_index = 0;
	buf_remux_audio.read_index = 0;
	pthread_mutex_init(&buf_remux_audio.mut,NULL);
	//buf_remux_audio.remux_ready = PTHREAD_COND_INITIALIZER;
	
	

	DEBUG_LOG("audio frame rate(%d) ",buf_remux_audio.frame_rate);
	

	//===启动音视频混合程序===================================================
	pthread_t tid;
	remux_run = 1;
	int ret = pthread_create(&tid, NULL, remuxVideoAudio, NULL);
	if (ret) 
	{
		ERROR_LOG("pthread_create fail: %d\n", ret);
		return -1;
	}
	DEBUG_LOG("remuxVideoAudio Thread start success! tid(%d) \n",tid);
	
	return 0;
	

}

int remux_exit(void)
{
	remux_run = 0;//通知remux 业务退出
	//需要等待remux业务将buf中最后没有存满1s的数据写入文件
	if(!remux_write_last_ok)
	{
		usleep(1000);//1ms
	}
	
	//===释放=======================================
	pthread_mutex_destroy(&buf_remux_video.mut);  
   	//pthread_cond_destroy(&buf_remux_audio.mut);
	free(buf_remux_video.remux_video_buf);
	free(buf_remux_audio.remux_audio_buf);

	buf_remux_video.remux_video_buf = NULL;
	buf_remux_audio.remux_audio_buf = NULL;

	fclose(file_handle);

	return 0;

}


/*=======================================================================================================
对外接口部分
========================================================================================================*/

/*
	开始编码前该接口需要先接收 sps/pps NALU 包，用来设置 avcC box参数，
	否则的话解码器不能正常解码！！！
	返回值： 成功：0 失败 -1;
	注意：该接口内部会对naluData自动偏移5个字节长度
*/
int sps_pps_parameter_set(unsigned char* naluData, int naluSize)
{
	
	avcc_box_info_t *avcc_buf = avcc_box_init(naluData,naluSize);
	if(NULL == avcc_buf)
	{
		ERROR_LOG("sps_pps_parameter_set failed !\n");
		return -1;
	}

	return 0;
}

int Fmp4AEncode(void * audio_frame, unsigned int frame_length, unsigned int frame_rate)
{
	int ret = remuxAudio(audio_frame,frame_length,frame_rate);
	return ret;
}

int Fmp4VEncode(void *video_frame,unsigned int frame_length,unsigned int frame_rate)
{
	int ret = remuxVideo(video_frame,frame_length,frame_rate);
	return ret;
}


int Fmp4_encode_exit(void)
{
	
	int ret = remux_exit();
	return ret;
}





