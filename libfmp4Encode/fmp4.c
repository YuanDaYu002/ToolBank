


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "my_inet.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>




#include "typeport.h"
#include "fmp4.h"
//#include "fmp4_interface.h"


FILE* file_handle = NULL;   //fmp4文件描述符


Fmp4TrackId AddVideoTrack(void)
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
	DEBUG_LOG("trak_video_init  \n");
	

	trakVideo.trakBox = trak_box_init(sizeof(trak_box));
	if(NULL == trakVideo.trakBox)
	{
		ERROR_LOG("trak_box_init failed!\n");
		return NULL;
	}

	trakVideo.tkhdBox = tkhd_box_init(VIDEO_TRACK, args->duration,args->width,args->height);
	if(NULL == trakVideo.tkhdBox)
	{
		ERROR_LOG("tkhd_box_init failed!\n");
		return NULL;
	}

	trakVideo.mdiaBox = mdia_box_init(sizeof(mdia_box));
	if(NULL == trakVideo.mdiaBox)
	{
		ERROR_LOG("mdia_box_init failed!\n");
		return NULL;
	}
	
	trakVideo.mdhdBox = mdhd_box_init(args->timescale, args->duration);
	if(NULL == trakVideo.mdhdBox)
	{
		ERROR_LOG("mdhd_box_init failed!\n");
		return NULL;
	}

	trakVideo.hdlrBox = hdlr_box_init(VIDEO_HANDLER);
	if(NULL == trakVideo.hdlrBox)
	{
		ERROR_LOG("hdlr_box_init failed!\n");
		return NULL;
	}
	

	trakVideo.minfBox = minf_box_init(sizeof(minf_box));
	if(NULL == trakVideo.minfBox)
	{
		ERROR_LOG("minf_box_init failed!\n");
		return NULL;
	}

	trakVideo.vmhdBox = vmhd_box_init();
	if(NULL == trakVideo.vmhdBox)
	{
		ERROR_LOG("vmhd_box_init failed!\n");
		return NULL;
	}

	trakVideo.dinfBox = dinf_box_init(sizeof(dinf_box));
	if(NULL == trakVideo.dinfBox)
	{
		ERROR_LOG("dinf_box_init failed!\n");
		return NULL;
	}

	trakVideo.drefBox = dref_box_init();
	if(NULL == trakVideo.drefBox)
	{
		ERROR_LOG("dref_box_init failed!\n");
		return NULL;
	}


	//trakVideo.urlBox  = url_box_init(); //不需要
	trakVideo.stblBox = stbl_box_init(sizeof(stbl_box));
	if(NULL == trakVideo.stblBox)
	{
		ERROR_LOG("stbl_box_init failed!\n");
		return NULL;
	}


	trakVideo.stsdBox = stsd_box_init(VIDEO,args->width,args->height,0,0);
	if(NULL == trakVideo.stsdBox)
	{
		ERROR_LOG("stsd_box_init failed!\n");
		return NULL;
	}
	//trakVideo.avc1Box; 	//归属在stsd_box中一起初始化了
	//trakVideo.avccBox;	//归属在stsd_box中一起初始化了
	//trakVideo.paspBox;	//暂未实现


	trakVideo.sttsBox = stts_box_init();
	if(NULL == trakVideo.sttsBox)
	{
		ERROR_LOG("stts_box_init failed!\n");
		return NULL;
	}


	trakVideo.stscBox = stsc_box_init();
	if(NULL == trakVideo.stscBox)
	{
		ERROR_LOG("stsc_box_init failed!\n");
		return NULL;
	}


	trakVideo.stszBox = stsz_box_init();
	if(NULL == trakVideo.stszBox)
	{
		ERROR_LOG("stsz_box_init failed!\n");
		return NULL;
	}


	trakVideo.stcoBox = stco_box_init();
	if(NULL == trakVideo.stcoBox)
	{
		ERROR_LOG("stco_box_init failed!\n");
		return NULL;
	}



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
	if(NULL == trakAudio.trakBox)
	{
		ERROR_LOG("trak_box_init failed !\n");
		return NULL;
	}

	
	trakAudio.tkhdBox = tkhd_box_init(AUDIO_TRACK, args->duration,0,0);
	if(NULL == trakAudio.tkhdBox)
	{
		ERROR_LOG("tkhd_box_init failed !\n");
		return NULL;
	}


	trakAudio.mdiaBox = mdia_box_init(sizeof(mdia_box));
	if(NULL == trakAudio.mdiaBox)
	{
		ERROR_LOG("mdia_box_init failed !\n");
		return NULL;
	}
	
	trakAudio.mdhdBox = mdhd_box_init(args->timescale,args->duration);
	if(NULL == trakAudio.mdhdBox)
	{
		ERROR_LOG("mdhd_box_init failed !\n");
		return NULL;
	}

	
	trakAudio.hdlrBox = hdlr_box_init(AUDIO_HANDLER);
	if(NULL == trakAudio.hdlrBox)
	{
		ERROR_LOG("hdlr_box_init failed !\n");
		return NULL;
	}
	
	trakAudio.minfBox = minf_box_init(sizeof(minf_box));
	if(NULL == trakAudio.minfBox)
	{
		ERROR_LOG("minf_box_init failed !\n");
		return NULL;
	}

	trakAudio.smhdBox = smhd_box_init();
	if(NULL == trakAudio.smhdBox)
	{
		ERROR_LOG("smhd_box_init failed !\n");
		return NULL;
	}
	
	trakAudio.dinfBox = dinf_box_init(sizeof(dinf_box));
	if(NULL == trakAudio.dinfBox)
	{
		ERROR_LOG("dinf_box_init failed !\n");
		return NULL;
	}
	
	trakAudio.drefBox = dref_box_init();
	if(NULL == trakAudio.drefBox)
	{
		ERROR_LOG("dref_box_init failed !\n");
		return NULL;
	}
		
	//trakAudio.url_box = url_box_init();//不需要
	trakAudio.stblBox = stbl_box_init(sizeof(stbl_box));
	if(NULL == trakAudio.stblBox)
	{
		ERROR_LOG("stbl_box_init failed !\n");
		return NULL;
	}

	trakAudio.stsdBox = stsd_box_init(AUDIO,0,0, args->channelCount, args->sampleRate);
	if(NULL == trakAudio.stsdBox)
	{
		ERROR_LOG("stsd_box_init failed !\n");
		return NULL;
	}
	
	#if 0
	  //------------------------------------------------------------------------DEBUG-----
while(1)
{
sleep(1);
DEBUG_LOG("sleeping ...\n");
}
#endif
		
	//trakAudio.mp4aBox;	//归属在stsd_box中一起初始化了
	//trakAudio.esdsBox;	//归属在stsd_box中一起初始化了
	trakAudio.sttsBox = stts_box_init();
	if(NULL == trakAudio.sttsBox)
	{
		ERROR_LOG("stts_box_init failed !\n");
		return NULL;
	}
		
	trakAudio.stscBox = stsc_box_init();
	if(NULL == trakAudio.stscBox)
	{
		ERROR_LOG("stsc_box_init failed !\n");
		return NULL;
	}
		
	trakAudio.stszBox = stsz_box_init();
	if(NULL == trakAudio.stszBox)
	{
		ERROR_LOG("stsz_box_init failed !\n");
		return NULL;
	}
		
	trakAudio.stcoBox = stco_box_init();
	if(NULL == trakAudio.stcoBox)
	{
		ERROR_LOG("stco_box_init failed !\n");
		return NULL;
	}

	return &trakAudio;
	
}

traf_video_t trafVideo = {0};
traf_video_t* traf_video_init(void)
{
	trafVideo.trafBox = traf_box_init(sizeof(traf_box));
	trafVideo.tfhdBox = tfhd_box_init(VIDEO_TRACK);
	trafVideo.tfdtBox = tfdt_box_init(0);//传参：初始化 baseMediaDecodeTime 为0 ，后边需要动态修改 
	trafVideo.trunBox = trun_box_init(VIDEO_TRACK);

	

	return &trafVideo;
}


traf_audio_t trafAudio = {0};
traf_audio_t* traf_audio_init(void)
{
	trafAudio.trafBox = traf_box_init(sizeof(traf_box));
	trafAudio.tfhdBox = tfhd_box_init(AUDIO_TRACK);
	trafAudio.tfdtBox = tfdt_box_init(0);//传参：初始化 baseMediaDecodeTime 为0 ，后边需要动态修改 
	trafAudio.trunBox = trun_box_init(AUDIO_TRACK);

	return &trafAudio;
}



fmp4_file_box_t fmp4BOX = {0}; 
fmp4_file_box_t* fmp4_box_init(unsigned short audio_sampling_rate)
{
	
	DEBUG_LOG("into fmp4_box_init 01\n");
	fmp4BOX.ftypBox = ftyp_box_init();
	if(NULL == fmp4BOX.ftypBox)
	{
		ERROR_LOG("ftyp_box_init failed!\n");
		return NULL;
	}
	
	fmp4BOX.moovBox = moov_box_init(sizeof(moov_box));
	if(NULL == fmp4BOX.moovBox)
	{
		ERROR_LOG("moov_box_init failed!\n");
		return NULL;
	}


	fmp4BOX.mvhdBox = mvhd_box_init(VIDEO_TIME_SCALE,0);
	if(NULL == fmp4BOX.mvhdBox)
	{
		ERROR_LOG("mvhd_box_init failed!\n");
		return NULL;
	}


	#if HAVE_VIDEO
	//初始化 video trak box
	trak_video_init_t args_video = {0};
	args_video.width  = 1920;
	args_video.height = 1080;
	args_video.timescale = VIDEO_TIME_SCALE;  	//作用 mdhd box
	args_video.duration = 0;					//作用 mdhd box
	fmp4BOX.trak_video = trak_video_init(&args_video);
	if(NULL == fmp4BOX.trak_video)
	{
		ERROR_LOG("trak_video_init failed!\n");
		return NULL;
	}

	#endif

	#if HAVE_AUDIO
		//初始化 audio trak box
		trak_audio_init_t args_audio = {0};
		args_audio.timescale = AUDIO_TIME_SCALE;  //作用 mdhd box
		args_audio.duration = 0;				  //作用 mdhd box
		args_audio.channelCount = 1;
		args_audio.sampleRate = audio_sampling_rate;//AUDIO_SOURCE_SAMPLE_RATE; //音频源数据样本率
		fmp4BOX.trak_audio = trak_audio_init(&args_audio);
		if(NULL == fmp4BOX.trak_audio)
		{
			ERROR_LOG("trak_audio_init failed!\n");
			return NULL;
		}
	#endif


	#if 0
							  //------------------------------------------------------------------------DEBUG-----
			  while(1)
			  {
				sleep(1);
				DEBUG_LOG("sleeping ...\n");
			  }
#endif

	fmp4BOX.mvexBox = mvex_box_init(sizeof(mvex_box));
	if(NULL == fmp4BOX.mvexBox)
	{
		ERROR_LOG("mvex_box_init failed!\n");
		return NULL;
	}
	
	
	#if HAVE_VIDEO
	
		fmp4BOX.trex_video = trex_box_init(VIDEO_TRACK);//里边涉及到sample duration等参数
		if(NULL == fmp4BOX.trex_video)
		{
			ERROR_LOG("trex_box_init failed!\n");
			return NULL;
		}
	#endif
	
		
	#if HAVE_AUDIO
		fmp4BOX.trex_audio = trex_box_init(AUDIO_TRACK);  //BUG
		if(NULL == fmp4BOX.trex_audio)
		{
			ERROR_LOG("trex_box_init failed!\n");
			return NULL;
		}
	#endif
		
	fmp4BOX.moofBox = moof_box_init(sizeof(moof_box));
	if(NULL == fmp4BOX.moofBox)
	{
		ERROR_LOG("moof_box_init failed!\n");
		return NULL;
	}
		
	fmp4BOX.mfhdBox = mfhd_box_init();
	if(NULL == fmp4BOX.mfhdBox)
	{
		ERROR_LOG("mfhd_box_init failed!\n");
		return NULL;
	}
		
	#if HAVE_VIDEO
		fmp4BOX.traf_video = traf_video_init();
		if(NULL == fmp4BOX.traf_video)
		{
			ERROR_LOG("traf_video_init failed!\n");
			return NULL;
		}
		
	#endif

	#if HAVE_AUDIO
		fmp4BOX.traf_audio = traf_audio_init();
		if(NULL == fmp4BOX.traf_audio)
		{
			ERROR_LOG("traf_audio_init failed!\n");
			return NULL;
		}
		
	#endif
	
	fmp4BOX.mdatBox = mdat_box_init(sizeof(mdat_box));
	if(NULL == fmp4BOX.mdatBox)
	{
		ERROR_LOG("mdat_box_init failed!\n");
		return NULL;
	}

	fmp4BOX.mfraBox = mfra_box_init();
	
	#if HAVE_VIDEO
		fmp4BOX.tfra_video = tfra_video_init();
	#endif

	#if HAVE_AUDIO
		fmp4BOX.tfra_audio = tfra_audio_init();
	#endif

	fmp4BOX.mfroBox = mfro_box_init();
		
	return &fmp4BOX;

}



/*
功能： 	创建一个fmp4文件并初始化各个box,返回文件描述符,初始化需要一帧IDR帧
返回： 	成功 ： 文件描述符
		失败 ： -1;
注意：初始化时，各个容器 box的长度信息都默认为无子box时的长度
*/
fmp4_out_info_t  out_info = {0};	//输出文件的存储信息
char out_mode = -1; 				//输出文件的存储模式
#define RECODE_AAC_FRAME_TO_FILE 0   //记录AAC帧，生成AAC文件 ，调试用
int AAC_fd;  //AAC文件描述符
//-------------------------------------------
#define USE_44100_AAC_FILE 0
#define AAC_4410_BUF_SIZE (1024)
int AAC_44100_fd; //标准44100HZ的AAC文件描述符
unsigned char * AAC_44100_buf = NULL; //临时缓存BUF
int  Fmp4_encode_init(fmp4_out_info_t * info,unsigned int Vframe_rate,unsigned int Aframe_rate,unsigned short audio_sampling_rate)
{

	if(NULL == info)
	{
		ERROR_LOG("info is NULL!\n");
		return -1;
	}
	memcpy(&out_info,info,sizeof(fmp4_out_info_t));
	
	if(out_info.buf_mode.buf_start != NULL)//两个模式都不为空时，默认使用buf_mode
	{
		out_mode = SAVE_IN_MEMORY;
	}
	else if(out_info.file_mode.file_name != NULL)
	{
		out_mode = SAVE_IN_FILE;
	}
	else
	{
		ERROR_LOG("you need init out put file info!\n");
		return -1;
	}

	if(RECODE_AAC_FRAME_TO_FILE) //DEBUG
		AAC_fd = open("/jffs0/aac.aac", O_CREAT | O_WRONLY | O_TRUNC, 0664);

	/***打开一个标准的AAC文件，获取帧来填充数据*/
	#if USE_44100_AAC_FILE
	AAC_44100_fd = open("/jffs0/test_AAC_44100.aac",O_RDONLY);
	if(AAC_44100_fd <0)
	{
		ERROR_LOG("open file failed !\n");
		return -1;
	}

	AAC_44100_buf = (unsigned char*)malloc(AAC_4410_BUF_SIZE);
	if(NULL != AAC_44100_buf)
		memset(AAC_44100_buf,0,AAC_4410_BUF_SIZE);
	#endif
	/******************************************/
	
		
	if(out_mode == SAVE_IN_FILE) //保存到文件
	{
		if(0 == access(out_info.file_mode.file_name,F_OK))
		{
			if(0 == remove(out_info.file_mode.file_name))
			{
				DEBUG_LOG("remove old file success!\n");
			}
			else
			{
				ERROR_LOG("remove old file failed!\n");
				return -1;
			}
		}

		file_handle = fopen(out_info.file_mode.file_name, "wb+");
		if(NULL == file_handle )
		{
			ERROR_LOG("open fmp4 file failed!\n");
			return -1;
		}
		DEBUG_LOG("open file success!\n");
	}
	else //保存到内存，初始化最终的fmp4文件存储内存
	{
		if(NULL == out_info.buf_mode.buf_start)
		{
			ERROR_LOG("buf_mode.buf_start is NULL!\n");
			return -1;
		}
		memset(out_info.buf_mode.buf_start , 0 , out_info.buf_mode.buf_size);
	}

	int ret = 0;
	int curr_offset = 0;

	
	//先要初始化 avcc box
	ret = sps_pps_parameter_set();
	if(ret < 0)
	{
		ERROR_LOG("sps_pps_parameter_set error!\n");
		if(out_mode == SAVE_IN_FILE )fclose(file_handle);
		return -1;
	}
	DEBUG_LOG("out sps_pps_parameter_set....\n");
	
#if 1
	//box_init
	DEBUG_LOG("into fmp4_box_init!\n");
	fmp4_file_box_t* box =  fmp4_box_init(audio_sampling_rate);
	if(NULL == box)
	{
		ERROR_LOG("fmp4_box_init failed !\n");
		if(out_mode == SAVE_IN_FILE )fclose(file_handle);
		return -1;
	}
	DEBUG_LOG("out fmp4_box_init!\n");



	//============更新所有容器box的长度(部分在初始化时已经更新)==================================================================================	
	//--- trak video 分支 -----------------------------------------------------------
	unsigned int count_stblV_size = 0;
	unsigned int count_dinfV_size = 0;
	unsigned int count_minfV_size = 0;
	unsigned int count_mdiaV_size = 0;
	unsigned int count_trakV_size = 0;
	#if HAVE_VIDEO
	count_stblV_size = t_ntohl(box->trak_video->stblBox->header.size)+\
					  				t_ntohl(box->trak_video->stsdBox->header.size)+\
					  				t_ntohl(box->trak_video->sttsBox->header.size)+\
					  				t_ntohl(box->trak_video->stscBox->header.size)+\
					  				t_ntohl(box->trak_video->stszBox->header.size)+\
					  				t_ntohl(box->trak_video->stcoBox->header.size);
	
	count_dinfV_size = t_ntohl(box->trak_video->dinfBox->header.size)+\
	  								t_ntohl(box->trak_video->drefBox->header.size);
	
	count_minfV_size = t_ntohl(box->trak_video->minfBox->header.size)+ \
									t_ntohl(box->trak_video->vmhdBox->header.size)+\
									count_dinfV_size + count_stblV_size;
									
	count_mdiaV_size = t_ntohl(box->trak_video->mdiaBox->header.size)+\
					  				t_ntohl(box->trak_video->mdhdBox->header.size)+\
					  				t_ntohl(box->trak_video->hdlrBox->header.size)+\
					  				count_minfV_size;

	count_trakV_size = t_ntohl(box->trak_video->trakBox->header.size)+\
	  								t_ntohl(box->trak_video->tkhdBox->header.size)+\
									count_mdiaV_size;
	#endif
	
	//--- trak audio 分支 -----------------------------------------------------------
	unsigned int count_stblA_size = 0;
	unsigned int count_dinfA_size = 0;
	unsigned int count_minfA_size = 0;
	unsigned int count_mdiaA_size = 0;
	unsigned int count_trakA_size = 0;
	
	#if HAVE_AUDIO
	count_stblA_size = t_ntohl(box->trak_audio->stblBox->header.size)+\
					  				t_ntohl(box->trak_audio->stsdBox->header.size)+\
					  				t_ntohl(box->trak_audio->sttsBox->header.size)+\
					  				t_ntohl(box->trak_audio->stscBox->header.size)+\
					  				t_ntohl(box->trak_audio->stszBox->header.size)+\
					  				t_ntohl(box->trak_audio->stcoBox->header.size);

									
	count_dinfA_size = t_ntohl(box->trak_audio->dinfBox->header.size)+\
	  								t_ntohl(box->trak_audio->drefBox->header.size);

	count_minfA_size = t_ntohl(box->trak_audio->minfBox->header.size)+\
	  								t_ntohl(box->trak_audio->smhdBox->header.size)+\
	  								count_dinfA_size + count_stblA_size;

	count_mdiaA_size = t_ntohl(box->trak_audio->mdiaBox->header.size)+\
					  				t_ntohl(box->trak_audio->mdhdBox->header.size)+\
					  				t_ntohl(box->trak_audio->hdlrBox->header.size)+\
					  				count_minfA_size;
	
	count_trakA_size = t_ntohl(box->trak_audio->trakBox->header.size)+\
	  								t_ntohl(box->trak_audio->tkhdBox->header.size)+\
	  								count_mdiaA_size;
	#endif
	
	//--- mvex 部分 -----------------------------------------------------------
	unsigned int count_trexV_size = 0;
	unsigned int count_trexA_size = 0;
	#if HAVE_VIDEO
	count_trexV_size = t_ntohl(box->trex_video->header.size);	
	#endif

	#if HAVE_AUDIO
	count_trexA_size = t_ntohl(box->trex_audio->header.size);
	#endif
	
	
	unsigned int count_mvex_size = t_ntohl(box->mvexBox->header.size)+\
									count_trexV_size +\
									count_trexA_size;
	lve1							
	unsigned int  count_moov_size = t_ntohl(box->moovBox->header.size) +\
									t_ntohl(box->mvhdBox->header.size)+\
									count_trakV_size +\
									count_trakA_size +\
									count_mvex_size;
								

	
	//============开始写文件==================================================================================
	//BUG：注意写入的大小不能直接使用 box->ftypBox->header.size 因在底层已经转化成大端模式了
	unsigned int self_size = 0; //记录box自身原本大小，不包括子box
	
	//write ftyp
	DEBUG_LOG("write ftyp size(%d)\n",t_ntohl(box->ftypBox->header.size));
	fwrite_box(box->ftypBox,1,t_ntohl(box->ftypBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.ftypBox_offset = curr_offset;


	//write moov
	DEBUG_LOG("write moov\n");
	self_size = t_ntohl(box->moovBox->header.size);//备份自身原本大小
	box->moovBox->header.size = t_htonl(count_moov_size);
	fwrite_box(box->moovBox,1,self_size,file_handle,ret);
	curr_offset +=ret;
	fmp4_file_lable.moovBox_offset = curr_offset;


	//write mvhd
	DEBUG_LOG("write mvhd\n");
	fwrite_box(box->mvhdBox,1,t_ntohl(box->mvhdBox->header.size),file_handle,ret);
	curr_offset +=ret;
	fmp4_file_lable.mvhdBox_offset = curr_offset;

	

#if HAVE_VIDEO
	//write trak_video
	//trak
	DEBUG_LOG("write trak_video\n");
	self_size = t_ntohl(box->trak_video->trakBox->header.size);//备份自身原本大小
	box->trak_video->trakBox->header.size = t_htonl(count_trakV_size);
	fwrite_box(box->trak_video->trakBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.trakBox_offset = curr_offset;

	
	//tkhd
	DEBUG_LOG("write tkhd\n");
	fwrite_box(box->trak_video->tkhdBox,1,t_ntohl(box->trak_video->tkhdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.tkhdBox_offset = curr_offset;

	
	//mdia
	DEBUG_LOG("write mdia\n");
	self_size = t_ntohl(box->trak_video->mdiaBox->header.size);//备份自身原本大小
	box->trak_video->mdiaBox->header.size = t_htonl(count_mdiaV_size);
	fwrite_box(box->trak_video->mdiaBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.mdiaBox_offset = curr_offset;



	//mdhd
	DEBUG_LOG("write mdhd\n");
	fwrite_box(box->trak_video->mdhdBox,1,t_ntohl(box->trak_video->mdhdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.mdhdBox_offset = curr_offset;



	//hdlr
	DEBUG_LOG("write hdlr\n");
	fwrite_box(box->trak_video->hdlrBox,1,t_ntohl(box->trak_video->hdlrBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.hdlrBox_offset = curr_offset;


	//minf
	DEBUG_LOG("write minf\n");
	self_size = t_ntohl(box->trak_video->minfBox->header.size);//备份自身原本大小
	box->trak_video->minfBox->header.size = t_htonl(count_minfV_size);
	fwrite_box(box->trak_video->minfBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.minfBox_offset = curr_offset;



	//vmhd
	DEBUG_LOG("write vmhd\n");
	fwrite_box(box->trak_video->vmhdBox,1,t_ntohl(box->trak_video->vmhdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.vmhdBox_offset = curr_offset;


	//dinf
	DEBUG_LOG("write dinf\n");
	self_size = t_ntohl(box->trak_video->dinfBox->header.size);//备份自身原本大小
	box->trak_video->dinfBox->header.size = t_htonl(count_dinfV_size);
	fwrite_box(box->trak_video->dinfBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.dinfBox_offset = curr_offset;


	//dref  初始化数组里边直接包含了URL
	DEBUG_LOG("write dref\n");
	fwrite_box(box->trak_video->drefBox,1,t_ntohl(box->trak_video->drefBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.drefBox_offset = curr_offset;


	//url  不需要,底层也没有初始化
	#if 0
	DEBUG_LOG("write url\n");
	fwrite_box(box->trak_video->urlBox,1,t_ntohl(box->trak_video->urlBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.urlBox_offset = curr_offset;
	#endif

	//stbl
	DEBUG_LOG("write stbl\n");
	self_size = t_ntohl(box->trak_video->stblBox->header.size);//备份自身原本大小
	box->trak_video->stblBox->header.size = t_htonl(count_stblV_size);
	fwrite_box(box->trak_video->stblBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stblBox_offset = curr_offset;


	//stsd
	DEBUG_LOG("write stsd size(%d)\n",t_ntohl(box->trak_video->stsdBox->header.size));
	fwrite_box(box->trak_video->stsdBox,1,t_ntohl(box->trak_video->stsdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stsdBox_offset = curr_offset;

	
	//stts
	DEBUG_LOG("write stts\n");
	fwrite_box(box->trak_video->sttsBox,1,t_ntohl(box->trak_video->sttsBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.sttsBox_offset = curr_offset;



	//stsc
	DEBUG_LOG("write stsc\n");
	fwrite_box(box->trak_video->stscBox,1,t_ntohl(box->trak_video->stscBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stscBox_offset = curr_offset;


	//stsz
	DEBUG_LOG("write stsz size(%d)\n",t_ntohl(box->trak_video->stszBox->header.size));
	fwrite_box(box->trak_video->stszBox,1,t_ntohl(box->trak_video->stszBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stszBox_offset = curr_offset;


	//stco
	DEBUG_LOG("write stco size(%d)\n",t_ntohl(box->trak_video->stcoBox->header.size));
	fwrite_box(box->trak_video->stcoBox,1,t_ntohl(box->trak_video->stcoBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_video_offset.stszBox_offset = curr_offset;


#endif

#if HAVE_AUDIO
	//write_trak_audio
	//trak
	DEBUG_LOG("write_trak_audio\n");
	self_size = t_ntohl(box->trak_audio->trakBox->header.size);//备份自身原本大小
	box->trak_audio->trakBox->header.size = t_htonl(count_trakA_size);
	fwrite_box(box->trak_audio->trakBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.trakBox_offset = curr_offset;


	//tkhd
	DEBUG_LOG("write tkhd\n");
	fwrite_box(box->trak_audio->tkhdBox,1,t_ntohl(box->trak_audio->tkhdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.tkhdBox_offset = curr_offset;



	//mdia
	DEBUG_LOG("write mdia\n");
	self_size = t_ntohl(box->trak_audio->mdiaBox->header.size);//备份自身原本大小
	box->trak_audio->mdiaBox->header.size = t_htonl(count_mdiaA_size);
	fwrite_box(box->trak_audio->mdiaBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.mdiaBox_offset = curr_offset;



	//mdhd
	DEBUG_LOG("write mdhd\n");
	fwrite_box(box->trak_audio->mdhdBox,1,t_ntohl(box->trak_audio->mdhdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.mdhdBox_offset = curr_offset;



	//hdlr
	DEBUG_LOG("write hdlr\n");
	fwrite_box(box->trak_audio->hdlrBox,1,t_ntohl(box->trak_audio->hdlrBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.hdlrBox_offset = curr_offset;



	//minf
	DEBUG_LOG("write minf\n");
	self_size = t_ntohl(box->trak_audio->minfBox->header.size);//备份自身原本大小
	box->trak_audio->minfBox->header.size = t_htonl(count_minfA_size);
	fwrite_box(box->trak_audio->minfBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.minfBox_offset = curr_offset;



	//smhd
	DEBUG_LOG("write smhd\n");
	fwrite_box(box->trak_audio->smhdBox,1,t_ntohl(box->trak_audio->smhdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.smhdBox_offset = curr_offset;



	//dinf
	DEBUG_LOG("write dinf\n");
	self_size = t_ntohl(box->trak_audio->dinfBox->header.size);//备份自身原本大小
	box->trak_audio->dinfBox->header.size = t_htonl(count_dinfA_size);
	fwrite_box(box->trak_audio->dinfBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.dinfBox_offset = curr_offset;



	//dref
	DEBUG_LOG("write dref\n");
	fwrite_box(box->trak_audio->drefBox,1,t_ntohl(box->trak_audio->drefBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.drefBox_offset = curr_offset;



	/* 不需要这玩意
	//url
	DEBUG_LOG("write url\n");
	fwrite_box(box->trak_audio->url_box,1,t_ntohl(box->trak_audio->url_box->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.url_box_offset = curr_offset;

	*/
	
	//stbl
	DEBUG_LOG("write stbl\n");
	self_size = t_ntohl(box->trak_audio->stblBox->header.size);//备份自身原本大小
	box->trak_audio->stblBox->header.size = t_htonl(count_stblA_size);
	fwrite_box(box->trak_audio->stblBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stblBox_offset = curr_offset;



	//stsd
	DEBUG_LOG("write stsd\n");
	fwrite_box(box->trak_audio->stsdBox,1,t_ntohl(box->trak_audio->stsdBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stsdBox_offset = curr_offset;



	//stts
	DEBUG_LOG("write stts\n");
	fwrite_box(box->trak_audio->sttsBox,1,t_ntohl(box->trak_audio->sttsBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.sttsBox_offset = curr_offset;


	//stsc
	DEBUG_LOG("write stsc\n");
	fwrite_box(box->trak_audio->stscBox,1,t_ntohl(box->trak_audio->stscBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stscBox_offset = curr_offset;


	//stsz
	DEBUG_LOG("write stsz\n");
	fwrite_box(box->trak_audio->stszBox,1,t_ntohl(box->trak_audio->stszBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stszBox_offset = curr_offset;


	//stco
	DEBUG_LOG("write stco\n");
	fwrite_box(box->trak_audio->stcoBox,1,t_ntohl(box->trak_audio->stcoBox->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trak_audio_offset.stcoBox_offset = curr_offset;

	
#endif

	//mvex
	DEBUG_LOG("write mvex size(%d)\n",t_ntohl(box->mvexBox->header.size));
	self_size = t_ntohl(box->mvexBox->header.size);//备份自身原本大小
	box->mvexBox->header.size = t_htonl(count_mvex_size);
	fwrite_box(box->mvexBox,1,self_size,file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.mvexBox_offset = curr_offset;

#if HAVE_VIDEO
	//trex_video
	DEBUG_LOG("write trex_video t_ntohl(box->trex_video->header.size) = %d\n",t_ntohl(box->trex_video->header.size));
	fwrite_box(box->trex_video,1,t_ntohl(box->trex_video->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trex_video_offset = curr_offset;
#endif

#if HAVE_AUDIO
	//trex_audio
	DEBUG_LOG("write trex_audio \n");
	fwrite_box(box->trex_audio,1,t_ntohl(box->trex_audio->header.size),file_handle,ret);
	curr_offset += ret;
	fmp4_file_lable.trex_audio_offset = curr_offset;
#endif

	/*随后就是 n*(moof + mdat)结构，将由“音视频混合相关业务”部分来写
	  包括更新已经写入文件的 box (trak、trex)
	*/
	
#endif
	
	DEBUG_LOG("remux_init...\n");
	ret = remux_init(Vframe_rate,Aframe_rate);// video/audio混合器 初始化
	if(ret < 0)
	{
		ERROR_LOG("remux init faied!\n");
		if(out_mode == SAVE_IN_FILE )fclose(file_handle);
		return -1;
	}

	DEBUG_LOG("fmp4 file init success!\n");
	if(out_mode == SAVE_IN_FILE )fflush(file_handle);
	
	return 0;
	
	
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
	frame_rate:是外部传入视频数据的原有帧率
	返回值：失败：-1  		 成功：0
*/
static unsigned char remux_run = 0;	//remux 运行标志（1：继续运行 0：退出）
pthread_mutex_t remux_write_last_mut;//	remux完成最后一轮数据的写入操作（即将进行释放操作）
pthread_cond_t remux_write_last_ok;

volatile static unsigned char remux_v_can_write = 0; // buf_remux_video 中video的数据是否已经缓存够1s,可以封box写入文件
volatile static unsigned char remux_A_can_write = 0; // buf_remux_audio 中audio的数据是否已经缓存够1s,可以封box写入文件
unsigned long long int V_pre_time_scale_ms = 0; //上一帧的时间戳，用于计算视频帧的时间 duration 
int	remuxVideo(void *video_frame,unsigned int frame_length,unsigned int frame_rate,unsigned long long time_scale)
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

	//初始化该帧的描述信息
	unsigned char  isLeading = 0;		//: 0,
	unsigned char  dependsOn = 0;		//: keyframe ? 2 : 1,
	unsigned char  isDependedOn = 0;	//: keyframe ? 1 : 0,
	unsigned char  hasRedundancy = 0;	//: 0,
	unsigned char  isNonSync = 0;		//: keyframe ? 0 : 1
	//if(NALU_I == FrameType((unsigned char*)video_frame))
	int nalutype = FrameType((unsigned char*)video_frame);
	
	if(NALU_SPS == nalutype) //IDR帧
	{
		DEBUG_LOG("NALU_IDR\n");
		dependsOn = 2;
		isDependedOn = 1;
		isNonSync = 0;

		#define ENABLE_CUT_SPS_PPS 1  //使能：裁剪掉SPS   	PPS SEI， 并用4字节大小填充帧头
		#if ENABLE_CUT_SPS_PPS
		//只接受 I/P帧，对IDR帧之前的PPS SPS SEI做剔除
		video_frame = (char*)video_frame + 37;//37字节为海思编码IDR帧的SPS + PPS + SEI总长度
		frame_length = frame_length - 37;
		#endif
	}
	else if(NALU_I == nalutype)
	{
		//DEBUG_LOG("NALU_I\n");
		dependsOn = 2;
		isDependedOn = 1;
		isNonSync = 0;
	}
	else if(NALU_P == nalutype)
	{
		//DEBUG_LOG("NALU_P\n");
		dependsOn = 1;
		isDependedOn = 0;
		isNonSync = 1;
	}
	else
	{
		ERROR_LOG("frame type error!\n");
		return -1;
	}
	
	 //将该帧放到暂存区，最大只存1S的数据
	// DEBUG_LOG("buf_remux_video.frame_rate(%d)\n",buf_remux_video.frame_rate);
	if(buf_remux_video.frame_count < buf_remux_video.frame_rate)//缓存区的帧数不够1S，继续将帧数据放入缓存区
	{
		if(buf_remux_video.write_pos + frame_length > buf_remux_video.remux_video_buf + REMUX_VIDEO_BUF_SIZE)
		{
			//缓存大小不够，退出。(指初始化的缓存区不够缓存一秒的编码数据的，因打包mdat box按照1S一个box打包)
			ERROR_LOG("remux_video_buf is not enough to story one mdat box data !\n");
			free(buf_remux_video.remux_video_buf);
			return -1;
		}
		pthread_mutex_lock(&buf_remux_video.mut);
       
			memcpy(buf_remux_video.write_pos , video_frame , frame_length);
			
			//四字节的头用长度替换，并且是大端模式
			/*
			char  IFrameLen[4] = {0}; 
            IFrameLen[0] = (frame_length-4) >>24;  
            IFrameLen[1] = (frame_length-4) >>16;  
            IFrameLen[2] = (frame_length-4) >>8;  
            IFrameLen[3] = (frame_length-4) &0xff;
            */
           #if ENABLE_CUT_SPS_PPS
           // DEBUG_LOG("current actual frame len (%d)\n",frame_length-4);
            unsigned int data_len = t_htonl(frame_length-4);
            memcpy(buf_remux_video.write_pos,&data_len,4);
           #endif
			
			buf_remux_video.frame_count ++;
			buf_remux_video.sample_info[buf_remux_video.write_index].sample_pos = buf_remux_video.write_pos;
			buf_remux_video.sample_info[buf_remux_video.write_index].sample_len = frame_length;
			buf_remux_video.write_pos += frame_length;
			
			//保存sample的信息，   用来更新moof 里边 video traf 下相关 box 的信息(主要是 trun box)
			//(当前帧时间 - 上一帧时间)后转换成编码系统的内部时间 = sample_duration;
			if(time_scale - V_pre_time_scale_ms < 0)
			{
				ERROR_LOG("time_scale error!\n");
				return -1;
			}
				
			unsigned int tmp_sample_duration =  (unsigned int)(time_scale - V_pre_time_scale_ms)* VIDEO_ONE_MSC_LEN;
			if(0 == V_pre_time_scale_ms )//首次进入，传入的是第一帧数据
			{
				tmp_sample_duration = VIDEO_TIME_SCALE/frame_rate; //修正 tmp_sample_duration 为一个默认值。
			}
					
			//DEBUG_LOG("tmp_sample_duration (%d)\n",tmp_sample_duration);
			V_pre_time_scale_ms = time_scale;//记录当前帧时间戳，下一帧来时使用。
			buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_duration = t_htonl(tmp_sample_duration);//t_htonl(VIDEO_TIME_SCALE/frame_rate);
			buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_size = t_htonl(frame_length);
			


			#if 1 //sample_flags 部分
			unsigned char sample_flags[4] = {(isLeading << 2)|(dependsOn),
											 (isDependedOn << 6) | (hasRedundancy << 4) | isNonSync,
											 0x00,0x00
											};
			memcpy((unsigned char*)&buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_flags,sample_flags,sizeof(sample_flags));

			DEBUG_LOG("buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_flags(%d)\n",\
					   buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_flags);

			//buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_flags = t_htonl(16842752);
			#endif
			
			// t_htonl(cts)  需要获取帧的cts,暂时不填
			//buf_remux_video.sample_info[buf_remux_video.write_index].trun_sample.sample_composition_time_offset = 0;
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
		buf_remux_video.need_remux = 1;
		while(remux_v_can_write)//等待写入操作成功
		{
			DEBUG_LOG("...\n");
			usleep(20);  
		}

		//指针归位，buf又循环重写 放到 remuxVideoAudio（）函数做了
		/*
		pthread_mutex_lock(&buf_remux_video.mut);		
		//pthread_cond_wait(&buf_remux_video.remux_ready, &buf_remux_video.mut);
		buf_remux_video.write_pos = buf_remux_video.remux_video_buf;
		buf_remux_video.read_pos = buf_remux_video.remux_video_buf;
		buf_remux_video.frame_count = 0;
		buf_remux_video.write_index = 0;
		buf_remux_video.need_remux = 0;
		pthread_mutex_unlock(&buf_remux_video.mut); 
		*/
	}

	return 0;
	
}

/*
	frame_rate:是外部传入音频数据的原有帧率
	返回值：失败：-1  		 成功：0
*/
typedef struct _adts_fixed_header_t  
{   
 unsigned int syncword:12;  				//0-11 	bit 帧同步标识一个帧的开始，固定为0xFFF
 unsigned int ID:1;  						//12 	bit MPEG 标示符。0表示MPEG-4，1表示MPEG-2
 unsigned int layer:2;  					//13-14 bit 固定为'00'
 unsigned int protection_absent:1;  		//15 	bit 标识是否进行误码校验。0表示有CRC校验，1表示没有CRC校验 
 unsigned int profile:2;  					//16-17 bit 标识使用哪个级别的AAC。1: AAC Main 2:AAC LC (Low Complexity) 3:AAC SSR (Scalable Sample Rate) 4:AAC LTP (Long Term Prediction)
 unsigned int sampling_frequency_index:4;  	//18-21 bit 标识使用的采样率的下标
 unsigned int private_bit:1;  				//22 	bit 私有位，编码时设置为0，解码时忽略
 unsigned int channel_configuration:3;  	//23-25 bit 标识声道数
 unsigned int original_copy:1;  			//26 	bit 编码时设置为0，解码时忽略
 unsigned int home:1;  						//27  	bit	编码时设置为0，解码时忽略
 //共28bit   注意字节对齐
}adts_fixed_header_t;

typedef struct _adts_variable_header_t   
{   
 unsigned int copyright_identification_bit:1;  		//0  	bit 编码时设置为0，解码时忽略
 unsigned int copyright_identification_start:1;  	//1  	bit 编码时设置为0，解码时忽略
 unsigned int frame_length:13;   					//2-14  bit ADTS帧长度包括ADTS长度和AAC声音数据长度的和。即 aac_frame_length = (protection_absent == 0 ? 9 : 7) + audio_data_length
 unsigned int adts_buffer_fullness:11;  			//15-25 bit 固定为0x7FF。表示是码率可变的码流
 unsigned int number_of_raw_data_blocks_in_frame:2; //26-27 bit 表示当前帧有number_of_raw_data_blocks_in_frame + 1 个原始帧(一个AAC原始帧包含一段时间内1024个采样及相关数据)。
 //共28bit  注意字节对齐
}adts_variable_header_t;
/* 实例：
FF	1111 1111 	0-7
F9 	1111 1001	8-15		MPEG-2 无 CRC
60 	0110 0000	16-23		AAC Main  88200HZ（使用的采样率）
40 	0100 		24-27

	0000		0-3					00 0001 0111 111
17 	0001 0111	4-11
FF 	1111 1111	12-19
FC 	1111 1100	20-27
DE 	1101 1110	56-63
0C 	0000 1100	64-71
*/
static unsigned long long int A_pre_time_scale_ms = 0; //上一帧的时间戳，用于计算视频帧的时间 duration    
int	remuxAudio(void *audio_frame,unsigned int frame_length,unsigned int frame_rate,unsigned long long time_scale)
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
	
	if(RECODE_AAC_FRAME_TO_FILE) //DEBUG
		write(AAC_fd, audio_frame, frame_length);

	/***读一个标准的AAC文件，获取帧来填充数据*******DEBUG*******************************/
	#if USE_44100_AAC_FILE    
	unsigned char ADTS[7]= {0};
	int aac_ret = read(AAC_44100_fd,AAC_44100_buf,7);//先读ADTS头部(7字节)，获取长度
	if(aac_ret <= 0)
	{
		ERROR_LOG("read AAC file error ret(%d) !\n",aac_ret);
		return -1;
	}
	if(AAC_44100_buf[0] != 0xFF)
	{
		ERROR_LOG("AAC_44100_buf[0] != 0xFF\n");
		return -1;
	}
	/* Get header bit 30 - 42 ,获取AAC帧的长度*/
    unsigned int AAC_frame_len = ((AAC_44100_buf[3]&0x03)<<11|(AAC_44100_buf[4]&0xFF)<<3|(AAC_44100_buf[5]&0xE0)>>5);
	DEBUG_LOG("AAC_frame_len(%d)\n",AAC_frame_len);
	if(AAC_frame_len > AAC_4410_BUF_SIZE)
	{
		ERROR_LOG("AAC_4410_BUF_SIZE is overflow!!\n");
		return -1;
	}
	
	aac_ret = read(AAC_44100_fd,AAC_44100_buf+7,AAC_frame_len-7);//读该帧剩余的部分到缓存
	if(aac_ret <= 0)
	{
		ERROR_LOG("read AAC file error ret(%d) !\n",aac_ret);
		return -1;
	}
	
	//音频帧指针重定向
	audio_frame = AAC_44100_buf;
	frame_length = AAC_frame_len;
	#endif
	/*************************************************************************************/
	

	/*
	关于AAC音频帧应该注意：
		Mp4 should not have ADTS headers in the data. Just raw aac frames plus a configuration record in esds.
	去掉头7个字节的ADTS头：
	*/
	adts_fixed_header_t *adts_header = (adts_fixed_header_t*)audio_frame;
	print_char_array("ADTS:", audio_frame, 10);
	if((unsigned char)adts_header->syncword == 0xFF)
	{
		#if 1
		if(1)//(adts_header->protection_absent == 0)//有CRC校验
		{
			DEBUG_LOG("AAC ADTS header have CRC  checker \n");
			audio_frame = (char*)audio_frame + 9;
			frame_length = frame_length -9;
		}
		else //无CRC校验
		{
			DEBUG_LOG("AAC ADTS header not have CRC  checker \n");
			audio_frame = (char*)audio_frame + 7;
			frame_length = frame_length -7;
		}
		#endif
	}

	
	 //将该帧放到暂存区
	 /*该条件的配置最好缓存达标的时长要大于视频，因是以视频帧为主导*/
	if(buf_remux_audio.frame_count < 2* buf_remux_audio.frame_rate)//缓存区的帧数不够2S，继续将帧数据放入缓存区（防止干扰视频）
	{
		if(buf_remux_audio.write_pos + frame_length > buf_remux_audio.remux_audio_buf + REMUX_VIDEO_BUF_SIZE)
		{
			//缓存大小不够，退出。
			ERROR_LOG("remux_audio_buf is not enough to story one mdat box data !\n");
			free(buf_remux_audio.remux_audio_buf);
			return -1;
		}
		print_char_array("input Audio mdat samples:",audio_frame,16); //debug
			
		pthread_mutex_lock(&buf_remux_audio.mut);
		memcpy(buf_remux_audio.write_pos , audio_frame , frame_length);
		buf_remux_audio.write_pos += frame_length;
		buf_remux_audio.frame_count ++;

		buf_remux_audio.sample_info[buf_remux_audio.write_index].sample_pos = buf_remux_audio.write_pos;
		buf_remux_audio.sample_info[buf_remux_audio.write_index].sample_len = frame_length;
		
		/*----保存sample的信息，   用来更新moof 里边 audio traf 下相关 box 的信息(主要是 trun box)--------------*/
		//(当前帧时间 - 上一帧时间)后转换成编码系统的内部时间 = sample_duration;
		if(time_scale - A_pre_time_scale_ms < 0)
		{
			ERROR_LOG("time_scale error!\n");
			return -1;
		}
			
		unsigned int tmp_sample_duration =  (unsigned int)(time_scale - A_pre_time_scale_ms)* AUDIO_ONE_MSC_LEN;
		if(0 == A_pre_time_scale_ms )//首次进入，传入的是第一帧数据
		{
			tmp_sample_duration = AUDIO_TIME_SCALE/frame_rate; //修正 tmp_sample_duration 为一个默认值。
		}
	//	DEBUG_LOG("A_tmp_sample_duration (%d)\n",tmp_sample_duration);
		A_pre_time_scale_ms = time_scale;//记录当前帧时间戳，下一帧来时使用。
		buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_duration = t_htonl(tmp_sample_duration);//t_htonl(AUDIO_FREAME_SAMPLES);//t_htonl(tmp_sample_duration);//t_htonl(ONE_AAC_FRAME_DURATION);//t_htonl(1024)
		buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_size = t_htonl(frame_length);
		#if 0  // sample_flags 部分
		buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_flags = t_htonl(33554432 ); //audio 不使用该参数
		#endif
		//buf_remux_audio.sample_info[buf_remux_audio.write_index].trun_sample.sample_composition_time_offset = ; //audio 不使用该参数

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
		buf_remux_audio.need_remux = 1;
		while(remux_A_can_write)//等待写入操作成功
		{
			//DEBUG_LOG("waiting remuxVideoAudio thread write AUDIO to file...\n");
			usleep(20);
		}

		//指针归位，buf又循环重写 ，放到 remuxVideoAudio（）函数做了
		/*
		pthread_mutex_lock(&buf_remux_audio.mut);
		//pthread_cond_wait(&buf_remux_audio.remux_ready, &buf_remux_audio.mut);
		buf_remux_audio.write_pos = buf_remux_audio.remux_audio_buf;
		buf_remux_audio.read_pos = buf_remux_audio.remux_audio_buf;
		buf_remux_audio.frame_count = 0;
		buf_remux_audio.write_index = 0;
		buf_remux_audio.need_remux = 0;
		pthread_mutex_unlock(&buf_remux_audio.mut);
		*/
	}

	return 0;
	
}

/*
offset :在 dst 目标缓存往后偏移的字节数。
buf_size:是整个dst buffer的总大小。

*/
unsigned int buf_cpy(void *dst,void *src ,unsigned int cpy_len, unsigned int offset,unsigned int buf_size)
{
	if(NULL == dst || NULL == src || cpy_len <= 0 || cpy_len <= 0)
	{
		ERROR_LOG("Illegal parameter!\n");
		return -1;
	}

	if(offset < 0 ||offset + cpy_len > buf_size)
	{
		ERROR_LOG("offset out of range ! \n");
		if(offset + cpy_len > buf_size)
		{
			ERROR_LOG("offset + cpy_len > buf_size!\n");
		}
		return -1;
	}
	
	memcpy((unsigned char*)dst + offset ,(unsigned char*)src,cpy_len);
	
	return 0;
}

/**************************************************************************
录制结束前重建mfra box及其子box
参数：
	mfraBox：重建的mfra box的指针（包含子box）
	len：重建的mfra box的长度（包含子box）
返回值：
	成功：0
	失败：-1
注意：该函数是在最后录制结束的时候调用，调用后就意味着之前所有的
	moof + mdat的数据以及数量都不能再变化，不然填入的该box将是错误的
**************************************************************************/
unsigned int mfra_box_rebuid(OUT mfra_box**mfraBox,OUT unsigned int *len)
{
	//重新计算mfra box的总长度
	unsigned int mfra_len = t_ntohl(fmp4BOX.mfraBox->header.size);
	unsigned int tfra_video_len = 0;
	unsigned int tfra_audio_len = 0;
	
	DEBUG_LOG("mfra_len(%d)\n",mfra_len);
	#if HAVE_VIDEO
		tfra_video_len = t_ntohl(fmp4BOX.tfra_video->tfraBox->header.size) + buf_remux_video.entry_info_num*(sizeof(tfra_entry_info_t)-1);
		mfra_len += tfra_video_len;
		DEBUG_LOG("mfra_len(%d)\n",mfra_len);
	#endif

	#if HAVE_AUDIO
		tfra_audio_len = t_ntohl(fmp4BOX.tfra_audio->tfraBox->header.size) +  buf_remux_audio.entry_info_num*(sizeof(tfra_entry_info_t)-1);
		mfra_len += tfra_audio_len;
	#endif

	mfra_len += t_ntohl(fmp4BOX.mfroBox->header.size);
	DEBUG_LOG("mfra_len(%d)\n",mfra_len);
	//重新申请内存
	DEBUG_LOG("mfra_item rebuid malloc size(%d)\n",mfra_len);
	mfra_box * mfra_item = (mfra_box *)malloc(mfra_len);
	if(NULL == mfra_item)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
		
	memset(mfra_item,0,mfra_len);

	unsigned char*offset = (unsigned char*)mfra_item;

	//填充数据：mfra
	memcpy(offset,fmp4BOX.mfraBox,t_ntohl(fmp4BOX.mfraBox->header.size));
	offset += t_ntohl(fmp4BOX.mfraBox->header.size);
	int i = 0;
	
	#if HAVE_VIDEO   //tfra video
		fmp4BOX.tfra_video->tfraBox->track_ID = t_htonl(VIDEO_TRACK);
		fmp4BOX.tfra_video->tfraBox->header.size = t_htonl(tfra_video_len);
		DEBUG_LOG("buf_remux_video.entry_info_num(%d)\n",buf_remux_video.entry_info_num);
		fmp4BOX.tfra_video->tfraBox->number_of_entry = t_htonl(buf_remux_video.entry_info_num);
		memcpy(offset,fmp4BOX.tfra_video->tfraBox ,sizeof(tfra_box));
		//
		offset = offset + sizeof(tfra_box);
		for(i = 0; i < buf_remux_video.entry_info_num;i++)
		{
			memcpy(offset,&buf_remux_video.entry_info[i],sizeof(tfra_entry_info_t)-1);//sizeof()会进行字节对齐，最后一个字节是不需要的
			offset = offset + sizeof(tfra_entry_info_t)-1;
		}
		//offset += tfra_video_len - sizeof(tfra_box);//sizeof(tfra_box)在前边已经加了
	#endif

	#if HAVE_AUDIO  //tfra audio
		fmp4BOX.tfra_audio->tfraBox->track_ID = t_htonl(AUDIO_TRACK);
		fmp4BOX.tfra_audio->tfraBox->header.size = t_htonl(tfra_audio_len);
		fmp4BOX.tfra_audio->tfraBox->number_of_entry = t_htonl(buf_remux_audio.entry_info_num);
		memcpy(offset,fmp4BOX.tfra_audio->tfraBox,sizeof(tfra_box));
		offset = offset + sizeof(tfra_box);
		for(i = 0; i < buf_remux_audio.entry_info_num;i++)
		{
			memcpy(offset,&buf_remux_audio.entry_info[i],sizeof(tfra_entry_info_t)-1);//sizeof()会进行字节对齐，最后一个字节是不需要的
			offset = offset + sizeof(tfra_entry_info_t)-1;
		}
	#endif

	//mfro 
	fmp4BOX.mfroBox->size = t_htonl((unsigned char*)offset + (t_ntohl(fmp4BOX.mfroBox->header.size)) - (unsigned char*)mfra_item); //要将mfro所有长度都算进去
	memcpy(offset,fmp4BOX.mfroBox,t_ntohl(fmp4BOX.mfroBox->header.size));
	offset += t_ntohl(fmp4BOX.mfroBox->header.size);
	


	//释放掉原来旧的mfra box
	free(fmp4BOX.mfraBox);
	
	//填入重建后的mfra box
	fmp4BOX.mfraBox = mfra_item;

	//最后修正总长度
	fmp4BOX.mfraBox->header.size = t_htonl((unsigned char*)offset - (unsigned char*)mfra_item);//上层调用增加child box后再及时修正


	//填充返回参数
	*mfraBox = fmp4BOX.mfraBox;
	*len = (unsigned char*)offset - (unsigned char*)mfra_item;
	
	return 0;
}



/*	
	音视频混合线程主要功能函数
	将获得的1S时间的音视频结合，生成 mdat box，写入fmp4文件
*/
#define moof_mdat_buf_size (1024*300)
static unsigned int Sequence_number = 0; //mfhd-->Sequence number的记录
static unsigned int tfra_audio_time = 0; //记录audio tfra box下的time信息
static unsigned int tfra_video_time = 1024; //记录video tfra box下的time信息
void * remuxVideoAudio(void *args)
{
	pthread_detach(pthread_self());	
	
	if(NULL == buf_remux_video.remux_video_buf||\
	   NULL == buf_remux_audio.remux_audio_buf)
	{
		ERROR_LOG("remux not init!\n");
		pthread_exit(0);
	}
	//当前 moof + mdat box 组合的缓冲buf ，直接写文件太慢
	unsigned char* moof_mdat_buf = (unsigned char*)malloc(moof_mdat_buf_size);
	   if(NULL == moof_mdat_buf)
	   {
	   		ERROR_LOG("malloc failed !\n");
			return NULL;
	   }
	   memset(moof_mdat_buf,0,moof_mdat_buf_size);
	  
	unsigned int buf_offset = 0;//描述moof_mdat_buf中的偏移
	int i = 0;
	unsigned int have_video = 0;
	unsigned int have_audio = 0;

	#if HAVE_VIDEO
		have_video = 1;
	#endif

	#if HAVE_AUDIO
		have_audio = 1;
	#endif

	unsigned int trun_box_len = 0;
	unsigned int traf_box_len = 0;
	unsigned int moof_box_len = 0;
	unsigned int mdat_box_len = 0;
	unsigned int data_offset = 0;  //描述 mdat video/audio数据起始位置相较于moof起始位置的数据偏移
	
	   
	do
	{
		/*
		!remux_run: 外部要求fmp4编码结束的情况，此时需要将remux buf 中剩余的数据写入到文件当中去才能够释放;
		remux_v_can_write||remux_A_can_write: remuxVideo 或者 remuxAideo 缓存满1S数据，就进行封装box写入文件的操作,
		*/
		if((!remux_run)||remux_v_can_write || remux_A_can_write)
		{

			trun_box_len = 0;
			traf_box_len = 0;
			moof_box_len = 0;
			mdat_box_len = 0;
			buf_offset = 0;
			
			//如果缓存中一帧数据都没有，当做没有处理,有BUG，音视频一个有数据一个没数据时会造成卡顿
			if(0 == buf_remux_video.frame_count)
			{
				DEBUG_LOG("have no video samples !\n");
				have_video = 0;
			}
			if(0 == buf_remux_audio.frame_count)
			{
				DEBUG_LOG("have no audio samples !\n");
				have_audio = 0;

			}
				
			//===公共部分长度处理================================
			//先加上mfhd box的长度
			moof_box_len = sizeof(moof_box);
			moof_box_len += sizeof(mfhd_box);
			fmp4BOX.moofBox->header.size = t_htonl(moof_box_len);

			//先加上mdat自身的长度
			mdat_box_len = t_ntohl(fmp4BOX.mdatBox->header.size );
			//先记录 moof在文件中的起始位置，tfhd box中需要该参数
			fmp4_file_lable.moofBox_offset = (out_mode == SAVE_IN_FILE) ? ftell(file_handle) : out_info.buf_mode.w_offset;


			
			if(have_video)
			{
				//---修正 moof   box下video相关 box的长度信息---------------------------------------------------------------------
				pthread_mutex_lock(&buf_remux_video.mut);
				
				//1.更新 trun box 的长度,需要加上samples的描述信息长度
				trun_box_len = t_ntohl(fmp4BOX.traf_video->trunBox->header.size) + \
								buf_remux_video.write_index * sizeof(trun_V_sample_t);

				/*
				for(i = 0 ; i < buf_remux_video.write_index; i++)
				{					
					//trun_box_len += buf_remux_video.sample_info[i].sample_len;
					trun_box_len += sizeof(trun_V_sample_t); //加的应该是单个描述信息的长度。
				}
				*/
				
				//再转换成网络字节序
				DEBUG_LOG("trun_box_len (%d)\n",trun_box_len);
				fmp4BOX.traf_video->trunBox->header.size = t_htonl(trun_box_len);
			
				//2.更新 traf box长度
				DEBUG_LOG("t_ntohl(fmp4BOX.traf_video->trafBox->header.size) = %d \n\
						   t_ntohl(fmp4BOX.traf_video->tfhdBox->header.size) = %d \n\
						   t_ntohl(fmp4BOX.traf_video->tfdtBox->header.size) = %d \n\
						   t_ntohl(fmp4BOX.traf_video->trunBox->header.size) = %d \n",\
						   t_ntohl(fmp4BOX.traf_video->trafBox->header.size),\
						   t_ntohl(fmp4BOX.traf_video->tfhdBox->header.size),\
						   t_ntohl(fmp4BOX.traf_video->tfdtBox->header.size),\
						   t_ntohl(fmp4BOX.traf_video->trunBox->header.size));
				
				traf_box_len += t_ntohl(fmp4BOX.traf_video->trafBox->header.size)\
							  +	t_ntohl(fmp4BOX.traf_video->tfhdBox->header.size)\
							  + t_ntohl(fmp4BOX.traf_video->tfdtBox->header.size)\
							  + t_ntohl(fmp4BOX.traf_video->trunBox->header.size);
				fmp4BOX.traf_video->trafBox->header.size = t_htonl(traf_box_len);
				
				//3.更新 moof box长度
				moof_box_len += t_ntohl(fmp4BOX.traf_video->trafBox->header.size);
				fmp4BOX.moofBox->header.size = t_htonl(moof_box_len);
			
				//4.更新 mdat box长度
				mdat_box_len += buf_remux_video.write_pos - buf_remux_video.remux_video_buf;
				fmp4BOX.mdatBox->header.size  = t_htonl(mdat_box_len);
				DEBUG_LOG("debug mdat_box_len (%d)\n",mdat_box_len);

				//记录 moof的偏移等信息
				buf_remux_video.entry_info[buf_remux_video.entry_info_num].moof_offset = t_htonl(fmp4_file_lable.moofBox_offset);
				buf_remux_video.entry_info[buf_remux_video.entry_info_num].time = t_htonl(tfra_video_time);//t_htonl((buf_remux_video.entry_info_num) * VIDEO_TIME_SCALE);
				buf_remux_video.entry_info[buf_remux_video.entry_info_num].traf_number = 1;	
				buf_remux_video.entry_info[buf_remux_video.entry_info_num].trun_number = 1;
				buf_remux_video.entry_info[buf_remux_video.entry_info_num].sample_number = 1;
				buf_remux_video.entry_info_num ++;
				pthread_mutex_unlock(&buf_remux_video.mut);
			}

			//---修正 moof   box下 audio 相关 box的长度信息--------------------------------------------------------------
			if(have_audio)
			{

				pthread_mutex_lock(&buf_remux_audio.mut);
				//DEBUG_LOG("remuxVideoAudio thread writing AUDIO Moof Box...\n");
				
				//更新 trun box 的长度,需要加上samples的描述信息长度
				trun_box_len = t_ntohl(fmp4BOX.traf_audio->trunBox->header.size) + \
								buf_remux_audio.write_index * sizeof(trun_A_sample_t);
				fmp4BOX.traf_audio->trunBox->header.size = t_htonl(trun_box_len);
				
				//更新 traf box长度
				traf_box_len = t_ntohl(fmp4BOX.traf_audio->trafBox->header.size);
				traf_box_len += t_ntohl(fmp4BOX.traf_audio->tfhdBox->header.size)\
							  + t_ntohl(fmp4BOX.traf_audio->tfdtBox->header.size)\
							  + t_ntohl(fmp4BOX.traf_audio->trunBox->header.size);
				fmp4BOX.traf_audio->trafBox->header.size = t_htonl(traf_box_len);
				
				//更新 moof box长度
				moof_box_len += t_ntohl(fmp4BOX.traf_audio->trafBox->header.size);
				fmp4BOX.moofBox->header.size = t_htonl(moof_box_len);

				//更新 mdat box长度
				mdat_box_len += buf_remux_audio.write_pos - buf_remux_audio.remux_audio_buf;
				fmp4BOX.mdatBox->header.size = t_htonl(mdat_box_len);

				//记录 moof的偏移等信息
				buf_remux_audio.entry_info[buf_remux_audio.entry_info_num].moof_offset = t_htonl(fmp4_file_lable.moofBox_offset);
				//time 存在bug，时间不能直接用如下方式，因，每个moof+mdat 结构里边的帧数量并不固定，导致时间分布并不均匀。
				DEBUG_LOG("audio entry_info_num = %d\n",buf_remux_audio.entry_info_num);
				
				buf_remux_audio.entry_info[buf_remux_audio.entry_info_num].time = t_htonl(tfra_audio_time); //t_htonl((buf_remux_audio.entry_info_num) * AUDIO_TIME_SCALE);
				buf_remux_audio.entry_info[buf_remux_audio.entry_info_num].traf_number = 1;	
				buf_remux_audio.entry_info[buf_remux_audio.entry_info_num].trun_number = 1;
				buf_remux_audio.entry_info[buf_remux_audio.entry_info_num].sample_number = 1;
				buf_remux_audio.entry_info_num ++;
				pthread_mutex_unlock(&buf_remux_audio.mut);
			}

			//===公共部分box 写入文件==========================================================================
			//moof
			fmp4_file_lable.moofBox_offset = ((out_mode == SAVE_IN_FILE)? \
											ftell(file_handle) + buf_offset : \
											out_info.buf_mode.w_offset + buf_offset);
			
				DEBUG_LOG("fmp4BOX.moofBox address (%x)",fmp4BOX.moofBox);
				DEBUG_LOG("fmp4BOX.moofBox->header.type = %4s\n",fmp4BOX.moofBox->header.type);
			buf_cpy(moof_mdat_buf , fmp4BOX.moofBox , sizeof(moof_box) , buf_offset,moof_mdat_buf_size);
			buf_offset += sizeof(moof_box);

			//mfhd
			fmp4_file_lable.mfhdBox_offset = (out_mode == SAVE_IN_FILE)?\
											 ftell(file_handle) + buf_offset:\
											 out_info.buf_mode.w_offset + buf_offset;
			Sequence_number = Sequence_number +1;
			fmp4BOX.mfhdBox->sequence_number = t_htonl(Sequence_number);//填充片段的序号，以递增的方式
			buf_cpy(moof_mdat_buf , fmp4BOX.mfhdBox , sizeof(mfhd_box) , buf_offset,moof_mdat_buf_size);
			buf_offset += sizeof(mfhd_box);

			//=== traf box 部分写入文件 ==========================================================================
			if(have_video)
			{
				//-----将视频部分的 traf  	 写入到文件---------------------------------------------------------------
				pthread_mutex_lock(&buf_remux_video.mut);
				//traf
				fmp4_file_lable.traf_video_offset.trafBox_offset = (out_mode == SAVE_IN_FILE)?\
											 						ftell(file_handle) + buf_offset:\
											 						out_info.buf_mode.w_offset + buf_offset;;	
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_video->trafBox , sizeof(traf_box) , buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(traf_box);
				
				//tfhd
				//更新 base_data_offset ,赋值为当前所属 moof box 在文件中的偏移
				DEBUG_LOG("fmp4_file_lable.moofBox_offset = %d\n",fmp4_file_lable.moofBox_offset);
				fmp4BOX.traf_video->tfhdBox->base_data_offset = t_htonll((unsigned long long)fmp4_file_lable.moofBox_offset);
				fmp4BOX.traf_video->tfhdBox->default_sample_duration = t_htonl(VIDEO_TIME_SCALE/buf_remux_video.frame_rate);
			//	fmp4BOX.traf_video->tfhdBox->default_sample_size = 0;
				fmp4_file_lable.traf_video_offset.tfhdBox_offset = (out_mode == SAVE_IN_FILE)?\
																	 ftell(file_handle) + buf_offset:\
																	 out_info.buf_mode.w_offset + buf_offset;	
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_video->tfhdBox , sizeof(tfhd_box) , buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(tfhd_box);

				//---tfdt--------------------------------------------------------------------------------------
				DEBUG_LOG("debug 001\n");
				fmp4_file_lable.traf_video_offset.tfdtBox_offset = (out_mode == SAVE_IN_FILE)?\
																	 ftell(file_handle) + buf_offset:\
																	 out_info.buf_mode.w_offset + buf_offset;	
				//修正 tfdt ---> Base media decode time 参数
				fmp4BOX.traf_video->tfdtBox->baseMediaDecodeTime = t_htonl(tfra_video_time);
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_video->tfdtBox ,\
						t_ntohl(fmp4BOX.traf_video->tfdtBox->header.size) , buf_offset,moof_mdat_buf_size);
				buf_offset += t_ntohl(fmp4BOX.traf_video->tfdtBox->header.size);

				//---trun--------------------------------------------------------------------------------------
				fmp4_file_lable.traf_video_offset.trunBox_offset = (out_mode == SAVE_IN_FILE)?\
																	 ftell(file_handle) + buf_offset:\
																	 out_info.buf_mode.w_offset + buf_offset;	
				
				//写之前需要更新box中样本点个数等描述信息
				// sample_count
				fmp4BOX.traf_video->trunBox->sample_count = t_htonl(buf_remux_video.write_index);

				// data_offset
				//moof开始到（音/视频）数据部分的偏移长度  
				//moof box的总长度 + 8字节的mdat box头，就是 video samples的数据起始位置
				data_offset = t_ntohl(fmp4BOX.moofBox->header.size) + 8; //8为mdat的头长度
				fmp4BOX.traf_video->trunBox->data_offset = t_htonl(data_offset);

				//fmp4BOX.traf_video->trunBox->first_sample_flags = ;//暂时不赋值 ，已经在初始化时做了。
				
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_video->trunBox ,\
						sizeof(trun_box), buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(trun_box);
				//拷贝trun--->samples info
				DEBUG_LOG("t_ntohl(buf_remux_video.sample_info[0].trun_sample.sample_size(%d)\n",\
								t_ntohl(buf_remux_video.sample_info[0].trun_sample.sample_size));
				for(i = 0;i < buf_remux_video.write_index;i++)
				{
					tfra_video_time +=  t_ntohl(buf_remux_video.sample_info[i].trun_sample.sample_duration); 
					buf_cpy(moof_mdat_buf , (unsigned char*)&buf_remux_video.sample_info[i].trun_sample,
						sizeof(trun_V_sample_t), buf_offset,moof_mdat_buf_size);
					buf_offset += sizeof(trun_V_sample_t);
				}
				DEBUG_LOG("debug 001\n");
				
				//----------------------------------------------------------------------------------------------
				pthread_mutex_unlock(&buf_remux_video.mut);
			}

			if(have_audio)
			{
				//-----将音频部分的 traf       box 写入到文件------------------------------------------------------------------
				pthread_mutex_lock(&buf_remux_audio.mut);
				//traf
				fmp4_file_lable.traf_audio_offset.trafBox_offset = (out_mode == SAVE_IN_FILE)?\
																 ftell(file_handle) + buf_offset:\
																 out_info.buf_mode.w_offset + buf_offset;	
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_audio->trafBox ,\
						sizeof(traf_box), buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(traf_box);

				//tfhd
				//更新 base_data_offset ,赋值为当前所属 moof box 在文件中的偏移
				DEBUG_LOG("audio fmp4_file_lable.moofBox_offset = %d\n",fmp4_file_lable.moofBox_offset);
				fmp4BOX.traf_audio->tfhdBox->base_data_offset = t_htonll((unsigned long long)fmp4_file_lable.moofBox_offset);
				
				fmp4_file_lable.traf_audio_offset.tfhdBox_offset = (out_mode == SAVE_IN_FILE)?\
																	 ftell(file_handle) + buf_offset:\
																	 out_info.buf_mode.w_offset + buf_offset;
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_audio->tfhdBox ,\
					sizeof(tfhd_box), buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(tfhd_box);

				//---tfdt--------------------------------------------------------------------------------------------------
				fmp4_file_lable.traf_audio_offset.tfdtBox_offset = (out_mode == SAVE_IN_FILE)?\
																	 ftell(file_handle) + buf_offset:\
																	 out_info.buf_mode.w_offset + buf_offset;
				//修正 tfdt ---> Base media decode time 参数
				fmp4BOX.traf_audio->tfdtBox->baseMediaDecodeTime = t_htonl(tfra_audio_time);
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_audio->tfdtBox ,\
					sizeof(tfdt_box), buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(tfdt_box);

				//---trun--------------------------------------------------------------------------------------------------
				fmp4_file_lable.traf_audio_offset.trunBox_offset = (out_mode == SAVE_IN_FILE)?\
																	 ftell(file_handle) + buf_offset:\
																	 out_info.buf_mode.w_offset + buf_offset;
				//写之前需要更新box中样本点个数等描述信息
				// sample_count
				fmp4BOX.traf_audio->trunBox->sample_count = t_htonl(buf_remux_audio.write_index);

				// data_offset
				//moof开始到（音/视频）数据部分的偏移长度  
				//moof box的总长度 + 8字节的mdat box头 + video samples的长度，就是 audio samples的数据起始位置
				data_offset  = t_ntohl(fmp4BOX.moofBox->header.size) + 8 + \
								buf_remux_video.write_pos - buf_remux_video.remux_video_buf;//加上 video samples 的长度
				fmp4BOX.traf_audio->trunBox->data_offset = t_htonl(data_offset);

				//fmp4BOX.traf_video->trunBox->first_sample_flags = ;//暂时不赋值，已经在初始化做了
				
																	 
				buf_cpy(moof_mdat_buf , fmp4BOX.traf_audio->trunBox ,\
					sizeof(trun_box), buf_offset,moof_mdat_buf_size);
				buf_offset += sizeof(trun_box);
				
				//trun--->samples info
				for(i = 0;i < buf_remux_audio.write_index;i++)
				{
					tfra_audio_time +=  t_ntohl(buf_remux_audio.sample_info[i].trun_sample.sample_duration); 
					buf_cpy(moof_mdat_buf , (unsigned char*)&buf_remux_audio.sample_info[i].trun_sample ,\
						sizeof(trun_A_sample_t), buf_offset,moof_mdat_buf_size);
					buf_offset += sizeof(trun_A_sample_t);
				}
				pthread_mutex_unlock(&buf_remux_audio.mut);
			}
  
			//===mdat box 部分写入文件======================================================================================
			//	公共部分写入文件（mdat的头部）
			/*
			if(!remux_run)
			{
				//代表是该文件最后一个box
				fmp4BOX.mdatBox->header.size = t_htonl(0);
			}
			*/

			fmp4_file_lable.mdatBox_offset = (out_mode == SAVE_IN_FILE)?\
											 ftell(file_handle) + buf_offset:\
											 out_info.buf_mode.w_offset + buf_offset;
			buf_cpy(moof_mdat_buf , fmp4BOX.mdatBox,\
				sizeof(mdat_box), buf_offset,moof_mdat_buf_size);
			buf_offset += sizeof(mdat_box);

			//视频 samples data部分
			if(have_video)
			{
				pthread_mutex_lock(&buf_remux_video.mut);		
				//mdat--->samples data
				unsigned int samp_cpy_len = buf_remux_video.write_pos-buf_remux_video.remux_video_buf;
				if(samp_cpy_len <= 0)
				{
					ERROR_LOG("samp_cpy_len <= 0!\n");
					pthread_mutex_unlock(&buf_remux_video.mut);
					pthread_exit(0);
				}
				//DEBUG_LOG("samp_cpy_len = %d\n",samp_cpy_len);   //// BUG 定位处
				buf_cpy(moof_mdat_buf , buf_remux_video.remux_video_buf,\
						samp_cpy_len, buf_offset,moof_mdat_buf_size);
				buf_offset += samp_cpy_len;
				//DEBUG_LOG("samp_cpy_len001 = %d\n",samp_cpy_len);   //// BUG 定位处
				pthread_mutex_unlock(&buf_remux_video.mut);
			}
			
			//音频 samples data  部分
			if(have_audio)
			{
				pthread_mutex_lock(&buf_remux_audio.mut);
				//mdat--->samples data
				unsigned int samp_cpy_len = buf_remux_audio.write_pos-buf_remux_audio.remux_audio_buf;
				if(samp_cpy_len <= 0)
				{
					ERROR_LOG("samp_cpy_len <= 0!\n");
					pthread_mutex_unlock(&buf_remux_audio.mut);
					pthread_exit(0);
				}
				//DEBUG_LOG("samp_cpy_len = %d\n",samp_cpy_len);   
				print_char_array("Audio mdat samples:",buf_remux_audio.remux_audio_buf,16); //debug
				buf_cpy(moof_mdat_buf , buf_remux_audio.remux_audio_buf,\
						samp_cpy_len, buf_offset,moof_mdat_buf_size);
				buf_offset += samp_cpy_len;
				
				
				pthread_mutex_unlock(&buf_remux_audio.mut);
			}
		
			//长度信息修改后写入文件后需要归位,下一次用时得继续保持刚初始化的状态
			fmp4BOX.moofBox->header.size = t_htonl(sizeof(moof_box));
			fmp4BOX.mfhdBox->header.size = t_htonl(sizeof(mfhd_box));
			fmp4BOX.traf_video->trafBox->header.size = t_htonl(sizeof(traf_box));
			fmp4BOX.traf_video->trunBox->header.size = t_htonl(sizeof(trun_box));
			fmp4BOX.traf_audio->trafBox->header.size = t_htonl(sizeof(traf_box));
			fmp4BOX.traf_audio->trunBox->header.size = t_htonl(sizeof(trun_box));
			fmp4BOX.mdatBox->header.size = t_htonl(sizeof(mdat_box));  //BUG 解决

			/*======音视频缓存buf指针归位，buf需要循环重写。=======================================
			该归位如果单独放在 remuxAudio/remuxVideo函数里边做会存在BUG,
			假设，AUDIO满足写的帧数，写入了新的moof+mdat box，但此时 video 帧并没有满一个 framerate数，
			但audio复位了，video缓存区并没有复位。还是放在一起做复位的好。
			还有下一次写时第一帧不会是I帧，打破了mdat box 第一帧视频帧是I（IDR）帧的规则（这点其实非必须满足）。
			所以，原则上保持mdat box 第一个video帧是 I（IDR）帧为主，而音频帧相对来说比较随意。
			*/ 
			#if HAVE_AUDIO
				pthread_mutex_lock(&buf_remux_audio.mut);
				//pthread_cond_wait(&buf_remux_audio.remux_ready, &buf_remux_audio.mut);
				buf_remux_audio.write_pos = buf_remux_audio.remux_audio_buf;
				buf_remux_audio.read_pos = buf_remux_audio.remux_audio_buf;
				buf_remux_audio.frame_count = 0;
				buf_remux_audio.write_index = 0;
				buf_remux_audio.need_remux = 0;
				pthread_mutex_unlock(&buf_remux_audio.mut);
			#endif

			#if HAVE_VIDEO
				pthread_mutex_lock(&buf_remux_video.mut);		
				//pthread_cond_wait(&buf_remux_video.remux_ready, &buf_remux_video.mut);
				buf_remux_video.write_pos = buf_remux_video.remux_video_buf;
				buf_remux_video.read_pos = buf_remux_video.remux_video_buf;
				buf_remux_video.frame_count = 0;
				buf_remux_video.write_index = 0;
				buf_remux_video.need_remux = 0;
				pthread_mutex_unlock(&buf_remux_video.mut); 
			#endif
			///*======end====================================================================
			
			//通知主线程，缓冲区复位，缓冲新数据,主线程循环下一轮工作
			remux_v_can_write = 0;
			remux_A_can_write = 0;
			
			have_video = 1; //标志复位
			have_audio = 1;

			/*	
				接下来再将 moof + mdat数据统一写入文件
			*/
			DEBUG_LOG("moof_mdat_buf address (%x)",moof_mdat_buf);
			moof_box *moof_tmp = (moof_box*)moof_mdat_buf; 
			DEBUG_LOG("moof_tmp->header.type = %4s\n",moof_tmp->header.type);
			//DEBUG_LOG("debug 001 write size(%d)\n",buf_offset);

			struct timeval front_time = {0};
			struct timeval tmp_time = {0};
			gettimeofday(&front_time,NULL);
			memcpy(&tmp_time,&front_time,sizeof(front_time));

			if(out_mode == SAVE_IN_FILE)
			{
				fseek(file_handle,0, SEEK_END); 
							int ret = fwrite(moof_mdat_buf ,1,buf_offset,file_handle); 
							if(ret < 0)
							{
								ERROR_LOG("fwrite file error!\n");
								pthread_exit(0);
							}
							fflush(file_handle);
							gettimeofday(&front_time,NULL);
							DEBUG_LOG("write file use time: (%d)s (%d)us\n",front_time.tv_sec - tmp_time.tv_sec,\
																			front_time.tv_usec - tmp_time.tv_usec);
			}
			else  //保存到内存
			{
				//print_char_array("ftyp15",out_info.buf_mode.buf_start,10);
				if(out_info.buf_mode.w_offset + buf_offset > out_info.buf_mode.buf_size)
				{
						ERROR_LOG("over write! fmp4 out put file memory is pool!\n");
						pthread_exit(0);
				}
				memcpy(out_info.buf_mode.buf_start + out_info.buf_mode.w_offset,moof_mdat_buf,buf_offset);
				out_info.buf_mode.w_offset += buf_offset;
				DEBUG_LOG("out_info.buf_mode.w_offset = %d\n",out_info.buf_mode.w_offset);
				//print_char_array("ftyp16",out_info.buf_mode.buf_start,10);
			}
			
			memset(moof_mdat_buf,0,moof_mdat_buf_size);
			buf_offset = 0; 
	
		}
		else
		{
			//DEBUG_LOG("===\n");
			usleep(100);
		}


	}
	while(remux_run);

	//最后写入一个结束box     		mfra box及其子box 
	mfra_box *mfraBox = NULL; 
	unsigned int mfraBox_len = 0;
	unsigned int mfre_ret = mfra_box_rebuid(&mfraBox,&mfraBox_len);
	
	fmp4_file_lable.mfraBox_offset =  (out_mode == SAVE_IN_FILE)?\
									  ftell(file_handle):\
									  out_info.buf_mode.w_offset;
	if(out_mode == SAVE_IN_FILE)
	{
		fseek(file_handle,0, SEEK_END); 
					int ret = fwrite(mfraBox ,1,mfraBox_len,file_handle); 
					if(ret < 0)
					{
						ERROR_LOG("fwrite file error!\n");
						pthread_exit(0);
					}
					fflush(file_handle);		
					
	}
	else  //保存到内存
	{
		print_char_array("mfra",(unsigned char*)mfraBox,10);
		if(out_info.buf_mode.w_offset + mfraBox_len > out_info.buf_mode.buf_size)
		{
				ERROR_LOG("over write! fmp4 out put file memory is pool!\n");
				pthread_exit(0);
		}
		memcpy(out_info.buf_mode.buf_start + out_info.buf_mode.w_offset,mfraBox,mfraBox_len);
		out_info.buf_mode.w_offset += mfraBox_len;
		DEBUG_LOG("out_info.buf_mode.w_offset = %d\n",out_info.buf_mode.w_offset);
		
	}

	/***DEBUG 保存到内存模式下最后整体再写入到文件*********************************/
	
	#if 1
		char* debug_file_name = "/jffs0/fmp4.mp4";
		if(0 == access(debug_file_name,F_OK))
		{
			if(0 == remove(debug_file_name))
			{
				DEBUG_LOG("remove old file success!\n");
			}
			else
			{
				ERROR_LOG("remove old file failed!\n");
				pthread_exit(0);
			}
		}

		FILE*debug_file = fopen(debug_file_name, "wb+");
		if(NULL == debug_file )
		{
			ERROR_LOG("debug open fmp4 file failed!\n");
			pthread_exit(0);
		}
		DEBUG_LOG("debug open file success!\n");

		print_char_array("ftyp17",out_info.buf_mode.buf_start,40);
		int debug_ret = fwrite(out_info.buf_mode.buf_start,1,out_info.buf_mode.w_offset,debug_file);
		if(debug_ret < 0)
		{
			ERROR_LOG("write file error!\n");
			fclose(debug_file);
			pthread_exit(0);
		}
		DEBUG_LOG("fewite file size(%d)\n",debug_ret);
		fclose(debug_file);
	#endif
	/*****************************************/

	
	DEBUG_LOG("Thread remuxVideoAudio exit!\n");

	//remux_write_last_ok = 1; //该句一定要放在该函数退出前的最尾巴上（置位后主线程将进行释放操作）其后再使用内存会造成错误
	pthread_mutex_lock(&remux_write_last_mut);
		pthread_cond_signal(&remux_write_last_ok);
	pthread_mutex_unlock(&remux_write_last_mut);
	
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
	pthread_mutex_init(&remux_write_last_mut, NULL);
    pthread_cond_init(&remux_write_last_ok, NULL);
	
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
	memset(buf_remux_video.entry_info,0,MAX_MOOF_MDAT_NUM);
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
	/*
	if(0 == remux_write_last_ok)
	{
		DEBUG_LOG("waiting  remux_write_last_ok(%d)...\n",remux_write_last_ok);
		usleep(1000*100);
	}
	*/
	DEBUG_LOG("into pthread_cond_wait !\n");
	pthread_mutex_lock(&remux_write_last_mut);
		pthread_cond_wait(&remux_write_last_ok, &remux_write_last_mut);
	pthread_mutex_unlock(&remux_write_last_mut);
		
	//===释放=======================================
	
	pthread_mutex_destroy(&buf_remux_video.mut);  
   	//pthread_cond_destroy(&buf_remux_audio.mut);
	free(buf_remux_video.remux_video_buf);
	free(buf_remux_audio.remux_audio_buf);
	buf_remux_video.remux_video_buf = NULL;
	buf_remux_audio.remux_audio_buf = NULL;

	
	free(fmp4BOX.traf_video->trafBox);
	fmp4BOX.traf_video->trafBox = NULL;
	free(fmp4BOX.traf_video->tfhdBox);
	fmp4BOX.traf_video->tfhdBox = NULL;
	free(fmp4BOX.traf_video->tfdtBox);
	fmp4BOX.traf_video->tfdtBox = NULL;
	free(fmp4BOX.traf_video->trunBox);
	fmp4BOX.traf_video->trunBox = NULL;
	
	
	free(fmp4BOX.traf_audio->trafBox);
	fmp4BOX.traf_audio->trafBox = NULL;
	free(fmp4BOX.traf_audio->tfhdBox);
	fmp4BOX.traf_audio->tfhdBox = NULL;
	free(fmp4BOX.traf_audio->tfdtBox);
	fmp4BOX.traf_audio->tfdtBox = NULL;
	free(fmp4BOX.traf_audio->trunBox);
	fmp4BOX.traf_audio->trunBox = NULL;

	
	//mfhd
	free(fmp4BOX.mfhdBox);
	fmp4BOX.mfhdBox = NULL;
	
	//moof的释放呢？？
	free(fmp4BOX.moofBox);
	fmp4BOX.moofBox = NULL;
	
	
	//mdat
	free(fmp4BOX.mdatBox);
	fmp4BOX.mdatBox = NULL;

	Sequence_number = 0;
	if(out_mode == SAVE_IN_FILE)
		fclose(file_handle);

	if(RECODE_AAC_FRAME_TO_FILE) //DEBUG
		close(AAC_fd);
	if(USE_44100_AAC_FILE)  //DEBUG
		close(AAC_44100_fd);
	
	DEBUG_LOG("remux_exit success!\n");
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
int sps_pps_parameter_set(void)
{
	
	avcc_box_info_t *avcc_buf = avcc_box_init();
	if(NULL == avcc_buf)
	{
		ERROR_LOG("sps_pps_parameter_set failed !\n");
		return -1;
	}
	DEBUG_LOG("out avcc_box_init....\n");

	
	return 0;
}

int Fmp4AEncode(void * audio_frame, unsigned int frame_length, unsigned int frame_rate,unsigned long long time_scale)
{
	int ret = remuxAudio(audio_frame,frame_length,frame_rate,time_scale);
	return ret;
}

int Fmp4VEncode(void *video_frame,unsigned int frame_length,unsigned int frame_rate,unsigned long long time_scale)
{
	int ret = remuxVideo(video_frame,frame_length,frame_rate,time_scale);
	return ret;
}


int Fmp4_encode_exit(void)
{
	
	int ret = remux_exit();
	//还需要释放每次申请的 box的空间
	return ret;
}























