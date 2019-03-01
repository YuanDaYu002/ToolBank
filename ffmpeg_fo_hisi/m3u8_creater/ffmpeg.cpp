#include "ffmpeg.h"

int nRet = 0;
AVFormatContext* icodec = NULL; 
AVFormatContext* ocodec = NULL;
char szError[256]; 
AVStream * ovideo_st = NULL;
AVStream * oaudio_st = NULL;
int video_stream_idx = -1;
int audio_stream_idx = -1;
AVCodec *audio_codec = NULL;
AVCodec *video_codec = NULL;
AVBitStreamFilterContext * vbsf_aac_adtstoasc = NULL;
AVBitStreamFilterContext * vbsf_h264_toannexb = NULL;
int IsAACCodes = 0;

//m3u8 param
unsigned int m_output_index = 1;       //生成的切片文件顺序编号
char m_output_file_name[256];          //输入的要切片的文件

int init_demux(char * Filename,AVFormatContext ** iframe_c)
{
	int i = 0;
	nRet = avformat_open_input(iframe_c, Filename,NULL, NULL);
	if (nRet != 0)
	{
		av_strerror(nRet, szError, 256);
		printf(szError);
		printf("\n");
		printf("Call avformat_open_input function failed!\n");
		return 0;
	}
	if (avformat_find_stream_info(*iframe_c,NULL) < 0)
	{
		printf("Call av_find_stream_info function failed!\n");
		return 0;
	}
	//输出视频信息
	av_dump_format(*iframe_c, -1, Filename, 0);

	//添加音频信息到输出context
	for (i = 0; i < (*iframe_c)->nb_streams; i++)
	{
		if ((*iframe_c)->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_idx = i;
		}
		else if ((*iframe_c)->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_idx = i;
		}
	}

	if ((strstr(icodec->iformat->name, "flv") != NULL) || 
		(strstr(icodec->iformat->name, "mp4") != NULL) || 
		(strstr(icodec->iformat->name, "mov") != NULL))    
	{
		if (icodec->streams[video_stream_idx]->codec->codec_id == AV_CODEC_ID_H264)  //AV_CODEC_ID_H264
		{
			//这里注意："h264_mp4toannexb",一定是这个字符串，无论是 flv，mp4，mov格式
			vbsf_h264_toannexb = av_bitstream_filter_init("h264_mp4toannexb"); 
		} 
		if (icodec->streams[audio_stream_idx]->codec->codec_id == AV_CODEC_ID_AAC) //AV_CODEC_ID_AAC
		{
			IsAACCodes = 1;
		}
	} 

	return 1;
}

int init_mux()
{
	int ret = 0;
	/* allocate the output media context */
	avformat_alloc_output_context2(&ocodec, NULL,NULL, m_output_file_name);
	if (!ocodec) 
	{
		return getchar();
	}
	AVOutputFormat* ofmt = NULL;
	ofmt = ocodec->oformat;

	/* open the output file, if needed */
	if (!(ofmt->flags & AVFMT_NOFILE))
	{
		if (avio_open(&ocodec->pb, m_output_file_name, AVIO_FLAG_WRITE) < 0)
		{
			printf("Could not open '%s'\n", m_output_file_name);
			return getchar();
		}
	}

	//这里添加的时候AUDIO_ID/VIDEO_ID有影响
	//添加音频信息到输出context
	if(audio_stream_idx != -1)//如果存在音频
	{
		oaudio_st = add_out_stream(ocodec, AVMEDIA_TYPE_AUDIO);
	}

	//添加视频信息到输出context
	if (video_stream_idx != -1)//如果存在视频
	{
		ovideo_st = add_out_stream(ocodec,AVMEDIA_TYPE_VIDEO);
	}

	av_dump_format(ocodec, 0, m_output_file_name, 1);

	ret = avformat_write_header(ocodec, NULL);
	if (ret != 0)
	{
		printf("Call avformat_write_header function failed.\n");
		return 0;
	}
	return 1;
}

int uinit_demux()
{
	/* free the stream */
	av_free(icodec);
	if (vbsf_h264_toannexb !=NULL)
	{
		av_bitstream_filter_close(vbsf_h264_toannexb);
		vbsf_h264_toannexb = NULL;
	}
	return 1;
}

int uinit_mux()
{
	int i = 0;
	nRet = av_write_trailer(ocodec);
	if (nRet < 0)
	{
		av_strerror(nRet, szError, 256);
		printf(szError);
		printf("\n");
		printf("Call av_write_trailer function failed\n");
	}
	if (vbsf_aac_adtstoasc !=NULL)
	{
		av_bitstream_filter_close(vbsf_aac_adtstoasc); 
		vbsf_aac_adtstoasc = NULL;
	}

	/* Free the streams. */
	for (i = 0; i < ocodec->nb_streams; i++) 
	{
		av_freep(&ocodec->streams[i]->codec);
		av_freep(&ocodec->streams[i]);
	}
	if (!(ocodec->oformat->flags & AVFMT_NOFILE))
	{
		/* Close the output file. */
		avio_close(ocodec->pb);
	}
	av_free(ocodec);
	return 1;
}

AVStream * add_out_stream(AVFormatContext* output_format_context,AVMediaType codec_type_t)
{
	AVStream * in_stream = NULL;
	AVStream * output_stream = NULL;
	AVCodecContext* output_codec_context = NULL;

	output_stream = avformat_new_stream(output_format_context,NULL);
	if (!output_stream)
	{
		return NULL;
	}

	switch (codec_type_t)
	{
	case AVMEDIA_TYPE_AUDIO:
		in_stream = icodec->streams[audio_stream_idx];
		break;
	case AVMEDIA_TYPE_VIDEO:
		in_stream = icodec->streams[video_stream_idx];
		break;
	default:
		break;
	}

	output_stream->id = output_format_context->nb_streams - 1;
	output_codec_context = output_stream->codec;
	output_stream->time_base  = in_stream->time_base;

	int ret = 0;
	ret = avcodec_copy_context(output_stream->codec, in_stream->codec);
	if (ret < 0) 
	{
		printf("Failed to copy context from input to output stream codec context\n");
		return NULL;
	}

	//这个很重要，要么纯复用解复用，不做编解码写头会失败,
	//另或者需要编解码如果不这样，生成的文件没有预览图，还有添加下面的header失败，置0之后会重新生成extradata
	output_codec_context->codec_tag = 0; 

	//if(! strcmp( output_format_context-> oformat-> name,  "mp4" ) ||
	//!strcmp (output_format_context ->oformat ->name , "mov" ) ||
	//!strcmp (output_format_context ->oformat ->name , "3gp" ) ||
	//!strcmp (output_format_context ->oformat ->name , "flv"))
	if(AVFMT_GLOBALHEADER & output_format_context->oformat->flags)
	{
		output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	return output_stream;
}

int write_index_file(const unsigned int first_segment, const unsigned int last_segment, const int end, const unsigned int actual_segment_durations[])
{
	FILE *index_fp = NULL;
	char *write_buf = NULL;
	unsigned int i = 0;
	char m3u8_file_pathname[256] = {0};

	sprintf(m3u8_file_pathname,"%s%s",URL_PREFIX,M3U8_FILE_NAME);
	
	index_fp = fopen(m3u8_file_pathname,"w");
	if (!index_fp)
	{
		printf("Could not open m3u8 index file (%s), no index file will be created\n",(char *)m3u8_file_pathname);
		return -1;
	}

	write_buf = (char *)malloc(sizeof(char) * 1024);
	if (!write_buf) 
	{
		printf("Could not allocate write buffer for index file, index file will be invalid\n");
		fclose(index_fp);
		return -1;
	}


	if (NUM_SEGMENTS)
	{
		//#EXT-X-MEDIA-SEQUENCE：<Number> 播放列表文件中每个媒体文件的URI都有一个唯一的序列号。URI的序列号等于它之前那个RUI的序列号加一(没有填0)
		sprintf(write_buf,"#EXTM3U\n#EXT-X-TARGETDURATION:%lu\n#EXT-X-MEDIA-SEQUENCE:%u\n",SEGMENT_DURATION,first_segment);
	}
	else 
	{
		sprintf(write_buf,"#EXTM3U\n#EXT-X-TARGETDURATION:%lu\n",SEGMENT_DURATION);
	}
	if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) 
	{
		printf("Could not write to m3u8 index file, will not continue writing to index file\n");
		free(write_buf);
		fclose(index_fp);
		return -1;
	}

	for (i = first_segment; i <= last_segment; i++) 
	{
		sprintf(write_buf,"#EXTINF:%u,\n%s%s-%u.ts\n",actual_segment_durations[i-1],URL_PREFIX,OUTPUT_PREFIX,i);
		if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) 
		{
			printf("Could not write to m3u8 index file, will not continue writing to index file\n");
			free(write_buf);
			fclose(index_fp);
			return -1;
		}
	}

	if (end) 
	{
		sprintf(write_buf,"#EXT-X-ENDLIST\n");
		if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1)
		{
			printf("Could not write last file and endlist tag to m3u8 index file\n");
			free(write_buf);
			fclose(index_fp);
			return -1;
		}
	}

	free(write_buf);
	fclose(index_fp);
	return 0;
}

void slice_up()
{
	int write_index = 1;
	unsigned int first_segment = 1;     //第一个分片的标号
	unsigned int last_segment = 0;      //最后一个分片标号
	int decode_done = 0;                //文件是否读取完成
	int remove_file = 0;                //是否要移除文件（写在磁盘的分片已经达到最大）
	char remove_filename[256] = {0};    //要从磁盘上删除的文件名称
	double prev_segment_time = 0;       //上一个分片时间
	int ret = 0;
	unsigned int actual_segment_durations[1024] = {0}; //各个分片文件实际的长度

	//填写第一个输出文件名称
	sprintf(m_output_file_name,"%s%s-%u.ts",URL_PREFIX,OUTPUT_PREFIX,m_output_index ++);

	//****************************************创建输出文件（写头部）
	init_mux();

	write_index = !write_index_file(first_segment, last_segment, 0, actual_segment_durations);

	do 
	{
		unsigned int current_segment_duration;
		double segment_time = prev_segment_time;
		AVPacket packet;
		av_init_packet(&packet);

		decode_done = av_read_frame(icodec, &packet);
		if (decode_done < 0) 
		{
			break;
		}

		if (av_dup_packet(&packet) < 0) 
		{
			printf("Could not duplicate packet");
			av_free_packet(&packet);
			break;
		}

		if (packet.stream_index == video_stream_idx ) 
		{
			segment_time = packet.pts * av_q2d(icodec->streams[video_stream_idx]->time_base);
		}
		else if (video_stream_idx < 0) 
		{
			segment_time = packet.pts * av_q2d(icodec->streams[audio_stream_idx]->time_base);
		}
		else 
		{
			segment_time = prev_segment_time;
		}

		//这里是为了纠错，有文件pts为不可用值
		if (packet.pts < packet.dts)
		{
			packet.pts = packet.dts;
		}

		//视频
		if (packet.stream_index == video_stream_idx ) 
		{
			if (vbsf_h264_toannexb != NULL)
			{
				AVPacket filteredPacket = packet; 
				int a = av_bitstream_filter_filter(vbsf_h264_toannexb,                                           
					ovideo_st->codec, NULL,&filteredPacket.data, &filteredPacket.size, packet.data, packet.size, packet.flags & AV_PKT_FLAG_KEY); 
				if (a > 0)             
				{                
					av_free_packet(&packet); 
					packet.pts = filteredPacket.pts;
					packet.dts = filteredPacket.dts;
					packet.duration = filteredPacket.duration;
					packet.flags = filteredPacket.flags;
					packet.stream_index = filteredPacket.stream_index;
					packet.data = filteredPacket.data;  
					packet.size = filteredPacket.size;        
				}             
				else if (a < 0)            
				{                
					fprintf(stderr, "%s failed for stream %d, codec %s",
						vbsf_h264_toannexb->filter->name,packet.stream_index,ovideo_st->codec->codec ?  ovideo_st->codec->codec->name : "copy");
					av_free_packet(&packet);                
					getchar();           
				}
			}

			packet.pts = av_rescale_q_rnd(packet.pts, icodec->streams[video_stream_idx]->time_base, ovideo_st->time_base, AV_ROUND_NEAR_INF);
			packet.dts = av_rescale_q_rnd(packet.dts, icodec->streams[video_stream_idx]->time_base, ovideo_st->time_base, AV_ROUND_NEAR_INF);
			packet.duration = av_rescale_q(packet.duration,icodec->streams[video_stream_idx]->time_base, ovideo_st->time_base);

			packet.stream_index = VIDEO_ID; //这里add_out_stream顺序有影响
			printf("video\n");
		}
		else if (packet.stream_index == audio_stream_idx)
		{
			packet.pts = av_rescale_q_rnd(packet.pts, icodec->streams[audio_stream_idx]->time_base, oaudio_st->time_base, AV_ROUND_NEAR_INF);
			packet.dts = av_rescale_q_rnd(packet.dts, icodec->streams[audio_stream_idx]->time_base, oaudio_st->time_base, AV_ROUND_NEAR_INF);
			packet.duration = av_rescale_q(packet.duration,icodec->streams[audio_stream_idx]->time_base, oaudio_st->time_base);

			packet.stream_index = AUDIO_ID; //这里add_out_stream顺序有影响
			printf("audio\n");
		}

		current_segment_duration = (int)(segment_time - prev_segment_time + 0.5);
		actual_segment_durations[last_segment] = (current_segment_duration > 0 ? current_segment_duration: 1);

		if (segment_time - prev_segment_time >= SEGMENT_DURATION) 
		{
			ret = av_write_trailer(ocodec);   // close ts file and free memory
			if (ret < 0) 
			{
				printf("Warning: Could not av_write_trailer of stream\n");
			}

			avio_flush(ocodec->pb);
			avio_close(ocodec->pb);

			if (NUM_SEGMENTS && (int)(last_segment - first_segment) >= NUM_SEGMENTS - 1)
			{
				remove_file = 1;
				first_segment++;
			}
			else 
			{
				remove_file = 0;
			}

			if (write_index) 
			{
				write_index = !write_index_file(first_segment, ++last_segment, 0,actual_segment_durations);
			}

			if (remove_file) 
			{
				sprintf(remove_filename,"%s%s-%u.ts",URL_PREFIX,OUTPUT_PREFIX,first_segment - 1);
				remove(remove_filename);
			}

			sprintf(m_output_file_name,"%s%s-%u.ts",URL_PREFIX,OUTPUT_PREFIX,m_output_index ++);
			if (avio_open(&ocodec->pb, m_output_file_name, AVIO_FLAG_WRITE) < 0)
			{
				printf("Could not open '%s'\n", m_output_file_name);
				break;
			}

			// Write a new header at the start of each file
			if (avformat_write_header(ocodec, NULL))
			{
				printf("Could not write mpegts header to first output file\n");
				exit(1);
			}

			prev_segment_time = segment_time;
		}

		ret = av_interleaved_write_frame(ocodec, &packet);
		if (ret < 0) 
		{
			printf("Warning: Could not write frame of stream\n");
		}
		else if (ret > 0) 
		{
			printf("End of stream requested\n");
			av_free_packet(&packet);
			break;
		}

		av_free_packet(&packet);
	} while (!decode_done);

	//****************************************完成输出文件（写尾部）
	uinit_mux();

	if (NUM_SEGMENTS && (int)(last_segment - first_segment) >= NUM_SEGMENTS - 1)
	{
		remove_file = 1;
		first_segment++;
	}
	else 
	{
		remove_file = 0;
	}

	if (write_index) 
	{
		write_index_file(first_segment, ++last_segment, 1, actual_segment_durations);
	}

	if (remove_file)
	{
		sprintf(remove_filename,"%s%s-%u.ts",URL_PREFIX,OUTPUT_PREFIX,first_segment - 1);
		remove(remove_filename);
	}

	return;
}