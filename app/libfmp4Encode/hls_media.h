/*
 * Definition of media types and handlers
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

#ifndef __HLS_MEDIA__
#define __HLS_MEDIA__

#include "hls_file.h"


//this is values for codec member of track_t
#define MPEG_AUDIO_L3  0x04
#define MPEG_AUDIO_L2  0x03
#define AAC_AUDIO		0x0F
#define H264_VIDEO		0x1B


#define KEY_FRAME_FLAG 1

typedef struct track_t{
	int 	codec;				//编解码类型：0x0F：mp4a（代表该trak为音频trak信息）;		0x1B：avc1（代表该trak为视频trak信息）;   		0：啥都不是
	int 	n_frames;			//该文件中对应trak的帧数目
	int 	bitrate;
	float*	pts;
	float*	dts;
	int 	repeat_for_every_segment;
	int*	flags;			//关键帧标记（stss box解析得来）
	int 	sample_rate;	//采样率
	int 	n_ch;			//声道数
	int 	sample_size; 	//PCM Sample size for audio
	int 	data_start_offset;
	int 	type;
} track_t;

typedef struct media_stats_t{
	int n_tracks;
	track_t* track[2];
} media_stats_t;


//一个TS文件对应从trak中取出帧数据的描述结构
typedef struct track_data_t{
	int 	n_frames; 		//取出的帧总数
	int 	first_frame; 	//当前TS分片对应帧区间的开始位置（下标）
	char* 	buffer;			//实际帧数据的开始位置
	int		buffer_size;	//实际帧数据的总大小
	int*	size;			//帧的大小信息的指针（理解为数组的指针）
	int* 	offset;			//帧的偏移量信息的指针（理解为数组的指针）（相较于 buffer/TS文件 开始位置 ）
	int 	frames_written; //记录已经写入到TS文件（缓冲区）的帧数
	int 	data_start_offset;
	int 	cc;				//mpeg2 ts continuity counter
} track_data_t;

typedef struct media_data_t{
	int n_tracks;
	track_data_t* track_data[2];
}media_data_t;

typedef struct media_handler_t{
	int (*get_media_stats)(void* context, file_handle_t* handle, file_source_t* file, media_stats_t* output_buffer, int output_buffer_size );
	int (*get_media_data)(void* context, file_handle_t* handle, file_source_t* file, media_stats_t* stats, int piece, media_data_t* output_buffer, int output_buffer_size );
} media_handler_t;

media_handler_t* get_media_handler(char* filename);

#endif


