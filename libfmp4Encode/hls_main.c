
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "curl/curl.h"


#include "hls_file.h"
#include "hls_media.h"
#include "hls_mux.h"
#include "mod_conf.h"
#include "typeport.h"


//#include "lame/lame.h"


#define SERVER_TEST			//服务端测试开关

#define INPUT_MP4_FILE      "/jffs0/fmp4.mp4"   	//输入的MP4源文件

//m3u8 param
#define TS_FILE_PREFIX      "ZWG_TEST"              //切片文件的前缀(ts文件)
#define M3U8_FILE_NAME      "ZWG_TEST"         //生成的m3u8文件名
#define URL_PREFIX          "/ramfs/"               //生成目录
#define MAX_NUM_SEGMENTS    50                      //最大允许存储多少个分片
#define SEGMENT_DURATION    5                       //每个分片文件的裁剪时长  


/*获取文件的名字，仅仅只要名字，不要路径*/
static char* get_pure_filename(char* filename)
{
    int len = strlen(filename);
    int pos = len - 1;
    while (pos >= 0 && filename[pos]!='/')
    	--pos;
    return &filename[pos + 1];
}

/*获取文件的名字，仅仅只要名字，不要路径,不要后缀*/
char mp4_name[64] = {0};
static char* get_pure_filename_without_postfix(char* filename)
{
    int len = strlen(filename);
    int pos = len - 1;
    while (pos >= 0 && filename[pos]!='/')
    	--pos;

	strcpy(mp4_name,&filename[pos + 1]);//+1为了去掉 ‘/’
	DEBUG_LOG("mp4_name = %s\n",mp4_name);
	
	len = strlen(mp4_name);
	int i = 0;
	for(i = 0 ; i<=len ; i++)
	{
		if(mp4_name[i] == '.')
		{
			mp4_name[i] = '\0'; //从 '.'处截断
			break;
		}
	}

	DEBUG_LOG("after cut , mp4_name = %s\n",mp4_name);
	
    return mp4_name;
}


/*
获取文件纯路径 /ramfs/aaa.m3u8--->  /ramfs/
注意：使用结束后需要释放 返回值
*/
char* get_pure_pathname(char* filename)
{
    int len = strlen(filename);
    int pos2 = len;
    while (pos2 >= 0 && filename[pos2]!='/') 
	{
    	pos2--;
    }
    char* str=(char*)malloc(sizeof(char)*(pos2+1));
	int bbb=0;
    for(bbb=0; bbb<=pos2; bbb++)
    	str[bbb]=filename[bbb];
    str[pos2+1]=0;
    return str;
}

/*
构造并生成 m3u8文件
@filename:输入文件（绝对路径）
@playlist：输出m3U8文件,播放列表要存储的位置（绝对路径）
@numberofchunks：TS分片文件的个数
返回：
	失败： -1;
	鞥个： 0;
*/
int  generate_playlist_test(char* filename, char* playlist, int* numberofchunks)
{
	media_handler_t* 	media;			//媒体操作句柄（系列函数）
	file_source_t*   	source;			//文件操作句柄（系列函数）
	file_handle_t* 		handle;			//文件句柄（要进行TS切片的文件）
	media_stats_t* 		stats;
	int 				piece;
	int 				stats_size;
	char* 				stats_buffer;
	int 				source_size;
	char*				pure_filename;
	FILE* f;

	//---获取媒体操作的函数句柄-----------------------------
	media  = get_media_handler(filename);
	if ( !media )
	{
		ERROR_LOG("get_media_handler faile !\n");
		return -1;
	}
		

	//---获取文件操作的函数句柄-----------------------------
	source_size = get_file_source(NULL, filename, NULL, 0);

	source 	= (file_source_t*)malloc(source_size);
	if ( !source )
	{
		ERROR_LOG("malloc failed!\n");
		return -1;
	}
		

	source_size  = get_file_source(NULL, filename, source, source_size);
	if ( source_size <= 0 )
	{
		ERROR_LOG("get_media_handler faile !\n");
		return -1;
	}
	
	//---打开mp4文件--------------------------------------------
	handle 	= (char*)malloc(source->handler_size);
	if ( !handle )
	{
		ERROR_LOG("malloc failed!\n");
		return -1;
	}

	if ( !source->open(source, handle, filename, FIRST_ACCESS) )
	{
		ERROR_LOG("open file failed !\n");
		return -1;
	}
	
	//---获取文件的状态信息--------------------------------------------
	stats_size 			= media->get_media_stats(NULL, handle, source, NULL, 0);
	stats_buffer		= (char*)malloc(stats_size);
	if ( !stats_buffer )
	{
		source->close(handle, 0);
		ERROR_LOG("malloc failed!\n");
		return -1;
	}
	memset(stats_buffer,0,stats_size);
	
	stats_size 	  = media->get_media_stats(NULL, handle, source, (media_stats_t*)stats_buffer, stats_size);

	//---生成m3u8文件--------------------------------------------
	DEBUG_LOG("into position G\n");	
	pure_filename = get_pure_filename_without_postfix(filename); //get only filename without any directory info
	if (pure_filename)
	{
		DEBUG_LOG("into position G1\n");	
		int playlist_size 		= generate_playlist((media_stats_t*)stats_buffer, pure_filename, NULL, 0, NULL, &numberofchunks);
		char* playlist_buffer 	= (char*)malloc( playlist_size);
		if ( !playlist_buffer )
		{
			source->close(handle, 0);
			ERROR_LOG("malloc failed!\n");
			return -1;
		}
		DEBUG_LOG("into position H\n"); 

		playlist_size 			= generate_playlist((media_stats_t*)stats_buffer, pure_filename, playlist_buffer, playlist_size, NULL, &numberofchunks);
		if (playlist_size <= 0)
		{
			source->close(handle, 0);
			ERROR_LOG("generate_playlist failed!\n");
			return -1;
		}

		f = fopen(playlist, "wb");
		if (f)
		{
			DEBUG_LOG("write m3u8 file....size(%d)...\n",playlist_size); 
			fwrite(playlist_buffer, 1, playlist_size, f);
			fclose(f);
		}
	
		if (playlist_buffer)
			free(playlist_buffer);
		
		DEBUG_LOG("into position L\n"); 

	}

	source->close(handle, 0);

	if (stats_buffer)
		free(stats_buffer);

	if (handle)
		free(handle);

	if (source)
		free(source);
	DEBUG_LOG("into position M\n"); 

	return 0;
}

char* buf_start = NULL;
char* buf_end = NULL;

/*
对媒体文件进行切片
@filename 为输入文件的名字
返回： 成功 0; 失败：-1
*/
int  generate_piece(char* filename, char* out_filename, int piece)
{
	media_handler_t* 	media;
	file_source_t*   	source;
	file_handle_t* 		handle;
	media_stats_t* 		stats;
	int 				stats_size;
	char* 				stats_buffer;
	int 				source_size;
	char*				pure_filename;
	int 				data_size;
	media_data_t* 		data_buffer;
	int 				muxed_size;
	char* 				muxed_buffer;
	FILE* f;
	
	//获取文件处理句柄（MP4、MP3、wav）
	media  = get_media_handler(filename);
	if ( !media )
	{
		ERROR_LOG("get_media_handler error!\n");
		return -1;
	}
		
	
	//---获取文件操作句柄的大小（打开、读写、关闭系列函数）----------------------------------
	source_size = get_file_source(NULL, filename, NULL, 0);//此处相当于 sizeof(file_source_t)

	source 	= (file_source_t*)malloc(source_size);
	if ( !source )
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}

	source_size  = get_file_source(NULL, filename, source, source_size);
	if ( source_size <= 0 )
	{
		ERROR_LOG("get_file_source error!\n");
		return -1;
	}
		
	//----------------------------------------------------------------------------------------

	handle 	= (char*)malloc(source->handler_size);
	if ( !handle )
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}


	if ( !source->open(source, handle, filename, FIRST_ACCESS) )
	{
		ERROR_LOG("open %s failed !\n",filename);
		return -1;
	}

	//---获取输入媒体文件的信息-------------------------------------------------------------------
	stats_size = media->get_media_stats(NULL, handle, source, NULL, 0);
	if ( stats_size <= 0)
	{
		source->close(handle, 0);
		ERROR_LOG("get_media_stats failed!\n");
		return -1;
	}

	stats_buffer = (char*)malloc( stats_size);
	if ( !stats_buffer )
	{
		source->close(handle, 0);
		ERROR_LOG("malloc faield !\n");
		return -1;
	}

	stats_size = media->get_media_stats(NULL, handle, source, (media_stats_t*)stats_buffer, stats_size);
	if ( stats_size <= 0)
	{
		source->close(handle, 0);
		ERROR_LOG("get_media_stats failed!\n");
		return -1;
	}
	//----------------------------------------------------------------------------------------------

	//---获取输入媒体文件的数据-------------------------------------------------------------------
	DEBUG_LOG("into position I\n");	
	data_size = media->get_media_data(NULL, handle, source, (media_stats_t*)stats_buffer, piece, NULL, 0);
	if (data_size <= 0)
	{
		source->close(handle, 0);
		ERROR_LOG("get_media_data failed!\n");
		return -1;
	}

	data_buffer = (media_data_t*)malloc(data_size);
	if ( !data_buffer )
	{
		source->close(handle, 0);
		ERROR_LOG("malloc failed ! malloc size = %d\n",data_size);
		return -1;
	}	
	memset(data_buffer,0,data_size);
	
	buf_start = (char*)data_buffer;
	buf_end = (char*)data_buffer + data_size;
	

	DEBUG_LOG("============Buffer size(%d) start_pos = %x  end_pos = %x\n",data_size,buf_start,buf_end); 
#if 1
	char* debug_write = (char*)((char*)data_buffer + data_size - 1);
	DEBUG_LOG("into debug_write...write pos = %x\n",debug_write);
	*debug_write = 1;
	DEBUG_LOG("back of debug_write...\n");
#endif

	data_size = media->get_media_data(NULL, handle, source, (media_stats_t*)stats_buffer, piece, data_buffer, data_size);
	if (data_size <= 0)
	{
		source->close(handle, 0);
		ERROR_LOG("get_media_data failed !\n");
		return -1;
	}
	//----------------------------------------------------------------------------------------------

	//----生成TS文件--------------------------------------------------------------------------------
	DEBUG_LOG("into position K\n");
	
	muxed_size = mux_to_ts((media_stats_t*)stats_buffer, data_buffer, NULL, 0);
	if ( muxed_size <= 0 )
	{
		source->close(handle, 0);
		ERROR_LOG("mux_to_ts failed !\n");
		return -1;
	}

	muxed_buffer = (char*)malloc(muxed_size);
	if ( !muxed_buffer )
	{
		source->close(handle, 0);
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
	
	DEBUG_LOG("into position L\n"); 

	muxed_size = mux_to_ts((media_stats_t*)stats_buffer, data_buffer, muxed_buffer, muxed_size);
	if ( muxed_size <= 0 )
	{
		source->close(handle, 0);
		ERROR_LOG("mux_to_ts faile !\n");
		return -1;
	}
	//----------------------------------------------------------------------------------------------

	//----写入输出的TS文件------------------------------------------------------------------------------------------
	f = fopen(out_filename, "wb");
	if (f)
	{
		DEBUG_LOG("fwrite TS file ...... size(%d)......\n",muxed_size);
		fwrite(muxed_buffer, 1, muxed_size, f);
		fclose(f);
	}
	else
	{
		ERROR_LOG("fopen %s failed !\n",out_filename);
	}
	DEBUG_LOG("into position N\n");

	// FOR TEST
	/*
	if(piece == 0) {
		media_stats_t* p;
		p=stats_buffer;
		for (int kkk=0; kkk<300; kkk++)
			printf("\nPTS %d = %f",kkk,p->track[0]->pts[kkk]);
	}
	 */




	///

	//---释放资源----------------------------------------------------------------
	source->close(handle, 0);

	if (source)
		free(source);

	if (handle)
		free(handle);

	if (stats_buffer)
		free(stats_buffer);

	if (data_buffer)
	{
		DEBUG_LOG("i need free data_buffer!\n");
		free(data_buffer);
	}
		

	if (muxed_buffer)
		free(muxed_buffer);


}



typedef struct stream_t{
	char* buf;
	int pos;
	int size;
	int alloc_buffer;
} stream_t;

/*
@size：一次拷贝的大小
@nmemb：一共拷贝多少块
将buffer 中的数据放到stream中的buf中，
*/
static size_t fileWriteCallback(void *buffer, size_t size, size_t nmemb, void *stream)
{
	int bs = size * nmemb;
	stream_t* s = (stream_t*)stream;
	if (s->alloc_buffer)
	{
		if (!s->buf || s->size == 0) //计算需要初始化的内存大小
		{
			int cs = bs + 100;
			if (cs < 10000)
				cs = 10000;

			s->buf 	= (char*)malloc(cs);
			s->size = cs;
			s->pos 	= 0;
		}
		else
		{
			if (s->pos + bs >= s->size)
			{
				s->buf = (char*)realloc(s->buf, s->size * 2);
				s->size *= 2;
			}
		}
	}

	if (s->buf && s->size > s->pos + bs) //确定分配的内存足够
	{
		memcpy(s->buf + s->pos, buffer, bs);
		s->pos += bs;
	}

	return bs;

}

/// \return number of bytes processed
static size_t headerfunc_short ( char * ptr, size_t size, size_t nmemb, int* error_code )
{
	if (!strncmp ( ptr, "HTTP/1.1", 8 )) 
	{
		*error_code   = atoi ( ptr + 9 );
	}
	return nmemb * size;
}


/*
@playlist：返回http接收到的数据buf

*/
int download_file_to_mem(char* http_url, char** playlist)
{
	return 0;

	#if 0
	CURL *curl;
	CURLcode res = CURLE_CHUNK_FAILED;

	char Buf[1024];
	char range[1024];
	int offset = 30000;
	int size = 4096;

	int http_error_code = 0;
	stream_t stream = {0};

	if (playlist)
		stream.alloc_buffer = 1;
	else
		stream.alloc_buffer = 0;

	curl = curl_easy_init();
	if(curl) 
	{
		curl_easy_setopt(curl, CURLOPT_URL, http_url);

		curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, fileWriteCallback );
		curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &stream ); //设置回调函数中(fileWriteCallback)的void *stream 指针的来源。
		curl_easy_setopt ( curl, CURLOPT_HEADERFUNCTION, headerfunc_short );
		curl_easy_setopt ( curl, CURLOPT_HEADERDATA, &http_error_code );
		curl_easy_setopt ( curl, CURLOPT_VERBOSE, 1 );

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", 	curl_easy_strerror(res));
		}

		curl_easy_cleanup(curl);
	}

	if (playlist)
	{
		*playlist = stream.buf;
	}
	return res;
	#endif

}

int get_segments_count(char* playlist)
{
	int c = 0;
	int i = 0;
	int len = strlen(playlist);
	for(i = 0; i < len - 1; ++i)
	{
		if (playlist[i] == 0x0A && playlist[i+1] != '#')
		{
			++c;
		}
	}
	return c;
}

void get_segment_name(char* segment_name, int segment_name_size, char* playlist, int segment_num){
	int c = 0;
	int i = 0;
	int k;
	int len = strlen(playlist);
	for(i = 0; i < len - 1; ++i){
		if (playlist[i] == 0x0A && playlist[i+1] != '#'){
			if (c == segment_num){
				k = 0;
				while( i + k + 1 < len && playlist[i+k+1] != 0x0A && k < segment_name_size ){
					segment_name[k] = playlist[i+k+1];
					++k;
				}
				segment_name[k] = 0;

				break;
			}
			++c;
		}
	}
}

void get_file_url(char* file_url, int file_url_size, char* http_url, char* segment_name){
	int pos = strlen(http_url);
	int i;
	while(pos >= 0 && http_url[pos] != '/'){
		--pos;
	}
	for(i = 0; i < pos; ++i){
		file_url[i] = http_url[i];
	}
	file_url[pos] = 0;
	strcat(file_url, "/");
	strcat(file_url, segment_name);
}

void process_hls_stream(char* http_url)
{
	char* playlist = NULL;
	int i;
	while (download_file_to_mem(http_url, &playlist) != CURLE_OK);

	if (playlist)
	{
		int segments_count = get_segments_count(playlist);

		for(i=0; i < segments_count; ++i)
		{
			char file_url[2048];
			char segment_name[2048];

			get_segment_name(segment_name, sizeof(segment_name), playlist, i);

			get_file_url(file_url, sizeof(file_url), http_url, segment_name);
			while( download_file_to_mem(file_url, NULL) != CURLE_OK);
		//	usleep(1000000);
		}

		free(playlist);
	}

}

//typedef struct size_error_t{
//	size_t size;
//	int error;
//}size_error_t;
//
///// \return number of bytes processed
//static size_t headerfunc_content_length ( char * ptr, size_t size, size_t nmemb, size_error_t* se )
//{
//	if (!strncmp ( ptr, "HTTP/1.1", 8 )) {
//		 se->error  = atoi ( ptr + 9 );
//	}
//	if (!strncmp ( ptr, "Content-Length:", 15 )) {
//		 se->size  = atoi ( ptr + 16 );
//	}
//
//	return nmemb * size;
//}
//
//CURLcode curl_get_resource_size(char* url, size_t* size, int* error_code){
//	 CURLcode res = CURLE_CHUNK_FAILED;
//	 CURL* ctx = curl_easy_init();
//	 if (ctx){
//		 size_error_t se;
//		 struct curl_slist *headers = NULL;
//
//		 headers = curl_slist_append(headers,"Accept: */*");
//		 if (headers){
//			 curl_easy_setopt(ctx,CURLOPT_HTTPHEADER, 	headers );
//			 curl_easy_setopt(ctx,CURLOPT_NOBODY,		1 );
//			 curl_easy_setopt(ctx,CURLOPT_URL,			url );
//			 curl_easy_setopt(ctx,CURLOPT_NOPROGRESS,	1 );
//			 curl_easy_setopt(ctx, CURLOPT_HEADERFUNCTION, headerfunc_content_length );
//			 curl_easy_setopt(ctx, CURLOPT_HEADERDATA,	&se );
//			 curl_easy_setopt(ctx, CURLOPT_VERBOSE, 0 );
//
//			 res = curl_easy_perform(ctx);
//			 curl_easy_cleanup(ctx);
//
//			 if (res == CURLE_OK){
//				 if (size){
//					 *size = se.size;
//				 }
//				 if (error_code){
//					 *error_code = se.error;
//				 }
//			 }
//		 }
//	 }
//	 return res;
//}
//
//typedef struct range_request_t{
//	char* buf;
//	int pos;
//	int size;
//} range_request_t;
//
//static size_t range_request_func(void *buffer, size_t size, size_t nmemb, void *stream)
//{
//	int bs = size * nmemb;
//	range_request_t* rr = (stream_t*)stream;
//
//	if (rr->buf && rr->size >= rr->pos + bs){
//		memcpy(rr->buf + rr->po s, buffer, bs);
//		rr->pos += bs;
//	}
//
//	return bs;
//
//}
//
//CURLcode curl_get_data(char* http_url, char* buffer, long long offset, long long requested_size, long long * received_size){
//	CURL *curl;
//	CURLcode res = CURLE_CHUNK_FAILED;
//	char range_str[128];
//	range_request_t rr;
//
//	rr.buf = buffer;
//	rr.pos = 0;
//	rr.size = requested_size;
//
//	curl = curl_easy_init();
//	if(curl) {
//		curl_easy_setopt(curl, CURLOPT_URL, http_url);
//
//		snprintf(range_str, sizeof(range_str), "%lld-%lld", (long long)offset, (long long)(offset + requested_size - 1));
//
//
//		curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, range_request_func );
//		curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &rr );
//		curl_easy_setopt ( curl, CURLOPT_VERBOSE, 0 );
//		curl_easy_setopt ( curl, CURLOPT_RANGE, range_str);
//
//		/* Perform the request, res will get the return code */
//		res = curl_easy_perform(curl);
//		/* Check for errors */
//		if(res != CURLE_OK){
//			fprintf(stderr, "curl_easy_perform() failed: %s\n", 	curl_easy_strerror(res));
//		}else{
//			if (received_size)
//				*received_size = rr.pos;
//		}
//
//		curl_easy_cleanup(curl);
//	}
//
//	return res;
//}

int hex_to_int(char c){
		switch (c){
			case '0': return 0;
			case '1': return 1;
			case '2': return 2;
			case '3': return 3;

			case '4': return 4;
			case '5': return 5;
			case '6': return 6;
			case '7': return 7;

			case '8': return 8;
			case '9': return 9;
			case 'a':
			case 'A': return 10;

			case 'b':
			case 'B': return 11;


			case 'c':
			case 'C': return 12;

			case 'd':
			case 'D': return 13;

			case 'e':
			case 'E': return 14;

			case 'f':
			case 'F': return 15;

		}
	}

	char convert_str_to_char(char c1, char c2){
		return (hex_to_int(c1) << 4) | hex_to_int(c2);
	}

	char* get_arg_value(/*request_rec * r,*/ char* args, char* key){
		int i,j;
		int args_len = strlen(args);
		int key_len= strlen(key);
		//char* result = apr_pcalloc(r->pool, args_len + 1);
		char* result = (char*)malloc(args_len + 1);
		int level = 0;
		memset(result, 0, args_len + 1);

		for(i = 0; i < args_len; ++ i){
			int found = 0;
			if (i == 0){
				found = (strncmp(&args[ i + j ], key, key_len) == 0) ? 1 : 0;
			}else{
				if (args[i - 1] == '&'){
					found = (strncmp(&args[ i + j ], key, key_len) == 0) ? 1 : 0;
				}
			}
			if (found){
				j = 0;
				i+=key_len+1;
				if (i + j + 3< args_len && args[i+j] == '%' && args[i+j+1] == '2' && args[i+j+2] == '2')
					i+=3;

				while ( i + j < args_len && args[j] != '&'){
					result[j] = args[i+j];
					++j;
				}

				if (j > 3 && result[j-3] == '%' && result[j-2] == '2' && result[j-1] == '2'){
					j-=3;
					result[j] =0 ;
				}
				break;
			}

		}

		return result;
	}

extern void hls_exit(void);

int hls_main (int argc, char* argv[])
{


	//argv[1]=("/home/bocharick/Work/testfiles/testfile5.mp4");
	//argv[2]=("/home/bocharick/Work/1/");
	//argv[3]=("/home/bocharick/Work/testfiles/logo.h264");
	printf("into hls_main!\n");
	set_encode_audio_bitrate(16000);
	set_segment_length(5);		// 设置单个TS文件切片时长
	set_allow_mp4(1);
	set_logo_filename(NULL);
	printf("into hls_main!01\n");

	char path[1024];
	sprintf(path,"%s%s.m3u8",URL_PREFIX,M3U8_FILE_NAME);

	int counterrr=0; //ts分片文件的个数


	/*---生成m3u8文件--------------------------------------------------------------------------*/
	if(generate_playlist_test(INPUT_MP4_FILE,path,&counterrr) < 0)
		ERROR_LOG("generate_playlist_test failed !\n");
	/*---生成TS切片文件------------------------------------------------------------------------*/
	int i = 0;
	
	for(i = 0; i < counterrr; ++i) //循环一次生成一个TS切片文件 
	{
		DEBUG_LOG("into position O\n"); 
		char tmp[1024];
		sprintf(tmp, "%s%s_%d.ts",get_pure_pathname(URL_PREFIX),get_pure_filename_without_postfix(INPUT_MP4_FILE), i);
		
		if(generate_piece(INPUT_MP4_FILE, tmp, i) < 0)
		{
			ERROR_LOG("generate_piece %s i=%d error!\n",INPUT_MP4_FILE,i);
			return -1;
		}
	}

	
	DEBUG_LOG("At the position of  END!\n");
	hls_exit();
	return 0;
}





