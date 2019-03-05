#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "write_file_to_flash.h"


//argv[1]:要读取的源文件
int  write_file_to_nor_flash(int argc,char**argv)
{
	if(argc < 2)
	{
		
		printf("ussage: test + inputfile \n");
		return -1;
	}
	
	int read_file = open((const char *)argv[1], O_RDONLY);
	if( read_file < 0)
	{
		
		printf("open read_file  error !\n");
		return -1;
	}
	
	memset((void*)FLASH_START_ADDRESS,0,ERRASE_SIZE);
	int write_file = open((const char *)FLASH_START_ADDRESS, O_WRONLY);
	if(write_file < 0)
	{
		
		printf("open norflash address error !\n");
		return -1;
	}
	
	char read_buffer[READ_BUFFER_SIZE] = {0};
	
	while(1)
	{
		
		int ret = read(read_file,read_buffer,READ_BUFFER_SIZE);
		if(0 == ret)
			break;
		
		ret = write(write_file, read_buffer, ret);
		if(ret < 0)
		{
			printf("write error!\n");
			return -1;
		}
		
	}
	
	
	return 0;
}
