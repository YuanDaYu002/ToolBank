/*
 * Interface of MPEG2 Transport stream muxer and playlist generation
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

#ifndef __HLS_MUX__
#define __HLS_MUX__

#include "hls_media.h"

int generate_playlist(media_stats_t* stats, char* filename_template, char* output_buffer, int output_buffer_size, char* url, int** numberofchunks);
int mux_to_ts(media_stats_t* stats, media_data_t* data, char* output_buffer, int output_buffer_size);
int get_frames_in_piece(media_stats_t* stats, int piece, int track, int* sf, int* ef, int recommended_length);
int get_num_of_mp3_frames(unsigned char* buf, int size, int sr, int br, int* frm_size, int* frm_offset);

#endif
