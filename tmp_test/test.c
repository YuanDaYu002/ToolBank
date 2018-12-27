 
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>





int main()
{
	
	int fd = open("./fmp4.mp4",O_RDONLY);
	if(fd < 0)
	{
		perror("open error!\n");
		return -1;		
	}

	
	unsigned int file_len =	lseek(fd,0,SEEK_END);
	printf("file_len (%d)\n",file_len);


	close(fd);
return 0;

}

