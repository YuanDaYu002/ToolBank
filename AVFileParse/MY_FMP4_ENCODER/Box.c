

#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdlib.h>



#include "Box.h"
#include "typeport.h"

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

/*
	请求一个 ftpy box 
	返回值为初始化后的box首地址
	注意：使用完后需要调用free释放
*/
ftyp_box * ftyp_box_init()
{
		
	 unsigned char Array[] = {
            0x69, 0x73, 0x6F, 0x6D, // major_brand: isom     isom  MP4  Base Media v1[IS0 14496-12:2003]ISO YES video/mp4
            0x0, 0x0, 0x0, 0x1, 	// minor_version: 0x01 (最低兼容性的版本)
            0x69, 0x73, 0x6F, 0x6D, // isom 
            0x61, 0x76, 0x63, 0x31 	// avc1
        	};

	int box_size = sizeof(Array)+sizeof(BoxHeader_t);
	ftyp_box *ftpy_item = (ftyp_box *)malloc(box_size);
	if(NULL == ftpy_item)
		return NULL;
	
	memset(ftpy_item,0,box_size);
	
	ftpy_item->header.size = htonl(box_size);
	//strncpy(ftpy_item->header->type,"ftpy",4);
	strncpy(ftpy_item->header.type,"yptf",4);//大端模式存储
	
	memcpy(((char*)ftpy_item) + sizeof(BoxHeader_t),Array,sizeof(Array));

	return ftpy_item;
	
}

moov_box* moov_box_init(unsigned int box_length)
{
	moov_box * moov_item = (moov_box *)malloc(box_length);
	if(NULL == moov_item)
		return NULL;
	memset(moov_item,0,box_length);
	moov_item->header.size = htonl(box_length);//上层调用增加child box后再及时修正
	strncpy(moov_item->header.type,"voom",4);//大端模式存储

	return moov_item;
	
}

moof_box* moof_box_init(unsigned int box_length)
{
	moof_box * moof_item = (moof_box *)malloc(box_length);
	if(NULL == moof_item)
		return NULL;
	memset(moof_item,0,box_length);
	moof_item->header.size = htonl(box_length);//上层调用增加child box后再及时修正
	strncpy(moof_item->header.type,"foom",4);//大端模式存储

	return moof_item;
}

mdat_box* mdat_box_init(unsigned int box_length)
{
	mdat_box * mdat_item = (mdat_box *)malloc(box_length);
	if(NULL == mdat_item)
		return NULL;
	memset(mdat_item,0,box_length);
	mdat_item->header.size = htonl(box_length);//上层调用增加child box后再及时修正
	strncpy(mdat_item->header.type,"tadm",4);//大端模式存储

	return mdat_item;
}

	
/***二级BOX初始化***************************************/
/*
	timescale = 1000;
	duration = 0;		
*/
mvhd_box* mvhd_box_init(unsigned int timescale,unsigned int duration)
{
	mvhd_box * mvhd_item = (mvhd_box *)malloc(sizeof(mvhd_box));
	if(NULL == mvhd_item)
		return NULL;
	memset(mvhd_item,0,sizeof(mvhd_box));

	mvhd_item->header.size = htonl(sizeof(mvhd_box));
	strncpy(mvhd_item->header.type,"dhvm",4);//大端模式存储
	mvhd_item->header.version = 0;

	/*struct timeval buf;
	gettimeofday(&buf,NULL);
	mvhd_item->creation_time = htonl(buf.tv_sec);
	*/

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
				0xFF, 0xFF, 0xFF, 0xFF  // next_track_ID 下一个track使用的id号
		};

		memcpy(((char*)mvhd_item) + sizeof(FullBoxHeader_t),MVHD,sizeof(MVHD));

		return mvhd_item;
	
}

trak_box* trak_box_init(unsigned int box_length)
{
	trak_box *track_item = (trak_box*)malloc(box_length);
	if(NULL == track_item)
		return NULL;
	memset(track_item,0,box_length);

	track_item->header.size = htonl(box_length);
	strncpy(track_item->header.type,"kart",4);//大端模式存储
	return track_item;
}

mvex_box*	mvex_box_init(unsigned int box_length)
{
	mvex_box *mvex_item = (mvex_box*)malloc(box_length);
	if(NULL == mvex_item)
		return NULL;
	memset(mvex_item,0,box_length);
	mvex_item->header.size  = htonl(box_length);
	strncpy(mvex_item->header.type,"xevm",4);//大端模式存储
	mvex_item->header.version = 0;

	return mvex_item;

}

mfhd_box* mfhd_box_init()
{
	mfhd_box *mfhd_item = (mfhd_box*)malloc(sizeof(mfhd_box));
	if(NULL == mfhd_item)
		return NULL;
	memset(mfhd_item,0,sizeof(mfhd_box));
	mfhd_item->header.size  = htonl(sizeof(mfhd_box));
	strncpy(mfhd_item->header.type,"dhfm",4);//大端模式存储
	mfhd_item->header.version = 0;

	return mfhd_item;
}

traf_box*	traf_box_init(unsigned int box_length)
{
	traf_box *traf_item = (traf_box*)malloc(box_length);
	if(NULL == traf_item)
		return NULL;
	memset(traf_item,0,box_length);
	traf_item->header.size  = htonl(box_length);
	strncpy(traf_item->header.type,"fart",4);//大端模式存储


	return traf_item;
}


/***三级BOX初始化***************************************/
tkhd_box* tkhd_box_init(unsigned int trackId,unsigned int duration,unsigned int width,unsigned int height)
{
	tkhd_box *tkhd_item = (tkhd_box*)malloc(sizeof(tkhd_box));
	if(NULL == tkhd_item)
		return NULL;
	tkhd_item->header.size = htonl(sizeof(tkhd_box));
	strncpy(tkhd_item->header.type,"dhkt",4);//大端模式存储
	
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
	
		memcpy(&tkhd_item->header.version,TKHD,sizeof(TKHD));
		return tkhd_item;
	
}

mdia_box* mdia_box_init(unsigned int box_length)
{
	mdia_box* mdia_item = (mdia_box*)malloc(box_length);
	if(NULL == mdia_item)
		return NULL;
	memset(mdia_item,0,box_length);

	mdia_item->header.size = htonl(box_length);
	strncpy(mdia_item->header.type ,"aidm",4);//大端模式存储
	
	return mdia_item;
	
}

trex_box*	trex_box_init(unsigned int trackId)
{
	trex_box* trex_item = (trex_box*)malloc(sizeof(trex_box));
	if(NULL == trex_item)
		return NULL;
	memset(trex_item,0,sizeof(trex_box));

	trex_item->header.size = htonl(sizeof(trex_box));
	strncpy(trex_item->header.type,"xert",4);//大端模式存储
	
	
		 const unsigned char TREX[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags
            (trackId >> 24) & 0xFF, // track_ID
            (trackId >> 16) & 0xFF,
            (trackId >> 8 ) & 0xFF,
            (trackId) & 0xFF,
            0x00, 0x00, 0x00, 0x01, // default_sample_description_index
            0x00, 0x00, 0x00, 0x00, // default_sample_duration
            0x00, 0x00, 0x00, 0x00, // default_sample_size
            0x00, 0x01, 0x00, 0x01 // default_sample_flags
        };
	memcpy(&trex_item->header.version,TREX,sizeof(TREX));

	return trex_item;
	
}

tfhd_box*	tfhd_box_init(unsigned int trackId)
{
	tfhd_box* tfhd_item = (tfhd_box*)malloc(sizeof(tfhd_box));
	if(NULL == tfhd_item)
		return NULL;
	memset(tfhd_item,0,sizeof(tfhd_box));
	tfhd_item->header.size = htonl(sizeof(tfhd_box));
	strncpy(tfhd_item->header.type,"dhft",4);//大端模式存储

    const unsigned char TFHD[] = {
        0x00, 0x00, 0x00, 0x00, // version(0) & flags
        (trackId >> 24) & 0xFF, // track_ID
        (trackId >> 16) & 0xFF,
        (trackId >> 8) & 0xFF,
        (trackId) & 0xFF
    	};

	memcpy(&tfhd_item->header.version,TFHD,sizeof(TFHD));
	
	return tfhd_item;
	
}

tfdt_box*	tfdt_box_init(int baseMediaDecodeTime)
{
	tfdt_box* tfdt_item = (tfdt_box*)malloc(sizeof(tfdt_box));
	if(NULL == tfdt_item)
		return NULL;
	memset(tfdt_item,0,sizeof(tfdt_box));

	tfdt_item->header.size = htonl(sizeof(tfdt_box));
	strncpy(tfdt_item->header.type,"tdft",4);//大端模式存储

    const unsigned char TFDT[] = {
		    0x00, 0x00, 0x00, 0x00, // version(0) & flags
		    (baseMediaDecodeTime >> 24) & 0xFF, // baseMediaDecodeTime: int32
		    (baseMediaDecodeTime >> 16) & 0xFF,
		    (baseMediaDecodeTime >> 8 ) & 0xFF,
		    (baseMediaDecodeTime) & 0xFF
			};

	memcpy(&tfdt_item->header.version,TFDT,sizeof(TFDT));

	return tfdt_item;

	
}
sdtp_box*	sdtp_box_init()
{
	sdtp_box* sdtp_item = (sdtp_box*)malloc(sizeof(sdtp_box));
	if(NULL == sdtp_item)
		return NULL;
	
	memset(sdtp_item,0,sizeof(sdtp_item));

	sdtp_item->header.size = htonl(sizeof(sdtp_box));
	strncpy(sdtp_item->header.type,"ptds",4);//大端模式存储

	//先不实现，好像没有也没事
	
}

trun_box*	trun_box_init()
{
	trun_box* trun_item = (trun_box*)malloc(sizeof(trun_box));
	if(NULL == trun_item)
		return NULL;

	memset(trun_item,0,sizeof(trun_box));
	trun_item->header.size = htonl(sizeof(trun_box)); 
	strncpy(trun_item->header.type,"nurt",4);//大端模式存储

	trun_item->sample_count = 0;
	trun_item->first_sample_flags = 0;
	
	//后边的samples相关数据需要   音视频混合程序来实时填充，初始化不做处理
	
}
	
/****四级BOX初始化*********************************************************/

mdhd_box* mdhd_box_init(unsigned int timescale,unsigned int duration)
{
	mdhd_box* mdhd_item = (mdhd_box*)malloc(sizeof(mdhd_box));
	if(NULL == mdhd_item)
		return NULL;

	memset(mdhd_item,0,sizeof(mdhd_item));

	mdhd_item->header.size = htonl(sizeof(mdhd_box));
	strncpy(mdhd_item->header.type,"dhdm",4);//大端模式存储

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
	
	memcpy(&mdhd_item->header.version,MDHD,sizeof(MDHD));

	return mdhd_item;
	
}

hdlr_box*	hdlr_box_init()
{
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
			
	int box_length = sizeof(FullBoxHeader_t)-4 + sizeof(HDLR);//-4代表除去version + flags的长度，因算到数组HDLR中了
	hdlr_box* hdlr_item = (hdlr_box*)malloc(box_length);
	if(NULL == hdlr_item)
		return NULL;
	memset(hdlr_item,0,sizeof(box_length));

	hdlr_item->header.size = htonl(box_length);
	strncpy(hdlr_item->header.type,"rldh",4);//大端模式存储

	return hdlr_item;
	
	
}
minf_box*	minf_box_init(unsigned int box_length)
{
	minf_box*minf_item = (minf_box*)malloc(box_length);
	if(NULL == minf_item)
		return NULL;

	memset(minf_item,0,box_length);

	minf_item->header.size = htonl(box_length);
	strncpy(minf_item->header.type,"fnim",4);//大端模式存储
	
	return minf_item;
}

/****五级BOX*********************************************************/

vmhd_box* vmhd_box_init()
{
	vmhd_box* vmhd_item = (vmhd_box*)malloc(sizeof(vmhd_box));
	if(NULL == vmhd_item)
		return NULL;

	memset(vmhd_item,0,sizeof(vmhd_box));
	
	vmhd_item->header.size = htonl(sizeof(vmhd_box));
	strncpy(vmhd_item->header.type,"dhmv",4);//大端模式存储

	unsigned char VMHD[] = {
            0x00, 0x00, 0x00, 0x01, // version(0) + flags
            0x00, 0x00, 			// graphicsmode: 2 bytes 视频合成模式，为0时拷贝原始图像，否则与opcolor进行合成
            0x00, 0x00, 0x00, 0x00, // opcolor: 3 * 2 bytes ｛red，green，blue｝
            0x00, 0x00
        	};
	memcpy(&vmhd_item->header.version,VMHD,sizeof(VMHD));

	return vmhd_item;
	
	
}

smhd_box*	smhd_box_init()
{
	smhd_box* smhd_item = (smhd_box*)malloc(sizeof(smhd_box));
	if(NULL == smhd_item)
		return NULL;
	memset(smhd_item,0,sizeof(smhd_box));

	smhd_item->header.size = htonl(sizeof(smhd_box));
	strncpy(smhd_item->header.type,"dhms",4);//大端模式存储

	unsigned char SMHD[] = {
            0x00, 0x00, 0x00, 0x00, // version(0) + flags   box版本，0或1，一般为0。
            0x00, 0x00, 0x00, 0x00 /* balance(2) + reserved(2) 立体声平衡，[8.8] 格式值，
            							一般为0，-1.0表示全部左声道，1.0表示全部右声道+2位保留位
									*/
        };
	memcpy(&smhd_item->header.version,SMHD,sizeof(SMHD));

	return smhd_item;
	
}

dinf_box* dinf_box_init(unsigned int box_length)
{
	dinf_box* dinf_item = (dinf_box*)malloc(box_length);
	if(NULL == dinf_item)
		return NULL;

	memset(dinf_item,0,box_length);

	dinf_item->header.size = htonl(box_length);
	strncpy(dinf_item->header.type,"fnid",4);//大端模式存储

	return dinf_item;
	
}

stbl_box*	stbl_box_init(unsigned int box_length)
{
	stbl_box* stbl_item = (stbl_box*)malloc(box_length);
	if(NULL == stbl_item)
		return NULL;

	memset(stbl_item,0,box_length);
	stbl_item->header.size = htonl(box_length);
	strncpy(stbl_item->header.type,"lbts",4);//大端模式存储

	return stbl_item;
}
	
/****六级BOX*********************************************************/
dref_box* dref_box_init()
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
	dref_box* dref_item = (dref_box*)malloc(box_length);
	if(NULL == dref_item)
		return NULL;
	memset(dref_item,0,box_length);

	dref_item->header.size = htonl(box_length);
	strncpy(dref_item->header.type,"ferd",4);//大端模式存储

	memcpy(&dref_item->header.version,DREF,sizeof(DREF));

	return dref_item;
	
	
}

void HintSampleEntry()
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
	AudioSampleEntry_t* mp4a_item = (AudioSampleEntry_t*)malloc(sizeof(AudioSampleEntry_t));
	if(NULL == mp4a_item )
		return NULL;
	
	memset(mp4a_item ,0,sizeof(AudioSampleEntry_t));
	mp4a_item->sample_entry.header.size = htonl(sizeof(AudioSampleEntry_t));
	strncpy(mp4a_item->sample_entry.header.type,"a4pm",4);//大端模式存储

	memcpy(&mp4a_item->sample_entry.reserved  ,mp4a,sizeof(mp4a));

	
	const unsigned char esds[] = {
			0x00, 0x00, 0x00, 0x00, // version 0 + flags

			/***Detailed-Information***/
            0x03, 				// tag1    
            0x06, 			// tag_size1
            0x00, 0x02, 	// es_id
            0x00, 		// Stream dependence flag
            0x00, 		// URL flag
            0x00, // OCR stream flag
            0x00, // Stream priority
            
            /**
             *当 objectTypeIndication 为0x40时，为MPEG-4 Audio（MPEG-4 Audio generally is thought of as AAC
             * but there is a whole framework of audio codecs that can Go in MPEG-4 Audio including AAC, BSAC, ALS, CELP,
             * and something called MP3On4），如果想更细分format为aac还是mp3，
             * 可以读取 MP4DecSpecificDescr 层 data[0] 的前五位
             */
             
             /*** Decoder Config Descriptor***/
            0x04, // tag1
            0x0B, // tag1_size  12字节
            0x40, // Object type indication
            0x05, // Stream type
            0x00, 	// Up stream
            0x01,//Reserved
            0x00,0x01,0xF4,0x00, //Max bitrate (12800)
            0x00,0x00,0x00,0x00, //Avg bitrate

			/***Audio Decoder Specific Info***/
			0x05,	//tag
			0x33,	//tag_size  后边51(0x33)个字节
			0x00,0x00,0x00,0x02, //Audio object type ; 02 - AAC LC or backwards compatible HE-AAC (Most realworld AAC falls in one of these cases) 
			0x00,0x00,0x00,0x04, //Sampling freq index
			0x00,0x00,0x00,0x00, //Sampling freq
			0x00,0x00,0x00,0x00, //Sbr Flag
			0x00,0x00,0x00,0x00, //Ext audio object type
			0x00,0x00,0x00,0x00, //Ext sampling freq index
			0x00,0x00,0x00,0x00, //Ext sampling freq
			0x00, //Frame length flag
			0x00, //Depends on core coder
			0x00,0x00,0x00,0x00, //Core coder delay
			0x00, //Extension flag
			0x00,0x00,0x00,0x00, //Layer nr
			0x00,0x00,0x00,0x00, //Num of subframe
			0x00,0x00,0x00,0x00, //Layer length
			0x00, //aac section data resilience flag
			0x00, //aac scale factor data resilience flag
			0x00, //aac spectral data resilience flag
			0x00, //Extension flag3
			
		};
	esds_box*esds_item = (esds_box*)malloc(sizeof(esds_box));
	if(NULL == esds_item)
	{
		free(mp4a_item);
		return NULL;
	}
		
	memset(esds_item,0,sizeof(esds_box));
	esds_item->header.size = htonl(sizeof(esds_box));
	strncpy(esds_item->header.type,"sdse",4);//大端模式存储
	memcpy(&esds_item->header.version,esds,sizeof(esds));

	//构造 stsd box
	int stsd_box_size = sizeof(mp4a_item) + sizeof(esds_item); 
	stsd_box* stsd_item = (stsd_box*)malloc(stsd_box_size);
	if(NULL == stsd_item)
	{
		free(esds_item);
		free(mp4a_item);
		return NULL;
	}

	memcpy(stsd_item->Sample,mp4a_item,sizeof(AudioSampleEntry_t));
	memcpy(stsd_item->Sample + sizeof(AudioSampleEntry_t),esds_item,sizeof(esds_box));
	//修正 stsd box长度
	stsd_item->header.size = htonl(stsd_box_size);

	
	free(esds_item);
	free(mp4a_item);
		

	return stsd_item; 
	
		
}

stsd_box* VideoSampleEntry(unsigned short width,unsigned short height)
{
	unsigned char AVC1[] = {
            0x00, 0x00, 0x00, 0x00, // reserved(4)    6个 保留位 Reserved：6个字节，设置为0；
            0x00, 0x00, 0x00, 0x01, // reserved(2) + data_reference_index(2)  数据引用索引
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
            0x00, 0x01, // frame_count      
            //frame_count表明多少帧压缩视频存储在每个样本。默认是1,每样一帧;它可能超过1每个样本的多个帧数
            0x04, //    strlen compressorname: 32 bytes        
            //32个8 bit    第一个8bit表示长度,剩下31个8bit表示内容
            0x67, 0x31, 0x31, 0x31, // compressorname: 32 bytes    翻译过来是g111
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

	int box_length = sizeof(stsd_box) +\
					 sizeof(BoxHeader_t) + sizeof(AVC1) +\
					 avcc_item->buf_length;//pasp box暂未实现
					 
	stsd_box* stsd_item  = (stsd_box*)malloc(box_length);
	if(NULL == stsd_item)
		return NULL;
	memset(stsd_item,0,box_length);

	//填充stsd 
	stsd_item->header.size = htonl(box_length);
	strncpy(stsd_item->header.type,"dsts",4);

	//偏移到avc1的位置
	int offset = sizeof(stsd_box);
	VideoSampleEntry_t*avc1_item = (VideoSampleEntry_t*)((unsigned char*)stsd_item + offset);  
	box_length = sizeof(VideoSampleEntry_t);
	
	//填充avc1的头部
	avc1_item->sample_entry.header.size = htonl(box_length);
	strncpy(avc1_item->sample_entry.header.type,"1cva",4);

	//填充AVC1 body部分
	offset += sizeof(BoxHeader_t); 
	memcpy((unsigned char*)stsd_item + offset,AVC1,sizeof(AVC1));

	//填充AVCC 
	offset += sizeof(AVC1);
	memcpy((unsigned char*)stsd_item + offset,avcc_item->avcc_buf ,avcc_item->buf_length);
	
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
	stts_box* stts_item = (stts_box*)malloc(box_length);
	if(NULL == stts_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
	memset(stts_item,0,box_length);
	stts_item->header.size = htonl(box_length);
	strncpy(stts_item->header.type,"stts",4);
	memcpy(&stts_item->header.version,STTS,sizeof(STTS));

	return stts_item;
}


//和stts box的初始化一样的
stsc_box*	stsc_box_init()
{

	stsc_box *stsc_item = (stsc_box *)stts_box_init();
	if(NULL == stsc_item)
	{
		ERROR_LOG("stsc box init failed!\n");
		return NULL;
	}
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
	stsz_box* stsz_item = (stsz_box*)malloc(box_length);
	if(NULL == stsz_item)
	{
		ERROR_LOG("malloc failed!\n");
		return NULL;
	}
	memset(stsz_item,0,box_length);
	stsz_item->header.size = htonl(box_length);
	strncpy(stsz_item->header.type,"zsts",4);
	memcpy(&stsz_item->header.version,STSZ,sizeof(STSZ));

	return stsz_item;

}

//和stts box的初始化一样的
stco_box*	stco_box_init()
{
	stco_box *stco_item = (stco_box *) stts_box_init();
	if(NULL == stco_item)
	{
		ERROR_LOG("stsc box init failed!\n");
		return NULL;
	}
	return stco_item;
}
	
/****七级BOX*********************************************************/

int url_box_init()
{
	return 0; //不需要
}
int	urn_box_init()
{
	return 0;//不需要
}
avc1_box* avc1_box_init()
{
		//已经在VideoSampleEntry里边实现
		//包括 avcc box 
}

mp4a_box* mp4a_box_init()
{
		//已经在AudioSampleEntry里边实现
		//包括 esds box
}


/*
	 该函数只能用来接收 SPS PPS SEI 类型的 NALU
	 并且该nalu需要用4字节的数据大小头替换原有的起始头，并且数据大小为big-endian格式
*/
avcc_box_info_t *	avcc_box_init(unsigned char* naluData, int naluSize)
{

	if(NULL == naluData)
	{
		ERROR_LOG("Illegal parameter!\n");
		return NULL;
	}
	
	unsigned int offset = 5;//略过前5个字节
	unsigned char configurationVersion =  naluData[offset+0];  // configurationVersion
	unsigned char AVCProfileIndication =  naluData[offset+1];  // AVCProfileIndication
	unsigned char profile_compatibility = naluData[offset+2];  // profile_compatibility
	unsigned char AVCLevelIndication  =   naluData[offset+3];  // AVCLevelIndication

		
	unsigned char reserved6 = 0xfc; //二进制：1111 1100
	unsigned char _naluLengthSize = (naluData[offset+4] & 3) + 1;  // lengthSizeMinusOne   上一个avc 长度
	unsigned char reserved_6_lengthSizeMinusOne_2 = reserved6|_naluLengthSize;//reserved_6_lengthSizeMinusOne_2

	unsigned char reserved3 = 0xe0;//二进制：1110 0000
	unsigned char spsCount = naluData[offset+5] & 31; // numOfSequenceParameterSets
	unsigned char reserved_3_numOfSequenceParameterSets_5 = reserved3|spsCount;  
	
	offset = offset + 6;
	if (spsCount == 0 || spsCount > 1) 
	{
		ERROR_LOG("Invalid H264 SPS count(%d)\n",spsCount);
        return NULL;
    }

	
	//一般就一个sps
		unsigned short sps_len = (unsigned short)naluData[offset];
		unsigned short sequenceParameterSetLength = htons(sps_len);  // sequenceParameterSetLength
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
		memcpy(sps,naluData + offset,sps_len);
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

	
		unsigned char ppsCount = naluData[offset];  // numOfPictureParameterSets
		unsigned char numOfPictureParameterSets = ppsCount;
        if (ppsCount == 0 || ppsCount > 1) 
		{
			ERROR_LOG("Invalid H264 PPS count(%d)\n",ppsCount);
            return NULL;
        }

        offset++;

		//一般也就1个pps
        unsigned int pps_len = (unsigned short)naluData[offset]; 
		unsigned short pictureParameterSetLength = htons(pps_len); // pictureParameterSetLength
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
		memcpy(pps,naluData + offset,pps_len);
	
        // pps is useless for extracting video information
        offset += pps_len;

		unsigned int box_len = sizeof(BoxHeader_t) + offset - 5;
		avcc_box* avcc_item = (avcc_box*)malloc(box_len);
		if(NULL == avcc_item)
		{
			ERROR_LOG("malloc failed !\n");
			free(sps);
			free(pps);
			return NULL;
		}
		memset(avcc_item,0,box_len);

		avcc_item->header.size = htonl(box_len);
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
		
		avcc_box_info.avcc_buf = avcc_item;
		avcc_box_info.buf_length = box_len;
	return &avcc_box_info;
	
}














