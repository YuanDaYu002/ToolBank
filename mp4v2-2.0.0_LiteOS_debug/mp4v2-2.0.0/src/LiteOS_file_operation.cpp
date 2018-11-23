

/***********************************************
提供LiteOS自己的一套文件操作函数，MP4V2源代码
里边默认的底层文件操作函数在LiteOS上运行会挂掉
************************************************/
//以下函数都是针对File.h中 结构 MP4FileProvider 中的成员函数来实现的

/** Structure of functions implementing custom file provider.
 *
 *  Except for <b>open</b>, all the functions must return a true value
 *  to indicate failure or false on success. The open function must return
 *  a pointer or handle which represents the open file, otherwise NULL.
 *
 *  maxChunkSize is a hint suggesting what the max size of data should be read
 *  as in underlying read/write operations. A value of 0 indicates there is no hint.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>
//#include "LiteOS_file_operation.h"


#ifndef true 
#define true 1
#endif

#ifndef false
#define false 0
#endif


	
enum Mode 
{
        MODE_UNDEFINED, //!< undefined
        MODE_READ,      //!< file may be read
        MODE_MODIFY,    //!< file may be read/written
        MODE_CREATE,    //!< file will be created/truncated for read/write
};

typedef struct _LiteOs_File_t
{
	int file_handle;
}LiteOs_File_t;

static LiteOs_File_t LiteOS_file_handle = {0};

void* LiteOS_open( const char* name, MP4FileMode mode )
{
	if(NULL == name)
		return NULL;
	
	int flags;
	mode_t my_mode = 0;
	int ret = -1;
	
	if(mode == MODE_UNDEFINED|| mode == MODE_CREATE)
	{
		flags = O_RDWR|O_CREAT;
		my_mode = 0666;
	}
	else if(mode == MODE_READ)
	{
		flags = O_RDONLY;
	}
	else if (mode == MODE_MODIFY)
	{
		flags = O_WRONLY;
	}
	else
	{
		printf("open error! file(%s)line(%d)\n",__FILE__,__LINE__);
		return NULL;
	}
	
	
	if(0 == my_mode)
	{
		ret = open(name,flags);
	}
	else
	{
		ret = open(name,flags,my_mode);
	}

	if(ret < 0)
	{
		printf("open error! file(%s)line(%d)\n",__FILE__,__LINE__);
		return NULL;
	}
	
	LiteOS_file_handle.file_handle = ret;
	return &LiteOS_file_handle;
	
}

int  LiteOS_seek( void* handle, int64_t pos )
{
	LiteOs_File_t* file_handle = (LiteOs_File_t*)handle;
	
	int fd = file_handle->file_handle;
	int ret = lseek(fd,pos,SEEK_SET);//默认从文件开头开始计算
	if(ret < 0)
	{
		
		printf("lseek error! file(%s)line(%d)\n",__FILE__,__LINE__);
		return false;
	}
	return true;
		
}

//nin 和 maxChunkSize没有使用
int LiteOS_read( void* handle, void* buffer, int64_t size, int64_t* nin, int64_t maxChunkSize )
{
	if(NULL == handle||NULL == buffer)
		return false;
	LiteOs_File_t* file_handle = (LiteOs_File_t*)handle;
	int fd = file_handle->file_handle;
	
	int ret = read(fd,buffer,size);
	if(ret < 0)
		return false;
	
	return true;
}


int LiteOS_write ( void* handle, const void* buffer, int64_t size, int64_t* nout, int64_t maxChunkSize )
{
		if(NULL == handle||NULL == buffer)
			return false;
		LiteOs_File_t* file_handle = (LiteOs_File_t*)handle;
		int fd = file_handle->file_handle;
		
		int ret = write(fd, buffer,size);
		if(ret < 0)
			return false;
		
		return true;

}

int LiteOS_close ( void* handle )
{
	if(NULL == handle)
		return false;
	LiteOs_File_t* file_handle = (LiteOs_File_t*)handle;
	int fd = file_handle->file_handle;
	
	close(fd);
	
	return true;
	
}



