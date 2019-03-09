
#ifndef _BOX_H
#define _BOX_H
#pragma pack(4)
#ifdef __cplusplus
extern "C"
{
#endif

#define VIDEO_TRACK 1   //视频轨道
#define AUDIO_TRACK 2   //音频轨道
#define HAVE_VIDEO 1    //视频轨道总控开关
#define HAVE_AUDIO 1	//音频轨道总控开关


#define 	NALU_SPS  0
#define		NALU_PPS  1
#define		NALU_I    2
#define		NALU_P    3
#define		NALU_SET  4

//#define ONE_SECOND_DURATION (12800)  //1秒时间分割数
#define VIDEO_TIME_SCALE (90000)   //视频的内部时间戳（1s的分割数）
#define AUDIO_TIME_SCALE (16000)   //音频的内部时间戳（1s的分割数）,填入采样率，用样本数来表述


#define VIDEO_ONE_MSC_LEN (VIDEO_TIME_SCALE/1000)    //视频实际每毫秒对应内部时间长度 
#define AUDIO_ONE_MSC_LEN (AUDIO_TIME_SCALE/1000)	//音频实际每毫秒对应内部时间长度

//#define AUDIO_SOURCE_SAMPLE_RATE (44100) //音频元数据的样本率 ,该参数已经对外层开放
#define AUDIO_FREAME_SAMPLES 	(1024) //一帧 AAC audio 帧的采样点数

//#define ONE_AAC_FRAME_DURATION  (AUDIO_TIME_SCALE * AUDIO_FREAME_SAMPLES/AUDIO_SOURCE_SAMPLE_RATE) //((AUDIO_FREAME_SAMPLES/AUDIO_SOURCE_SAMPLE_RATE)* AUDIO_TIME_SCALE) 1帧AAC音频帧播放时所占内部时长 


typedef unsigned int Fmp4TrackId; //轨道ID

#define VIDEO 1
#define AUDIO 2
#define HINT  3 //一般不用 



typedef struct _BoxHeader_t
{
	unsigned int size;	//BOX size
	unsigned char type[4];	//BOX type
}BoxHeader_t;

typedef struct _FullBoxHeader_t
{
	unsigned int size;	//BOX size
	unsigned char type[4];	//BOX type
	char     version;	//BOX 版本，0或1，一般为0 ，该套代码只支持 0
	char 	flags[3];	//保留位
	
}FullBoxHeader_t;


/***一级BOX***************************************/
typedef struct FileTypeBox_t
{
	BoxHeader_t 	  header;
	unsigned int  major_brand;		//4字节的品牌名称 默认值：iso6
   	unsigned int  minor_version;	//4字节的版本号
   //	unsigned int compatible_brands[0];//内容4字节的兼容品牌数组
}ftyp_box;

typedef struct MovieBox_t
{
	BoxHeader_t 	  header;
	//char 		child_box[0]; //0长数组，占位
}moov_box;

typedef struct MovieFragmentBox_t
{
	BoxHeader_t 	  header;
	//char 		child_box[0]; //0长数组，占位
}moof_box;

typedef struct MediaDataBox_t
{
	BoxHeader_t 	  header;
	char 			  data[0];	//真实数据
}mdat_box;


typedef struct MovieFragmentRandomAccessBox_t
{
	BoxHeader_t 	  header;
	char 			  data[0];	//真实数据
}mfra_box;


/***二级BOX***************************************/

typedef struct MovieHeaderBox_t
{
	FullBoxHeader_t header;
	unsigned int  	creation_time;  	 	//媒体创建时间 相对于UTC时间1904-01-01零点的秒数）
	unsigned int  	modification_time; 		//修改时间
	unsigned int  	timescale;				//刻度值,可以理解为1秒的长度
	unsigned int  	duration;				/* duration: 4 bytes  该track的时间长度，用duration和time scale值可以计算track时长，
											比如audio track的time scale = 8000, duration = 560128，时长为duration/timescale=70.016，
											video track的time scale = 600, duration = 42000，时长为duration/timescale=70
											*/
											
	int  			rate; 					//typically 1.0  //播放速率,高16和低16分别表示小数点前后整数部分和小数部分
	short   		volume;   			//typically, full volume  //声音,高8位和低8位分别表示小数点前后整数和小数部分
	short  			reserved_2;  		//2字节保留位
	unsigned int  	reserved_8[2]; 		//4+4字节保留位
	int 			matrix[9];          	// Unity matrix视频变换矩阵
	int 			pre_defined [6];  		//6*4保留位
	unsigned int  	next_track_ID; 			//4字节的下一个track id

}mvhd_box;

typedef struct  TrackBox_t
{
	BoxHeader_t header;
	//char 		child_box[0]; //0长数组，占位
	
}trak_box;


/*
	mvex 是 fMP4 的标准盒子。它的作用是告诉解码器这是一个 fMP4 的文件，
	具体的 samples 信息内容不再放到 trak 里面，而是在每一个 moof 中
*/
typedef struct MovieExtendsBox_t
{
	BoxHeader_t header;
	char 			child_box[0]; //0长数组，占位
}mvex_box;

typedef struct MovieFragmentHeaderBox_t
{
	FullBoxHeader_t header;
	unsigned int  sequence_number;//the ordinal number of this fragment, in increasing order
}mfhd_box;

typedef struct TrackFragmentBox_t
{
	BoxHeader_t 	header;
	//char 			child_box[0]; //0长数组，占位
}traf_box;
	
typedef struct TrackFragmentRandomAccessBox_t
{
	FullBoxHeader_t 	header;
	unsigned int 		track_ID;
	//const unsigned char reserved[26];
	const unsigned char reserved[1];
	unsigned char 		length_size_of_traf_num[1];
	unsigned char 		length_size_of_trun_num[1];
	unsigned char 		length_size_of_sample_num[1];
	unsigned int 		number_of_entry;  /*number_of_entry是一个整数，它给出了入口的条目数（moof数）。如果这个值是零，它
											指示每个样本都是同步样本，后面没有表项。
										   */
	
	//for(i=1; i <= number_of_entry; i++){
	/*放在后边，单独处理
	unsigned int 		time;
	unsigned int 		moof_offset;
	unsigned int((length_size_of_traf_num+1) * 8) traf_number;
	unsigned int((length_size_of_trun_num+1) * 8) trun_number;
	unsigned int((length_size_of_sample_num+1) * 8) sample_number;
	*/
	//}
}tfra_box;

typedef struct _tfra_entry_info_t
{
	//采用flag = 0的模式
	unsigned int 		time;
	unsigned int 		moof_offset;
	//unsigned int((length_size_of_traf_num+1) * 8) traf_number;
	unsigned char traf_number;
	//unsigned int((length_size_of_trun_num+1) * 8) trun_number;
	unsigned char trun_number;
	//unsigned int((length_size_of_sample_num+1) * 8) sample_number;
	unsigned char sample_number;
	unsigned char please_delete;	// + 1字节（字节对齐）实际填充时该字节需要减去
}tfra_entry_info_t;

typedef struct MovieFragmentRandomAccessOffsetBox_t
{
	FullBoxHeader_t 	header;
	unsigned int 		size;
	/*size是一个整数，给出了包含“mfra”框的字节数。
		这个字段放在最后，以协助读者从档案结尾搜寻“mfra”框。
	*/
}mfro_box;



/***三级BOX***************************************/
typedef struct TrackHeaderBox_t
{
	FullBoxHeader_t 	header;
	unsigned int  		creation_time; 		//创建时间（相对于UTC时间1904-01-01零点的秒数）
	unsigned int  		modification_time;	//修改时间
	unsigned int  		track_ID;			//id号，不能重复且不能为0
	unsigned int  		reserved;			//4字节保留位
	unsigned int  		duration;			//时长

	unsigned int 		reserved2[2];		// reserved: 2 * 4 bytes    保留位
	short  				layer;
	short  				alternate_group;	// layer(2bytes) + alternate_group(2bytes)  视频层，默认为0，值小的在上层.track分组信息，默认为0表示该track未与其他track有群组关系
	short 				volume; 			//volume(2bytes)
	unsigned short 		reserved_2;			//reserved(2bytes) 
	int matrix[9];							//视频变换矩阵// unity matrix
	unsigned int width;						//宽
	unsigned int height;					//高

}tkhd_box;

typedef struct MediaBox_t
{
	BoxHeader_t 	header;
	//char 			child_box[0]; //0长数组，占位
}mdia_box;

typedef struct TrackExtendsBox_t
{
	FullBoxHeader_t 	header;
	unsigned int track_ID;
    unsigned int default_sample_description_index;
    unsigned int default_sample_duration;
    unsigned int default_sample_size;
    unsigned int default_sample_flags;
}trex_box;


#define  E_base_data_offset  			0x000001   //使能 base_data_offset 参数
#define  E_sample_description_index  	0x000002    //使能 sample_description_index 参数
#define  E_default_sample_duration  	0x000008    //使能 default_sample_duration 参数
#define  E_default_sample_size 			0x000010    //使能 default_sample_size 参数
#define  E_default_sample_flags			0x000020    //使能 default_sample_flags 参数

#define  DURATION_IS_EMPTY 				0x010000    
/*0x010000 duration-is-empty: this indicates that the duration provided in either default-sample-duration,
or by the default-duration in the Track Extends Box, is empty, i.e. that there are no samples for this
time interval. It is an error to make a presentation that has both edit lists in the Movie Box, and emptyduration fragments*/

#define  DEFAULT_BASE_IS_MOOF 			0x020000    //从每个视频的偏移位置从moof算起。
/*0x020000 default-base-is-moof: if base-data-offset-present is zero, 
this indicates that the base-dataoffset for this track fragment is the position of the first byte of the enclosing Movie Fragment Box.
Support for the default-base-is-moof flag is required under the ‘iso5’ brand, and it shall not be used in
brands or compatible brands earlier than iso5

NOTE The use of the default-base-is-moof flag breaks the compatibility to earlier brands of the file format, because it
sets the anchor point for offset calculation differently than earlier. Therefore, the default-base-is-moof flag cannot be set
when earlier brands are included in the File Type box.*/

typedef struct TrackFragmentHeaderBox_t
{
	FullBoxHeader_t 		header;
//12
	unsigned int 			track_ID;
//16
	/* all the following are optional fields */ 
	unsigned long long 		base_data_offset; 
//24 
	//unsigned int 			sample_description_index; //多余了四个字节，与解析器对不上,如有问题请删除
	unsigned int 			default_sample_duration; 
	//unsigned int 			default_sample_size; 
	//unsigned int 			default_sample_flags;
//40字节	
}tfhd_box;

typedef struct TrackFragmentBaseMediaDecodeTimeBox_t
{
	FullBoxHeader_t 	header;
	unsigned int  		baseMediaDecodeTime;//version = 1 时将为8字节，但本代码只支持 version = 0模式
	
	
}tfdt_box;

typedef struct SampleDependencyTypeBox_t
{
	FullBoxHeader_t 	header;
	/*
	for (i=0; i < sample_count; i++){
      unsigned int(2) reserved = 0;
      unsigned int(2) sample_depends_on;
      unsigned int(2) sample_is_depended_on;
      unsigned int(2) sample_has_redundancy;
	} */
	//char placeholder[0];//占位，实例化后自己实现
	

}sdtp_box;


#define  E_data_offset  					0x000001  //使能 data_offset 参数 
#define  E_first_sample_flags  				0x000004  //使能 first_sample_flags 参数 
//sample 部分
#define  E_sample_duration  				0x000100  //使能 sample_duration 参数
#define  E_sample_size  					0x000200  //使能 sample_size 参数 
#define  E_sample_flags  					0x000400  //使能 sample_flags 参数
#define  E_sample_composition_time_offset  	0x000800  //使能 sample_composition_time_offset 参数

typedef struct TrackFragmentRunBox_t
{
	FullBoxHeader_t 	header;
//12
	unsigned int  		sample_count; //样本的个数
	
   // the following are optional fields ,以下是可选字段，需要通过设置 flags 值来选择有无该项
   signed int  			data_offset;
//20 字节 
   //unsigned int  		first_sample_flags;
//24字节
   // all fields in the following array are optional
   /*等实例化时再确认，先留下一个占位符
   {
        unsigned int sample_duration; //样本（可理解为1帧）的持续时间
        unsigned int sample_size;
        unsigned int sample_flags;
        unsigned int sample_composition_time_offset; 
   }[ sample_count ]
   */
   //unsigned char placeholder[0];

}trun_box;

//将上方的 trun box里边的samples部分提取出来专门处理
//trun box 的sample数组元素结构
typedef struct _trun_V_sample_t// video
{
    unsigned int sample_duration; //样本（可理解为1帧）的持续时间
    unsigned int sample_size;
   	unsigned int sample_flags;
   // unsigned int sample_composition_time_offset; 
}trun_V_sample_t;
typedef struct _trun_A_sample_t //audio
{
   unsigned int sample_duration; //样本（可理解为1帧）的持续时间
   unsigned int sample_size;
  // unsigned int sample_flags;
  //unsigned int sample_composition_time_offset; 
}trun_A_sample_t;


/****四级BOX*********************************************************/
typedef struct  MediaHeaderBox_t
{
	FullBoxHeader_t 	header;
	unsigned int	  	creation_time;		//创建时间		
    unsigned int  		modification_time;	//修改时间
    unsigned int  		timescale;			//文件媒体在1秒时间内的刻度值，可以理解为1秒长度
    unsigned int  		duration;			//track的时间长度
	//bit(1) pad = 0;		//放在下方language的第0个bit位了，组合成2个字节
	unsigned char 		pad_language[2] ;	// language: und (undetermined) 媒体语言码。最高位为0，后面15位为3个字符（见ISO 639-2/T标准中定义）
	
}mdhd_box;

typedef struct HandlerReferenceBox_t
{
	FullBoxHeader_t 	header;
	unsigned int 		pre_defined;  		//4字节保留位
	unsigned int 		handler_type;      //4字节 vide对应视频 soun对应只有音频
	unsigned int  		reserved[3]; //3*4保留位

	//string   name;
	char name[0];//name,长度不定以'\0'结尾 ,占位
	
}hdlr_box;

typedef struct MediaInformationBox_t
{
	BoxHeader_t 	header;
	//char 			child_box[0]; //0长数组，占位
}minf_box;

/****五级BOX*********************************************************/

typedef struct VideoMediaHeaderBox_t
{
	FullBoxHeader_t 	header;
	unsigned short graphicsmode; //视频合成模式，为0时拷贝原始图像，否则与opcolor进行合成
	unsigned short opcolor[3];
	
}vmhd_box;

typedef struct SoundMediaHeaderBox_t
{
	FullBoxHeader_t 	header;
	short 				balance;
   	unsigned short  	reserved;

}smhd_box;

typedef struct DataInformationBox_t
{
	BoxHeader_t 	header;  
	//char 			child_box[0]; //0长数组，占位

}dinf_box;

typedef struct SampleTableBox_t
{
	BoxHeader_t 	header;
	//char 			child_box[0]; //0长数组，占位
}stbl_box;


/****六级BOX*********************************************************/
typedef struct DataReferenceBox_t
{
	FullBoxHeader_t 	header;
	unsigned int  	entry_count;
	/*for (i=1; i • entry_count; i++) {
	DataEntryBox(entry_version, entry_flags) data_entry; }
	*/
	//unsigned char opcolor[0];//占位
}dref_box;


typedef struct _SampleEntry_t
{
	BoxHeader_t 	header;			//
	unsigned char 	reserved[6];
	unsigned short 		data_reference_index;
}SampleEntry_t;

// Hint Sequences 一般不用
typedef struct _HintSampleEntry_t
{
	SampleEntry_t sample_entry;
	unsigned char data [];
}HintSampleEntry_t;

// Visual(video) Sequences 
//有两种格式
#if USE_AVC1_VERSION_1
typedef struct _VideoSampleEntry_t
{
	BoxHeader_t 				header;
	unsigned short 				width;
	unsigned short 				height;
	unsigned int 				horizresolution; // 72 dpi 
	unsigned int 				vertresolution; // 72 dpi
	char 						compressorname [32];
	
}VideoSampleEntry_t ;//就是 avc1_box，本代码采用avc1_box
#else
typedef struct _VideoSampleEntry_t
{
	SampleEntry_t 				sample_entry;  
	//16
	unsigned short 				pre_defined;
	unsigned short 				reserved_A;
	//20
	unsigned int 				pre_defined12[3];
	//32
	unsigned short 				width;
	unsigned short 				height;
	//36
	unsigned int 				horizresolution; // 72 dpi 

	unsigned int 				vertresolution; // 72 dpi
	unsigned int 				reserved_B;
	//48
	unsigned short 				frame_count;  //frame_count表明多少帧压缩视频存储在每个样本。默认是1,每样一帧;它可能超过1每个样本的多个帧数
	char 						compressorname [32];
	//82
	unsigned short 				depth;
	//84
	short 						pre_defined2;
	//86 
	//补2字节（4字节对齐） 88字节一共
	
}VideoSampleEntry_t ;//就是 avc1_box，本代码采用avc1_box

#endif



// Audio Sequences
typedef struct _AudioSampleEntry_t
{
	SampleEntry_t 				sample_entry;
	
	unsigned int 				reserved8[2] 		;
	unsigned short 				channelcount;//单声道还是双声道
	unsigned short 				samplesize;	//样本大小（位宽）
	unsigned short 				pre_defined;
	unsigned short 				reserved2;
	unsigned int 				samplerate;//{timescale of media}<<16;samplerate 显然要右移16位才有意义

}AudioSampleEntry_t;//就是 mp4a_box，本代码采用mp4a_box


typedef struct SampleDescriptionBox_t 
{
	FullBoxHeader_t 	header;
	unsigned int 		entry_count;
	char Sample[0];	//分Visual(video)/Audio/Hint sample;依据 hdlr box 中的handler_type来确定 
	
}stsd_box;

typedef struct TimeToSampleBox_t
{
	FullBoxHeader_t 	header;
	unsigned int  		entry_count;
  
	//实际初始化再来插入变量空间，这里只做占位处理
	//int 				i;
   /*for (i=0; i < entry_count; i++) {
      unsigned int(32)  sample_count;
      unsigned int(32)  sample_delta;
   	}*/
   //	unsigned int sample_count_sample_delta[0];
   	

}stts_box;

typedef struct SampleToChunkBox_t
{
	FullBoxHeader_t 	header;
	unsigned int  		entry_count;
  
	//实际初始化再来插入变量空间，这里只做占位处理
	//int 				i;
   /*for (i=0; i < entry_count; i++) {
      unsigned int(32)  sample_count;
      unsigned int(32)  sample_delta;
   	}*/
   //	unsigned int sample_count_sample_delta[0];
   	

}stsc_box;

typedef struct SampleSizeBox_t
{
	FullBoxHeader_t 	header;
	unsigned int sample_size;
	unsigned int sample_count;

	/* 实际初始化再来插入变量空间，这里只做占位处理
	if (sample_size==0) {
		for (i=1; i < sample_count; i++){
	      unsigned int(32)  entry_size;
	} }*/
	//unsigned int entry_size[0];		//占位处理
}stsz_box;

typedef struct ChunkOffsetBox_t
{
	FullBoxHeader_t 	header;
	unsigned int  		entry_count;
  
	//实际初始化再来插入变量空间，这里只做占位处理
	//int 				i;
   /*for (i=0; i < entry_count; i++) {
      unsigned int(32)  sample_count;
      unsigned int(32)  sample_delta;
   	}*/
  // 	unsigned int sample_count_sample_delta[0];
   	

}stco_box;

/****七级BOX*********************************************************/

typedef struct DataEntryUrlBox_t
{
	FullBoxHeader_t 	header;
//	char 				location[0];
}url_box;

typedef struct DataEntryUrnBox_t
{
	FullBoxHeader_t 	header;
	//char 				location[0];
}urn_box;

/**
*AVCDecoderConfigurationRecord.包含着是H.264解码相关比较重要的sps和pps信息，
*在给AVC解码器送数据 流之前一定要把sps和pps信息送出，否则的话解码器不能正常解码。
*而且在解码器stop之后再次start之前，如seek、快进快退状态切换等，
*都 需要重新送一遍sps和pps的信息.AVCDecoderConfigurationRecord在FLV文件中一般情况也是出现1次，
*也就是第一个 video tag.
*/

typedef struct AVCDecoderConfigurationRecord_t
{
	BoxHeader_t 		header; 
//8
	unsigned char 		configurationVersion; 	//版本号，1
	unsigned char 		AVCProfileIndication; 	//sps[1]
	unsigned char 		profile_compatibility; 	//sps[2]
	unsigned char 		AVCLevelIndication;		//sps[3]
//12
	//bit(6) reserved = ‘111111’b;
	//unsigned int(2) lengthSizeMinusOne; //H264中的NALU长度，计算方法为 1+(lengthSizeMinusOne & 3)
	unsigned char 		reserved_6_lengthSizeMinusOne_2;//替代上边注释掉的两个变量
	
	//bit(3) reserved = ‘111’b;
	//unsigned int(5) numOfSequenceParameterSets; //sps 个数，一般为1，计算方法为 numOfSequenceParameterSets & 0x1f
	unsigned char 		reserved_3_numOfSequenceParameterSets_5;//替代上边注释掉的两个变量
	//for (i=0; i< numOfSequenceParameterSets; i++) {	//(sps_size + sps)数组
	  unsigned short sequenceParameterSetLength ;		//sps长度
//16
	  unsigned char sequenceParameterSetNALUnit[0];		//sps内容
	//}
	
	
	unsigned char numOfPictureParameterSets;		//pps个数，一般为1
	//for (i=0; i< numOfPictureParameterSets; i++) {	(pps_size + pps)数组
	  unsigned short pictureParameterSetLength;		//pps长度
	  unsigned char pictureParameterSetNALUnit[0]; 	//pps内容
//19 字节 + 1字节填充（注意0长数组不占空间）
//20字节
	//}
}avcc_box;
typedef struct _avcc_box_info_t
{
	avcc_box*		avcc_buf;	
	unsigned int 	buf_length;

}avcc_box_info_t;




/*
typedef struct MPEG4AudioSampleEntry_t
{
	BoxHeader_t 			header;
	
	unsigned int 			reserved[2];
	unsigned short 			channelcount;//单声道还是双声道
	unsigned short 			samplesize;	//样本大小（位宽）
	unsigned short 			pre_defined;
	unsigned short 			reserved2;
	unsigned int 			samplerate; 	//{timescale of media}<<16;

}mp4a_box;
*/

//#define avc1_box VideoSampleEntry_t
typedef VideoSampleEntry_t avc1_box;

//#define mp4a_box AudioSampleEntry_t
typedef AudioSampleEntry_t mp4a_box;



/****第八级box**********************************************************/
#pragma pack(1) //临时指定1字节对齐
//该结构和标准文件解析出来的结构差异较大，如有问题后期进一步修改
typedef struct  ESDescriptorBox_t
{

	FullBoxHeader_t header;
//12 bytes

	//---Detailed-Information--------------------------------------
	unsigned char 	tag1;		// 3	(0x03)   
	unsigned char   ex_string1[3];//0x80 0x80 0x80 
	
	unsigned char 	tag_size1;	
	unsigned short 	es_id;
//19 bytes
	unsigned char 	Stream_dependence_flag:1; 
	unsigned char 	URL_flag:1; 
	unsigned char 	OCR_stream_flag:1;
	unsigned char	Stream_priority:5;
//20 bytes

	
	//---Decoder Config Descriptor----------------------------------
	unsigned char 	tag2;		   //4 (0X04)
	unsigned char   ex_string2[3]; //0x80 0x80 0x80 
	unsigned char 	tag_size2;
//25 bytes
	unsigned char 	Object_type_indication;	//MPEG-4 audio (0X40)
	unsigned char 	Stream_type:6;
	unsigned char 	Up_stream:1;
	unsigned char	Reserved:1;
//27 bytes
	unsigned char   bufferSizeDB[3];  //如若4字节对齐后边会少了2字节填充
//30 bytes
	unsigned int	Max_bitrate;
	unsigned int 	Avg_bitrate;
//38 bytes

	//---Audio Decoder Specific Info--------------------------------
	unsigned char	Tag3;			//5 (0X05)
	unsigned char   ex_string3[3];  //0x80 0x80 0x80
	unsigned char	Tag_size3;		
//43 bytes
	unsigned short	Audio_object_type:5;		
	unsigned short	Sampling_freq_index:4;	
	unsigned short	channelConfiguration:4;			
	unsigned short	cpConfig:2;
	unsigned short  directMapping:1;
//45 bytes
	//---SLConfigDescrTag--------------------------------
	unsigned char	Tag4;			//6 (0X06)
	unsigned char   ex_string4[3];  //0x80 0x80 0x80
	unsigned char 	Length_Field;	//0x01
	unsigned char	predefined;		//0x02
//51 bytes


}esds_box;


/*============================================================================
*/
#pragma pack(4) //恢复4字节对齐

//box 的等级 宏，便于区分
#define lve1
#define lve2
#define lve3
#define lve4
#define lve5
#define lve6
#define lve7
#define lve8
typedef struct _trak_video_t
{
	lve2 trak_box *trakBox;
		lve3 tkhd_box *tkhdBox;
		lve3 mdia_box *mdiaBox;
			lve4 mdhd_box *mdhdBox;
			lve4 hdlr_box *hdlrBox;
			lve4 minf_box *minfBox;
				lve5 vmhd_box *vmhdBox;
				lve5 dinf_box *dinfBox;
					lve6 dref_box *drefBox;
						//lve7 url_box *urlBox; //未使用
				lve5 stbl_box *stblBox;
					lve6 stsd_box *stsdBox;
						//lve7 avc1_box *avc1Box; 		//归属在stsd_box中一起初始化了
							//lve8 avcc_box *avccBox;	//归属在stsd_box中一起初始化了
							//lve8 pasp_box *paspBox;	//暂未实现
					lve6 stts_box *sttsBox;
					lve6 stsc_box *stscBox;
					lve6 stsz_box *stszBox;
					lve6 stco_box *stcoBox;
}trak_video_t;

typedef struct _trak_audio_t
{
	lve2 trak_box *trakBox;
		lve3 tkhd_box *tkhdBox;
		lve3 mdia_box *mdiaBox;
			lve4 mdhd_box *mdhdBox;
			lve4 hdlr_box *hdlrBox;
			lve4 minf_box *minfBox;
				lve5 smhd_box *smhdBox;
				lve5 dinf_box *dinfBox;
					lve6 dref_box *drefBox;
						//lve7 url_box *url_box;	//	暂时不使用
				lve5 stbl_box *stblBox;
					lve6 stsd_box *stsdBox;
						//lve7 mp4a_box *mp4aBox;		//归属在stsd_box中一起初始化了
							//lve8 esds_box *esdsBox;	//归属在stsd_box中一起初始化了
					lve6 stts_box *sttsBox;
					lve6 stsc_box *stscBox;
					lve6 stsz_box *stszBox;
					lve6 stco_box *stcoBox;
					

}trak_audio_t;

typedef struct _traf_video_t
{
	lve2 traf_box *trafBox;
		lve3 tfhd_box *tfhdBox;
		lve3 tfdt_box *tfdtBox;
		lve3 trun_box *trunBox;
		
}traf_video_t;

typedef struct _traf_audio_t
{
	lve2 traf_box *trafBox;
		lve3 tfhd_box *tfhdBox;
		lve3 tfdt_box *tfdtBox;
		lve3 trun_box *trunBox;
}traf_audio_t;


typedef struct _tfra_video_t
{
	lve2 tfra_box *tfraBox;
}tfra_video_t;

typedef struct _tfra_audio_t
{
	lve2 tfra_box *tfraBox;
}tfra_audio_t;

typedef struct _fmp4_file_box_t
{
	lve1 ftyp_box *ftypBox;
	lve1 moov_box *moovBox;
		lve2 mvhd_box *mvhdBox;
		lve2 trak_video_t *trak_video;
		lve2 trak_audio_t *trak_audio;
		lve2 mvex_box *mvexBox;
			lve3 trex_box *trex_video;
			lve3 trex_box *trex_audio;
		//lve2 udta_box *udtaBox;
	
	lve1 moof_box *moofBox;
		lve2 mfhd_box *mfhdBox;
		lve2 traf_video_t *traf_video;
		lve2 traf_audio_t *traf_audio;
	lve1 mdat_box *mdatBox;
	lve1 mfra_box *mfraBox;
		lve2 tfra_video_t *tfra_video;
		lve2 tfra_audio_t *tfra_audio;
		lve2 mfro_box *mfroBox;

}fmp4_file_box_t;


typedef struct _all_box_t
{
 	ftyp_box *ftypBox;
	moov_box *moovBox;
	moof_box *moofBox;
	mdat_box *mdatBox;
	mvhd_box *mvhdBox;
	trak_box *trakBox;
	mvex_box *mvexBox;
	mfhd_box *mfhdBox;
	traf_box *trafBox;
	
	tkhd_box *tkhdBox;
	mdia_box *mdiaBox;
	trex_box *trexBox;
	tfhd_box *tfhdBox;
	tfdt_box *tfdtBox;
	sdtp_box *sdtpBox;
	trun_box *trunBox;

	mdhd_box *mdhdBox;
	hdlr_box *hdlrBox;
	minf_box *minfBox;
	vmhd_box *vmhdBox;
	smhd_box *smhdBox;
	dinf_box *dinfBox;
	stbl_box *stblBox;

	dref_box *drefBox;
	avc1_box *avc1Box;
	mp4a_box *mp4aBox;
	stsd_box *stsdBox;
	stts_box *sttsBox;
	stsc_box *stscBox;
	stsz_box *stszBox;
	stco_box *stcoBox;

	url_box  *urlBox;
	urn_box  *urnBox;
	avcc_box *avccBox;
	esds_box *esdsBox;
	
}all_box_t;

ftyp_box * ftyp_box_init(void);
moov_box* moov_box_init(unsigned int box_length);
moof_box* moof_box_init(unsigned int box_length);
mdat_box* mdat_box_init(unsigned int box_length);
mvhd_box* mvhd_box_init(unsigned int timescale,unsigned int duration);
trak_box* trak_box_init(unsigned int box_length);
mvex_box*	mvex_box_init(unsigned int box_length);
mfhd_box* mfhd_box_init(void);
traf_box*	traf_box_init(unsigned int box_length);
tkhd_box* tkhd_box_init(unsigned int trackId,unsigned int duration,unsigned short width,unsigned short height);
mdia_box* mdia_box_init(unsigned int box_length);
trex_box*	trex_box_init(unsigned int trackId);
tfhd_box*	tfhd_box_init(unsigned int trackId);
tfdt_box*	tfdt_box_init(int baseMediaDecodeTime);
sdtp_box*	sdtp_box_init(void);
trun_box*	trun_box_init(unsigned int trackId);
mdhd_box* mdhd_box_init(unsigned int timescale,unsigned int duration);

#define VIDEO_HANDLER 1
#define AUDIO_HANDLER 2
hdlr_box*	hdlr_box_init(unsigned int handler_type);
minf_box*	minf_box_init(unsigned int box_length);
vmhd_box* vmhd_box_init(void);
smhd_box*	smhd_box_init(void);
dinf_box* dinf_box_init(unsigned int box_length);
stbl_box*	stbl_box_init(unsigned int box_length);
dref_box* dref_box_init(void);
void HintSampleEntry(void);
stsd_box*  AudioSampleEntry(unsigned char channelCount,unsigned short sampleRate);
stsd_box* VideoSampleEntry(unsigned short width,unsigned short height);
stsd_box* stsd_box_init(unsigned int handler_type,
							 unsigned short width,unsigned short height, // for video tracks
							 unsigned char channelCount,unsigned short sampleRate // for audio tracks
							);

stts_box*	stts_box_init(void);
stsc_box*	stsc_box_init(void);
stsz_box* stsz_box_init(void);
stco_box*	stco_box_init(void);
int url_box_init(void);
int	urn_box_init(void);
avc1_box* avc1_box_init(void);
mp4a_box* mp4a_box_init(void);
//avcc_box_info_t *	avcc_box_init(unsigned char* naluData, int naluSize);
avcc_box_info_t *	avcc_box_init(void);
int FrameType(unsigned char* naluData);
void print_char_array(unsigned char* box_name,unsigned char*start,unsigned int length);
mfra_box* mfra_box_init(void);
tfra_video_t * tfra_video_init(void);
tfra_audio_t * tfra_audio_init(void);
mfro_box * mfro_box_init(void);


/***一般mp4文件部分特有 box结构********************************************************************************
专门针对普通mp4文件部分
*********************************************************************************************************/

typedef struct FreeSpaceBox_t
{
	BoxHeader_t header;
	unsigned char data[0];
	
}free_box;

typedef struct EditBox_t
{
	BoxHeader_t header;
	
}edts_box;

typedef struct EditListBox_t
{
	FullBoxHeader_t header;
	unsigned int entry_count; //一般为1
	/*
	for (i=1; i <= entry_count; i++) 
	{
		{ // version==0
			unsigned int(32) segment_duration;
			int(32) media_time;
		}
		int(16) media_rate_integer;
		int(16) media_rate_fraction = 0;
	}
	*/
	unsigned int 	segment_duration;
	int 			media_time;
	
}elst_box;


typedef struct SyncSampleBox_t
{
	FullBoxHeader_t header;
	unsigned int entry_count; //一般为 1
	/*
	int i;
	for (i=0; i < entry_count; i++) {
	unsigned int(32) sample_number;
	}
	*/
	unsigned int sample_number; //entry_count 个 
}stss_box;


typedef struct CompositionOffsetBox_t
{
	FullBoxHeader_t header;
	unsigned int entry_count;
	/* 该部分独立出去
	int i;
	if (version==0) {
	for (i=0; i < entry_count; i++) {
	unsigned int(32) sample_count;
	unsigned int(32) sample_offset;
	}
	}
	*/


}ctts_box;

typedef struct _CttsEntries_t
{
	unsigned int sample_count;
	unsigned int sample_offset;

}CttsEntries_t;

typedef struct UserDataBox_t
{
	BoxHeader_t header;

}udta_box;


 
typedef struct MetaBox_t
{
	FullBoxHeader_t header;
	hdlr_box  theHandler;
	/*
	PrimaryItemBox primary_resource; // optional
	DataInformationBox file_locations; // optional
	ItemLocationBox item_locations; // optional
	ItemProtectionBox protections; // optional
	ItemInfoBox item_infos; // optional
	IPMPControlBox IPMP_control; // optional
	ItemReferenceBox item_refs; // optional
	ItemDataBox item_data; // optional
	Box other_boxes[]; // optional
	*/

}Meta_box;



#ifdef __cplusplus
}
#endif

#endif





