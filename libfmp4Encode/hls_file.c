/*
 * Implementations for file operations
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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "string.h"
/*#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "http_request.h"
#include "util_script.h"
#include "http_connection.h"*/

#include "hls_file.h"
#include "mod_conf.h"

//#define DISABLE_CACHE
//apr_pool_t* server_pool = NULL;

/*void set_server_pool(apr_pool_t* p){
	server_pool = p;
}*/
#define HLS_GLOBAL_CACHE_NAME "voicebase-hls-plugin-cache"

#define TEST_APP

typedef struct apache_file_handler_t{
#ifndef TEST_APP
	apr_file_t* f;
	request_rec* r;
#endif
} apache_file_handler_t;

int file_apache_get_file_size(file_handle_t* handler, int flags){
#ifndef TEST_APP
	apache_file_handler_t* afh = (apache_file_handler_t*)handler;
	apr_off_t offset = 0;
	apr_size_t size = 0;
	int rc;

	apr_finfo_t finfo;
	int exists;
	char* filename = NULL;
	rc = apr_file_name_get(&filename, afh->f);
	if (rc == APR_SUCCESS){
		rc = apr_stat(&finfo, filename, APR_FINFO_MIN, afh->r->pool);
		if (rc == APR_SUCCESS) {
			return finfo.size;
		}
	}
#endif
	return 0;
}

int file_apache_open(file_source_t* src, file_handle_t* handler, char* filename, int flags){
	apache_file_handler_t* afh = (apache_file_handler_t*)handler;
	int rc;
#ifndef TEST_APP
	afh->f = NULL;
	afh->r = (request_rec*)src->context;

	rc = apr_file_open(&afh->f, filename, APR_READ | APR_BINARY, APR_OS_DEFAULT, afh->r->pool);
	return rc == APR_SUCCESS ? 1 : 0;
#endif
	return 0;//APR_SUCCESS ? 1 : 0;
}

int file_apache_read(file_handle_t* handler, void* output_buffer, int data_size, int offset_from_file_start, int flags){
#ifndef TEST_APP
	apache_file_handler_t* afh = (apache_file_handler_t*)handler;
	apr_off_t offset = offset_from_file_start;
	apr_size_t s = data_size;

	if (apr_file_seek(afh->f, APR_SET, &offset) == APR_SUCCESS && offset == offset_from_file_start){
		if (apr_file_read(afh->f, output_buffer, &s) == APR_SUCCESS ) {
			return s;
		}
	}
#endif
	return 0;
}

int file_apache_close(file_handle_t* handler, int flags){
	apache_file_handler_t* afh = (apache_file_handler_t*)handler;
#ifndef TEST_APP
	apr_file_close(afh->f);
#endif
	return 0;
}

typedef struct os_file_handler_t{
	FILE* f;
} os_file_handler_t;

int file_os_open(file_source_t* src, file_handle_t* handler, char* filename, int flags){
	os_file_handler_t* ofh = (os_file_handler_t*)handler;
	ofh->f = fopen(filename, "rb");
	return ofh->f ? 1 : 0;
}

int file_os_read(file_handle_t* handler, void* output_buffer, int data_size, int offset_from_file_start, int flags){
	os_file_handler_t* ofh = (os_file_handler_t*)handler;
	FILE* f = ofh->f;
	fseek(f, offset_from_file_start, SEEK_SET);
	return fread(output_buffer,1 , data_size, f);
}

int file_os_get_file_size(file_handle_t* handler, int flags){
	os_file_handler_t* ofh = (os_file_handler_t*)handler;
	FILE* f = ofh->f;
	int cp = ftell(f);
	int result;
	fseek(f, 0, SEEK_END);
	result = ftell(f);
	fseek(f, cp, SEEK_SET);
	return result;
}

int file_os_close(file_handle_t* handler, int flags){
	os_file_handler_t* ofh = (os_file_handler_t*)handler;
	FILE* f = ofh->f;
	if (f)
		fclose(f);
	ofh->f = NULL;
	return 0;
}

#ifndef TEST_APP

typedef struct http_file_handler_t{
	char 			link[2048];
	size_t			content_length;
	int				last_error_code;
#ifndef TEST_APP
	request_rec* 	r;
#endif
} http_file_handler_t;

typedef struct size_error_t{
	size_t size;
	int error;
}size_error_t;

/// \return number of bytes processed
static size_t headerfunc_content_length ( char * ptr, size_t size, size_t nmemb, size_error_t* se )
{
	if (!strncmp ( ptr, "HTTP/1.1", 8 )) {
		 se->error  = atoi ( ptr + 9 );
	}
	if (!strncmp ( ptr, "Content-Length:", 15 )) {
		 se->size  = atoi ( ptr + 16 );
	}

	return nmemb * size;
}

CURLcode curl_get_resource_size(char* url, size_t* size, int* error_code, int flags){
	 CURLcode res = CURLE_CHUNK_FAILED;
	 char mod_url[2048];
	 if (!(flags & FIRST_ACCESS))
		 snprintf(mod_url, sizeof(mod_url), "%s?access=server", url);
	 else
		 strncpy(mod_url, url, sizeof(mod_url));

	 CURL* ctx = curl_easy_init();
	 if (ctx){
		 size_error_t se;
		 struct curl_slist *headers = NULL;

		 headers = curl_slist_append(headers,"Accept: */*");
		 if (headers){
			 curl_easy_setopt(ctx,CURLOPT_HTTPHEADER, 	headers );
			 curl_easy_setopt(ctx,CURLOPT_NOBODY,		1 );
			 curl_easy_setopt(ctx,CURLOPT_URL,			mod_url );
			 curl_easy_setopt(ctx,CURLOPT_NOPROGRESS,	1 );
			 if (get_allow_redirect())
				 curl_easy_setopt(ctx, CURLOPT_FOLLOWLOCATION, 1);
			 curl_easy_setopt(ctx, CURLOPT_HEADERFUNCTION, headerfunc_content_length );
			 curl_easy_setopt(ctx, CURLOPT_HEADERDATA,	&se );
			 curl_easy_setopt(ctx, CURLOPT_VERBOSE, 0 );

			 res = curl_easy_perform(ctx);
			 curl_easy_cleanup(ctx);

			 if (res == CURLE_OK){
				 if (size){
					 *size = se.size;
				 }
				 if (error_code){
					 *error_code = se.error;
				 }
			 }
		 }
	 }
	 return res;
}

typedef struct range_request_t{
	char* buf;
	int pos;
	int size;
	long long full_size;
	int error;
} range_request_t;

void parse_content_range(char* str, int* start, int* stop, long long* full_size){
	char ss[128];
	long long b;
	long long e;
	int n = 0;
	while(n + 1 < sizeof(ss) && str[n] >= 0x20){
		ss[n] = str[n];
		++n;
	}
	ss[n] = 0;

	sscanf(ss, "%lld-%lld/%lld",&b, &e, full_size);
	if (start)
		*start = b;
	if (stop)
		*stop = e;
}

/// \return number of bytes processed
static size_t headerfunc_range_request ( char * ptr, size_t size, size_t nmemb, range_request_t* se )
{
	int start = 0;
	int stop  = 0;

	if (!strncmp ( ptr, "HTTP/1.1", 8 )) {
		 se->error  = atoi ( ptr + 9 );
	}
	if (!strncmp ( ptr, "Content-Range", 13 )) {
		parse_content_range(ptr + 21, &start, &stop, &se->full_size);
	}

	return nmemb * size;
}

static size_t range_request_func(void *buffer, size_t size, size_t nmemb, void *stream)
{
	int bs = size * nmemb;
	range_request_t* rr = (range_request_t*)stream;

	if (rr->buf && rr->size >= rr->pos + bs){
		memcpy(rr->buf + rr->pos, buffer, bs);
		rr->pos += bs;
	}

	return bs;

}

static size_t min(size_t a, size_t b){
	return  a > b ? b : a;
}

CURLcode curl_get_data(char* http_url, char* buffer, long long offset, long long requested_size, long long * received_size, int flags, long long* full_size){
	CURL *curl;
	CURLcode res = CURLE_CHUNK_FAILED;
	char range_str[128];
	range_request_t rr;
	char mod_url[2048];

	rr.buf = buffer;
	rr.pos = 0;
	rr.size = requested_size;

	if (!(flags & FIRST_ACCESS))
		 snprintf(mod_url, sizeof(mod_url), "%s?access=server", http_url);
	 else
		 strncpy(mod_url, http_url, sizeof(mod_url));

	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, mod_url);
		snprintf(range_str, sizeof(range_str), "%lld-%lld", (long long)offset, (long long)(offset + requested_size - 1));

		curl_easy_setopt ( curl, CURLOPT_WRITEFUNCTION, range_request_func );
		if (get_allow_redirect())
			curl_easy_setopt ( curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt ( curl, CURLOPT_WRITEDATA, &rr );
		curl_easy_setopt ( curl, CURLOPT_VERBOSE, 0 );
		curl_easy_setopt ( curl, CURLOPT_RANGE, range_str);
		curl_easy_setopt ( curl, CURLOPT_HEADERFUNCTION, headerfunc_range_request);
		curl_easy_setopt( curl, CURLOPT_HEADERDATA,	&rr );

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if(res != CURLE_OK){
			fprintf(stderr, "curl_easy_perform() failed: %s\n", 	curl_easy_strerror(res));
		}else{
			if (received_size)
				*received_size 	= rr.pos;
			if (full_size)
				*full_size 		= rr.full_size;
		}

		curl_easy_cleanup(curl);
	}

	return res;
}

typedef struct hls_cache_line_t{
	size_t 	size;
	char	buffer[256];
} hls_cache_line_t;

int http_open(file_source_t* src, file_handle_t* handler, char* filename, int flags){
	http_file_handler_t* ofh = (http_file_handler_t*)handler;
	request_rec* req_r = (request_rec*)src->context;
	size_t size;
	int error_code;
	hls_cache_line_t cl;

	if (filename[0] == 0)
		return 0;

	ap_log_error(APLOG_MARK, APLOG_WARNING, get_log_level(), req_r->server, "HLS file: trying to open HTTP data source %s", filename);

	strncpy(ofh->link, filename, sizeof(ofh->link));

	ofh->r = req_r;
	ofh->content_length = 0;
	ofh->last_error_code = 0;

	ap_log_error(APLOG_MARK, APLOG_WARNING, get_log_level(), req_r->server, "HLS file: trying to read first %d bytes to and cache it", (int)(sizeof(cl.buffer)));

	http_read(ofh, cl.buffer, sizeof(cl.buffer), 0, flags);

	ap_log_error(APLOG_MARK, APLOG_WARNING, get_log_level(), req_r->server, "HLS file: got content length %d bytes", (int)ofh->content_length);

	return ofh->content_length != 0;
}

int http_read(file_handle_t* handler, void* output_buffer, int data_size, int offset_from_file_start, int flags){
	http_file_handler_t* ofh = (http_file_handler_t*)handler;
	long long bytes_read = 0;
	request_rec* req_r = ofh->r;
	apr_pool_t* cache_pool = NULL;
	hls_cache_line_t* cache_line = NULL;
	long long full_size = 0;

//	ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 0");

	if (offset_from_file_start == 0 && data_size == sizeof(cache_line->buffer)){

		apr_pool_userdata_get(&cache_pool, HLS_GLOBAL_CACHE_NAME, req_r->server->process->pool);

		if (cache_pool){
//			ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 2");

			apr_pool_userdata_get(&cache_line, ofh->link, cache_pool);
		}else{
			apr_pool_create(&cache_pool, req_r->server->process->pool);
			if (cache_pool){
				//ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "cache created");
				apr_pool_userdata_set(cache_pool, HLS_GLOBAL_CACHE_NAME, NULL, req_r->server->process->pool);
			}
		}

//		ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 3");

		if (cache_pool && !cache_line){
//			ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 4");

			cache_line = (char*)apr_pcalloc(cache_pool, sizeof(hls_cache_line_t));
			if (cache_line){
//				ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 5");

				cache_line->size = 0;
				memset(cache_line->buffer, 0 , sizeof(cache_line->buffer));
				apr_pool_userdata_set(cache_line, ofh->link, NULL, cache_pool);
				//ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "put file size to cache. file =%s, size=%d", filename, (int)size);
			}
		}

		if (cache_line){
			//ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "cache found");

			if (cache_line->buffer[0] == 0 && cache_line->buffer[1] == 0 && cache_line->buffer[2] == 0 && cache_line->buffer[3] == 0 || (flags & FIRST_ACCESS)){
//				ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 6 %s with data offset = %d, data_size=%d", ofh->link, (int)offset_from_file_start, (int)data_size );

				CURLcode res = curl_get_data(ofh->link, output_buffer, offset_from_file_start, data_size, &bytes_read, flags, &full_size);
				ap_log_error(APLOG_MARK, APLOG_WARNING, get_log_level(), req_r->server, "HLS file: get header data returns result %d", (int)res);

				if (res == CURLE_OK){
					ofh->content_length 	= cache_line->size = full_size;
					memcpy(cache_line->buffer, output_buffer, min(sizeof(cache_line->buffer),bytes_read));
					return bytes_read;
				}
			}else{
				//ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "get values from cache");
				memcpy(output_buffer, cache_line->buffer,  min(sizeof(cache_line->buffer),cache_line->size));
			}
//			ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 8");

			ofh->content_length 	= cache_line->size;
			ofh->last_error_code 	= 0;
			return cache_line->size;
		}
	}

//	ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 9 %s with data offset = %d, data_size=%d", ofh->link, (int)offset_from_file_start, (int)data_size );

	CURLcode res = curl_get_data(ofh->link, output_buffer, offset_from_file_start, data_size, &bytes_read, flags, &full_size);
	if (ofh->content_length == 0)
		ofh->content_length = full_size;

	ap_log_error(APLOG_MARK, APLOG_WARNING, get_log_level(), req_r->server, "HLS file: get data returns result %d", (int)res);

//	ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "http read 10");

	//ap_log_error(APLOG_MARK, APLOG_ERR, 0, req_r->server, "reading non cached data filename=%s, offset=%d, size=%d", ofh->link, offset_from_file_start, data_size);
	if (res == CURLE_OK){
		return bytes_read;
	}

	return 0;//return number of bytes
}

int http_get_file_size(file_handle_t* handler, int flags){
	http_file_handler_t* ofh = (http_file_handler_t*)handler;
	return ofh->content_length;
}

int http_close(file_handle_t* handler, int flags){
	http_file_handler_t* ofh = (http_file_handler_t*)handler;
	ofh->content_length = 0;
	ofh->link[0] = 0;
	return 0;
}
#endif

/*
获取文件打开、读写、关闭的函数句柄
*/
int get_file_source(void* context, char* filename, file_source_t* buffer, int buffer_size)
{
	int len = strlen(filename);
	int http = 0;
	if (len > 4)
	{
		#ifndef TEST_APP
			if ((filename[0] == 'h' || filename[0] == 'H') &&
				(filename[1] == 't' || filename[1] == 'T') &&
				(filename[2] == 't' || filename[2] == 'T') &&
				(filename[3] == 'p' || filename[3] == 'P'))
			{
					http = 1;
			}
		#endif
	}

	if (http)
	{

		#ifndef TEST_APP
			if (buffer && buffer_size >= sizeof(file_source_t)){
				buffer->handler_size 	= sizeof(http_file_handler_t);
				buffer->get_file_size 	= http_get_file_size;
				buffer->open			= http_open;
				buffer->read			= http_read;
				buffer->close			= http_close;
				buffer->context			= context;
			}
		#endif

		return sizeof(file_source_t);
	}
	else
	{
		if (buffer && buffer_size >= sizeof(file_source_t))
		{
			memset(buffer, 0, buffer_size);
			if (context != NULL)
			{
				buffer->handler_size 	= sizeof(apache_file_handler_t);
				buffer->get_file_size 	= file_apache_get_file_size;
				buffer->open			= file_apache_open;
				buffer->read			= file_apache_read;
				buffer->close			= file_apache_close;
			}
			else  //使用操作系统的文件操作函数
			{
				buffer->handler_size 	= sizeof(os_file_handler_t);
				buffer->get_file_size 	= file_os_get_file_size;
				buffer->open			= file_os_open;
				buffer->read			= file_os_read;
				buffer->close			= file_os_close;
			}

			buffer->context = context;
		}
		return sizeof(file_source_t);
	}
}



