/*
 * Configuration implementation for module
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


#include "mod_conf.h"
#include <stdlib.h>
#include <string.h>


typedef struct hls_config{
	int 	encode_audio_bitrate;	//mpeg audio bitrate for output stream
	char* 	logo_filename;			//video track file to initialize video decoder to make stream playable in jwplayer (works for mp3 only)
	int 	encode_audio_codec;		// 0 - mpeg audio layer 2
									// 1 - mpeg audio layer 3
	int		allow_wav;
	int 	allow_mp3;
	int 	allow_mp4;
	int 	allow_http;
	int  	segment_length;			//T文件的切片时长
	int 	allow_redirect;
	int 	log_level;

	char 	data_path[2048];

} hls_config;

static hls_config config = {0};

void set_allow_redirect(int redirect){
	config.allow_redirect = redirect;
}

int get_allow_redirect(){
	return config.allow_redirect;
}

void set_encode_audio_bitrate(int bitrate){
	config.encode_audio_bitrate = bitrate;
}

int get_encode_audio_bitrate(){
	return config.encode_audio_bitrate;
}

void set_allow_wav(int allow_wav){
	config.allow_wav = allow_wav;
}

int get_allow_wav(){
	return config.allow_wav;
}

void set_allow_mp3(int allow_mp3){
	config.allow_mp3 = allow_mp3;
}

int get_allow_mp3(){
	return config.allow_mp3;
}

void set_allow_mp4(int allow_mp4){
	config.allow_mp4 = allow_mp4;
}

int get_allow_mp4(){
	return config.allow_mp4;
}


void set_allow_http(int allow_http){
	config.allow_http = allow_http;
}

int get_allow_http(){
	return config.allow_http;
}

void set_encode_audio_codec(int codec){
	config.encode_audio_codec = codec;
}

int get_encode_audio_codec(){
	return config.encode_audio_codec;
}

void set_logo_filename(char* filename){
	config.logo_filename = filename;
}

char* get_logo_filename(){
	return config.logo_filename;
}

void set_segment_length(int sec){
	 config.segment_length = sec;
}

int get_segment_length(){
	return config.segment_length;
}


int get_log_level(){
	return config.log_level;
}

void set_log_level(int level){
	config.log_level = level;
}

char* get_data_path(){
	if (config.data_path[0] == 0)
		return NULL;
	return &config.data_path[0];
}

void set_data_path(char* path){
	if (path == NULL)
		config.data_path[0] = 0;
	else
		strncpy(config.data_path, path, sizeof(config.data_path));

}


