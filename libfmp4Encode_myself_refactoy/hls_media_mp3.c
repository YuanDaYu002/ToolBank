/*
 * Implementation of mp3 media
 * Copyright (c) 2012-2013 Voicebase Inc.
 *
 * Authors: Alexander Ustinov, Pomazan Nikolay
 * email: alexander@voicebase.com, pomazannn@gmail.com
 *
 * This file is the part of the mod_hls apache module
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser General Public License version 3. See the LICENSE file
 * at the top of the source tree.
 *
 */

#include "hls_media.h"
#include "mod_conf.h"
#include <string.h>
//#include "lame/lame.h"
#include "hls_mux.h"


int get_mpeg_version(unsigned char data){

	data = (data >> 3 ) & 3;

	switch (data){
		case 0:	return 2;
		case 2:	return 1;
		case 3:	return 0;

		case 1:
		default:
			return -1;
	}

	return -1;
}

int get_frame_duration(int sr){
	switch(sr){
		case 44100:
		case 32000:
		case 48000:
			return 1152;
		case 8000:
			return 576;
	}
	return 576;
}

int parse_mp3_header_buffer(unsigned char* data, int buffer_size, int* sample_rate, int* frame_size, int* pframe_duration, int* pbitrate){
	//should return mpeg data offset in file (skip id3 tag)
	if (data[0] == 'I' && data[1] == 'D' && data[2] == '3'){
		int s = 10 + (int)data[6] * 128 * 128 * 128 + (int)data[7] * 128 * 128 + (int)data[8] * 128 + (int)data[9];
		if (s >= buffer_size + 4)
			return -1;

		parse_mp3_header_buffer(data + s, buffer_size - s, sample_rate, frame_size, pframe_duration, pbitrate);

		return s;
	}else{
		int ver = get_mpeg_version(data[1]); //0 - mpeg 1, 1 - mpeg 2, 2 - mpeg 2.5

		if (!(data[0] == 0xFF && ( (data[1] >> 5) & 0x7) == 0x7 && ver >= 0 && ver < 3)){
			return 0;
		}



		int bitrate_table[3][16] = { 	{ 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1  },
										{ 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1},
										{ 0,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, -1} };

		int sample_rate_table[3][4] = {	{ 44100, 48000, 32000, 0 },
										{ 22050, 24000, 16000, 0},
										{ 11025, 12000, 8000, 0}};
		int frame_duration[3]			= {1152, 576,576};

		int mpg_idx 		= (data[1] >> 3) & 0x3;
		int layer_idx 		= (data[1] >> 1) & 0x3;
		int crc_idx 		= (data[1] & 1);
		int bitrate_idx 	= (data[2] >> 4) & 0x0F;
		int sr_idx			= (data[2] >> 2) & 0x03;
		int padding			= (data[2] >> 1) & 0x01;
		int frame_duration_v = frame_duration[ver];
		int bitrate			= bitrate_table[ver][bitrate_idx] * 1000;

		if (pframe_duration)
			*pframe_duration = frame_duration_v;

		if (pbitrate)
			*pbitrate = bitrate;

		if (sample_rate)
			*sample_rate 		= sample_rate_table[ver][sr_idx];

		if (frame_size)
			*frame_size = (frame_duration_v/8 * bitrate / sample_rate_table[ver][sr_idx] + padding);

		return (frame_duration_v/8 * bitrate / sample_rate_table[ver][sr_idx] + padding);
	}
	return 0;
}

int parse_video_file(char *filename, int* n_video_frames);
int load_data_from_file(char* buf, int size, char* filename);

int mp3_media_get_stats(void* context, file_handle_t* handle, file_source_t* source, media_stats_t* output_buffer, int output_buffer_size ){
	char buf[16384];
	int sample_rate;
	int num_of_channels;
	int sample_size;
	int data_start_offset;
	int type;

	int rb;
	double duration;
	int n_frames;
	media_stats_t* stats;
	track_t* track;
	track_t* video_track;
	float* pts;
	float* video_pts;

	int* flags;
	int* video_flags;

	int i;
	char* ptr;
	int n_video_frames;
	int frame_size;
	int file_size;
	int frame_duration;
	int bitrate;
	int probe_size = 256;

	char* video_logo_filename = get_logo_filename();

	file_size 			= source->get_file_size(handle, 0);
	do {
		rb 					= source->read(handle, buf, probe_size, 0, 0);
		data_start_offset 	= parse_mp3_header_buffer(buf, rb, &sample_rate, &frame_size, &frame_duration, &bitrate);
		probe_size *= 2;
	}while(data_start_offset < 0 && probe_size < 16384);

	if (data_start_offset < 0)
		return -1;

	n_frames 			= file_size/(frame_size);
	duration			= frame_duration * n_frames * 1.0 / sample_rate;

	if (!output_buffer || !output_buffer_size){

		n_video_frames = 0;
		if (video_logo_filename)
			parse_video_file(video_logo_filename, &n_video_frames);

		return	sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float) + n_video_frames * sizeof(float) + n_frames * sizeof(int) + n_video_frames * sizeof(int);//return required buffer size for media_stats_t
	}


	n_video_frames = 0;

	ptr 		= (char*)output_buffer;
	stats		= (media_stats_t*)ptr;
	track		= (track_t*)(ptr + sizeof(media_stats_t));
	video_track = (track_t*)(ptr + sizeof(media_stats_t) + sizeof(track_t));
	pts			= (float*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2);
	video_pts 	= (float*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float));
	flags		= (int*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float) + n_video_frames * sizeof(float));
	video_flags = (int*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float) + n_video_frames * sizeof(float) + n_frames * sizeof(int));

	stats->n_tracks = 1;
	stats->track[0]	= track;
	if (video_logo_filename){
		stats->track[1] = video_track;
		stats->n_tracks++;
	}

	track->codec 	= MPEG_AUDIO_L3;
	track->dts	 	= 0;
	track->n_frames = n_frames;
	track->pts		= pts;
	track->bitrate  = bitrate;
	track->repeat_for_every_segment = 0;
	track->sample_rate 			= sample_rate;
	track->n_ch 				= num_of_channels;
	track->sample_size 			= sample_size;
	track->data_start_offset 	= data_start_offset;
	track->type					= type;
	track->flags 				= flags;

	for(i = 0; i < n_frames; ++i){
		pts[i] = (float)(i * frame_duration / (double)(sample_rate));
		flags[i] = KEY_FRAME_FLAG;
	}

	if (video_logo_filename){
		video_track->codec 						= H264_VIDEO;
		video_track->bitrate 					= 0;
		video_track->dts 						= 0;
		video_track->pts 						= video_pts;
		video_track->repeat_for_every_segment 	= 1;
		video_track->flags 						= video_flags;

		parse_video_file(video_logo_filename, &n_video_frames);
		for(i = 0; i < n_video_frames; ++i){
			video_pts[i] = 0.1 * i; //we force 10 fps here
			video_flags[i] = KEY_FRAME_FLAG;
		}
		video_track->n_frames = n_video_frames;
	}

	return sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * (sizeof(float) + sizeof(int)) + n_video_frames * (sizeof(float) + sizeof(int));// return number of bytes
}

int mp3_media_get_data(void* context, file_handle_t* handle, file_source_t* source, media_stats_t* stats, int piece, media_data_t* output_buffer, int output_buffer_size ){
	int start=0;
	int stop=0;
	int n_frames;

	track_data_t* tdata;
	track_data_t* vdata;
	media_data_t* mdata;
	char* ptr;
	int* size;
	int* offset;
	char* buf;

	int n_out_frames;
	int n_out_bytes;
	int i;
	//lame_global_flags* encopts = 0;
	int n_ch;
	int sample_rate;
	int encoding_sample_rate;
	int bit_per_sample;
	int data_offset;
	int type;
	int n_video_frames;
	char* video_logo_filename = get_logo_filename();
	int video_frame_size = 0;
	int r;
	int frame_size;
	int ef = 4;

	n_frames = get_frames_in_piece(stats, piece, 0, &start, &stop, get_segment_length() );

	video_frame_size = 0;
	n_video_frames 	 = 0;
	if (video_logo_filename)
		video_frame_size = parse_video_file(video_logo_filename, &n_video_frames);


	if ( !output_buffer || !output_buffer_size ){
		return (stats->track[0]->pts[stop] - stats->track[0]->pts[start] + 0.5) * stats->track[0]->bitrate / 8 +
				sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int) + video_frame_size;
	}

	ptr = (char*)output_buffer;

	mdata = output_buffer;
	tdata = (track_data_t*)(ptr + sizeof(media_data_t));
	vdata = (track_data_t*)(ptr + sizeof(media_data_t) + sizeof(track_data_t));

	size   = (int*)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2);
	offset = (int*)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int));

	mdata->n_tracks = 1;
	mdata->track_data[0] = tdata;
	if (video_logo_filename){
		mdata->track_data[1] = vdata;
		++mdata->n_tracks;
	}

	buf = (char*)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int));

	tdata->buffer = buf;
	tdata->offset = offset;
	tdata->size   = size;

	n_out_bytes 	= 0;
	n_out_frames 	= 0;

	sample_rate 	= stats->track[0]->sample_rate;
	n_ch 			= stats->track[0]->n_ch;
	bit_per_sample 	= stats->track[0]->sample_size;
	data_offset		= stats->track[0]->data_start_offset;
	type			= stats->track[0]->type;


	frame_size		= (get_frame_duration(stats->track[0]->sample_rate) * stats->track[0]->bitrate / (stats->track[0]->sample_rate * 8) + 0);

	n_out_bytes = (n_frames + 1) * frame_size;
	n_out_bytes = source->read(handle, tdata->buffer, n_out_bytes, start * frame_size + data_offset, 0);


	tdata->first_frame  	= start;
	tdata->n_frames			= get_num_of_mp3_frames(tdata->buffer, n_out_bytes, sample_rate, stats->track[0]->bitrate, size, offset);
	tdata->buffer_size		= n_out_bytes;
	tdata->cc 				= 0;
	tdata->frames_written 	= 0;

	if (video_logo_filename){
		vdata->size 			= (int *)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int));
		vdata->offset 			= (int *)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int));

		vdata->buffer 			= tdata->buffer + tdata->buffer_size;
		vdata->buffer_size 		= vdata->size[0] = load_data_from_file(vdata->buffer, video_frame_size, video_logo_filename);
		vdata->offset[0] 		= 0;
		vdata->cc 				= 0;
		vdata->frames_written 	= 0;
		vdata->first_frame 		= 0;
		vdata->n_frames 		= 1;
	}

	return 	sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int) +
			(n_video_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int) + (video_logo_filename ? vdata->buffer_size : 0) + tdata->buffer_size;
}

media_handler_t mp3_file_handler = {
										.get_media_stats = mp3_media_get_stats,
										.get_media_data  = mp3_media_get_data
									};
