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
#include <stdio.h>


static int get_close_rate(int input_sample_rate){
    int out_sample_rate = 0;
    switch(input_sample_rate){
        case 8000:  out_sample_rate = 32000.0; //nearest possible samplerate 32000
	    break;
	case 11025: out_sample_rate = 44100.0; //nearest possible samplerate 44100
	    break;

	case 16000: out_sample_rate = 32000.0; //nearest possible samplerate 32000
	    break;
   	case 22050: out_sample_rate = 44100.0; //nearest possible samplerate 44100
	    break;
	case 32000: out_sample_rate = 32000.0; //nearest possible samplerate 32000
	    break;
	case 44100: out_sample_rate = 44100.0; //nearest possible samplerate 44100
  	    break;
	case 48000: out_sample_rate = 48000.0; //nearest possible samplerate 48000
	    break;
    }
    return out_sample_rate;
}

static int check_tag(char* buffer,char* tag){	//return 1 if success
    return (buffer[0] == tag[0] && buffer[1] == tag[1] && buffer[2] == tag[2] && buffer[3] == tag[3]);
}

static int get_tag_size(char* buffer){
    return (((int*)(buffer + 4))[0] + 1) & 0xFFFFFFFE;
}

static double parse_wav_header_buffer(char* buffer, int buffer_size, int* sample_rate, int* num_of_channels, int* sample_size, int* data_start_offset, int* type){
    int n_ch;
    int sam_siz;
    int sr;
    char* cp;
    int wav_file_size;
    int data_size;
    int data_offset;
    int audio_format;
    int byte_rate;
    int block_align;

    if (buffer_size < 44)
        return 0.0;

    if (!check_tag(buffer,"RIFF"))
    	return 0.0;

    wav_file_size = get_tag_size(buffer);

    if (!check_tag(buffer + 8 ,"WAVE"))
    	return 0.0;

    cp = buffer + 12;

    while (cp - buffer <= buffer_size - 8){

		if (check_tag(cp, "fmt ")){
	//20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
	//                               Values other than 1 indicate some
	//                               form of compression.
	//				 MuLaw = 7
	//				 ALaw = 6
	//22        2   NumChannels      Mono = 1, Stereo = 2, etc.
	//24        4   SampleRate       8000, 44100, etc.
	//28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
	//32        2   BlockAlign       == NumChannels * BitsPerSample/8
	//                               The number of bytes for one sample including
	//                               all channels. I wonder what happens when
	//                               this number isn't an integer?
	//34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
	//	    2   ExtraInfoSize
			audio_format = ((unsigned short*)(cp+8) )[0];
			n_ch 		= ((unsigned short*)(cp+10) )[0];
			sr   		= ((unsigned int*)(cp+12) )[0];
			byte_rate   = ((unsigned int*)(cp+16) )[0];
			block_align = ((unsigned short*)(cp+20) )[0];
			sam_siz     = ((unsigned short*)(cp+22) )[0];


		}else if (check_tag(cp, "data")){
			data_size = get_tag_size(cp);
			data_offset = (cp - buffer) + 8;
			break;
		}
		cp += 8 + get_tag_size(cp);
    }

    if (type)
    	*type = audio_format;

    if (sample_rate)
        *sample_rate = sr;
    if (num_of_channels)
        *num_of_channels = n_ch;
    if (sample_size)
        *sample_size = sam_siz;
    if (data_start_offset)
        *data_start_offset = data_offset;

    return (double)data_size / (double)(n_ch * (sam_siz / 8) * sr);
}

#define MPEG_LAYER_2_PACKET_SIZE 1152

static short mulaw_decode(unsigned char mulaw) {
    mulaw = ~mulaw;
    int sign = mulaw & 0x80;
    int exponent = (mulaw & 0x70) >> 4;
    int data = mulaw & 0x0f;
    data |= 0x10;
    data <<= 1;
    data += 1;
    data <<= exponent + 2;
    data -= 0x84;
    return (short)(sign == 0 ? data : -data);
}

static int convert_linear_pcm_sample_rate(char* out_buf, int out_buf_size, char* in_buf, int in_buf_size, int in_sr, int out_sr, int num_of_channels, int sample_size, int num_of_in_samples){
    int result = 0;
    int i,j,k;
    if (sample_size == 8){
		unsigned char* ip = (unsigned char*)in_buf;
		short* op = (short*)out_buf;
		int multiplier = out_sr / in_sr;
		int iss = num_of_channels;
		int oss = num_of_channels * multiplier;
		int ocs = num_of_channels;
		for(i = 0; i < num_of_in_samples; ++i){
			for(k = 0; k < multiplier; ++k){
				for(j = 0; j < num_of_channels; ++j){
				op[i * oss + k * ocs + j] = ((int)(ip[i * iss + j]) - 128) * 256;
			}
				}
		}
		if (((num_of_in_samples * multiplier) % MPEG_LAYER_2_PACKET_SIZE) == 0)
			result = ((num_of_in_samples * multiplier) / MPEG_LAYER_2_PACKET_SIZE ) ;
		else
			result = ((num_of_in_samples * multiplier) / MPEG_LAYER_2_PACKET_SIZE + 1) * MPEG_LAYER_2_PACKET_SIZE;
    }else if (sample_size == 16){
		short* ip = (short*)in_buf;
		short* op = (short*)out_buf;
		int multiplier = out_sr / in_sr;
		int iss = num_of_channels;
		int oss = num_of_channels * multiplier;
		int ocs = num_of_channels;
		for(i = 0; i < num_of_in_samples; ++i){
			for(k = 0; k < multiplier; ++k){
				for(j = 0; j < num_of_channels; ++j){
					op[i * oss + k * ocs + j] = ip[i * iss + j];
				}
			}
		}
		if (((num_of_in_samples * multiplier) % MPEG_LAYER_2_PACKET_SIZE) == 0)
			result = ((num_of_in_samples * multiplier) / MPEG_LAYER_2_PACKET_SIZE ) ;
		else
			result = ((num_of_in_samples * multiplier) / MPEG_LAYER_2_PACKET_SIZE + 1) * MPEG_LAYER_2_PACKET_SIZE;
    }
    return result;
}

static int convert_mulaw_pcm_sample_rate(char* out_buf, int out_buf_size, char* in_buf, int in_buf_size, int in_sr, int out_sr, int num_of_channels, int sample_size, int num_of_in_samples){
    int result = 0;
    int i,j,k;
    if (sample_size == 8){
		unsigned char* ip = (unsigned char*) in_buf;
		short* op = (short*)out_buf;
		int multiplier = out_sr / in_sr;
		int iss = num_of_channels;
		int oss = num_of_channels * multiplier;
		int ocs = num_of_channels;
		for(i = 0; i < num_of_in_samples; ++i){
			for(k = 0; k < multiplier; ++k){
				for(j = 0; j < num_of_channels; ++j){
					op[i * oss + k * ocs + j] = mulaw_decode(ip[i * iss + j]);
				}
			}
		}
		if (((num_of_in_samples * multiplier) % MPEG_LAYER_2_PACKET_SIZE) == 0)
			result = ((num_of_in_samples * multiplier) / MPEG_LAYER_2_PACKET_SIZE ) ;
		else
			result = ((num_of_in_samples * multiplier) / MPEG_LAYER_2_PACKET_SIZE + 1) * MPEG_LAYER_2_PACKET_SIZE;
    }
    return result;
}

static int convert_sample_rate(char* out_buf, int out_buf_size, char* in_buf, int in_buf_size, int in_sr, int out_sr, int num_of_channels, int sample_size, int num_of_in_samples, int type){//return number of converted samples (integer number of frames)
    int number_of_converted_samples = 0;
    switch(type){
        case 1://linear PCM
		number_of_converted_samples = convert_linear_pcm_sample_rate(out_buf, out_buf_size, in_buf, in_buf_size, in_sr, out_sr, num_of_channels, sample_size, num_of_in_samples);
            break;
	case 7://MuLaw PCM
		number_of_converted_samples = convert_mulaw_pcm_sample_rate(out_buf, out_buf_size, in_buf, in_buf_size, in_sr, out_sr, num_of_channels, sample_size, num_of_in_samples);
	    break;
	case 6://ALaw PCM
		number_of_converted_samples = 0; // not implemented yet
	    break;
    }
    return number_of_converted_samples;
}

static const int frame_size = 1152;

int parse_video_file(char *filename, int* n_video_frames){
	FILE* f;
	int size = 0;

	*n_video_frames = 1;
	f = fopen(filename, "rb");
	if (f){
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fclose(f);
	}
	return size;
}

int load_data_from_file(char* buf, int size, char* filename){
	FILE* f;
	f = fopen(filename, "rb");
	if (f){
		size = fread(buf, 1, size, f);

		fclose(f);
	}
	return size;
}

int wav_media_get_stats(void* context, file_handle_t* handle, file_source_t* source, media_stats_t* output_buffer, int output_buffer_size ){
	char buf[4096];
	int sample_rate;
	int num_of_channels;
	int sample_size;
	int data_start_offset;
	int type;

	int rb;
	double duration;
	int encoding_sample_rate;
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

	char* video_logo_filename = get_logo_filename();

	if (!output_buffer || !output_buffer_size){
		rb = source->read(handle, buf, 4096, 0, 0);
		duration = parse_wav_header_buffer(buf, rb, &sample_rate, &num_of_channels, &sample_size, &data_start_offset, &type);
		encoding_sample_rate = get_close_rate(sample_rate);
		n_frames = (duration * encoding_sample_rate + (frame_size - 1)) / frame_size;

		parse_video_file(video_logo_filename, &n_video_frames);

		return	sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float) + n_video_frames * sizeof(float) + n_frames * sizeof(int) + n_video_frames * sizeof(int);//return required buffer size for media_stats_t
	}

	rb 					 = source->read(handle, buf, 4096, 0, 0);
	duration 			 = parse_wav_header_buffer(buf, rb, &sample_rate, &num_of_channels, &sample_size, &data_start_offset, &type);
	encoding_sample_rate = get_close_rate(sample_rate);
	n_frames 			 = (duration * encoding_sample_rate + (frame_size - 1)) / frame_size;

	ptr = (char*)output_buffer;

	stats	= (media_stats_t*)ptr;
	track	= (track_t*)(ptr + sizeof(media_stats_t));
	video_track = (track_t*)(ptr + sizeof(media_stats_t) + sizeof(track_t));
	pts		= (float*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2);
	video_pts = (float*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float));
	flags	= (int*)(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float) + n_video_frames * sizeof(float));
	video_flags = (int* )(ptr + sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * sizeof(float) + n_video_frames * sizeof(float) + n_frames * sizeof(int));

	stats->n_tracks = 2;
	stats->track[0]	= track;
	stats->track[1] = video_track;

	track->codec 	= MPEG_AUDIO_L3;
	track->dts	 	= 0;
	track->n_frames = n_frames;
	track->pts		= pts;
	track->bitrate  = get_encode_audio_bitrate();
	track->repeat_for_every_segment = 0;
	track->sample_rate 			= sample_rate;
	track->n_ch 				= num_of_channels;
	track->sample_size 			= sample_size;
	track->data_start_offset 	= data_start_offset;
	track->type					= type;
	track->flags 				= flags;

	video_track->codec 						= H264_VIDEO;
	video_track->bitrate 					= 0;
	video_track->dts 						= 0;
	video_track->pts 						= video_pts;
	video_track->repeat_for_every_segment 	= 1;
	video_track->flags 						= video_flags;

	for(i = 0; i < n_frames; ++i){
		pts[i] = (float)(i * frame_size / (double)encoding_sample_rate);
		flags[i] = KEY_FRAME_FLAG;
	}
	parse_video_file(video_logo_filename, &n_video_frames);
	for(i = 0; i < n_video_frames; ++i){
		video_pts[i] = 0.1 * i; //we force 10 fps here
		video_flags[i] = KEY_FRAME_FLAG;
	}
	video_track->n_frames = n_video_frames;

	return sizeof(media_stats_t) + sizeof(track_t) * 2 + n_frames * (sizeof(float) + sizeof(int)) + n_video_frames * (sizeof(float) + sizeof(int));// return number of bytes
}

int get_samples(	file_handle_t* handle, file_source_t* source, double first_time, double last_time, char* buf,
					int sample_rate, int n_ch, int bps, int data_offset, int type){
	int rb = source->read(handle, buf, (last_time - first_time) * sample_rate * n_ch * bps / 8, data_offset + first_time * sample_rate * n_ch * bps / 8, 0);
	return rb / (n_ch * bps / 8);
}

int decode_wave(char* raw_wave_data, int in_samples, char* frame_data, int sample_rate, int encoding_sample_rate, int n_ch, int bit_per_sample, int type){

	return convert_sample_rate(frame_data, encoding_sample_rate * n_ch * bit_per_sample / 8, raw_wave_data, in_samples * n_ch * 2, sample_rate, encoding_sample_rate, n_ch, bit_per_sample, in_samples, type);
}

int wav_media_get_data(void* context, file_handle_t* handle, file_source_t* source, media_stats_t* stats, int piece, media_data_t* output_buffer, int output_buffer_size ){
	int start;
	int stop;
	int n_frames;

	track_data_t* tdata;
	track_data_t* vdata;
	media_data_t* mdata;
	char* ptr;
	int* size;
	int* offset;
	char* buf;

	char raw_wave_data[1152 * 2 * 2];
	char frame_data[1152 * 2 * 2];
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

	int ef = 4;

	n_frames = get_frames_in_piece(stats, piece, 0, &start, &stop, 10);
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

	mdata->n_tracks = 2;
	mdata->track_data[0] = tdata;
	mdata->track_data[1] = vdata;

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


	encoding_sample_rate = get_close_rate(sample_rate);

#ifdef TWO_LAME


#else
//	encopts = lame_init();
//
//	r = lame_set_num_channels(encopts, n_ch);
//
//	/* sample rate */
//	r = lame_set_in_samplerate (encopts, encoding_sample_rate);
//	r = lame_set_out_samplerate(encopts, encoding_sample_rate);
//
//	/* algorithmic quality */
//	r = lame_set_quality(encopts, 8);
//
//	r = lame_set_mode(encopts, n_ch > 1 ? JOINT_STEREO : MONO);
//
//	/* rate control */
//	r = lame_set_padding_type(encopts, PAD_ALL);
//
//	r = lame_set_num_channels(encopts, n_ch);
////	lame_set_no_short_blocks(encopts, 1);
//
//	r = lame_set_VBR(encopts, vbr_off);
//
//	r = lame_set_brate(encopts, stats->track[0]->bitrate / 1000);
//
//	if (lame_init_params(encopts) == 0){
//		int frame_size = lame_get_framesize(encopts);
//
//		for(i = start; i < stop; ++i){
//			int n_samples = get_samples(handle, source, i * frame_size / (double)encoding_sample_rate, (i + 1) * frame_size / (double)encoding_sample_rate,
//										 raw_wave_data, sample_rate, n_ch, bit_per_sample, data_offset, type);
//
//			decode_wave(raw_wave_data, n_samples, frame_data, sample_rate, encoding_sample_rate, n_ch, bit_per_sample, type);
//
//			if (n_ch > 1){
//				n_out_bytes += lame_encode_buffer_interleaved(	encopts, (short*)(frame_data), frame_size, (unsigned char*)tdata->buffer + n_out_bytes, output_buffer_size - n_out_bytes );
//			}else{
//				n_out_bytes += lame_encode_buffer(	encopts, (short*)(frame_data), 0, frame_size, (unsigned char*)tdata->buffer + n_out_bytes, output_buffer_size - n_out_bytes );
//			}
//		}
//
//		n_out_bytes += lame_encode_flush(encopts,        tdata->buffer + n_out_bytes, output_buffer_size - n_out_bytes);
//
//		lame_close(encopts);
//	}
#endif

	tdata->first_frame  	= start;
	tdata->n_frames			= get_num_of_mp3_frames(tdata->buffer, n_out_bytes, encoding_sample_rate, stats->track[0]->bitrate, size, offset);
	tdata->buffer_size		= n_out_bytes;
	tdata->cc 				= 0;
	tdata->frames_written 	= 0;

	vdata->size 			= (int *)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int));
	vdata->offset 			= (int *)(ptr + sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int));

	vdata->buffer 			= tdata->buffer + tdata->buffer_size;
	vdata->buffer_size 		= vdata->size[0] = load_data_from_file(vdata->buffer, video_frame_size, video_logo_filename);
	vdata->offset[0] 		= 0;
	vdata->cc 				= 0;
	vdata->frames_written 	= 0;
	vdata->first_frame 		= 0;
	vdata->n_frames 		= 1;

	return 	sizeof(media_data_t) + sizeof(track_data_t) * 2 + (n_frames + ef) * sizeof(int) + (n_frames + ef) * sizeof(int) +
			(n_video_frames + ef) * sizeof(int) + (n_video_frames + ef) * sizeof(int) + vdata->buffer_size + tdata->buffer_size;
}

media_handler_t wav_file_handler = {
										.get_media_stats = wav_media_get_stats,
										.get_media_data  = wav_media_get_data
									};

extern media_handler_t mp3_file_handler;
extern media_handler_t mp4_file_handler;

/*
依据文件的后缀识别媒体文件的类型，返回对应的处理句柄
*/
media_handler_t* get_media_handler(char* filename)
{
	int fn_len = strlen(filename);
	if ( get_allow_wav() && fn_len > 3 && filename[fn_len-4] == '.' 
		 && ((filename[fn_len - 3] == 'W' || filename[fn_len - 3] == 'w') 
		 && (filename[fn_len - 2] == 'A' || filename[fn_len - 2] == 'a') 
		 && (filename[fn_len - 1] == 'V' || filename[fn_len - 1] == 'v')))
	{
		return &wav_file_handler;
	}else if ( 	get_allow_mp3() && fn_len > 3 && filename[fn_len-4] == '.' 
				&& ((filename[fn_len - 3] == 'M' || filename[fn_len - 3] == 'm') 
				&& (filename[fn_len - 2] == 'P' || filename[fn_len - 2] == 'p') 
				&& (filename[fn_len - 1] == '3')) )
	{
		return &mp3_file_handler;
	}else if ( get_allow_mp4() && fn_len > 3 && filename[fn_len-4] == '.' 
				&& ((filename[fn_len - 3] == 'M' || filename[fn_len - 3] == 'm') 
				&& (filename[fn_len - 2] == 'P' || filename[fn_len - 2] == 'p') 
				&& (filename[fn_len - 1] == '4')) )
	{
		return &mp4_file_handler;
	}
	return 0;
}



