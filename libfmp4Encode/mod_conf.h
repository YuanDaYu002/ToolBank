/*
 * Interface of configuration for module
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

#ifndef __MOD_CONF_H__
#define __MOD_CONF_H__

void set_encode_audio_bitrate(int bitrate);
int get_encode_audio_bitrate(void);

void set_allow_wav(int allow_wav);
int get_allow_wav(void);

void set_allow_mp3(int allow_wav);
int get_allow_mp3(void);

void set_allow_mp4(int allow_mp4);

int get_allow_mp4(void);


void set_allow_http(int allow_http);
int get_allow_http(void);

void set_encode_audio_codec(int codec);
int get_encode_audio_codec(void);

void set_logo_filename(char* filename);
char* get_logo_filename(void);

void set_segment_length(int sec);
int get_segment_length(void);

void set_allow_redirect(int redirect);
int get_allow_redirect(void);

int get_log_level(void);
void set_log_level(int level);

char* get_data_path(void);
void set_data_path(char* path);

#endif

