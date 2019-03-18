

#include <stdio.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



int main(int argc, const char *argv[])
{
    
    SDL_PixelFormat *fmt;
    TTF_Font *font;  
    SDL_Surface *text, *temp;  
	
	char* 	str = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	int 	str_num = strlen(str);
	char 	str_i[2] = {0};
	printf("str_num = %d\n",str_num);

	/*TTF系统初始化*/
    if (TTF_Init() < 0 ) 
    {  
        fprintf(stderr, "Couldn't initialize TTF: %s\n", SDL_GetError());  
        SDL_Quit();
    }  
	
	/*
 	打开TTF字体包文件:
		按实际字体点阵大小来看,12号字体最接近8*16点阵
		48适合 1920*1080分辨率
		24适合 960*544分辨率
		12适合 480*272分辨率
	*/
	
    //font = TTF_OpenFont("./MSYHBD.TTF", 12); 
    font = TTF_OpenFont("./Microsoft_Yahei.ttf", 48); 
    if ( font == NULL ) 
    {  
        fprintf(stderr, "Couldn't load %d pt font from %s: %s\n", 18, "ptsize", SDL_GetError());  
    }  

    SDL_Color forecol = { 0xff, 0xff, 0xff, 0xff };  //字体颜色

	int i = 0;
	fmt = (SDL_PixelFormat*)malloc(sizeof(SDL_PixelFormat));
	memset(fmt,0,sizeof(SDL_PixelFormat));
	fmt->BitsPerPixel = 16;  //每个像素点16个bit位
    fmt->BytesPerPixel = 2;	 //每个像素点2字节
   // fmt->colorkey = 0xffffffff; //RGB24 (888) 白色
   // fmt->alpha = 0xff;
	
	for(i = 0 ; i<str_num ; i++)
	{
		str_i[0] = str[i];
		str_i[1] = '\0';
		
		/*
		渲染UTF8字符串，按照：
			font：给定的字体包
			pstr：需要渲染的字符串
			forecol：给定的颜色
		*/
	    text = TTF_RenderUTF8_Solid(font, str_i, forecol);
		if(NULL == text)
		{
			perror("TTF_RenderUTF8_Solid failed !\n");
			return -1;
		}

		/*
		对 数据 按照 fmt 设置的参数进行转换
		得到 BMP数据、转换后的像素数据
		*/
	    temp = SDL_ConvertSurface(text,fmt,0);
		if(NULL == temp)
		{
			SDL_FreeSurface(text); 
			perror("SDL_ConvertSurface failed !\n");
			goto ERR;
		}
		
		printf("%s:BMP_w(%d) BMP_h(%d)\n",str_i,temp->w,temp->h);
		
		/*从内存拷贝出来直接保存到文件*/
		#if 0
		unsigned int malloc_size = temp->w * temp->h * 3;
		char* bmp_buf = (char*)malloc(malloc_size);
		if(NULL == bmp_buf)
		{
			perror("malloc bmp buf failed !\n");
			SDL_FreeSurface(text); 
			SDL_FreeSurface(temp);
			goto ERR;
		}
		memset(bmp_buf,0,malloc_size);

		
		memcpy(bmp_buf,temp->pixels,malloc_size);
		if('A' == str_i[0])
		{
			int fd = open("./A_buf_bmp.bmp", O_WRONLY | O_CREAT, 0766);
			write(fd, bmp_buf, temp->w * temp->h * 2);
			close(fd);
		}
		#endif
		
		/*调用接口直接保存到文件*/
		#if 0
		SDL_RWops  dst;
		SDL_SaveBMP_RW(temp,&dst,0);
		unsigned raw_size = dst.seek(&dst,0,RW_SEEK_END);
		printf("raw_size = %d\n",raw_size);
		#endif
		
		/*保存到bmp文件*/
		char name[64] = {0};
		snprintf(name,19,"./bmp/save_%03d.bmp",(unsigned char)str_i[0]);
	    SDL_SaveBMP(temp, name); //将BMP数据保存为文件


		
	    SDL_FreeSurface(text);  
	    SDL_FreeSurface(temp);
	    //	free(bmp_buf);

	}

	free(fmt);
    TTF_CloseFont(font);  
    TTF_Quit();
	return 0;
ERR:
	free(fmt);
    TTF_CloseFont(font);  
    TTF_Quit();  

    return -1;
}

















