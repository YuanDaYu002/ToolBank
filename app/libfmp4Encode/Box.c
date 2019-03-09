




#include <string.h>
#include <sys/time.h>
#include <stdlib.h>

#include "Box.h"
#include "typeport.h"
#include "my_inet.h"


avcc_box_info_t avcc_box_info = {0}; //主要外部传入sps/pps nalu 包来初始化



//大小端互相转换,传入大端返回小端，传入小端返回大端
int switch_small_BigEndian(int num)
{

	int ret_num = (( num&0x000000ff)<< 24)|\
						(( num&0x0000ff00)<< 8 )|\
						(( num&0x00ff0000)>> 8 )|\
						(( num&0xff000000)>> 24);
	
	return ret_num;

}

//用来以十六进制字节流打印box,用于调试
void print_char_array(unsigned char* box_name,unsigned char*start,unsigned int length)
{
	#ifdef HLE_DEBUG
		unsigned char*p = start;
		unsigned int i = 0;
		printf("\n  %s[]: ",box_name);
		for(i=0;i<length;i++)
		{
			printf("%x ",p[i]);
		}
		printf("\n");
	#endif
	
}

/*
	请求一个 ftpy box 
	返回值为初始化后的box首地址
	注意：使用完后需要调用free释放
*/
ftyp_box * ftyp_box_init(void)
{
		
	 unsigned char Array[] = {
            0x69, 0x73, 0x6F, 0x6D, // major_brand: isom     isom  MP4  Base Media v1[IS0 14496-12:2003]ISO YES video/mp4
            0x00, 0x00, 0x02, 0x00,		//0x0, 0x0, 0x0, 0x1, 	// minor_version: 0x01 (最低兼容性的版本)
            0x69, 0x73, 0x6F, 0x6D, // isom 
            0x69, 0x73, 0x6F, 0x32,	// iso2
            0x61, 0x76, 0x63, 0x31, 	// avc1
            0X69, 0x73, 0x6F, 0x36,	// iso6
            0X6D, 0x70, 0x34, 0x31		// mp41	
        	};

	int box_size = sizeof(Array)+sizeof(BoxHeader_t);
	DEBUG_LOG("ftpy_item malloc size(%d)\n",box_size);
	ftyp_box *ftyp_item = (ftyp_box *)malloc(box_size);
	if(NULL == ftyp_item)
		return NULL;
	
	memset(ftyp_item,0,box_size);
	ftyp_item->header.size = t_htonl(box_size);
	strncpy(ftyp_item->header.type,"ftyp",4);
	
	memcpy(((char*)ftyp_item) + sizeof(BoxHeader_t),Array,sizeof(Array));

	return ftyp_item;
	
}

moov_box* moov_box_init(unsigned int box_length)
{
	DEBUG_LOG("moov_item malloc size(%d)\n",box_length);
	moov_box * moov_item = (moov_box *)malloc(box_length);
	if(NULL == moov_item)
		return NULL;
	memset(moov_item,0,box_length);
	moov_item->header.size = t_htonl(box_length);//上层调用增加child box后再及时修正
	strncpy(moov_item->header.type,"moov",4);


	return moov_item;
	
}

moof_box* moof_box_init(unsigned int box_length)
{
	DEBUG_LOG("moof_item malloc size(%d)\n",box_length);
	moof_box * moof_item = (moof_box *)malloc(box_length);
	if(NULL == moof_item)
		return NULL;
	memset(moof_item,0,box_length);
	moof_item->header.size = t_htonl(box_length);//上层调用增加child box后再及时修正
	strncpy(moof_item->header.type,"moof",4);

	return moof_item;
}

mdat_box* mdat_box_init(unsigned int box_length)
{
	DEBUG_LOG("mdat_item malloc size(%d)\n",box_length);
	mdat_box * mdat_item = (mdat_box *)malloc(box_length);
	if(NULL == mdat_item)
		return NULL;
	memset(mdat_item,0,box_length);
	mdat_item->header.size = t_htonl(box_length);//上层调用增加child box后再及时修正
	strncpy(mdat_item->header.type,"mdat",4);

	return mdat_item;
}

mfra_box* mfra_box_init(void)
{
	DEBUG_LOG("mfra_item malloc size(%d)\n",sizeof(mfra_box));
	mfra_box * mfra_item = (mfra_box *)malloc(sizeof(mfra_box));
	if(NULL == mfra_item)
		return NULL;
	memset(mfra_item,0,sizeof(mfra_box));
	mfra_item->header.size = t_htonl(sizeof(mfra_box));//上层调用增加child box后再及时修正
	strncpy(mfra_item->header.type,"mfra",4);

	return mfra_item;
}

	
/***二级BOX初始化***************************************/
/*
	timescale = 90000;
	duration = 0;		
*/
mvhd_box* mvhd_box_init(unsigned int timescale,unsigned int duration)
{
	DEBUG_LOG("mvhd_item malloc size(%d)\n",sizeof(mvhd_box));
	mvhd_box * mvhd_item = (mvhd_box *)malloc(sizeof(mvhd_box));
	if(NULL == mvhd_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
		
	memset(mvhd_item,0,sizeof(mvhd_box));

	mvhd_item->header.size = t_htonl(sizeof(mvhd_box));
	strncpy(mvhd_item->header.type,"mvhd",4);
	mvhd_item->header.version = 0;

	/*struct timeval buf;
	gettimeofday(&buf,NULL);
	mvhd_item->creation_time = t_htonl(buf.tv_sec);
	*/
		timescale = 1000;  //DEBUG
	unsigned char MVHD[] = {
				0x00, 0x00, 0x00, 0x00, 	// version(0) + flags	  1位的box版本+3位flags   box版本，0或1，一般为0。（以下字节数均按version=0）
				0x00, 0x00, 0x00, 0x00, 	// creation_time	创建时间  （相对于UTC时间1904-01-01零点的秒数）
				0x00, 0x00, 0x00, 0x00, 	// modification_time   修改时间
				(timescale >> 24) & 0xFF, 	// timescale: 4 bytes		文件媒体在1秒时间内的刻度值，可以理解为1秒长度
				(timescale >> 16) & 0xFF,
				(timescale >> 8) & 0xFF,
				(timescale) & 0xFF,
				(duration >> 24) & 0xFF, // duration: 4 bytes	该track的时间长度，用duration和time scale值可以计算track时长，比如video track的time scale = 600, duration = 42000，时长为70
				(duration >> 16) & 0xFF,
				(duration >> 8) & 0xFF,
				(duration) & 0xFF,
				0x00, 0x01, 0x00, 0x00, // Preferred rate: 1.0	 推荐播放速率，高16位和低16位分别为小数点整数部分和小数部分，即[16.16] 格式，该值为1.0（0x00010000）表示正常前向播放
				0x01, 0x00, 0x00, 0x00, // PreferredVolume(1.0, 2bytes) + reserved(2bytes)	与rate类似，[8.8] 格式，1.0（0x0100）表示最大音量 
				0x00, 0x00, 0x00, 0x00, // reserved: 4 + 4 bytes	保留位
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x01, 0x00, 0x00, // ----begin composition matrix----
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, //视频变换矩阵   线性代数
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x01, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x40, 0x00, 0x00, 0x00, // ----end composition matrix----
				0x00, 0x00, 0x00, 0x00, // ----begin pre_defined 6 * 4 bytes----
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, //pre-defined 保留位
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, // ----end pre_defined 6 * 4 bytes----
				0x00, 0x00, 0x00, 0x02  // next_track_ID 下一个track使用的id号
		};
		DEBUG_LOG("sizeof(MVHD) = %d\n",sizeof(MVHD));
		memcpy((char*)mvhd_item + 8,MVHD,sizeof(MVHD));//+8是为了跳过header的size和type

		return mvhd_item;
	
}

trak_box* trak_box_init(unsigned int box_length)
{
	DEBUG_LOG("track_item malloc size(%d)\n",box_length);
	
	trak_box *track_item = (trak_box*)malloc(box_length);
	if(NULL == track_item)
	{
		ERROR_LOG("trak_box_init error!\n");
		return NULL;
	}
	memset(track_item,0,box_length);

	track_item->header.size = t_htonl(box_length);
	strncpy(track_item->header.type,"trak",4);
	
	return track_item;
}

mvex_box*	mvex_box_init(unsigned int box_length)
{
	DEBUG_LOG("mvex_item malloc size(%d)\n",box_length);
	mvex_box *mvex_item = (mvex_box*)malloc(box_length);
	if(NULL == mvex_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
	
	memset(mvex_item,0,box_length);
	mvex_item->header.size  = t_htonl(box_length);	
	strncpy(mvex_item->header.type,"mvex",4);
//	mvex_item->header.version = 0;

	return mvex_item;

}

mfhd_box* mfhd_box_init(void)
{
	DEBUG_LOG("mfhd_item malloc size(%d)\n",sizeof(mfhd_box));
	mfhd_box *mfhd_item = (mfhd_box*)malloc(sizeof(mfhd_box));
	if(NULL == mfhd_item)
		return NULL;
	memset(mfhd_item,0,sizeof(mfhd_box));
	mfhd_item->header.size  = t_htonl(sizeof(mfhd_box));
	strncpy(mfhd_item->header.type,"mfhd",4);
	mfhd_item->header.version = 0;

	return mfhd_item;
}

traf_box*	traf_box_init(unsigned int box_length)
{
	DEBUG_LOG("traf_item malloc size(%d)\n",box_length);
	traf_box *traf_item = (traf_box*)malloc(box_length);
	if(NULL == traf_item)
		return NULL;
	memset(traf_item,0,box_length);
	traf_item->header.size  = t_htonl(box_length);
	strncpy(traf_item->header.type,"traf",4);


	return traf_item;
}

tfra_box * tfra_box_init(void)
{
	DEBUG_LOG("tfra_item malloc size(%d)\n",sizeof(tfra_box));
	tfra_box *tfra_item = (tfra_box*)malloc(sizeof(tfra_box));
	if(NULL == tfra_item)
	{
		ERROR_LOG("malloc failed !\n");
		return NULL;
	}
	memset(tfra_item,0,sizeof(tfra_box));
	
	tfra_item->header.size = t_htonl(sizeof(tfra_box));
	strncpy(tfra_item->header.type,"tfra",4);

	//memset 已经搞定，不做重复操作
	//tfra_item->length_size_of_traf_num = {0};
	//tfra_item->length_size_of_trun_num = {0};
	//tfra_item->length_size_of_sample_num = {0};
	//tfra_item->number_of_entry = 0;

	return tfra_item;
	
}

static tfra_video_t  tfra_video = {0};
tfra_video_t * tfra_video_init(void)
{
	tfra_video.tfraBox = tfra_box_init();
	if(NULL == tfra_video.tfraBox)
	{
		ERROR_LOG("tfra_video_init failed !\n");
		return NULL;
	}
	return &tfra_video;
}

static tfra_audio_t  tfra_audio = {0};
tfra_audio_t * tfra_audio_init()
{
	tfra_audio.tfraBox = tfra_box_init();
	if(NULL == tfra_audio.tfraBox)
	{
		ERROR_LOG("tfra_audio_init failed !\n");
		return NULL;
	}
	return &tfra_audio;
}



mfro_box * mfro_box_init(void)
{
	DEBUG_LOG("mfro_item malloc size(%d)\n",sizeof(mfro_box));
	mfro_box *mfro_item = (mfro_box*)malloc(sizeof(mfro_box));
	if(NULL == mfro_item)
	{
		ERROR_LOG("malloc failed !\n");
		return NULL;
	}
	memset(mfro_item,0,sizeof(mfro_box));
	
	mfro_item->header.size = t_htonl(sizeof(mfro_box));
	strncpy(mfro_item->header.type,"mfro",4);

	mfro_item->size = t_htonl(sizeof(mfro_box));//最终要和mfra的大小一样。这里只初始化为自身大小

	return mfro_item;
	
}


/***三级BOX初始化***************************************/
tkhd_box* tkhd_box_init(unsigned int trackId,unsigned int duration,unsigned short width,unsigned short height)
{
	DEBUG_LOG("tkhd_item malloc size(%d)\n",sizeof(tkhd_box));
	tkhd_box *tkhd_item = (tkhd_box*)malloc(sizeof(tkhd_box));
	if(NULL == tkhd_item)
	{
		ERROR_LOG("malloc failed !\n");
		return NULL;
	}
	tkhd_item->header.size = t_htonl(sizeof(tkhd_box));
	strncpy(tkhd_item->header.type,"tkhd",4);
	
	//tkhd_item->header->version = 0;//放到下边的数组了
	unsigned char TKHD[] = {
            0x00, 0x00, 0x00, 0x07, // version(0) + flags 1位版本 box版本，0或1，一般为0。（以下字节数均按version=0）按位或操作结果值，预定义如下：
            // 0x000001 track_enabled，否则该track不被播放；
            // 0x000002 track_in_movie，表示该track在播放中被引用；
            // 0x000004 track_in_preview，表示该track在预览时被引用。
            // 一般该值为7，1+2+4 如果一个媒体所有track均未设置track_in_movie和track_in_preview，将被理解为所有track均设置了这两项；对于hint track，该值为0
            // hint track  这个特殊的track并不包含媒体数据，而是包含了一些将其他数据track打包成流媒体的指示信息。
            0x00, 0x00, 0x00, 0x00, // creation_time	创建时间（相对于UTC时间1904-01-01零点的秒数）
            0x00, 0x00, 0x00, 0x00, // modification_time	修改时间
            (trackId >> 24) & 0xFF, // track_ID: 4 bytes	id号，不能重复且不能为0
            (trackId >> 16) & 0xFF,
            (trackId >> 8) & 0xFF,
            (trackId) & 0xFF,
            0x00, 0x00, 0x00, 0x00, // reserved: 4 bytes    保留位
            (duration >> 24) & 0xFF, // duration: 4 bytes  	track的时间长度
            (duration >> 16) & 0xFF,
            (duration >> 8) & 0xFF,
            (duration) & 0xFF,
            0x00, 0x00, 0x00, 0x00, // reserved: 2 * 4 bytes    保留位
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, // layer(2bytes) + alternate_group(2bytes)  视频层，默认为0，值小的在上层.track分组信息，默认为0表示该track未与其他track有群组关系
            0x00, 0x00, 0x00, 0x00, // volume(2bytes) + reserved(2bytes)    [8.8] 格式，如果为音频track，1.0（0x0100）表示最大音量；否则为0   +保留位
            0x00, 0x01, 0x00, 0x00, // ----begin composition matrix----
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x01, 0x00, 0x00, // 视频变换矩阵
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x40, 0x00, 0x00, 0x00, // ----end composition matrix----
            (width >> 8) & 0xFF, // //宽度
            (width) & 0xFF,
            0x00, 0x00,
            (height >> 8) & 0xFF, // 高度
            (height) & 0xFF,
            0x00, 0x00
        };
			
		DEBUG_LOG("sizeof(TKHD) = %d\n",sizeof(TKHD));	
		memcpy((unsigned char*)tkhd_item + 8,TKHD,sizeof(TKHD));//+ 8 为了跳过 header 里边的size和type
		return tkhd_item;
	
}

mdia_box* mdia_box_init(unsigned int box_length)
{
	DEBUG_LOG("mdia_item malloc size(%d)\n",box_length);
	mdia_box* mdia_item = (mdia_box*)malloc(box_length);
	if(NULL == mdia_item)
	{
		ERROR_LOG("malloc failed !\n");
		return NULL;
	}
		
	memset(mdia_item,0,box_length);

	mdia_item->header.size = t_htonl(box_length);
	strncpy(mdia_item->header.type ,"mdia",4);
	
	return mdia_item;
	
}


trex_box*	trex_box_init(unsigned int trackId)
{

	DEBUG_LOG("trex_item malloc size(%d)\n",sizeof(trex_box));	
	trex_box* trex_item = (trex_box*)malloc(sizeof(trex_box));
	if(NULL == trex_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}

	memset(trex_item,0,sizeof(trex_box));
	trex_item->header.size = t_htonl(sizeof(trex_box));
	strncpy(trex_item->header.type,"trex",4);
	DEBUG_LOG("trackId = %d\n",trackId);
	
		 const unsigned char TREX[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags
            (trackId >> 24) & 0xFF, // track_ID
            (trackId >> 16) & 0xFF,
            (trackId >> 8 ) & 0xFF,
            (trackId) & 0xFF,
            0x00, 0x00, 0x00, 0x01, // default_sample_description_index
            0x00, 0x00, 0x00, 0x00, // default_sample_duration
            0x00, 0x00, 0x00, 0x00, // default_sample_size
             0x00, 0x00, 0x00, 0x00 //0x00, 0x01, 0x00, 0x01 // default_sample_flags
        };
	memcpy((unsigned char*)trex_item + 8 ,TREX,sizeof(TREX));//+ 8为偏移过header的size和type 
	return trex_item;
	
}

tfhd_box*	tfhd_box_init(unsigned int trackId)
{
	DEBUG_LOG("tfhd_item malloc size(%d)\n",sizeof(tfhd_box));
	tfhd_box* tfhd_item = (tfhd_box*)malloc(sizeof(tfhd_box));
	if(NULL == tfhd_item)
		return NULL;
	memset(tfhd_item,0,sizeof(tfhd_box));
	tfhd_item->header.size = t_htonl(sizeof(tfhd_box));
	strncpy(tfhd_item->header.type,"tfhd",4);
	unsigned int flag = 0;

	#if 1
		if(trackId == VIDEO_TRACK)
		{
			flag = DEFAULT_BASE_IS_MOOF + \
				   E_default_sample_duration + \
				   E_base_data_offset;  //E_default_sample_flags + 
		}
		else
		{
			flag = DEFAULT_BASE_IS_MOOF + \
				   E_default_sample_duration + \
				   E_base_data_offset; // E_default_sample_flags + 
		}
		unsigned char version_flags[4] = {
										 0x00,  //version: 1bytes 
										 flag >> 16 & 0xFF, //flags 4 bytes
										 flag >> 8  & 0xFF, 
										 flag & 0xFF
										 };// version(0) & flags  //从每个视频的偏移位置从moof算起。
		memcpy(&tfhd_item->header.version,&version_flags,4);
		tfhd_item->track_ID = t_htonl(trackId);
		tfhd_item->base_data_offset = 0x00;
		//tfhd_item->sample_description_index = 0x00;
		tfhd_item->default_sample_duration = t_htonl(VIDEO_TIME_SCALE/15);//默认15帧，填充前还得修正
		//tfhd_item->default_sample_size = 0x00;
	
		if(trackId == VIDEO_TRACK) //video
		{
			//tfhd_item->default_sample_flags = t_htonl(16842752); // video samples 的标志
		}
		else					  //audio
		{
			//tfhd_item->default_sample_flags = t_htonl(33554432); //audio samples 的标志
		}
			
	
	#else
	
    const unsigned char TFHD[] = {
        //0x00, 0x00, 0x00, 0x00, // version(0) & flags
        0x00, 0x02, 0x00, 0x19,	// version(0) & flags  //从每个视频的偏移位置从moof算起。
        (trackId >> 24) & 0xFF, // track_ID
        (trackId >> 16) & 0xFF,
        (trackId >> 8) & 0xFF,
        (trackId) & 0xFF,
        
        //后边为新增部分
        0x00,0x00,0x00,0x00,	//base_data_offset
       // 0x00,0x00,0x00,0x00,	//sample_description_index
       	(default_sample_duration >> 24) & 0xFF, //default_sample_duration 
        (default_sample_duration >> 16) & 0xFF,
        (default_sample_duration >> 8) & 0xFF,
        (default_sample_duration) & 0xFF,

        0x00,0x00,0x00,0x00, //default_sample_size
       // 0x00,0x00,0x00,0x00, //default_sample_flags
        
    	};
	DEBUG_LOG("ziseof(TFHD) = %d\n",sizeof(TFHD));
	memcpy((unsigned char*)tfhd_item + 8,TFHD,sizeof(TFHD));
	#endif
	
	
	return tfhd_item;
	
}

tfdt_box*	tfdt_box_init(int baseMediaDecodeTime)
{
	unsigned int box_size = sizeof(tfdt_box);	
	tfdt_box* tfdt_item = (tfdt_box*)malloc(box_size);
	if(NULL == tfdt_item)
		return NULL;
	memset(tfdt_item,0,box_size);
	DEBUG_LOG("tfdt_item malloc size(%d)\n",box_size);
	
	tfdt_item->header.size = t_htonl(box_size);
	strncpy(tfdt_item->header.type,"tfdt",4);
	
    const unsigned char TFDT[] = {
		    0x00, 0x00, 0x00, 0x00, // version(0) & flags
		    (baseMediaDecodeTime >> 24) & 0xFF, // baseMediaDecodeTime: int32  该moof + mdat box 开始解码的时间（video/audio各自独立）
		    (baseMediaDecodeTime >> 16) & 0xFF,
		    (baseMediaDecodeTime >> 8 ) & 0xFF,
		    (baseMediaDecodeTime) & 0xFF
			};
	DEBUG_LOG("sizeof(TFDT) = %d\n",sizeof(TFDT));
	memcpy((unsigned char *)tfdt_item + 8,TFDT,sizeof(TFDT));

	return tfdt_item;

	
}
sdtp_box*	sdtp_box_init(void)
{
	DEBUG_LOG("sdtp_item malloc size(%d)\n",sizeof(sdtp_box));
	sdtp_box* sdtp_item = (sdtp_box*)malloc(sizeof(sdtp_box));
	if(NULL == sdtp_item)
		return NULL;
	
	memset(sdtp_item,0,sizeof(sdtp_item));

	sdtp_item->header.size = t_htonl(sizeof(sdtp_box));
	strncpy(sdtp_item->header.type,"sdtp",4);

	//先不实现，好像没有也没事
	
}



trun_box*	trun_box_init(unsigned int trackId)
{
	DEBUG_LOG("trun_item malloc size(%d)\n",sizeof(trun_box));
	trun_box* trun_item = (trun_box*)malloc(sizeof(trun_box));
	if(NULL == trun_item)
		return NULL;

	memset(trun_item,0,sizeof(trun_box));
	trun_item->header.size = t_htonl(sizeof(trun_box)); 
	strncpy(trun_item->header.type,"trun",4);
	unsigned int flag = 0;
	
	if(trackId == VIDEO_TRACK)//video
	{
		flag = E_data_offset + /*E_first_sample_flags +*/ E_sample_duration + E_sample_size + E_sample_flags ;
	}
	else 					 //audio
	{
		
		flag = E_data_offset + /* E_first_sample_flags*/ + E_sample_duration + E_sample_size /* + E_sample_flags*/;
	}
	
	unsigned char version_flags[] = {
									0x00,  					//version: 1bytes
									flag >> 16 & 0xFF, 		//flag: 3bytes
									flag >> 8  & 0xFF, 
									flag & 0xFF
									};//注意flag会决定下边可选的项目哪些有哪些没有
	memcpy((unsigned char*)trun_item + 8,version_flags,4);//初始化version & flags
	

	trun_item->sample_count = 0; //样本的个数，后续依据样本再动态修改
	#if 0 // first_sample_flags 部分
	if(trackId == VIDEO_TRACK) //video
	{
		if(flag & E_first_sample_flags)
			trun_item->first_sample_flags = t_htonl(16842752); 
	}
	else 
	{
		if(flag & E_first_sample_flags) //audio
			trun_item->first_sample_flags = t_htonl(33554432); 
	}
	#endif
	
	//后边的samples相关数据需要   音视频混合程序来实时填充，初始化不做处理

	return trun_item;
}
	
/****四级BOX初始化*********************************************************/

mdhd_box* mdhd_box_init(unsigned int timescale,unsigned int duration)
{
	DEBUG_LOG("mdhd_item malloc size(%d)\n",sizeof(mdhd_box));
	mdhd_box* mdhd_item = (mdhd_box*)malloc(sizeof(mdhd_box));
	if(NULL == mdhd_item)
		return NULL;

	memset(mdhd_item,0,sizeof(mdhd_item));

	mdhd_item->header.size = t_htonl(sizeof(mdhd_box));
	strncpy(mdhd_item->header.type,"mdhd",4);

	unsigned char MDHD[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags // version(0) + flags		box版本，0或1，一般为0。
            0x00, 0x00, 0x00, 0x00, // creation_time    创建时间
            0x00, 0x00, 0x00, 0x00, // modification_time修改时间
            (timescale >> 24) & 0xFF, // timescale: 4 bytes    文件媒体在1秒时间内的刻度值，可以理解为1秒长度
            (timescale >> 16) & 0xFF,
            (timescale >> 8) & 0xFF,
            (timescale) & 0xFF,
            (duration >> 24) & 0xFF, // duration: 4 bytes  track的时间长度
            (duration >> 16) & 0xFF,
            (duration >> 8) & 0xFF,
            (duration) & 0xFF,
            0x55, 0xC4, // language: und (undetermined) 媒体语言码。最高位为0，后面15位为3个字符（见ISO 639-2/T标准中定义）
            0x00, 0x00 // pre_defined = 0
        	};
	DEBUG_LOG("sizeof(MDHD) = %d\n",sizeof(MDHD));
	memcpy((unsigned char*)mdhd_item + 8,MDHD,sizeof(MDHD));

	return mdhd_item;
	
}

hdlr_box*	hdlr_box_init(unsigned int handler_type)
{
	unsigned char HandlerType[4] = {0};
	
	unsigned char HDLR[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags
            0x00, 0x00, 0x00, 0x00, // pre_defined
            0x76, 0x69, 0x64, 0x65, // handler_type: 'vide' 在media box中，该值为4个字符     “vide”— video track
            0x00, 0x00, 0x00, 0x00, // reserved: 3 * 4 bytes
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, //保留位
            0x56, 0x69, 0x64, 0x65,
            0x6F, 0x48, 0x61, 0x6E,
            0x64, 0x6C, 0x65, 0x72, 0x00 // name: VideoHandler 长度不定     track type name，以‘\0’结尾的字符串
        	};
			
	unsigned int box_length = sizeof(FullBoxHeader_t)-4 + sizeof(HDLR);//-4代表除去version + flags的长度，因算到数组HDLR中了
	DEBUG_LOG("hdlr_item malloc size(%d) sizeof(HDLR) = %d\n",box_length,sizeof(HDLR));
	hdlr_box* hdlr_item = (hdlr_box*)malloc(box_length);
	if(NULL == hdlr_item)
		return NULL;
	memset(hdlr_item,0,sizeof(box_length));

	hdlr_item->header.size = t_htonl(box_length);
	strncpy(hdlr_item->header.type,"hdlr",4);
	memcpy((unsigned char*)hdlr_item + 8,HDLR,sizeof(HDLR));

	if(handler_type == VIDEO_HANDLER)
	{
		//默认就是 video ,啥都不用做
	}
	else  //音频 hdlr box
	{
		
		unsigned char tmp_handler[4] = {0x73,0x6F,0x75,0x6E}; //字符串：soun  
		memcpy(&hdlr_item->handler_type ,tmp_handler,4);
		unsigned char name[] = {0x53,0x6F,0x75,0x6E,0x64,0x48,0x61,0x6E,0x64,0x6C,0x65,0x72,0x00}; //字符串：SoundHandler
		memcpy(hdlr_item->name,name,sizeof(name));
		//重新修正长度信息，其实是一样长的。
		DEBUG_LOG("hdlr_item->header.size = %d\n",sizeof(hdlr_box) + sizeof(name));
		hdlr_item->header.size = t_htonl(sizeof(hdlr_box) + sizeof(name));
	}
	
	return hdlr_item;
	
	
}

minf_box*	minf_box_init(unsigned int box_length)
{
	DEBUG_LOG("minf_item malloc size(%d)\n",box_length);
	minf_box*minf_item = (minf_box*)malloc(box_length);
	if(NULL == minf_item)
		return NULL;

	memset(minf_item,0,box_length);

	minf_item->header.size = t_htonl(box_length);
	strncpy(minf_item->header.type,"minf",4);
	
	return minf_item;
}

/****五级BOX*********************************************************/

vmhd_box* vmhd_box_init(void)
{
	DEBUG_LOG("vmhd_item malloc size(%d)\n",sizeof(vmhd_box));
	vmhd_box* vmhd_item = (vmhd_box*)malloc(sizeof(vmhd_box));
	if(NULL == vmhd_item)
		return NULL;

	memset(vmhd_item,0,sizeof(vmhd_box));
	
	vmhd_item->header.size = t_htonl(sizeof(vmhd_box));
	strncpy(vmhd_item->header.type,"vmhd",4);

	unsigned char VMHD[] = {
            0x00, 0x00, 0x00, 0x01, // version(0) + flags
            0x00, 0x00, 			// graphicsmode: 2 bytes 视频合成模式，为0时拷贝原始图像，否则与opcolor进行合成
            0x00, 0x00, 0x00, 0x00, // opcolor: 3 * 2 bytes ｛red，green，blue｝
            0x00, 0x00
        	};
	DEBUG_LOG("sizeof(VMHD) = %d\n",sizeof(VMHD));
	memcpy((unsigned char*)vmhd_item + 8,VMHD,sizeof(VMHD));

	return vmhd_item;
	
	
}

smhd_box*	smhd_box_init(void)
{
	DEBUG_LOG("smhd_item malloc size(%d)\n",sizeof(smhd_box));
	smhd_box* smhd_item = (smhd_box*)malloc(sizeof(smhd_box));
	if(NULL == smhd_item)
		return NULL;
	memset(smhd_item,0,sizeof(smhd_box));

	smhd_item->header.size = t_htonl(sizeof(smhd_box));
	strncpy(smhd_item->header.type,"smhd",4);

	unsigned char SMHD[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags   box版本，0或1，一般为0。
            0x00, 0x00, 0x00, 0x00 /* balance(2) + reserved(2) 立体声平衡，[8.8] 格式值，
            							一般为0，-1.0表示全部左声道，1.0表示全部右声道+2位保留位
									*/
        };
	DEBUG_LOG("sizeof(SMHD) = %d\n",sizeof(SMHD));
	memcpy((unsigned char *)smhd_item + 8,SMHD,sizeof(SMHD));

	return smhd_item;
	
}

dinf_box* dinf_box_init(unsigned int box_length)
{
	DEBUG_LOG("dinf_item malloc size(%d)\n",box_length);
	dinf_box* dinf_item = (dinf_box*)malloc(box_length);
	if(NULL == dinf_item)
		return NULL;

	memset(dinf_item,0,box_length);

	dinf_item->header.size = t_htonl(box_length);
	strncpy(dinf_item->header.type,"dinf",4);

	return dinf_item;
	
}

stbl_box*	stbl_box_init(unsigned int box_length)
{
	DEBUG_LOG("stbl_item malloc size(%d)\n",box_length);
	stbl_box* stbl_item = (stbl_box*)malloc(box_length);
	if(NULL == stbl_item)
	{
		ERROR_LOG("malloc failed !\n");
		return NULL;
	}
		

	memset(stbl_item,0,box_length);
	stbl_item->header.size = t_htonl(box_length);
	strncpy(stbl_item->header.type,"stbl",4);

	return stbl_item;
}
	
/****六级BOX*********************************************************/
dref_box* dref_box_init(void)
{

	unsigned char DREF[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags
            0x00, 0x00, 0x00, 0x01, // entry_count 1个url    
            //url   box开始
            0x00, 0x00, 0x00, 0x0C, // entry_size
            0x75, 0x72, 0x6C, 0x20, // type 'url '
            0x00, 0x00, 0x00, 0x01 // version(0) + flags 当“url”或“urn”的box flag为1时，字符串均为空。
        	};
	int box_length = sizeof(FullBoxHeader_t)-4 + sizeof(DREF);
	DEBUG_LOG("dref_item malloc size(%d) sizeof(DREF) = %d\n",box_length,sizeof(DREF));
	dref_box* dref_item = (dref_box*)malloc(box_length);
	if(NULL == dref_item)
		return NULL;
	memset(dref_item,0,box_length);

	dref_item->header.size = t_htonl(box_length);
	strncpy(dref_item->header.type,"dref",4);
	DEBUG_LOG("(unsigned char*)&dref_item->header.version - (unsigned char*)dref_item = %d\n",\
			   (unsigned char*)&dref_item->header.version - (unsigned char*)dref_item);
	memcpy((unsigned char*)dref_item + 8,DREF,sizeof(DREF));

	return dref_item;
	
	
}

void HintSampleEntry(void)
{

}

/*
channelCount: 单声道还是双声道 ,默认：2（双声道） 单声道：1
sampleRate：采样率

*/
stsd_box*  AudioSampleEntry(unsigned char channelCount,unsigned short sampleRate)
{
	/*
		   - mp4a
           - esds
	*/
	const unsigned char mp4a[] = {
			//---前边是header------------------
			0x00, 0x00, 0x00, 0x00, // reserved(4) 6个字节，设置为0；
		   	0x00, 0x00, 	// reserved(2)
		   	0x00, 0x01,  	// data_reference_index(2)
		   	
		   	0x00, 0x00, 0x00, 0x00, // reserved: 2 * 4 bytes 保留位
		   	0x00, 0x00, 0x00, 0x00,
		   	0x00, channelCount, // channelCount(2) 单声道还是双声道
		   	0x00, 0x10, // sampleSize(2):默认 16 （样本位宽）
		   	0x00, 0x00, 0x00, 0x00, // reserved(4) 4字节保留位
		   	(sampleRate >> 8) & 0xFF, // Audio sample rate 显然要右移16位才有意义    template unsigned int(32) samplerate = {timescale of media}<<16;
		   	(sampleRate) & 0xFF,
		   	0x00, 0x00
		};

	
	unsigned int mp4a_length = sizeof(mp4a_box);
	DEBUG_LOG("mp4a_item malloc size(%d) sizeof(mp4a[]) = %d\n",mp4a_length,sizeof(mp4a));
	mp4a_box* mp4a_item = (mp4a_box*)malloc(mp4a_length);
	if(NULL == mp4a_item )
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
		
	
	memset(mp4a_item ,0,mp4a_length);
	mp4a_item->sample_entry.header.size = t_htonl(mp4a_length);
	strncpy(mp4a_item->sample_entry.header.type,"mp4a",4);

	memcpy((unsigned char*)mp4a_item + 8  ,mp4a,sizeof(mp4a));

	unsigned short Audio_object_type = 0;
	if(44100 == sampleRate)
	{
		Audio_object_type = 0x1208; //值详解见下方调用处
	}
	else if(16000 == sampleRate) 
	{
		Audio_object_type = 0x1408;
	}
	else if(8000 == sampleRate)
	{
		Audio_object_type = 0x1588;
	}
	else
	{
		ERROR_LOG("sampleRate not support !\n");
		return NULL;
	}
	DEBUG_LOG("sampleRate (%d)\n",sampleRate);
			
		const unsigned char esds[] = {
		0x00, 0x00, 0x00, 0x00, // version 0 + flags
		
		/***Detailed-Information***/
        0x03, 				// tag1 
        0x80,0x80,0x80,		//可选的扩展描述符类型标记字符串
		0x25,//0x1C,//0x22,				//Tag1 size
		0x00, 0x02,			//ES_ID
		0x00,				//Stream dependence flag+URL_Flag+OCRstreamFlag+streamPriority
             
        /*** Decoder Config Descriptor***/
		0x04,				//Tag2 
		0x80,0x80,0x80,		//可选的扩展描述符类型标记字符串
		0x17,//0x1A,		//Tag size2
		0x40,				//Object type indication  
		/*
		* 当 objectTypeIndication 为0x40时，为MPEG-4 Audio（MPEG-4 Audio generally is thought of as AAC
		* but there is a whole framework of audio codecs that can Go in MPEG-4 Audio including AAC, BSAC, ALS, CELP,
		* and something called MP3On4），如果想更细分format为aac还是mp3，
		* 可以读取 MP4DecSpecificDescr 层 data[0] 的前五位
		* { CODEC_ID_AAC , 0x66 }, /MPEG2 AAC Main/ 
		* { CODEC_ID_AAC , 0x67 }, /MPEG2 AAC Low/  
		* { CODEC_ID_AAC , 0x40 },  
		*/
		0x15,				//streamType+upStream+reserved
		/*----------15的详细解释--------------------------------------------------
             		:0001 01        :streamType  5是Audio Stream, 14496-1 Table6
             		:0       		:upStream
             		:1      		:reserved
         ------------------------------------------------------------------------*/    		
		0x00,0x00,0x00,			//bufferSizeDB ,解码缓冲区大小（640字节0x280）
		(sampleRate >>24)&0xFF, //Max bitrate :44100(0xAC44) 16000(0x3E80) 8000(0x1F40)
		(sampleRate >>16)&0xFF,
		(sampleRate >>8)&0xFF,
		(sampleRate)&0xFF,
		(sampleRate >>24)&0xFF, //Avg bitrate 
		(sampleRate >>16)&0xFF,
		(sampleRate >>8)&0xFF,
		(sampleRate)&0xFF,
		
		/***Audio Decoder Specific Info***/
		0x05,					//Tag3
		0x80,0x80,0x80,			//可选的扩展描述符类型标记字符串
		0x02,//0x08, 			//Tag size3

		/*	0x15,0x88,	(8000HZ) // Audio object type
		  	0x14,0x08,	(16000HZ)			
		 	0x12,0x08,  	(44100HZ)*/
		(Audio_object_type >> 8)&0xFF,
		(Audio_object_type)&0xFF,
		/*----------1408的详细解释----------------------
             :1408(hex) = 0001 0100 0000 1000(bit)
             :0001 0          :audioObjectType 2 GASpecificConfig (观察好几个文件都是这个值，没有找到根源)
             :100 0           :samplingFrequencyIndex 0xb(1011)-->8000  0x8(1000)--> 16000  0x04(0100)-->44100 
             :000 1           :channelConfiguration 1 
             :00         	  :cpConfig
             :0        	  	  :directMapping
		-------------------------------------------------*/
		/***新版本新加的部分***/
		0x06,
		0x80,0x80,0x80,
		0x01,
		0x02 
		};

	unsigned int esds_length = sizeof(esds_box);
	DEBUG_LOG("esds_item malloc size(%d) sizeof(esds[]) = %d\n",esds_length,sizeof(esds));
	esds_box*esds_item = (esds_box*)malloc(esds_length);
	if(NULL == esds_item)
	{
		free(mp4a_item);
		return NULL;
	}
		
	memset(esds_item,0,esds_length);
	esds_item->header.size = t_htonl(esds_length);
	strncpy(esds_item->header.type,"esds",4);
	memcpy((unsigned char*)esds_item + 8,esds,sizeof(esds));

	//构造 stsd box
	//DEBUG_LOG("sizeof(stsd_box) = %d\n",sizeof(stsd_box));
	int stsd_box_size = sizeof(stsd_box) + mp4a_length + esds_length; 
	DEBUG_LOG("stsd_item malloc size(%d)\n",stsd_box_size);
	stsd_box* stsd_item = (stsd_box*)malloc(stsd_box_size);
	if(NULL == stsd_item)
	{
		free(esds_item);
		free(mp4a_item);
		return NULL;
	}
	memset(stsd_item,0,stsd_box_size);

	stsd_item->entry_count = t_htonl(1); //entry的个数 默认赋值成 1
	unsigned int offset = sizeof(stsd_box);//偏移到mp4a box的开始位置
	//拷贝mp4a
	//先修正 MP4a box的长度
	mp4a_item->sample_entry.header.size = t_htonl(mp4a_length +  esds_length);
	memcpy((unsigned char*)stsd_item + offset,mp4a_item,mp4a_length);
	offset += mp4a_length;
	unsigned int tmp = (unsigned char*)stsd_item + sizeof(stsd_box) + mp4a_length + esds_length - (unsigned char*)stsd_item;
	DEBUG_LOG("tmp = %d\n",tmp);
	
	//拷贝 esds
	memcpy((unsigned char*)stsd_item + offset ,esds_item,esds_length);

	
	//修正 stsd box长度
	stsd_item->header.size = t_htonl(stsd_box_size);
	strncpy(stsd_item->header.type,"stsd",4);

	

	
	free(esds_item);
	free(mp4a_item);
		

	return stsd_item; 
	
		
}

stsd_box* VideoSampleEntry(unsigned short width,unsigned short height)
{
	unsigned char AVC1[] = {
			//-----BoxHeader 没填------------------------
			//-----SampleEntry_t的数据-------------------
            0x00, 0x00, 0x00, 0x00, // reserved(4)    6个 保留位 Reserved：6个字节，设置为0；
            0x00, 0x00, 0x00, 0x01, // reserved(2) + data_reference_index(2)  数据引用索引
         	//-----VideoSampleEntry_t的数据--------------
            0x00, 0x00, 0x00, 0x00, // pre_defined(2) + reserved(2)
            0x00, 0x00, 0x00, 0x00, // pre_defined: 3 * 4 bytes  3*4个字节的保留位
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            (width >> 8) & 0xFF, // width: 2 bytes
            (width) & 0xFF,
            (height >> 8) & 0xFF, // height: 2 bytes
            (height) & 0xFF,
            0x00, 0x48, 0x00, 0x00, // horizresolution: 4 bytes 常数
            0x00, 0x48, 0x00, 0x00, // vertresolution: 4 bytes 常数
            0x00, 0x00, 0x00, 0x00, // reserved: 4 bytes 保留位
            0x00, 0x01, // frame_count ，此处字节无法对齐的！！！！     
            //frame_count表明多少帧压缩视频存储在每个样本。默认是1,每样一帧;它可能超过1每个样本的多个帧数
            0x04, //    strlen compressorname: 32 bytes        
            //32个8 bit    第一个8bit表示长度,剩下31个8bit表示内容
            0x00, 0x00, 0x00, 0x00,//0x67, 0x31, 0x31, 0x31, // compressorname: 32 bytes    翻译过来是g111
            0x00, 0x00, 0x00, 0x00, //
            0x00, 0x00, 0x00, 0x00, //
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00,
            0x00, 0x18, // depth 颜色深度
            0xFF, 0xFF // pre_defined = -1
        };

	/***avcC box 的构造(AVCDecoderConfigurationRecord)*******************************/
	avcc_box_info_t *avcc_item  = &avcc_box_info;
	if(NULL == avcc_item->avcc_buf)
	{
		ERROR_LOG("avcc_box_info not init !\n");
		return NULL;
	}
	if(avcc_item->buf_length != t_ntohl(avcc_item->avcc_buf->header.size))
	{
		free(avcc_item->avcc_buf);
		ERROR_LOG("avcc_item buf err! avcc_item->buf_length(%d) t_ntohl(avcc_item->avcc_buf->header.size)(%d)\n",\
					avcc_item->buf_length,t_ntohl(avcc_item->avcc_buf->header.size));
		return NULL;
	}
	print_char_array("avcc",(unsigned char*)avcc_item->avcc_buf,avcc_item->buf_length);
	
	unsigned int box_length = 0;//申请的内存长度
	unsigned int write_len = 0;//实际写入的长度，用于安全校验
	//---avc1----------------------------------------------
	DEBUG_LOG("sizeof(avc1_box) = %d t_ntohl(avcc_item->avcc_buf->header.size)= %d\n",sizeof(avc1_box),t_ntohl(avcc_item->avcc_buf->header.size));
	box_length = sizeof(avc1_box)-2 + t_ntohl(avcc_item->avcc_buf->header.size);//pasp box暂未实现	//-2 为了配合解析器，结构体对齐时会补全2字节
	avc1_box*avc1_item = (avc1_box*)malloc(box_length);
	if(NULL == avc1_item)
	{
		ERROR_LOG("malloc failed !\n");
		free(avcc_item->avcc_buf);
		return NULL;
	}
	memset(avc1_item,0,box_length);
	
	avc1_item->sample_entry.header.size = t_htonl(box_length);
	DEBUG_LOG("avc1_item->sample_entry.header.size  = %x  sizeof(avc1_box) = %d\n",avc1_item->sample_entry.header.size,sizeof(avc1_box));
	strncpy(avc1_item->sample_entry.header.type,"avc1",4);
	//拷贝AVC1数组
	DEBUG_LOG("sizeof(avc1_item->sample_entry) = %d",sizeof(avc1_item->sample_entry.header));
	memcpy((unsigned char*)avc1_item + sizeof(avc1_item->sample_entry.header),AVC1,sizeof(AVC1));

	//拷贝 avcc box
	//memcpy((unsigned char*)avc1_item + sizeof(avc1_box),avcc_item->avcc_buf,avcc_item->buf_length);
	memcpy((unsigned char*)avc1_item + sizeof(avc1_box)-2,avcc_item->avcc_buf,avcc_item->buf_length);//-2 为了配合解析器

	write_len = sizeof(avc1_box)-2 + avcc_item->buf_length;//-2 为了配合解析器
	if(box_length != write_len)
	{
		ERROR_LOG("avc1_item malloc size(%d) but write sizeof(%d)\n",box_length,write_len);
		free(avcc_item->avcc_buf);
		free(avc1_item);
		return NULL;
	}
	
	print_char_array("avc1",(unsigned char*)avc1_item,box_length);


	//---stsd----------------------------------------------
	box_length += sizeof(stsd_box);
	DEBUG_LOG("stsd_item malloc size(%d)\n",box_length);				 
	stsd_box* stsd_item  = (stsd_box*)malloc(box_length);
	if(NULL == stsd_item)
	{
		free(avcc_item->avcc_buf);
		free(avc1_item);
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
		
	memset(stsd_item,0,box_length);

	int offset = 0;
	//填充stsd 头部 
	stsd_item->header.size = t_htonl(box_length);
	strncpy(stsd_item->header.type,"stsd",4);
	
	//还有额外的 4字节 entry_count 变量  ，，不要漏了
	stsd_item->entry_count = t_htonl(1); //entry的个数 默认赋值成 1
	
	

	//偏移到avc1的位置
	offset = sizeof(stsd_box);
	DEBUG_LOG("sizeof(stsd_box) = %d offset = %d\n",sizeof(stsd_box),offset);
	//填充AVC1 box (已经包含了avcc box)
	memcpy((unsigned char*)stsd_item + offset,avc1_item,t_ntohl(avc1_item->sample_entry.header.size));
	write_len = offset + t_ntohl(avc1_item->sample_entry.header.size);
	if(box_length != write_len)
	{	free(avcc_item->avcc_buf);
		free(avc1_item);
		free(stsd_item);
	
		ERROR_LOG("stsd_item malloc size(%d) but write sizeof(%d)\n",box_length,write_len);
		return NULL;
	}
	//DEBUG***************************************
	print_char_array("stsd",(unsigned char*)stsd_item,box_length);

	//********************************************
		

	//释放已经无用的空间

	free(avcc_item->avcc_buf);
	avcc_item->avcc_buf = NULL;
	avcc_item->buf_length = 0;
	free(avc1_item);
	avc1_item = NULL;
	
	return stsd_item;
		
}


//参数handler_type对应 hdlr box 中的handler_type
stsd_box* stsd_box_init(unsigned int handler_type,
							 unsigned short width,unsigned short height, // for video tracks
							 unsigned char channelCount,unsigned short sampleRate // for audio tracks
							)
{
		 
	stsd_box* stsd_item = NULL;
	
	switch (handler_type) //对应 hdlr box 中的handler_type
	{ 
	 case AUDIO: // for audio tracks
	   stsd_item = AudioSampleEntry(channelCount,sampleRate);
	    break;
	 case VIDEO: // for video tracks
	    stsd_item = VideoSampleEntry(width,height);
	    break;
	 case HINT: // Hint track
	   // stsd_item = HintSampleEntry();
		break; 
	}
	
	if(NULL == stsd_item)
		return NULL;
	
	return stsd_item;


}
							
stts_box*	stts_box_init(void)
{
	unsigned char STTS[] ={
            0x00, 0x00, 0x00, 0x00, // version(0) + flags
            0x00, 0x00, 0x00, 0x00 // entry_count     0个索引
        	};
	unsigned int box_length = sizeof(STTS)+sizeof(FullBoxHeader_t)-4;//-4:减去 version(0) + flags
	DEBUG_LOG("stts_item malloc size(%d) sizeof(STTS) = %d\n",box_length,sizeof(STTS));
	stts_box* stts_item = (stts_box*)malloc(box_length);
	if(NULL == stts_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
	memset(stts_item,0,box_length);
	stts_item->header.size = t_htonl(box_length);
	strncpy(stts_item->header.type,"stts",4);

	memcpy(&stts_item->header.version,STTS,sizeof(STTS));

	return stts_item;
}


//和stts box的初始化一样的
stsc_box*	stsc_box_init(void)
{
	
	stsc_box *stsc_item = (stsc_box *)stts_box_init();
	if(NULL == stsc_item)
	{
		ERROR_LOG("stsc box init failed!\n");
		return NULL;
	}
	strncpy(stsc_item->header.type,"stsc",4);
	return stsc_item;
}

stsz_box* stsz_box_init(void)
{

	unsigned char STSZ[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags
            0x00, 0x00, 0x00, 0x00, // sample_size
            0x00, 0x00, 0x00, 0x00 // sample_count
        	};
			
	unsigned int box_length = sizeof(STSZ)+sizeof(FullBoxHeader_t)-4;//-4:减去 version(0) + flags
	DEBUG_LOG("stsz_item malloc size(%d) sizeof(STSZ) = %d\n",box_length,sizeof(STSZ));
	stsz_box* stsz_item = (stsz_box*)malloc(box_length);
	if(NULL == stsz_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
	memset(stsz_item,0,box_length);
	stsz_item->header.size = t_htonl(box_length);
	strncpy(stsz_item->header.type,"stsz",4);

	memcpy(&stsz_item->header.version,STSZ,sizeof(STSZ));

	return stsz_item;

}

//和stts box的初始化一样的
stco_box*	stco_box_init(void)
{
	stco_box *stco_item = (stco_box *) stts_box_init();
	if(NULL == stco_item)
	{
		ERROR_LOG("stsc box init failed!\n");
		return NULL;
	}
	strncpy(stco_item->header.type,"stco",4);
	return stco_item;
}
	
/****七级BOX*********************************************************/

int url_box_init(void)
{
	//dref里边的初始化数组直接包含了url，如有问题，后期修改
	return 0; //不需要
}
int	urn_box_init(void)
{
	return 0;//不需要
}
avc1_box* avc1_box_init(void)
{
		//已经在VideoSampleEntry里边实现
		//包括 avcc box 
}

mp4a_box* mp4a_box_init(void)
{
		//已经在AudioSampleEntry里边实现
		//包括 esds box
}

/*
	判断nalu 包的类型
	返回：
		NALU_SPS  0
		NALU_PPS  1
		NALU_I    2
		NALU_P    3
		NALU_SET  4
		
*/
#if 1

int FrameType(unsigned char* naluData)
{
	int index = -1; 
     
    if(naluData[0]==0 && naluData[1]==0 && naluData[2]==0 && naluData[3]==1 && naluData[4]==0x67)
    {
        index = NALU_SPS;
        printf("---NALU_SPS---\n");
    }
	else if(naluData[0]==0 && naluData[1]==0 && naluData[2]==0 && naluData[3]==1 && naluData[4]==0x68)
    {
        index = NALU_PPS;
        printf("---NALU_PPS---\n");
    }
    else if(naluData[0]==0 && naluData[1]==0 && naluData[2]==0 && naluData[3]==1 && naluData[4]==0x65)
    {
        index = NALU_I;
        printf("---NALU_I---\n");
    }
    else if(naluData[0]==0 && naluData[1]==0 && naluData[2]==0 && naluData[3]==1 && naluData[4]==0x61)
    {
        index = NALU_P;
       //printf("---NALU_P---\n");
    }
    else if(naluData[0]==0 && naluData[1]==0 && naluData[2]==0 && naluData[3]==1 && naluData[4]==0x6)
    {
        index = NALU_SET;
       printf("---NALU_SET---\n");
    }
	else
	{
		printf("---NALU_unknow!!---\n");
	}

	return index;
}

#endif

/*
	 该函数只能用来接收 SPS PPS SEI 类型的 NALU
	 并且该nalu需要用4字节的数据大小头替换原有的起始头，并且数据大小为big-endian格式
*/
/************************************************************************************************************
**									avvC Box  海思的h264sps套不上

configurationVersion			1		0x4D	版本;
AVCProfileIndication			1		0x00	10进制的77 对照profile的情况 知道是main profile;
profile_compatibility			1		0x2A	一般0
AVCLevelIndication				1		0x96	10进制的31
lengthSizeMinusOne				1		0x35	1111 1111	前面6位为reserved，后面2位（0b11）为：lengthSizeMinusOne，表示3.
												那么用来表示size的字节就有3 + 1 = 4个

numOfSequenceParameterSets		1		0xC0	1110 0001	前面3位是reserved, 后面5bit是numOfSequenceParameterSets，表示有1个sps.
sequenceParameterSetLength		2			
SPS 							23				SPS的长度就只包括该部分数据长度

numOfPictureParameterSets		1			有1个pps
pictureParameterSetLength		2			pictureParameterSetLength
PPS 							4			PPS的内容

************************************************************************************************************/
//手动输入的NALU头信息
unsigned char IDR_NALU[] = {
0x00,0x00,0x00,0x01, 0x67,0x4D,0x00,0x2A, 0x96,0x35,0xC0,0xF0, 0x04,0x4F,0xCB,0x37,
0x01,0x01,0x01,0x02, 0x00,0x00,0x00,0x01, 0x68,0xEE,0x3C,0x80, 0x00,0x00,0x00,0x01,
0x06,0xE5,0x01,0x2E, 0x80/*后边省略 I帧部分*/
};

//以下数组都不包含起始头部分 
//SPS:0x00,0x00,0x00,0x01, 0x67
//PPS:0x00,0x00,0x00,0x01, 0x68
//SEI:0x00,0x00,0x00,0x01, 0x06
unsigned char my_SPS[] = {0x67,0x4D,0x00,0x2A, 0x96,0x35,0xC0,0xF0, 0x04,0x4F,0xCB,0x37,0x01,0x01,0x01,0x02};

unsigned char my_PPS[] = {0x68,0xEE,0x3C,0x80};
unsigned char my_SEI[] = {0xE5,0x01,0x2E, 0x80};


#if 1
avcc_box_info_t *	avcc_box_init(void)
{


	//计算 PPS的数据长度
	DEBUG_LOG("start avcc_box_init..\n");
	unsigned char * SPS_data_start_POS = my_SPS;		//记录 sps数据的起始位置
	unsigned char * SPS_data_end_POS = my_SPS + sizeof(my_SPS)/sizeof(my_SPS[0]);	//记录 sps数据的结束位置
	unsigned int SPS_len = sizeof(my_SPS);	//	记录SPS的实际长度
	DEBUG_LOG("SPS_len =  %d\n",SPS_len);

	unsigned char * PPS_data_start_POS = my_PPS;
	unsigned char * PPS_data_end_POS = my_PPS + sizeof(my_PPS)/sizeof(my_PPS[0]);
	unsigned int PPS_len = sizeof(my_PPS);
	DEBUG_LOG("PPS_len =  %d\n",PPS_len);

	


	#if 1 //和 海思的PPS数据头内容套不上
	unsigned int offset = 5;//略过前5个字节 略过SPS的识别头（0x00 00 00 01 67），5字节
	#if 1
		unsigned char configurationVersion =  0x1;	//naluData[offset+0];  // configurationVersion
		unsigned char AVCProfileIndication =  my_SPS[1];	//naluData[offset+1];  // AVCProfileIndication
		unsigned char profile_compatibility = my_SPS[2];	//naluData[offset+2];  // profile_compatibility
		unsigned char AVCLevelIndication  =   my_SPS[3];	//naluData[offset+3];  // AVCLevelIndication
	#else
		unsigned char configurationVersion =  0x01;	//naluData[offset+0];  // configurationVersion
		unsigned char AVCProfileIndication =  0x4D;	//naluData[offset+1];  // AVCProfileIndication
		unsigned char profile_compatibility = 0x00;	//naluData[offset+2];  // profile_compatibility
		unsigned char AVCLevelIndication  =   0x2A;	//naluData[offset+3];  // AVCLevelIndication
	#endif
		
	unsigned char reserved6 = 0xfc; //二进制：1111 1100
	unsigned char _naluLengthSize = (my_SPS[4] & 3) + 1;  // lengthSizeMinusOne   上一个avc 长度
	unsigned char reserved_6_lengthSizeMinusOne_2 = reserved6|_naluLengthSize;//reserved_6_lengthSizeMinusOne_2

	unsigned char reserved3 = 0xe0;//二进制：1110 0000
	unsigned char spsCount = 1;
	unsigned char reserved_3_numOfSequenceParameterSets_5 = reserved3|spsCount;  
	DEBUG_LOG("start avcc_box_init..02\n");
	
	if (spsCount == 0 || spsCount > 1) 
	{
		ERROR_LOG("Invalid H264 SPS count(%d)\n",spsCount);
        return NULL;
    }
	
		//sps内容:my_SPS数组
		//一般就一个sps
		// Notice: Nalu without startcode header (00 00 00 01)
		unsigned short sps_len = SPS_len;	//(unsigned short)naluData[offset];
		unsigned short sequenceParameterSetLength = t_htons(sps_len);  // sequenceParameterSetLength

		
		//PPS内容:my_PPS数组
		//一般也就1个pps
		unsigned char ppsCount = 1;			  // numOfPictureParameterSets
		unsigned char numOfPictureParameterSets = ppsCount;
		unsigned short pictureParameterSetLength = t_htons(PPS_len); //pictureParameterSetLength
 
		

		unsigned int box_len =	sizeof(avcc_box)-1 + PPS_len + SPS_len;//-1用来修正字节对齐的填充字节，不算在box内
		DEBUG_LOG("avcc_item malloc size(%d)\n",box_len);
		avcc_box* avcc_item = (avcc_box*)malloc(box_len);
		if(NULL == avcc_item)
		{
			ERROR_LOG("malloc failed !\n");
			return NULL;
		}
		memset(avcc_item,0,box_len);

		avcc_item->header.size = t_htonl(box_len);
		strncpy(avcc_item->header.type,"avcC",4);
		avcc_item->configurationVersion = configurationVersion;
		avcc_item->AVCProfileIndication = AVCProfileIndication;
		avcc_item->profile_compatibility = profile_compatibility;
		avcc_item->AVCLevelIndication = AVCLevelIndication;
		avcc_item->reserved_6_lengthSizeMinusOne_2 = reserved_6_lengthSizeMinusOne_2;
		
		//SPS
		avcc_item->reserved_3_numOfSequenceParameterSets_5 = reserved_3_numOfSequenceParameterSets_5;	
		memcpy(&avcc_item->sequenceParameterSetLength,&sequenceParameterSetLength,sizeof(sequenceParameterSetLength));
		memcpy((unsigned char*)&avcc_item->sequenceParameterSetNALUnit,my_SPS,SPS_len);
	
		//PPS 注意：avcc_box 数据结构里边并没有包括sps的数据的空间，所以填充PPS时要注意加上sps数据的偏移量
		//错误赋值方式：avcc_item->numOfPictureParameterSets = ppsCount;
		memcpy((unsigned char*)avcc_item->sequenceParameterSetNALUnit + SPS_len,&ppsCount,sizeof(ppsCount));		//numOfPictureParameterSets
		memcpy((unsigned char*)avcc_item->sequenceParameterSetNALUnit + SPS_len + 1,&pictureParameterSetLength,sizeof(pictureParameterSetLength)); //pictureParameterSetLength
		memcpy((unsigned char*)avcc_item->sequenceParameterSetNALUnit + SPS_len + 1 + 2,my_PPS,PPS_len);//PPS数据
	
		#endif
		DEBUG_LOG("start avcc_box_init..06\n");
		avcc_box_info.avcc_buf = avcc_item;
		avcc_box_info.buf_length = box_len;
	return &avcc_box_info;
	
}

#else

avcc_box_info_t *	avcc_box_init(unsigned char* naluData, int naluSize)
{

	if(NULL == naluData)
	{
		ERROR_LOG("Illegal parameter!\n");
		return NULL;
	}

	//计算 PPS的数据长度
	unsigned char * SPS_data_start_POS = NULL;//记录 sps数据的起始位置
	unsigned char * SPS_data_end_POS = NULL;//记录 sps数据的结束位置
	unsigned int SPS_len = 0;	//	记录SPS的实际长度

	unsigned char * PPS_data_start_POS = NULL;
	unsigned char * PPS_data_end_POS = NULL;
	unsigned int PPS_len = 0;
	
	int i = 0;
	for(i = 0;i < naluSize;i += sizeof(int))
	{
		int type = NaluType(naluData + i);

		if(NALU_SPS == type)//SPS NALU
		{
			SPS_data_start_POS = naluData + i;
		}
		else if(NALU_PPS == type)//PPS NALU
		{
			PPS_data_start_POS = naluData + i;
			SPS_data_end_POS = 	 naluData + i;
		}
		else if(NALU_SET == type)//SET NALU
		{
			PPS_data_end_POS = naluData + i;
		}
		else
		{
			// do noting
		}

		//找到 SPS PPS的起始结束位置 并 计算出长度，才能退出
		if(NULL != SPS_data_start_POS && NULL != SPS_data_end_POS &&\
			NULL != PPS_data_end_POS && NULL != PPS_data_start_POS)
		{
			
			SPS_len = SPS_data_end_POS - SPS_data_start_POS;
			PPS_len = PPS_data_end_POS - PPS_data_start_POS;
			DEBUG_LOG("SPS PPS length calculate success!  SPS_len(%d) PPS_len(%d)\n ",SPS_len,PPS_len);
			break;
		}
		
		
	}

	if(i >= naluSize)//没有成功计算出 PPS的实际数据长度
	{
		ERROR_LOG("calculate PPS length failed!\n");
		return NULL;
	}

	


	#if 1 //和 海思的PPS数据头内容套不上
	unsigned int offset = 5;//略过前5个字节 略过SPS的识别头（0x00 00 00 01 67），5字节
	unsigned char configurationVersion =  1;	//naluData[offset+0];  // configurationVersion
	unsigned char AVCProfileIndication =  0x4D;	//naluData[offset+1];  // AVCProfileIndication
	unsigned char profile_compatibility = 0;	//naluData[offset+2];  // profile_compatibility
	unsigned char AVCLevelIndication  =   0x1F;	//naluData[offset+3];  // AVCLevelIndication

		
	unsigned char reserved6 = 0xfc; //二进制：1111 1100
	unsigned char _naluLengthSize = (naluData[offset+4] & 3) + 1;  // lengthSizeMinusOne   上一个avc 长度
	unsigned char reserved_6_lengthSizeMinusOne_2 = reserved6|_naluLengthSize;//reserved_6_lengthSizeMinusOne_2

	unsigned char reserved3 = 0xe0;//二进制：1110 0000
	//unsigned char spsCount = naluData[offset+5] & 31; // numOfSequenceParameterSets  有问题,和海思的不匹配
	unsigned char spsCount = 1;
	unsigned char reserved_3_numOfSequenceParameterSets_5 = reserved3|spsCount;  
	
	offset = offset + 6;
	if (spsCount == 0 || spsCount > 1) 
	{
		ERROR_LOG("Invalid H264 SPS count(%d)\n",spsCount);
        return NULL;
    }

	
	//一般就一个sps
		unsigned short sps_len = SPS_len;	//(unsigned short)naluData[offset];
		unsigned short sequenceParameterSetLength = t_htons(sps_len);  // sequenceParameterSetLength
        offset += 2;

        // Notice: Nalu without startcode header (00 00 00 01)
     
		//sps内容
		if(sps_len <= 0 )
		{
			ERROR_LOG("sps_len error!\n");
			return NULL;
		}
		unsigned char *sps = (unsigned char*)malloc(sps_len);
		if(NULL == sps)
		{
			ERROR_LOG("malloc failed!\n");
			return NULL;
		}
		memset(sps,0,sps_len);
		memcpy(sps,SPS_data_start_POS,sps_len);
		//memcpy(sps,naluData + offset,sps_len);
		offset += sps_len;

		
	/*中间这部分解析 sps 序列的部分先省略
        let config = SPSParser.parseSPS(sps);
        meta.codecWidth = config.codec_size.width;
        meta.codecHeight = config.codec_size.height;
        meta.presentWidth = config.present_size.width;
        meta.presentHeight = config.present_size.height;

        meta.profile = config.profile_string;
        meta.level = config.level_string;
        meta.bitDepth = config.bit_depth;
        meta.chromaFormat = config.chroma_format;
        meta.sarRatio = config.sar_ratio;
        meta.frameRate = config.frame_rate;

        if (config.frame_rate.fixed === false ||
            config.frame_rate.fps_num === 0 ||
            config.frame_rate.fps_den === 0) {
            meta.frameRate = this._referenceFrameRate;
        }

        let fps_den = meta.frameRate.fps_den;
        let fps_num = meta.frameRate.fps_num;
        meta.refSampleDuration = Math.floor(meta.timescale * (fps_den / fps_num));

        let codecArray = sps.subarray(1, 4);
        let codecString = 'avc1.';
        for (let j = 0; j < 3; j++) {
            let h = codecArray[j].toString(16);
            if (h.length < 2) {
                h = '0' + h;
            }
            codecString += h;
        }
        meta.codec = codecString;

        let mi = this._mediaInfo;
        mi.width = meta.codecWidth;
        mi.height = meta.codecHeight;
        mi.fps = meta.frameRate.fps;
        mi.profile = meta.profile;
        mi.level = meta.level;
        mi.chromaFormat = config.chroma_format_string;
        mi.sarNum = meta.sarRatio.width;
        mi.sarDen = meta.sarRatio.height;
        mi.videoCodec = codecString;

        if (mi.hasAudio) {
            if (mi.audioCodec != null) {
                mi.mimeType = 'video/x-flv; codecs="' + mi.videoCodec + ',' + mi.audioCodec + '"';
            }
        } else {
            mi.mimeType = 'video/x-flv; codecs="' + mi.videoCodec + '"';
        }
        if (mi.isComplete()) {
            this._onMediaInfo(mi);
        }
       */
		
		//填充 PPS
		unsigned char ppsCount = 1;	//naluData[offset];  // numOfPictureParameterSets
		unsigned char numOfPictureParameterSets = ppsCount;
        if (ppsCount == 0 || ppsCount > 1) 
		{
			ERROR_LOG("Invalid H264 PPS count(%d)\n",ppsCount);
            return NULL;
        }

        offset++;

		//一般也就1个pps
        unsigned int pps_len = PPS_len; //(unsigned short)naluData[offset]; 
		unsigned short pictureParameterSetLength = t_htons(pps_len); // pictureParameterSetLength
        offset += 2;

		if(pps_len <= 0)
		{
			ERROR_LOG("pps_len error!\n");
			free(sps);
			return NULL;
		}
		unsigned char* pps = (unsigned char*)malloc(pps_len);
		if(NULL == pps)
		{
			ERROR_LOG("malloc failed !\n");
			free(sps);
			return NULL;
		}
		memset(pps,0,pps_len);
		memcpy(pps,PPS_data_start_POS,pps_len);
		
		//memcpy(pps,naluData + offset,pps_len);
	
        // pps is useless for extracting video information
        //offset += pps_len;

		unsigned int box_len =	sizeof(avcc_box) + PPS_len + SPS_len;//sizeof(BoxHeader_t) + offset - 5;
		avcc_box* avcc_item = (avcc_box*)malloc(box_len);
		if(NULL == avcc_item)
		{
			ERROR_LOG("malloc failed !\n");
			free(sps);
			free(pps);
			return NULL;
		}
		memset(avcc_item,0,box_len);

		avcc_item->header.size = t_htonl(box_len);
		strncpy(avcc_item->header.type,"Ccva",4);
		memcpy(&avcc_item->configurationVersion,&configurationVersion,sizeof(configurationVersion));
		memcpy(&avcc_item->AVCProfileIndication,&AVCProfileIndication,sizeof(AVCProfileIndication));
		memcpy(&avcc_item->profile_compatibility,&profile_compatibility,sizeof(profile_compatibility));
		memcpy(&avcc_item->AVCLevelIndication,&AVCLevelIndication,sizeof(AVCLevelIndication));
		memcpy(&avcc_item->reserved_6_lengthSizeMinusOne_2,&reserved_6_lengthSizeMinusOne_2,sizeof(reserved_6_lengthSizeMinusOne_2));
		memcpy(&avcc_item->reserved_3_numOfSequenceParameterSets_5,&reserved_3_numOfSequenceParameterSets_5,sizeof(reserved_3_numOfSequenceParameterSets_5));
		memcpy(&avcc_item->sequenceParameterSetLength,&sequenceParameterSetLength,sizeof(sequenceParameterSetLength));
		memcpy(avcc_item->sequenceParameterSetNALUnit,sps,sps_len);
		memcpy(&avcc_item->numOfPictureParameterSets,&ppsCount,sizeof(ppsCount));
		free(sps);
		
		memcpy(&avcc_item->pictureParameterSetLength,&pps_len,sizeof(pps_len));
		memcpy(avcc_item->pictureParameterSetNALUnit,pps,pps_len);
		free(pps);
		#endif
		
		avcc_box_info.avcc_buf = avcc_item;
		avcc_box_info.buf_length = box_len;
	return &avcc_box_info;
	
}

#endif



/***一般mp4文件部分********************************************************************************
专门针对普通mp4文件部分
*********************************************************************************************************/























