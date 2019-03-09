/*
 * File operations definition
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

#ifndef __HLS_FILE__
#define __HLS_FILE__

typedef void file_handle_t;

/*文件基本操作函数句柄*/
typedef struct file_source_t{
	int (*open)(struct file_source_t* src,file_handle_t* handler, char* filename, int flags);
	int (*read)(file_handle_t* handler, void* output_buffer, int data_size, int offset_from_file_start, int flags);
	int (*get_file_size)(file_handle_t* handler, int flags);
	int (*close)(file_handle_t* handler, int flags);

	void* context;
	int handler_size;
} file_source_t;

int get_file_source(void* context, char* filename, file_source_t* buffer, int buffer_size);
#define FIRST_ACCESS 1

#endif

