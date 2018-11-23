#include "libplatform/impl.h"
#include <sys/stat.h>
#include <stdio.h>


namespace mp4v2 { namespace platform { namespace io {

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::exists( string path_ )
{
    struct stat buf;
    return stat( path_.c_str(), &buf ) == 0;
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isDirectory( string path_ )
{
    struct stat buf;
    if( stat( path_.c_str(), &buf ))
        return false;
    return S_ISDIR( buf.st_mode );
}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::isFile( string path_ )
{
    struct stat buf;
    if( stat( path_.c_str(), &buf ))
        return false;
    return S_ISREG( buf.st_mode );
}

///////////////////////////////////////////////////////////////////////////////
//成功：返回0      ; 失败：返回 1
bool
FileSystem::getFileSize( string path_, File::Size& size_ )
{
#if 0   // have BUG in LiteOS 
    size_ = 0;
    struct stat buf;
	printf("into getFileSize!\n");
	
    if( stat( path_.c_str(), &buf ))//stst 返回0 为成功  -1为失败
    {
		printf("getFileSize failed !\n");
		return true;  //代表失败
	}
	printf("getFileSize success ! buf.st_size = %d\n",buf.st_size);
     
    size_ = buf.st_size;

#endif

	   	FILE * fImage;
		int Length;
		if((fImage=fopen(path_.c_str(),"rb"))!=NULL)//寻找文件的大小!
		{
			fseek(fImage,0,SEEK_END);
			Length=ftell(fImage);
			printf("file  size = %ld\n",Length);
			fseek(fImage,0,SEEK_SET);
	
			fclose(fImage);
		}
		else
		{
			printf("getFileSize failed! \n");
			return true; //代表失败
		}
		
		size_ = Length;

    return false;  //代表成功

}

///////////////////////////////////////////////////////////////////////////////

bool
FileSystem::rename( string from, string to )
{
    return ::rename( from.c_str(), to.c_str() ) != 0;
}

///////////////////////////////////////////////////////////////////////////////

string FileSystem::DIR_SEPARATOR  = "/";
string FileSystem::PATH_SEPARATOR = ":";

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::io

