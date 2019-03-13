
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bmp.h"
#include "ziku.h"





/*******************************************************
功能：处理一张bmp图片---> 点阵数据
参数：
	@bmpname ：（入）BMP图片名称
	@BMP_data：（返）点阵数据
返回：
	成功：数据的长度
	失败：-1
注意：使用后需要free（*BMP_data）
*******************************************************/
int parse_bmp(const char *bmpname,char**BMP_data)
{
	if(NULL == bmpname || NULL == BMP_data)
	{
		printf("parameter error\r\n");
		return -1;
	}

	BitMapFileHeader FileHead;
	BitMapInfoHeader InfoHead;
	RgbQuad rgb;

	//打开.bmp文件
	FILE *fb = fopen(bmpname, "rb");
	if (fb == NULL)
	{
		printf("fopen bmp error\r\n");
		return -1;
	}

	//读文件信息
	if (1 != fread( &FileHead, sizeof(BitMapFileHeader),1, fb))
	{
		printf("read BitMapFileHeader error!\n");
		fclose(fb);
		return -1;
	}
	if (memcmp(FileHead.bfType, "BM", 2) != 0)
	{
		printf("it's not a BMP file\n");
		fclose(fb);
		return -1;
	}

	//读位图信息
	if (1 != fread( (char *)&InfoHead, sizeof(BitMapInfoHeader),1, fb))
	{
		printf("read BitMapInfoHeader error!\n");
		fclose(fb);
		return -1;
	}


	//计算BMP数据部分的长度
	fseek(fb,0,SEEK_END);
	unsigned int file_size = ftell(fb);
	fseek(fb, FileHead.bfOffBits, SEEK_SET);
	unsigned int data_start_pos = ftell(fb);
	unsigned int data_size = file_size - data_start_pos;
	printf("BMP data len = %d\n",data_size);

	//分配缓存空间
	unsigned int  buffer_size = 1024*1024;//sizeof(struct CHARACTER_INFO);
	//buffer_size += (InfoHead.biWidth + 7)/8  * (InfoHead.biHeight + 7)/8; //对应点阵的大小，向上8bit对齐（1bit位表示一个点） 
	printf("InfoHead.biWidth(%d) InfoHead.biHeight(%d)\n",InfoHead.biWidth,InfoHead.biHeight);

	unsigned char* data = (unsigned char*)malloc(buffer_size);
	if(NULL == data)
	{
		printf("malloc buf error!\n");
		fclose(fb);
		return -1;
	}
	memset(data,0,buffer_size);
	struct CHARACTER_INFO * info = (struct CHARACTER_INFO *)data;
	if(InfoHead.biWidth >= 65536)//保障下边的类型转化安全
	{
		printf("data overflow !\n");
		return -1;
	}
	info->width = (HLE_U16)InfoHead.biWidth; 
	info->height =(HLE_U16)InfoHead.biHeight;
	info->matrix_size = 0; 

	
	int len = InfoHead.biBitCount / 8;     //原图一个像素占几字节
	printf("matrix len = %d\n",len);
	
	/*
	printf("sizeof(char) = %d \n",sizeof(char));
	printf("sizeof(short) = %d \n",sizeof(short));
	printf("sizeof(int) = %d \n",sizeof(int));
	printf("sizeof(float) = %d \n",sizeof(float));
	printf("sizeof(long) = %d \n",sizeof(long));
	*/
	
	unsigned int offset = sizeof(struct CHARACTER_INFO); //记录写 data 的偏移量
	unsigned int j = 0;//控制打印输出"\n"
	while(!feof(fb))
	{
		unsigned char i = 0;
		unsigned char pixel = 0; //记录8个像素点，满一个字节
		
		for (i = 0;i<8;i++)//凑8个像素
		{
						
			/*一次读取一个像素点的字节数，进行解析*/
			if (len != fread((char *)&rgb, 1, len, fb))
			{
				if(feof(fb))
				{
					break;
					//跳出后，8bit 剩余部分（默认）为零
				}
							
				printf("fread error!!\n");
				free(data);
				fclose(fb);
				return -1;
			}
			//printf("rgb.Red(%x) rgb.Green(%x) rgb.Blue(%x) rgb.Reserved(%x)\n",rgb.Red,rgb.Green,rgb.Blue,rgb.Reserved);

			if(rgb.Red != 0 || rgb.Green != 0 || rgb.Blue != 0)
			{
				pixel = pixel|(1<<(7-i));
				printf("*");
			}
			else
			{
				printf("-");
			}
			
			j++;
			if(j == InfoHead.biWidth)
			{
				
				j = 0;
				/*保证每一行的像素点数是8的整数倍,满了一行，但最后凑不足一个char，自动补0*/
				if( i < 7 )
				{
					unsigned char diff = 7 - i;
					i = 7;
					info->width = (HLE_U16)InfoHead.biWidth + diff;//对图像宽度进行修正 
					unsigned char m = 0;
					for(m=0;m<diff;m++)
						printf("-");
					printf("\n");
					break;
				}
				printf("\n");
			}
			
		}

		if(i>0)//读到至少有一个像素点的数据，才记录，否则不能记录
		{
			data[offset] = pixel;
			offset ++;
			info->matrix_size ++; 
		}


		if(feof(fb))
		{
			break;
		}
						
		if(offset > buffer_size)
		{
			printf("error!!, matrix buf overflow!\n");
			free(data);
			fclose(fb);
			return -1;
		}
		
	}
	printf("After the modification,info->width(%d) info->height(%d)\n",info->width,info->height);
	
	fclose(fb);
	*BMP_data = data;
	
	printf("offset(%d) matrix_size(%d)\n",offset,buffer_size);

	/*---解决点阵数据上下颠倒的BUG，如若打开该部分代码反而导致点阵数据颠倒那就注释掉----------------------*/
	//对点阵图像进行上下翻转（按照最上边的边对折上去）
	#if 1
	unsigned int bytes_per_line = info->width/8;
	char* tmp_buf = (char*)malloc(bytes_per_line);
	if(NULL == tmp_buf)
	{
		printf("malloc error!\n");
		free(data);
		fclose(fb);
		return -1;	
	}
	memset(tmp_buf,0,bytes_per_line);

	/*以宽度的字节数为单位对尾部进行倒序读取，再与头部对应位置进行交换*/
	int i=0;
	char* tail_pos = data + sizeof(struct CHARACTER_INFO) + info->matrix_size - bytes_per_line;//尾巴读取的（初始）位置
	char* header_pos = data + sizeof(struct CHARACTER_INFO); //头读取的（初始）位置
	for(i = 0 ; i < info->height ; i++)
	{
		//1.备份尾巴的数据
		memcpy(tmp_buf,tail_pos,bytes_per_line);

		//2.读出头的数据，放到尾巴处
		memcpy(tail_pos,header_pos,bytes_per_line);
		
		//3.将尾巴备份的数据放到头的位置处
		memcpy(header_pos,tmp_buf,bytes_per_line);
		
		//4.更新头和尾巴指针的位置
		header_pos +=  bytes_per_line;
		tail_pos -= bytes_per_line;

		//5.判断结束条件
		if(header_pos >= tail_pos)
			break;

	}
	free(tmp_buf);
	
	#endif
	/*----------------------------------------------------------------------------------------------------*/
	
	return (info->matrix_size + sizeof(struct CHARACTER_INFO));
}

#define ZIKU_MAX_SIZE 1024*1024*3   //点阵字库的最大缓存空间
#define ZIKU_FILE_NAME "./ASCII_vector"
void main(int argc, const char * argv [ ])
{
	char*	str = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
	unsigned int str_len = strlen(str);
	

	/*分配点阵字库缓存空间*/
	char* ziku = (char*)malloc(ZIKU_MAX_SIZE);
	if(NULL == ziku)
	{
		printf("malloc failed !\n");
		return ;
	}
	memset(ziku,0,ZIKU_MAX_SIZE);

	struct ZK_HEAD_VECTOR* header = (struct ZK_HEAD_VECTOR*)ziku;
	char* data_w_pos = (char*)ziku + sizeof(struct ZK_HEAD_VECTOR);
	header->magic = ZK_MAGIC;
	
	HLE_U32		ASCII_offset[256] = {0};//记录各个字符点阵数据在字库文件中的偏移

	
	int i = 0;
	int ret= 0;
	
	/*记录最大最小的ASICC码值*/
	unsigned char ASICC_max = (unsigned char)str[0];
	unsigned char ASICC_min = (unsigned char)str[0];
	for(i = 0 ; i < str_len ; i++)
	{
	
		if((unsigned char)str[i] > ASICC_max)
			ASICC_max = (unsigned char)str[i];
		if((unsigned char)str[i] < ASICC_min)
			ASICC_min = (unsigned char)str[i];
		 
		char file_name[36] = {0};
		snprintf(file_name, 19, "./bmp/save_%03d.bmp",(unsigned char)str[i]);
		printf("current file name : %s\n",file_name);

		
		/*解析BMP图片数据*/
		char* data = NULL;
		ret = parse_bmp(file_name,&data);
		if(ret < 0)
		{
			printf("call parse_bmp error ！\n");
			free(ziku);
			return;
		}

		/*写入字库包*/
		if((unsigned int)(data_w_pos + ret - ziku) > ZIKU_MAX_SIZE)
		{
			printf("ERROR! ziku buf overflow!\n");
			free(data);
			free(ziku);
			return ;
		}
		memcpy(data_w_pos,data,ret);
		if(0 == ASCII_offset[(unsigned char)str[i]])
		{
			ASCII_offset[(unsigned char)str[i]] = (unsigned int)(data_w_pos - ziku);
		}
		else
		{
			printf("ERROR! Have Repeated characters! ：%c\n",str[i]);
			free(data);
			free(ziku);
			return;
			
		}
			
		data_w_pos += ret;
		header->data_size += ret;
		
		free(data);
		
		
	}

	header->q_start = ASICC_min;
	header->q_end = ASICC_max;
	header->w_start = 0;
	header->w_end = 0;
	
		
	memcpy(header->ASCII_offset,ASCII_offset,sizeof(ASCII_offset));

	if(header->data_size + sizeof(struct ZK_HEAD_VECTOR) != (unsigned int)(data_w_pos-ziku))
	{
		printf("file size and font size mismatch !\n");
		free(ziku);
	}
	
	/*-----写入ziku文件--------------*/
	int fd = open(ZIKU_FILE_NAME, O_WRONLY | O_CREAT, 0766);
	write(fd, ziku, (unsigned int)(data_w_pos-ziku));
	close(fd);
	printf("write %s success!\n",ZIKU_FILE_NAME);
	free(ziku);

	
}











