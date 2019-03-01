/*
 * Implementation of MPEG2 Transport stream muxer and playlist generation functions
 * Copyright (c) 2012-2013 Voicebase Inc.
 *
 * Author: Alexander Ustinov
 * email: alexander@voicebase.com
 *
 * This file is the part of the mod_hls apache module
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser General Public License version 3. See the LICENSE file
 * at the top of the source tree.
 *
 */
#include "hls_mux.h"
#include "string.h"
#include "math.h"
#include <arpa/inet.h>
#include "crc.h"
#include "bitstream.h"
#include "mod_conf.h"
#include <stdlib.h>
#include <stdio.h>
#include "my_inet.h"
#include "typeport.h"
 
#define PID_PMT		0x0100
#define PID_AUDIO	0x0101
#define PID_VIDEO  0x0102
#define PES_VIDEO_STREAM 0x1B
#define PES_AUDIO_STREAM 0x04
#define TRANSPORT_STREAM_ID 0x0001
#define PROGRAM_NUMBER 0x0001

int get_num_of_mp3_frames(unsigned char* buf, int size, int sr, int br, int* frm_size, int* frm_offset){
	int i = 0;
	int rfs = 0;
	int fs = 1152;
	if (sr == 32000 || sr == 44100 || sr == 48000)
		fs = 1152;
	else
		fs = 576;

	while (rfs < size){
		int sz = fs * (br) / (sr * 8) + ((buf[rfs + 2] & 2) >> 1);
		if (frm_size)
			frm_size[i] = sz;
		if (frm_offset)
			frm_offset[i] = rfs;
		rfs += sz;
		++i;
	}
	return i;
}


//return number of header bytes
static int generate_ts_header(char* out_buf, int out_buf_size, int cont_count, int payload_unit_start, int need_pcr,
								  double fpcr, int pid, int discontiniuty, int payload_size){
	PutBitContext bs;
	int pos;
	long long pcr;
	int stuffing_size = 0;
	int i;
	int adaptation_field = 0;

	if (payload_size < 184 || need_pcr || discontiniuty){
		adaptation_field = 3;
	}else{
		adaptation_field = 1;
	}
	if (payload_size == 0)
		adaptation_field = 2;


	init_put_bits(&bs, out_buf, out_buf_size);
	put_bits(&bs, 8, 0x47); //sync byte
	//put_bits(&bs, 4, 0x7); //sync byte
	put_bits(&bs, 1, 0x00); //error indicatior
	put_bits(&bs, 1, payload_unit_start);
	put_bits(&bs, 1, 0x00); //transport pget_frame_durationriority
	put_bits(&bs, 13, pid ); //pid
	put_bits(&bs, 2, 0x00); //scrambling control
	put_bits(&bs, 2, adaptation_field); //adaptation
	put_bits(&bs, 4, cont_count & 0x0F); //continiuty counter

	if (adaptation_field == 2 || adaptation_field == 3){
		int adaptation_field_length = 0;
		if (discontiniuty || need_pcr || payload_size < 184 - adaptation_field_length)
			++adaptation_field_length;
		if (need_pcr)
			adaptation_field_length += 6;
		if (payload_size < 184 - adaptation_field_length){
			stuffing_size = 184 - 1 - adaptation_field_length - payload_size;
			adaptation_field_length += stuffing_size;
		}

		put_bits(&bs, 8, adaptation_field_length);//adaptation field length
		///TODO:need to calcuate
		if (adaptation_field_length > 0){
			put_bits(&bs, 1, discontiniuty);//discontinuity indicator
			put_bits(&bs, 1, 0);//random access indicator
			put_bits(&bs, 1, 0);//ES priority indicator
			put_bits(&bs, 1, need_pcr);//PCR flag
			put_bits(&bs, 1,0x00);//OPCR flag
			put_bits(&bs, 1,0x00);//splicing point flag
			put_bits(&bs, 1,0x00);//transport private data flag
			put_bits(&bs, 1,0x00);//adaptation field extention flag
			if (need_pcr){
				pcr = fpcr * 27000000LL;
				put_bits64(&bs, 33,(pcr / 300LL) & 0x01FFFFFFFFLL);//program clock reference base
				put_bits(&bs, 6,0x3F);//reserved
				put_bits(&bs, 9,(pcr % 300) & 0x01FFLL);//program clock reference ext
			}
			//here we have to put stuffing byte
			for(i = 0; i < stuffing_size; i++){
				put_bits(&bs, 8, 0xFF);
			}
		}
	}

	flush_put_bits(&bs);
	return put_bits_count(&bs)/8;
}


static int generate_pes_header(char* out_buf, int out_buf_size, int data_size, double fpts, double fdts, int es_id){
	PutBitContext bs;

	init_put_bits(&bs, out_buf, out_buf_size);
	int pts_dts_length = 5;
	long long pts;
	long long dts;

	pts = (fpts + 0.300) * 90000LL;
	dts = (fdts + 0.300) * 90000LL;

	if (abs(pts-dts) > 10)
		pts_dts_length += 5;

	//pes packet start
	put_bits(&bs, 24,0x0000001);// pes packet start code

	put_bits(&bs, 8,	es_id);					// stream id
	if (es_id >= 0xE0)
		put_bits(&bs, 16,	0);	// pes packet length
	else
		put_bits(&bs, 16,	data_size + 3 + pts_dts_length);	// pes packet length


	put_bits(&bs, 2, 0x02);			// have to be '10b'
	put_bits(&bs, 2, 0x00);			// pes scrambling control
	put_bits(&bs, 1, 0x00);			// pes priority
	put_bits(&bs, 1, 0x01);			// data alignment
	put_bits(&bs, 1, 0x00);			// copyright
	put_bits(&bs, 1, 0x00);			// original or copy

	if (pts_dts_length > 5){
		put_bits(&bs, 2, 0x03);			// pts/dts flags we have only pts
	}else{
		put_bits(&bs, 2, 0x02);
	}
	put_bits(&bs, 1, 0x00);			// escr flag
	put_bits(&bs, 1, 0x00);			// es rate flag
	put_bits(&bs, 1, 0x00);			// dsm trick mode flag
	put_bits(&bs, 1, 0x00);			// additional copy info flag
	put_bits(&bs, 1, 0x00);			// pes crc flag
	put_bits(&bs, 1, 0x00);			// pes extention flag
	put_bits(&bs, 8, pts_dts_length);			// pes headder data length

	if (pts_dts_length > 5){
		put_bits(&bs, 4,  0x03);// have to be '0011b'
		put_bits(&bs, 3,  (pts >> 30) & 0x7);// pts[32..30]
		put_bits(&bs, 1,  0x01);// marker bit
		put_bits(&bs, 15, (pts >> 15) & 0x7FFF);// pts[29..15]
		put_bits(&bs, 1,  0x01);// marker bit
		put_bits(&bs, 15, pts & 0x7FFF);// pts[14..0]
		put_bits(&bs, 1,  0x01);// marker bit

		put_bits(&bs, 4,  0x01);// have to be '0011b'
		put_bits(&bs, 3,  (dts >> 30) & 0x7);// pts[32..30]
		put_bits(&bs, 1,  0x01);// marker bit
		put_bits(&bs, 15, (dts >> 15) & 0x7FFF);// pts[29..15]
		put_bits(&bs, 1,  0x01);// marker bit
		put_bits(&bs, 15, dts & 0x7FFF);// pts[14..0]
		put_bits(&bs, 1,  0x01);// marker bit

	}else{
		put_bits(&bs, 4,  0x02);// have to be '0010b'

		put_bits(&bs, 3,  (pts >> 30) & 0x7);// pts[32..30]
		put_bits(&bs, 1,  0x01);// marker bit
		put_bits(&bs, 15, (pts >> 15) & 0x7FFF);// pts[29..15]
		put_bits(&bs, 1,  0x01);// marker bit
		put_bits(&bs, 15, pts & 0x7FFF);// pts[14..0]
		put_bits(&bs, 1,  0x01);// marker bit
	}


	flush_put_bits(&bs);
	return put_bits_count(&bs)/8;

}

void pack_pcr(char* ts_buf, int* frame_count, int* cc, double start_time, int pid){
	char ts_header_buffer[188];
	int ts_header_size = generate_ts_header(ts_header_buffer, sizeof(ts_header_buffer), cc[0], 0,	1, start_time, pid, 0, 0);
	memcpy(ts_buf + 188 * frame_count[0], ts_header_buffer, ts_header_size);
	++frame_count[0];
	++cc[0];
}

void pack_data(char* ts_buf, int* frame_count, int* cc, double pts, double dts, int es_id, int pid, char* data, int frame_size, int pcr_pid)
{
	char pes_header_buffer[128];
	char ts_header_buffer[188];

	int pes_header_size = 0;
	int ts_header_size;
	int pos = 0;
	if (frame_size > 0){
		pes_header_size = generate_pes_header(pes_header_buffer, sizeof(pes_header_buffer), frame_size, pts, dts, es_id);
	}
	pos = 0;

	if ( pcr_pid){
		ts_header_size = generate_ts_header(ts_header_buffer, sizeof(ts_header_buffer), cc[0], 1,
												1, dts, pid, 0, frame_size + pes_header_size);
	}else{
		ts_header_size = generate_ts_header(ts_header_buffer, sizeof(ts_header_buffer), cc[0], 1,
												0, 0, pid, 0, frame_size + pes_header_size);
	}

	memcpy(ts_buf + 188 * frame_count[0], ts_header_buffer, ts_header_size);
	memcpy(ts_buf + 188 * frame_count[0] + ts_header_size, pes_header_buffer, pes_header_size);
	memcpy(ts_buf + 188 * frame_count[0] + ts_header_size + pes_header_size, data, 188 - ts_header_size - pes_header_size);

	++frame_count[0];
	++cc[0];

	pos += 188 - ts_header_size - pes_header_size;

	while (pos < frame_size){

//		ts_header_size = generate_ts_header(ts_header_buffer, sizeof(ts_header_buffer), cc[0],
	//										0, 1, start_time + pos / 8000.0, pid, 0, frame_size - pos);
		ts_header_size = generate_ts_header(ts_header_buffer, sizeof(ts_header_buffer), cc[0],
													0, 0, 0, pid, 0, frame_size - pos);

		memcpy(ts_buf + 188 * frame_count[0], ts_header_buffer, ts_header_size);
		memcpy(ts_buf + 188 * frame_count[0] + ts_header_size , data + pos, 188 - ts_header_size);

		pos += 188 - ts_header_size;
		++frame_count[0];
		++cc[0];
	}
}


/*
选择 lead_track
释：lead_track优先级：video trak > audio trak > other trak 
返回值：
	对应lead_track的数组下标（stats-->trak[i]中的i）
*/
int find_lead_track(media_stats_t* stats)
{
	int i;
	int result = -1; //lead_track的搜索结果
	/*在trak数组中，查找video trak 的下标*/
	for(i = 0; i < stats->n_tracks; ++i)
	{
		if (stats->track[i]->codec == H264_VIDEO && stats->track[i]->repeat_for_every_segment == 0)
		{
			result = i;
			break;
		}
	}
	
	if (result < 0)//没有找到video trak ，将音频 trak 设置为lead_track
	{
		for(i = 0; i < stats->n_tracks; ++i)
		{
			if (stats->track[i]->codec == MPEG_AUDIO_L3 ||
				stats->track[i]->codec == MPEG_AUDIO_L2 ||
				stats->track[i]->codec == AAC_AUDIO)
			{

				result = i;
				break;
			}
		}
	}
	
	if (result < 0)//音视频trak都没找到，默认第0条trak为lead_track
	{
		result = 0;
	}
	return result;
}

/*
功能：依据传入的TS分片的序号计算所对应帧的区间起始位置和结束位置。
参数：
	@piece：当前TS切片文件的序号
	@sf: 区间的起始位置
	@eg: 区间的结束位置
	@recommended_length：（输入）我们设置的（希望的）TS文件切片时长
返回：区间下标之差
*/
int get_frames_in_piece_for_track(media_stats_t* stats, int piece, int track, int* sf, int* ef, int recommended_length)
{
	float csp = 0; //current segment pos （值为当前帧dts的值，segments可理解为帧）
	int cspi = 0;//current segment index pos

	float* ts;	//dts/pts
	float max_len = 0;

	int n_frames = stats->track[track]->n_frames;
	
	
	int i;
	int ns = 0;//num of segments （已经切片好的TS文件块数）
	int* flags = stats->track[track]->flags;

	ts = stats->track[track]->dts;

	if (ts == NULL)
	{
		ts = stats->track[track]->pts;
	}
	
	DEBUG_LOG("piece(%d) n_frames = %d recommended_length = %d ts[0] = %f ts[n_frames-1] = %f\n",
				piece,n_frames,recommended_length,ts[0],ts[n_frames-1]);
	
	/*---寻找当前序号（piece）的TS文件，对应到mp4文件中的帧，的下标区间位置-------*/
	for(i = 0; i < n_frames; ++i)
	{
		//DEBUG_LOG("flags[i] = %d\n",flags[i]);
		
		if (ts[i] - csp > recommended_length && (flags[i] & KEY_FRAME_FLAG) )//时长达到recommended_length + 当前帧是关键帧（被归入到下一个TS文件中）
		{
			if (ns == piece)//该部分只会进一次
			{
				*sf = cspi;//区间的起始位置
				*ef = i;//区间的结束位置
			}
			//重新定位开始和结束位置
			csp = ts[i];
			cspi = i;
			++ns; //完成一个分片的划分，片数+1
		}
	}
	//最后出来时csp为最后一个关键帧的dts值；cspi为最后一个关键帧的下标值

	DEBUG_LOG("AT position AAAA\n");

	//处理最后不够分一个TS分片文件的部分帧
	if (cspi != n_frames - 1)
	{
		if (ns == piece)//将他们归到最后的这个TS分片中
		{
			*sf = cspi;
			*ef = n_frames - 1;
		}
		++ns;
	}

	DEBUG_LOG("AT position AAAA1 *ef(%d) - *sf(%d) = %d \n",*ef,*sf,*ef - *sf);
	return (int )(*ef - *sf);
}



/*
功能：传入当前TS文件的序号（piece），获取所对应帧的起始和结束位置
参数：
	@stats：trak信息描述结构的指针（文件的所有trak信息）
	@piece：TS文件的分序号
	@track：trak序号
	@sf：（输出）帧区间开始位置
	@ef：（输出）帧区间结束位置
	@recommended_length：设置的TS文件时长
返回：区间下标之差
*/
int flag = 0;
int get_frames_in_piece(media_stats_t* stats, int piece, int track, int* sf, int* ef, int recommended_length)
{

	int lead_track = find_lead_track(stats);

	int nsf = 0; //帧区间开始位置（lead track）
	int nef = 0; //帧区间结束位置（lead track）

	/*
	如果当前trak是lead_trak,则直接返回在lead_trak里边找到的区间
	如果当前trak不是lead_trak,则需要参照lead_trak的时间区间来对当前trak进行区间分割
	*/
	if (track == lead_track)
	{
		return get_frames_in_piece_for_track(stats, piece, lead_track, sf, ef, recommended_length);
	}
	
	//获取lead trak的对应TS分片的帧区间信息(开始结束的下标区间)
	get_frames_in_piece_for_track(stats, piece, lead_track, &nsf, &nef, recommended_length);

	DEBUG_LOG("at position 406\n");

	float csp = 0; //current segment pos
	int cspi = 0;//current segment index pos

	float* ts;		//dts/pts (当前的trak，不一定是lead_trak)
	float max_len = 0;

	int n_frames = stats->track[track]->n_frames;
	int i;
	int ns = 0;//num of segments
	int* flags = stats->track[track]->flags;
	
	DEBUG_LOG("at position 407\n");
	
	ts = stats->track[track]->dts;
	if (ts == 0)
	{
		ts = stats->track[track]->pts;
	}

	DEBUG_LOG("at position 408\n");
	
	float* lts = stats->track[lead_track]->dts;//lead_trak的 dts/pts
	if (lts == 0)
	{
		lts = stats->track[lead_track]->pts;
	}

	DEBUG_LOG("FFFFFFFFFFFFF\n");


	//DEBUG_LOG("nsf = %d nef = %d\n",nsf,nef);
	//DEBUG_LOG("at position 409 n_frames(%d) lts[nsf] = %f lts[nef] = %f\n",n_frames,lts[nsf],lts[nef]);
	
	//sleep(3); 睡眠后死掉了

	/*---参照lead_trak的时间区间来对当前trak进行区间分割------------*/
	*sf = 0;
	
	DEBUG_LOG("back of *sf = 0 !\n");
	for(i = 0; i < n_frames; ++i)
	{
		DEBUG_LOG("ts[%d] = %f\n",i,ts[i]);
		if (ts[i] <= lts[nsf])//在当前trak中，找到对应TS区间的起始位置
		{
			*sf = i;
		}

		if (ts[i] < lts[nef])//在当前trak中，找到对应TS区间的结束位置
		{
			*ef = i;
		}
	}
	DEBUG_LOG("at position 410 *ef - *sf = %d\n",*ef - *sf);
	return *ef - *sf;
}

/*
获取可生成的TS文件的总个数
参数：
	@max_piece_len：（输出）单个TS切片文件的最大时长
	@recommended_length：（输入）我们设置的（希望的）TS文件切片时长
	
返回： 能够分片成TS文件的数量
*/
int get_number_of_pieces(media_stats_t* stats, float* max_piece_len, int recommended_length)
{
	float csp = 0; //current segment pos （值为当前帧dts的值，segments可理解为帧）

	int lead_track = find_lead_track(stats);
	float* ts; //dts
	float max_len = 0;//记录实际TS分片时，TS时长的最大值

	int n_frames = stats->track[lead_track]->n_frames; //lead trak 的总帧数
	int i;
	int ns = 0;//num of segments  （已经切片好的TS文件块数）
	int* flags = stats->track[lead_track]->flags;

	ts = stats->track[lead_track]->dts;
	if (ts == 0)
	{
		ts = stats->track[lead_track]->pts;
	}

	//计算截取推荐时长（我们设置的TS文件时长）的帧数，一共需要的帧数 ns 
	for(i = 0; i < n_frames; ++i)
	{
		if (ts[i] - csp > recommended_length && (flags[i] & KEY_FRAME_FLAG) )
		{
			if (ts[i] - csp > max_len)
				max_len = ts[i] - csp; //重新修正 max_len
			csp = ts[i];
			++ns; //TS文件块数增加1
		}
	}//该循环出来后，i为n_frames;注意：总帧数不一定恰好凑成recommended_length的整数倍，很可能不够

	/*---将最后剩余的帧作为最后一个TS切片（不足以凑成一个整的recommended_length 长度的TS文件）-------------*/
	if (fabs(csp - ts[n_frames - 1]) > 1e-4)//ts[n_frames - 1]:最后一个帧的 dts信息 ;
	{
		if (ts[i] - csp > max_len) //该条件好像不会满足了 ts[i] - csp 为剩余的时长
			max_len = ts[i] - csp;
		++ns;
	}
	if (max_piece_len)
		*max_piece_len = max_len;

	return ns;

}

/*
m3u8文件的生成函数，输出到output_buffer
返回：output_buffer 的使用量（偏移量）
*/
int generate_playlist(media_stats_t* stats, char* filename_template, char* output_buffer, int output_buffer_size, char* url, int** numberofchunks)
{
	float csp = 0; //current segment pos

	int lead_track;
	float* ts;		//dts/pts

	int n_frames;
	int i;
	int ns;//num of segments
	int* flags;
	int recommended_length;
	float max_piece_len;
	char* out;
	int ci;  //记录（m3u8）输出缓存的偏移量
	float seg_len;

	if ( !output_buffer || !output_buffer_size )//传入了空指针或者缓存buf大小为0，则计算应分配的缓存buf大小，返回该数值
	{

		max_piece_len = 0.0f;
		int n_of_pieces = get_number_of_pieces(stats, &max_piece_len, get_segment_length());
		int size = 0;
		int i;
		**numberofchunks=n_of_pieces;
		size = strlen("#EXTM3U\n" "#EXT-X-TARGETDURATION:000\n" "#EXT-X-VERSION:3\n" "#EXT-X-MEDIA-SEQUENCE:0\n" "#EXT-X-PLAYLIST-TYPE:VOD\n" "#EXT-X-ENDLIST\n");
		size += strlen("#EXTINF:000.000000,\n" "_000000.ts  \n") * n_of_pieces;
		if (url)
		{
			size += (strlen("?source=") + strlen(url)) * n_of_pieces;
		}

		return size + strlen(filename_template) * n_of_pieces;
	}

	csp = 0; //current segment pos
	lead_track = find_lead_track(stats);
	n_frames = stats->track[lead_track]->n_frames;
	ns = 0;//num of segments
	flags = stats->track[lead_track]->flags;
	recommended_length = get_segment_length();// we set reccomended duration to 5
	max_piece_len = 0.0f;

	get_number_of_pieces(stats, &max_piece_len, recommended_length); //只为了获得单个TS文件的最大时长

	ci = 0;
	out = output_buffer;

	ci += sprintf(out + ci, "#EXTM3U\n");
	ci += sprintf(out + ci, "#EXT-X-TARGETDURATION:%d\n", ((int)max_piece_len) + 1);
	ci += sprintf(out + ci, "#EXT-X-VERSION:3\n");
	ci += sprintf(out + ci, "#EXT-X-MEDIA-SEQUENCE:0\n");
	ci += sprintf(out + ci, "#EXT-X-PLAYLIST-TYPE:VOD\n");

	ts = stats->track[lead_track]->dts;
	if (ts == 0)
	{
		ts = stats->track[lead_track]->pts;
	}

	for(i = 0; i < n_frames; ++i)
	{
		seg_len = ts[i] - csp;
		if (seg_len > recommended_length && (flags[i] & KEY_FRAME_FLAG) )//满足时长的前提下，还要保证
		{
			float csl = ts[i] - csp;
			ci += sprintf(out + ci, "#EXTINF:%f,\n", (float)csl); //描述当前TS片的时长
			ci += sprintf(out + ci, "%s_%d.ts\n", filename_template, ns);//描述当前TS片的文件名

			csp = ts[i];
			++ns;
		}
	}

	seg_len = ts[n_frames - 1] - csp; //最后剩余的那部分帧的时长（内部时长）
	if (seg_len > 1e-4)
	{
		ci += sprintf(out + ci, "#EXTINF:%f,\n", (float)seg_len);//描述最后一片TS片的时长

//		if (url)
//			ci += sprintf(out + ci, "%s_%d.ts?source=%s\n", filename_template, ns, url);
//		else
			ci += sprintf(out + ci, "%s_%d.ts\n", filename_template, ns);//描述最后一片TS片的文件名

		++ns;
	}

	ci += sprintf(out + ci,"#EXT-X-ENDLIST\n"); //写入 endlist （注意，这样就不是live m3u8模式）

	return ci;
}

int put_pat(char* buf, media_stats_t* stats, int* pat_cc)
{
	PutBitContext bs;
	int pos;
	long long pcr;
	int stuffing_size;
	int i;
	unsigned char tmp[188] = {0xFF};

	int pat_size = 0;
	int header_size = 0;

	init_put_bits(&bs, tmp, 188);
	put_bits(&bs, 8, 0);//table offset
	put_bits(&bs, 8, 0);//table id PAT

	put_bits(&bs, 1, 0x01);//section syntax indicator
	put_bits(&bs, 1, 0);//have to be 0
	put_bits(&bs, 2, 0x03);//reserved
	put_bits(&bs, 12, (16 + 2 + 5 + 1 + 8 + 8 + 16 + 3 + 13 + 32)/8);//section length
	put_bits(&bs, 16, TRANSPORT_STREAM_ID);//transport stream id
	put_bits(&bs, 2, 0x03);//reserved
	put_bits(&bs, 5, 0x00);//version number
	put_bits(&bs, 1, 0x01);//current next indicator
	put_bits(&bs, 8, 0x00);//section number
	put_bits(&bs, 8, 0x00);//last section number


		put_bits(&bs, 16, PROGRAM_NUMBER);//program number
		put_bits(&bs, 3, 0x07);//reserved
		put_bits(&bs, 13, PID_PMT);//program map id

	put_bits(&bs, 32, t_htonl (crc(crc_get_table(CRC_32_IEEE), -1, (const uint8_t*)(&tmp[0]) + 1,  put_bits_count(&bs)/8 - 1) ));

	pat_size = put_bits_count(&bs)/8;
	memset(&tmp[pat_size], 0xff, 188-pat_size);

	header_size = generate_ts_header(buf, 188, pat_cc[0], 1, 0, 0, 0, 0, 184);
	memcpy(buf + header_size, tmp, 184);
	++pat_cc[0];
	return 188;
}

int put_pmt(char* buf, media_stats_t* stats, int* pmt_cc, int pcr_track){
	PutBitContext bs;
	int pos;
	long long pcr;
	int stuffing_size;
	int i;
	unsigned char tmp[188];
	int pmt_size = 0;
	int header_size = 0;

	init_put_bits(&bs, tmp, 188);

	//	CBitstream bs;
	//	bs.Open(result->ptr);
	put_bits(&bs, 8, 0);//table offset
	put_bits(&bs, 8, 0x02);//table id for PMT
	put_bits(&bs, 1, 0x01);//section syntax indicator
	put_bits(&bs, 1, 0x00);//0
	put_bits(&bs, 2, 0x03);//reserved
	put_bits(&bs, 12, (16 + 2 + 5 + 1 + 8 + 8 + 16 + 16 + (8 + 16 + 16)*stats->n_tracks + 32)/8);//section length
	put_bits(&bs, 16, PROGRAM_NUMBER);//program number
	put_bits(&bs, 2, 0x03);//reserved
	put_bits(&bs, 5, 0x00);//version number
	put_bits(&bs, 1, 0x01);//current next indicator
	put_bits(&bs, 8, 0x00);//section number
	put_bits(&bs, 8, 0x00);//last section number

	put_bits(&bs, 3, 0x07);//reserved
	put_bits(&bs, 13, PID_PMT + stats->n_tracks - pcr_track);//PCR PID

	put_bits(&bs, 4, 0x0F);//reserved
	put_bits(&bs, 12, 0x0000);//program info length

	for(i = stats->n_tracks - 1; i >= 0; --i){
		put_bits(&bs, 8, stats->track[i]->codec);//stream type
		put_bits(&bs, 3, 0x07);//reserved
		put_bits(&bs, 13, PID_PMT + stats->n_tracks - i);//elementary PID
		put_bits(&bs, 4, 0x0F);//reserved
		put_bits(&bs, 12, 0x0000);//es info lenght
	}

	put_bits(&bs, 32, htonl( crc(crc_get_table(CRC_32_IEEE), -1,  (const uint8_t*)(&tmp[0]) + 1,  put_bits_count(&bs)/8 - 1) ));//here we calculate CRC of section without first Pointer byte

	pmt_size = put_bits_count(&bs)/8;

	memset(&tmp[pmt_size], 0xff, 188- pmt_size);

	header_size = generate_ts_header(buf, 188, pmt_cc[0], 1, 0, 0, PID_PMT, 0, 184);
	memcpy(buf + header_size, tmp, 184);
	++pmt_cc[0];
	return 188;
}

int put_data_frame(char* buf, media_stats_t* stats, media_data_t* data, int track, int lead_track, int num_of_frames)
{

	int fc =  0;
	int first_frame = data->track_data[track]->first_frame;
	int fn = data->track_data[track]->frames_written;
	char* data_buf = data->track_data[track]->buffer + data->track_data[track]->offset[fn];
	int data_buf_size = 0;
	int i;
	double pts;
	double dts;

	for(i = 0; i < num_of_frames && ((i + fn) < data->track_data[track]->n_frames); ++i)
	{
		data_buf_size += data->track_data[track]->size[fn + i];
	}

	int es_id = 0xC0;

	if (stats->track[track]->codec == H264_VIDEO)
		es_id = 0xE0;

	pts = stats->track[track]->pts[first_frame + fn];
	dts = stats->track[track]->dts[first_frame + fn];

	if (stats->track[track]->repeat_for_every_segment)
	{
		pts += stats->track[lead_track]->pts[data->track_data[lead_track]->first_frame];
		dts += stats->track[lead_track]->dts[data->track_data[lead_track]->first_frame];
	}

	pack_data(buf, &fc, &data->track_data[track]->cc, pts, dts, es_id, PID_PMT + stats->n_tracks - track,
			data_buf,data_buf_size, track == lead_track ? 1 : 0);

	data->track_data[track]->frames_written += num_of_frames;

	return fc * 188;//return number of written bytes
}

int find_video_track(media_stats_t* stats){
	int i;
	for(i = 0; i < stats->n_tracks; ++i){
		if (stats->track[i]->codec == H264_VIDEO)
			return i;
	}
	return -1;
}

int select_current_track(media_stats_t* stats, media_data_t* data){
	int ct = -1;
	float min_time = 1000000000.0;
	int  i;

	for(i = 0; i < stats->n_tracks; ++i)
	{
		track_data_t* td = data->track_data[i];
		track_t* ts = stats->track[i];
		float* pts = ts->dts;
		if ( !pts )
			pts = ts->pts;


		if (td->frames_written + td->first_frame < ts->n_frames && td->frames_written < td->n_frames)
		{
			if (pts[td->first_frame + td->frames_written] < min_time)
			{
				min_time = pts[td->first_frame + td->frames_written];
				ct = i;
			}
		}
	}
	return ct;
}


int mux_to_ts(media_stats_t* stats, media_data_t* data, char* output_buffer, int output_buffer_size)
{
	int i;
	int j;
	int size;
	int pos;
	int pat_cc;
	int pmt_cc;
	int lead_track;
	int video_track;

	/*---初始化需要的内存空间大小--------------------------------------*/
	if ( !output_buffer || ! output_buffer_size )
	{
		DEBUG_LOG("into position K1\n");	
		size = 0;
		for(i = 0; i < stats->n_tracks; ++i)
		{
			for(j = 0; j < data->track_data[i]->n_frames; ++j)
			{
				size += data->track_data[i]->size[j];
			}
		}
		return size * 4.0 + 20000; //we estimate transport stream overhead 20% and should add buffer if data amount too low
	}
	/*-----------------------------------------------------------------*/

	DEBUG_LOG("into position K2\n"); 

	
	lead_track = find_lead_track(stats);
	video_track = find_video_track(stats);
	pos = 0;
	pat_cc = 0;
	pmt_cc = 0;


	//put pat/pmt
	//put video frame
	if (video_track >= 0)
	{
		DEBUG_LOG("into position K3\n");	
		pos += put_pat(output_buffer + pos, stats, &pat_cc);
		pos += put_pmt(output_buffer + pos, stats, &pmt_cc, lead_track);

		pos += put_data_frame(output_buffer + pos, stats, data, lead_track, lead_track, 1);
//		if (video_track != lead_track)
//			pos += put_data_frame(output_buffer + pos, stats, data, video_track, lead_track, 1);

	}

	//put other frames
	while(1)
	{
		DEBUG_LOG("into position K4\n");	
		int ct = select_current_track(stats, data);
		if (ct < 0)
			break;

//		track_data_t* td = data->track_data[ct];
//		track_t* ts = stats->track[ct];

//		if (ct == lead_track){
//			pos += put_pat(output_buffer + pos, stats, &pat_cc);
//			pos += put_pmt(output_buffer + pos, stats, &pmt_cc, lead_track);
//		}

		pos += put_data_frame(output_buffer + pos, stats, data, ct, lead_track, ct != video_track ? 6 : 1);
	}
	DEBUG_LOG("into position K5\n"); 

	return pos;
}



