/*
 * Implementation of mp3 media
 * Copyright (c) 2012-2013 Voicebase Inc.
 *
 * Authors: Alexander Ustinov, Pomazan Nikolay
 * email: alexander@voicebase.com, pomazannn@gmail.com
 *
 * This file is the part of the mod_hls apache module
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser General Public License version 3. See the LICENSE file
 * at the top of the source tree.
 *
 */

#include "hls_media.h"
#include "mod_conf.h"
//#include "lame/lame.h"
#include "hls_mux.h"
#include "typeport.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <math.h>
#include "Box.h"
#include "my_inet.h"



//#define HLS_PLUGIN
extern char*	buf_start;
extern char* 	buf_end;


#ifdef HLS_PLUGIN
#define HLS_MALLOC(X,Y) apr_pcalloc(X,Y)
#define HLS_FREE(X)

#else
#define HLS_MALLOC(X,Y) malloc(Y)
#define HLS_FREE(X) free(X)
/*
#define HLS_FREE(X) do{\
						if( (char*)X >= (char*)buf_start && (char*)X <= (char*)buf_end)\
						{\
							ERROR_LOG("funck free!!\n");\
							exit(-1);\
						}\
						else \
						{\
							free(X);\
						}\
					}while(0)

*/
#endif



#define RESERVED_SPACE 160     //该空间用来干嘛？？？

typedef struct _frame_info_t
{
	unsigned int 	video_frames; 		//fMp4文件中的视频帧总数
	unsigned int*	video_size_array;	//每个视频sample（帧）大小信息数据的头指针
	unsigned int* 	video_offset_array;	//每个视频sample（帧）偏移信息数据的头指针
	char*			video_buf;			//video帧缓存空间
	
	unsigned int 	audio_frames;		//fmp4文件中的音频帧总数
	unsigned int*	audio_size_array;	//每个音频sample（帧）大小信息数据的头指针
	unsigned int* 	audio_offset_array;	//每个音频sample（帧）偏移信息数据的头指针
	char*			audio_buf;			//audio帧缓存空间
}frame_info_t;
	


//用于记录音视频trak的id
static int audio_trak_id = -1;
static int video_trak_id = -1;

int get_audio_trak_id(void)
{
	//DEBUG_LOG("audio_trak_id = %d\n",audio_trak_id);
	return audio_trak_id;
}
int get_video_trak_id(void)
{
	//DEBUG_LOG("video_trak_id = %d\n",video_trak_id);
	return video_trak_id;
}
void set_audio_trak_id(int id)
{
	//DEBUG_LOG("audio_trak_id = %d\n",audio_trak_id);
	audio_trak_id = id;
}
void set_video_trak_id(int id)
{
	//DEBUG_LOG("video_trak_id = %d\n",video_trak_id);
	video_trak_id = id;
}


const int SamplingFrequencies[16]={96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
									  16000, 12000, 11025, 8000, 7350, -1, -1, -1};
const int ChannelConfigurations[16]={0, 1, 2, 3, 4, 5, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1};

/*
判断传入的box是不是有子box
返回：有子box ：1 	无子box：0
*/
int if_subbox(char* box_type) 
{
	int yeap=0;
	//该数组里边的box描述的还不是很全面
	char types_with_subboxes[][4]={ {'m','o','o','v'},/* {'i','o','d','s'},*/ {'t','r','a','k'}, {'m','d','i','a'},
									{'m','i','n','f'}, {'d','i','n','f'}, /*{'d','r','e','f'},*/ {'s','t','b','l'},
									{'s','t','s','d'}, {'m','p','4','a'}, /*{'e','s','d','s'},*//* ES Descriptor and Decoder, */
									{'a','v','c','1'}, {'u','d','t','a'}, 
									{'m','o','o','f'},{'t','r','a','f'},{'m','f','r','a'},{0,0,0,0}  //fmp4 special
									//注意，最后边的 0 为结束标志，不能删除
								  };
	#if 0
	char types_with_subboxes[][4]={ {'m','o','o','v'},/* {'i','o','d','s'},*/ {'t','r','a','k'}, {'m','d','i','a'},
									{'m','i','n','f'}, {'d','i','n','f'}, /*{'d','r','e','f'},*/ {'s','t','b','l'},
									/*{'s','t','s','d'}, {'m','p','4','a'}, {'e','s','d','s'},*//* ES Descriptor and Decoder, */
									/*{'a','v','c','1'},*/ {'u','d','t','a'}, {0,0,0,0}};
	#endif
	
	int i=0;
	int counter;
	while (types_with_subboxes[i][0]!=0 && types_with_subboxes[i][1]!=0 && types_with_subboxes[i][2]!=0 && types_with_subboxes[i][3]!=0 && yeap==0) 
	{
		counter=0;
		for(int j=0; j<4; j++) 
		{
			if (types_with_subboxes[i][j]==box_type[j])
				counter++;
			if (counter == 4)
				yeap=1;
		}
		i++;
	}
	return yeap>0 ? 1:0;
}

//stsc box里单个入口的描述结构
typedef struct 
{
	int first_chunk;   				//当前入口的第一个 chunk 的序号
	int samples_per_chunk; 			//当前入口的每个 chunk 里边sample（帧）的个数
	int sample_description_index;
} sample_to_chunk;

typedef struct MP4_BOX_s{
	struct MP4_BOX_s *parent; 			//上一级的box
	int child_count;					//子box的总数
	struct MP4_BOX_s *child_ptr[70];	//子box数组
	char box_type[4];					//当前box的类型
	int box_size;						//当前box的大小
	int box_first_byte;					//box的第一个字节开始的位置（相较于文件开头）
} MP4_BOX;

/*
给MP4文件的box节点添加一个新子节点（指的是解析MP4文件的box时，形成的box节点的描述信息）
*/
struct MP4_BOX_s* add_item(void* context, MP4_BOX* self, int box_size, char* box_type, int first_byte) {
	MP4_BOX* new_item;
	new_item = (MP4_BOX*) HLS_MALLOC (context, sizeof(MP4_BOX));
	new_item->parent = self;
	new_item->box_size = box_size;
	memcpy(new_item->box_type, box_type, 4);
	new_item->box_first_byte = first_byte;
	new_item->child_count=0;
	return new_item;
}

//OLD VERSION
/*
void scan_one_level(FILE* mp4, MP4_BOX* head) {
	int c;
	int child_count=0;
	int byte_counter = head->box_first_byte;
	int first_byte = 0;//head->box_first_byte;
	int box_size = 0;
	char box_type[4];
	if (head->box_first_byte != 0) {
		fseek(mp4, head->box_first_byte+8, SEEK_SET);
		first_byte = head->box_first_byte+8;
	}
	else
		fseek(mp4, head->box_first_byte, SEEK_SET);
	while(byte_counter < (head->box_size + head->box_first_byte-9)) {
		first_byte += box_size;
		if(box_size!=0)
			fseek(mp4, box_size-8, SEEK_CUR);
		fread(&box_size, 4, 1, mp4);
		box_size = ntohl(box_size);
		fread(box_type, 1, 4, mp4);

		head->child_ptr[child_count] = add_item(head, box_size, box_type, first_byte);
		child_count++;
		byte_counter += box_size;

	}
	head->child_count=child_count;
	printf("\ntype - '%.4s'; size - '%d'; child count - '%d'; first byte - '%d'",head->box_type, head->box_size, head->child_count, head->box_first_byte);
	if (child_count>0) {
		for (int count=0; count<head->child_count; count++) {
			if ((c=if_subbox(head->child_ptr[count]->box_type)) > 0)
				scan_one_level(mp4, head->child_ptr[count]);
			else
				printf("\ntype - '%.4s'; size - '%d'; child count - '%d'; first byte - '%d'",head->child_ptr[count]->box_type, head->child_ptr[count]->box_size, head->child_ptr[count]->child_count, head->child_ptr[count]->box_first_byte);
		}
	}
}
*/

//NEW Version
/*
void scan_one_level(FILE* mp4, MP4_BOX* head) {
	int c;
	int child_count=0;
	int first_byte = head->box_first_byte;
	int box_size = 0;
	char box_type[4];

	while(first_byte < (head->box_size + head->box_first_byte-9)) {

		fseek(mp4, first_byte, SEEK_SET);

		fread(&box_size, 4, 1, mp4);
		fread(box_type, 1, 4, mp4);

		box_size = ntohl (box_size);

		child_count++;
		head->child_ptr[child_count] = add_item(&head, box_size, box_type, first_byte+8);

		first_byte   += box_size;

	}
	head->child_count=child_count;

	if (child_count>0) {
		for (int count=0; count<head->child_count; count++) {
			printf("\ntype - '%.4s'; size - '%d'; child count - '%d'; first byte - '%d'",head->child_ptr[count+1]->box_type, head->child_ptr[count+1]->box_size, head->child_ptr[count+1]->child_count, head->child_ptr[count+1]->box_first_byte);

			if ((c=if_subbox(head->child_ptr[count+1]->box_type)) > 0)
				scan_one_level(mp4, head->child_ptr[count+1]);
		}
	}
}
*/

int compare_box_type(MP4_BOX* root, char boxtype[5]) ;

//NEW NEW VERSION (works)
/*扫描（只同一层次的，不能进行深度检索box）box，但该函数多次递归，即可遍历MP4文件的整个box结构*/
void scan_one_level(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* head) 
{
	int child_count=0;
	int byte_counter = head->box_first_byte;
	int first_byte = head->box_first_byte; //0
	int box_size = 0;
	char box_type[4];
	
	if (head->box_first_byte != 0) //如果当前的box的第一个字节不处于文件的开头（事实上除了ftyp，都不处于文件的开头）
		first_byte = head->box_first_byte+8; //往后跳过box头部“ type + size”字段所占的空间(容器box只有头部,接下来就是子box开始了，几个特殊的除外)

	if(compare_box_type(head,"stsd")) //stsd 比较特殊，头是full box类型的（16字节）
	{
		//DEBUG_LOG("\nparse stsd box !\n");
		first_byte = head->box_first_byte+16;
	}
		
	if(compare_box_type(head,"avc1")) //avc1 比较特殊，头是full box类型,且有数据部分（86字节）
	{
		first_byte = head->box_first_byte+86;
	}

	if(compare_box_type(head,"mp4a")) //mp4a 比较特殊，头是容器box类型,且有数据部分（36字节）
	{
		//DEBUG_LOG("parse mp4a box !");
		first_byte = head->box_first_byte+36;
	}

	byte_counter = first_byte; 
		
	//该while循环结束会将当前层次的box信息都遍历出来
	//while(byte_counter < (head->box_size + head->box_first_byte-9)) //如果距离box尾部都不足9字节了，不认为还有box（box头最小8字节），遍历结束
	while(byte_counter < (head->box_size + head->box_first_byte)) 
	{
		int how_much=0;
		first_byte += box_size;
		//	fseek(mp4, first_byte, SEEK_SET);
		//	fread(&box_size, 4, 1, mp4);

		//获取box大小信息，注意how_much接收的是read所读取的字节数
		how_much = source->read(mp4, &box_size, 4, first_byte, 0);
		box_size = t_ntohl(box_size);
		
		//	fread(box_type, 1, 4, mp4);
		//获取box类型信息
		source->read(mp4, box_type, 4, first_byte+how_much, 0);
	
		
		head->child_ptr[child_count] = add_item(context, head, box_size, box_type, first_byte);//添加子节点（描述信息）
		child_count++;
		byte_counter += box_size;

		/*if(compare_box_type(head,"mp4a"))
		{
			//------------------------------------51-----------------938---------------------------------------965----------------
			printf("\nmp4a child box --> box_zise(%d)  byte_counter(%d) head->box_size + head->box_first_byte = %d \n",box_size,byte_counter,head->box_size + head->box_first_byte);
		}
		*/


	}
	head->child_count=child_count;

	//printf("type - '%.4s'; size - '%d'; child count - '%d'; first byte - '%d'\n",head->box_type, head->box_size, head->child_count, head->box_first_byte);
	//以下对每个子box的区域再进行递归遍历，得出整个mp4文件的box描述信息
	if (child_count>0) 
	{
		for (int count=0; count < head->child_count; count++) 
		{
			if ((if_subbox(head->child_ptr[count]->box_type)) > 0)//该box有子box,则递归解析
				scan_one_level(context, mp4, source, head->child_ptr[count]);
			//else
				//printf("type - '%.4s'; size - '%d'; child count - '%d'; first byte - '%d'\n",head->child_ptr[count]->box_type, head->child_ptr[count]->box_size, head->child_ptr[count]->child_count, head->child_ptr[count]->box_first_byte);
		}
	}
}

/*
@mp4：文件句柄
@source:文件操作函数句柄
返回：mp4文件的box根结点指针
*/
MP4_BOX* mp4_looking(void* context, file_handle_t* mp4, file_source_t* source) 
{
//	fseek (mp4, 0, SEEK_END);
	int fSize;
//	fSize = ftell (mp4);
	fSize = source->get_file_size(mp4, 0);
	MP4_BOX* head;
	head = (MP4_BOX*) HLS_MALLOC (context, sizeof(MP4_BOX));

	//初始化box的根节点
	head->box_type[0] = 'f';
	head->box_type[1] = 'i';
	head->box_type[2] = 'l';
	head->box_type[3] = 'e';
	head->parent = NULL;
	head->box_first_byte = 0;
	head->child_count=0;
	head->box_size = fSize; //根节点的大小初始化为文件的大小
	//扫描MP4文件的box节点并记录节点信息。
	scan_one_level(context, mp4, source, head);
	return head;
}

void i_want_to_break_free(MP4_BOX* root) {
	if(root->child_count>0) {
		for(int i=0; i<root->child_count; i++) {
			i_want_to_break_free(root->child_ptr[i]);
		}
	}
	memset(root, '\0', sizeof(MP4_BOX));
	HLS_FREE(root);
}

/*
比较box节点的类型是否为要查找的类型
返回：是：1			不是：0
*/
int compare_box_type(MP4_BOX* root, char boxtype[5]) 
{
	for (int i=0; i<4; i++) 
	{
		if (root->box_type[i]!=boxtype[i])
			return 0;
	}
	return 1;
}

/*
在box节点的子节点中查找某个box
该函数单次只能遍历同一层次的box，递归实现遍历有层次的box
找到box后就会返回，如果存在多个相同box只能找到第一个
返回：
	找到：box节点指针 			 未找到：NULL
*/
MP4_BOX* compare_one_level(MP4_BOX* root, char boxtype[5]) 
{
	int find_box=0;
	MP4_BOX* box;
	if (root->child_count>0) 
	{
		for(int i=0; i<root->child_count; i++) 
		{
					if (find_box==0) 
					{
						box=root->child_ptr[i];
						find_box=compare_box_type(box, boxtype);
					}
					if (find_box>0)
						break;
		}
	}
 return find_box!=0 ? box:NULL;
}

/*
查找单个moof box里边的traf box

*/
int find_traf_box_in_moof(void* context,MP4_BOX* moof,MP4_BOX**traf_array)
{
	if(NULL == moof )
	{
		ERROR_LOG("Illegal parameter!\n");
		return -1;
	}

	
}

/*
查找（root节点下）第一层次的某个box
针对fmp4文件查找moof、mdat这种数量非常多（重复率高）的box
@root：根节点
@boxtype：box类型
@box_array: 返回box节点的指针数组指针

返回：
		成功：box 的个数  
		失败：-1
注意：moof_array需要free才能释放

*/
int  find_box_one_level(void* context,MP4_BOX* root,char boxtype[5],MP4_BOX***box_array)
{
	if(NULL == root )
	{
		ERROR_LOG("Illegal parameter!\n");
		return -1;
	}

	unsigned int box_count = 0;//box的数量
	MP4_BOX* box=NULL;
	int i;
	//box=compare_one_level(root, "moof");//在 root 对应节点下的第一层（子节点中）找，moof就只存在与第一层，不需要再遍历子节点	
	if (root->child_count > 0) 
	{
		//第一遍循环，统计moof box的总个数
		//DEBUG_LOG("root->child_count = %d\n",root->child_count);
		for(i=0 ; i<root->child_count ; i++) 
		{
			box = root->child_ptr[i];		
			if (compare_box_type(box, boxtype) > 0)
			{
				box_count++;
			}
				
		}
		
		//分配内存空间
		MP4_BOX** tmp_array =  (MP4_BOX**) HLS_MALLOC(context, box_count * sizeof(MP4_BOX*));
		if(NULL == tmp_array)
		{
			ERROR_LOG("<error!!> malloc failed!\n");
			return -1;
		}
		
		//第二遍循环，填充moof节点指针数组
		unsigned int box_array_i = 0;
		for(i=0; i<root->child_count; i++) 
		{
			box=root->child_ptr[i];	
			if (compare_box_type(box, boxtype) > 0)
			{
				tmp_array[box_array_i] = box;
				box_array_i ++;
			}
				
		}

		if(box_array_i != box_count)
		{
			ERROR_LOG("box_array_i(%d) != box_count(%d)",box_array_i,box_count);
			return -1;
		}
		*box_array = tmp_array;
		
				
	}
	else
	{
		ERROR_LOG("%.4s->child_count = %d \n",root->box_type ,root->child_count);
		return -1;
	}

	return box_count;

}


/*
在box的节点信息链表中寻找对应类型的box
注意：若果box结构中存在多个相同的box，在找到第一个box后就会返回，不适合查找moof这类型box
返回： 成功：box节点指针				失败：NULL
*/
MP4_BOX* find_box(MP4_BOX* root, char boxtype[5]) 
{
	MP4_BOX* box=NULL;
	box=compare_one_level(root, boxtype);//在 root 对应节点下的第一层（子节点中）找，看子节点是不是有匹配的
	if (box==NULL) //如果第一层子节点中没有匹配上的，就以第一层所有子节点为root节点进行循环遍历
	{
		if (root->child_count>0) 
		{
			for (int i=0; i<root->child_count; i++) 
			{
				box=find_box(root->child_ptr[i], boxtype);
				if (box!=NULL)
					break;
			}
		}
	}
	return box;
}



/*
读取文件中对应box的内部参数，比较是不是和希望的值一致
这里主要用来查找trak下的hdlr box来判断trak的类型。
返回：1：比较一致  			0：不一致
*/
int handlerType(file_handle_t* mp4, file_source_t* source, MP4_BOX* root, char handler[5]) 
{
	char hdlr_type[4];
//	fseek(mp4, root->box_first_byte+16, SEEK_SET); //+16 cause of 'hdlr' syntax.
//	fread(hdlr_type, 1, 4, mp4);
	source->read(mp4, hdlr_type, 4, root->box_first_byte+16, 0);//+16是考虑到了box header后边紧接着又4字节的保留位

	for (int i=0; i<4; i++) 
	{
		if(hdlr_type[i]!=handler[i])
			return 0;
	}
	return 1;
}

MP4_BOX* find_necessary_trak(file_handle_t* mp4, file_source_t* source, MP4_BOX* root, char trak_type[5]) {
	MP4_BOX* moov;
	MP4_BOX* necessary_trak=NULL;
	MP4_BOX* temp_ptr;
	moov=find_box(root, "moov");
	for(int i=0; i<moov->child_count; i++) {
		if (compare_box_type(moov->child_ptr[i],"trak")) {
			temp_ptr=find_box(moov->child_ptr[i], "hdlr");
			if(handlerType(mp4, source, temp_ptr, trak_type))
				necessary_trak=moov->child_ptr[i];
		}
	}
	return necessary_trak;
}

/*
功能：解析 stsc box，获取入口描述信息指针数组（的）指针
参数：
	@stsc_entry_count:（返回）chunk的入口个数（每两个相邻的入口所包含的
	chunk（同一个入口中的chunk是同类型的）类型不一样）
返回：
	stsc box 的入口描述信息指针数组（的）指针 
*/
sample_to_chunk** read_stsc(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* stsc, int* stsc_entry_count) 
{
	int entry_count=0;
	int how_much=0;
//  fseek(mp4, stsc->box_first_byte+12, SEEK_SET);
//	fread(&entry_count, 4, 1, mp4);
	source->read(mp4, &entry_count, 4, stsc->box_first_byte+12, 0);
	entry_count = t_ntohl(entry_count);
	*stsc_entry_count = entry_count;
	
	sample_to_chunk **stc; //stsc box 的入口描述信息指针数组（的）指针 
	stc = HLS_MALLOC (context, sizeof(sample_to_chunk*)*entry_count);
	for (int j=0; j<entry_count; j++)
		stc[j] = (sample_to_chunk*) HLS_MALLOC (context, sizeof(sample_to_chunk));
	
	how_much = stsc->box_first_byte+12+4;
	
	for(int i=0; i<entry_count; i++) 
	{
	//fread(&stc[i]->first_chunk,4,1,mp4);
		how_much += source->read(mp4, &stc[i]->first_chunk, 4, how_much, 0);
		stc[i]->first_chunk = t_ntohl(stc[i]->first_chunk);
	//fread(&stc[i]->samples_per_chunk,4,1,mp4);
		how_much += source->read(mp4, &stc[i]->samples_per_chunk, 4, how_much, 0);
		stc[i]->samples_per_chunk = t_ntohl(stc[i]->samples_per_chunk);
	//fread(&stc[i]->sample_description_index,4,1,mp4);
		how_much += source->read(mp4, &stc[i]->sample_description_index, 4, how_much, 0);
		stc[i]->sample_description_index = t_ntohl(stc[i]->sample_description_index);
	}
	return stc;
}

/*
“stco”定义了每个thunk在媒体流中的位置。位置有两种可能，32位的和64位的，后者对非常大的电影很有用。
在一个表中只会有一种可能，这个位置是在整个文件中的，而不是在任何box中的，
这样做就可以直接在文件中找到媒体数据，而不用解释box。
需要注意的是一旦前面的box有了任何改变，这张表都要重新建立，因为位置信息已经改变了。
功能：解析 stco box
参数：
	@stco_entry_count ： stco 的入口数 （每一个入口都对应一个trunk,这里的 trunk 是
						将stsc box 中描述的trunk按顺序全部展开的基础上谈的）
返回：每个chunk相对于文件头的偏移位置数组（的）指针
*/
int* read_stco(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* stco, int* stco_entry_count) 
{
	int entry_count=0;
	int how_much=0;
//	fseek(mp4, stco->box_first_byte+12, SEEK_SET);
//	fread(&entry_count, 4,1,mp4);
	source->read(mp4, &entry_count, 4, stco->box_first_byte+12, 0);
	entry_count = t_ntohl(entry_count);
	*stco_entry_count = entry_count;
	
	int* stco_data; //每个chunk相对于文件头的偏移位置数组（的）指针
	stco_data=(int*) HLS_MALLOC (context, sizeof(int)*entry_count);
	how_much = stco->box_first_byte+12+4;
	for (int i=0; i<entry_count; i++) 
	{
		//fread(&stco_data[i],4,1,mp4);
		how_much += source->read(mp4, &stco_data[i], 4, how_much, 0);
		stco_data[i] = t_ntohl(stco_data[i]);
	}
	return stco_data;
}


/*
功能：获取fmp4文件 一个traf-->trun 音视频 	   帧的大小，数组指针
参数：
	@size_array:(返回)每个帧的大小数组指针
返回：成功：数组元素个数		   		失败：-1
注意：使用后需要 free(duration_array)  
*/
int get_fmp4_sample_size(void* context, file_handle_t* mp4, file_source_t* source,MP4_BOX* traf,int**size_array)
{
	if(NULL == mp4 || NULL == source || NULL == traf)
	{
		ERROR_LOG("Illegal parameter !\n");
		return -1;
	}

	unsigned int flag = 0;
	unsigned int Sample_count = 0;//当前trun下的 samples（帧） 个数
	unsigned int first_size_offset = 0;//第一个size数据开始的位置偏移
	unsigned int size_offset = 0; //统计 两个size 字段之间的距离
	MP4_BOX* trun =  find_box(traf, "trun");
	if(trun)
	{
		//sample_count
		source->read(mp4, &Sample_count, 4, trun->box_first_byte + 12, 0);
		Sample_count = t_ntohl(Sample_count);
		int *size_array_tmp = (int *)HLS_MALLOC(context, sizeof(int) * Sample_count);
		if(NULL == size_array_tmp)
		{
			ERROR_LOG("malloc failed !\n");
			return -1;
		}

		//flag  注意值占 3 字节 
		source->read(mp4, &flag, 4, trun->box_first_byte + 8, 0);
		DEBUG_LOG("flag = %x\n",flag);
		
		flag = t_ntohl(flag);
		flag = flag & 0x00FFFFFF; //去掉最高位一个字节
		
		DEBUG_LOG("flag = %x\n",flag);

		/*---计算读取第一个 sample_size 的偏移位置----------------------*/
		first_size_offset = trun->box_first_byte + 16 ; //计算到sample_count字段
		//以下两个字段为可选字段，不一定每个mp4文件都填充，所以需判断flag来动态变换 first_size_offset 的偏移长度
		if(flag & E_data_offset)
		{
			first_size_offset += 4;
			DEBUG_LOG("E_data_offset\n");
		}
			
		if(flag & E_first_sample_flags)
		{
			first_size_offset += 4;
			DEBUG_LOG("E_first_sample_flags\n");
		}
			
		if(flag & E_sample_duration)
		{
			first_size_offset += 4;
			DEBUG_LOG("E_sample_duration\n");

		}
		DEBUG_LOG("first_size_offset = %d\n",first_size_offset);
		
		/*------------------------------------------------------------------*/

		/*---统计两个sample_size 字段之间的间隔字节数-------------------*/
		if(flag & E_sample_duration)
		{
			size_offset += 4;
			DEBUG_LOG("E_sample_duration\n");
		}
			
		if(flag & E_sample_size)
		{
			size_offset += 4;
			DEBUG_LOG("E_sample_size\n");
		}
			
		if(flag & E_sample_flags)
		{
			size_offset += 4;
			DEBUG_LOG("E_sample_size\n");
		}
			
		if(flag & E_sample_composition_time_offset)
		{
			size_offset += 4;
			DEBUG_LOG("E_sample_size\n");
		}
			

		DEBUG_LOG("size_offset = %d\n",size_offset);
		/*------------------------------------------------------------------*/

		//开始填充 size_array 信息数组
		int i = 0;
		for (i=0 ; i<Sample_count ; i++)
		{
			source->read(mp4, &size_array_tmp[i], 4 , first_size_offset + i*size_offset , 0);
			size_array_tmp[i] = t_ntohl(size_array_tmp[i]);
			
			//DEBUG_LOG("size_array[%d] = %d\n",i,size_array_tmp[i]);
			
		}
		
		*size_array = size_array_tmp;

		return Sample_count;
		
	}

	return -1;
	

}

/*
//fmp4文件Sample count 信息描述结构(video/audio 帧在每个mdat box中的sample（帧）数量信息)
typedef struct _sample_count_t
{
	unsigned int	init_done;		//初始化已经完成 1：完成 			 0：未完成（未完成不能使用）
	unsigned int 	V_trun_num;		//视频 trun box 的总个数----对应 mdat box总个数
	unsigned int* 	V_sample_array;	//视频Sample count 信息数组的头指针
	unsigned int 	A_trun_num;		//音频 trun box 的总个数----对应 mdat box总个数
	unsigned int*	A_sample_array;	//音频Sample count 信息数组的头指针
}sample_count_t;

sample_count_t sample_count = {0};	//描述音视频 Sample count 信息。(video/audio 帧在每个mdat box中的sample（帧）数量信息)
*/


//fmp4文件samples（帧）大小信息描述结构
typedef struct _sample_size_t
{
	unsigned int	init_done;		//初始化已经完成 1：完成 			 0：未完成（未完成不能使用）
	unsigned int 	V_sample_num;	//视频sample（帧）的总个数
	unsigned int* 	V_size_array;	//每个视频sample（帧）大小信息数据的头指针
	unsigned int 	A_sample_num;	//音频sample（帧）的总个数
	unsigned int*	A_size_array;	//每个音频sample（帧）大小信息数据的头指针
}sample_size_t;

sample_size_t sample_size = {0};	//描述音视频sample大小信息。

//fmp4文件samples（帧）偏移信息描述结构
typedef struct _sample_offset_t
{
	unsigned int	init_done;		//初始化已经完成 1：完成 			 0：未完成（未完成不能使用）
	unsigned int 	V_sample_num;	//视频sample（帧）的总个数
	unsigned int* 	V_offset_array;	//每个视频sample（帧）偏移信息数据的头指针
	unsigned int 	A_sample_num;	//音频sample（帧）的总个数
	unsigned int*	A_offset_array;	//每个音频sample（帧）偏移信息数据的头指针
}sample_offset_t;

sample_offset_t sample_offset = {0};	//描述音视频 sample 偏移信息。

/*
初始化（整个文件）video/audio帧的 size 信息
返回：成功：0	失败-1
注意：在调用该接口前需确保 video_frames 和 audio_frames已经初始化。
	  不能重复调用
*/
int init_sample_size_array(void* context, file_handle_t* mp4, file_source_t* source, mp4_file_status_t*m_stat_ptr)
{
	if(NULL == mp4 || NULL == source || NULL == m_stat_ptr)
	{
		ERROR_LOG("Illegal parameter !\n");
		return -1;
	}
	
	if(m_stat_ptr->frame_info.video_frames <= 0 || m_stat_ptr->frame_info.audio_frames <= 0)
	{
		ERROR_LOG("audio_frames or video_frames not init!\n");
		return -1;
	}
		
	/*---初始化全局参数sample_size------------------------------------------------------------------*/
	
	m_stat_ptr->frame_info.video_size_array = (unsigned int *)HLS_MALLOC(context, m_stat_ptr->frame_info.video_frames * sizeof(int));
	if(NULL == m_stat_ptr->frame_info.video_size_array)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
	memset(m_stat_ptr->frame_info.video_size_array,0, m_stat_ptr->frame_info.video_frames * sizeof(int));

	m_stat_ptr->frame_info.audio_size_array = (unsigned int *)HLS_MALLOC(context, m_stat_ptr->frame_info.audio_frames * sizeof(int));
	if(NULL == m_stat_ptr->frame_info.audio_size_array)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
	memset(m_stat_ptr->frame_info.audio_size_array, 0 ,m_stat_ptr->frame_info.audio_frames * sizeof(int));
	/*-----------------------------------------------------------------------------------------------*/	
	
	/*===获取每个音视频帧的 大小信息数组==============================================================*/
	//1.找出所有moof box节点，构成moof节点数组
	MP4_BOX** moof_array = NULL;
	MP4_BOX** traf_array = NULL;
	MP4_BOX* root = m_stat_ptr->root;
	unsigned int *video_size_array = m_stat_ptr->frame_info.video_size_array;
	unsigned int *audio_size_array = m_stat_ptr->frame_info.audio_size_array;
	
	unsigned int delta_counter = 0;
	unsigned int V_index = 0;
	unsigned int A_index = 0;
	int i = 0;
	
	int moof_num = find_box_one_level(context,root,"moof",&moof_array);
	DEBUG_LOG("moof_num = %d\n",moof_num);
	for(i=0 ; i<moof_num ; i++)
	{
		int traf_num = find_box_one_level(context,moof_array[i],"traf",&traf_array);
		DEBUG_LOG("moof_array[%d] --> traf_num = %d\n",i,traf_num);
		int j = 0;
		for(j=0 ; j<traf_num ; j++)
		{
			/*---区分开 video/audio traf 的根节点---*/	
			//先解析tfhd确定当前traf的trak_id
			int track_id;
			MP4_BOX* tfhd =  find_box(traf_array[j],"tfhd");
			if(NULL == tfhd)
			{
				ERROR_LOG("find_box failed!\n");
				goto ERR;
			}
			else
			{
				source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
				track_id = t_ntohl(track_id);
				
			}

			if(track_id == get_video_trak_id()) // 是 video traf
			{
				int* size_array = NULL;
				int array_num = 0;
				DEBUG_LOG("parse video  traf_array[%d].......................\n",j);
				array_num = get_fmp4_sample_size(context,mp4,source,traf_array[j],&size_array);
				if(array_num <= 0)
				{
					ERROR_LOG("get_fmp4_sample_size failed!\n");
					HLS_FREE(size_array);
					goto ERR;
				}
				
				memcpy(&video_size_array[V_index],size_array,array_num*sizeof(int));
				V_index += array_num;

				
				HLS_FREE(size_array);
			}

			if(track_id == get_audio_trak_id()) // 是 audio traf
			{
				int* size_array = NULL;
				int array_num = 0;
				DEBUG_LOG("parse audio  traf_array[%d].......................\n",j);
				array_num = get_fmp4_sample_size(context,mp4,source,traf_array[j],&size_array);
				if(array_num <= 0)
				{
					ERROR_LOG("get_fmp4_sample_size failed!\n");
					HLS_FREE(size_array);
					goto ERR;
				}
				memcpy(&audio_size_array[A_index],size_array,array_num*sizeof(int));
				A_index += array_num;
				HLS_FREE(size_array);
			}
			
		}
			
	}

	
	/*============================================================================================*/
	/*--DEBUG------------------------------------*/
	if(m_stat_ptr->frame_info.video_frames != V_index || m_stat_ptr->frame_info.audio_frames != A_index)
	{
		ERROR_LOG("video_frames(%d) != V_index(%d) \n",m_stat_ptr->frame_info.video_frames,V_index);
		ERROR_LOG("aideo_frames(%d) != V_index(%d) \n",m_stat_ptr->frame_info.audio_frames,A_index);
	}
	/*-------------------------------------------*/
	
	HLS_FREE(traf_array);
	HLS_FREE(moof_array);
	return 0;
ERR:
	HLS_FREE(traf_array);
	HLS_FREE(moof_array);
	return -1;

}


/*
初始化（整个文件）video/audio帧  的 offset信息
返回：成功：0	失败-1
注意：在调用该接口前需确保 video_frames 和 audio_frames已经初始化
*/
int init_sample_offset_array(void* context, file_handle_t* mp4, file_source_t* source,mp4_file_status_t*m_stat_ptr)
{
	if(NULL == mp4 || NULL == source || NULL == m_stat_ptr)
	{
		ERROR_LOG("Illegal parameter !\n");
		return -1;
	}

	if(m_stat_ptr->frame_info.video_frames <= 0 || m_stat_ptr->frame_info.audio_frames <= 0)
	{
		ERROR_LOG("audio_frames or video_frames not init!\n");
		return -1;
	}


	unsigned int Vtrun_num = 0; //video trun box的个数
	unsigned int Atrun_num = 0; //audio trun box的个数
	unsigned int* Vtrun_sample_array = NULL; //记录每个 video trun里边 sample_count 字段信息
	unsigned int* Atrun_sample_array = NULL; //记录每个 audio trun里边 sample_count 字段信息
	MP4_BOX** moof_array = NULL;
	MP4_BOX** traf_array = NULL;

	/*---确定 Atrun_num Vtrun_num 的值----------------------------------------------------------------*/
	int moof_num = find_box_one_level(context,root,"moof",&moof_array);
	DEBUG_LOG("moof_num = %d\n",moof_num);
	int i = 0;
	for(i=0 ; i<moof_num ; i++)
	{
		int traf_num = find_box_one_level(context,moof_array[i],"traf",&traf_array);
		DEBUG_LOG("moof_array[%d] --> traf_num = %d\n",i,traf_num);
		int j = 0;
		for(j=0 ; j<traf_num ; j++)
		{
			/*---区分开 video/audio traf 的根节点---*/	
			//先解析tfhd确定当前traf的trak_id
			int track_id;
			MP4_BOX* tfhd =  find_box(traf_array[j],"tfhd");
			if(NULL == tfhd)
			{
				ERROR_LOG("find_box failed!\n");
				HLS_FREE(moof_array);
				HLS_FREE(traf_array);
				return -1;
			}
			else
			{
				source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
				track_id = t_ntohl(track_id);
				
			}

			if(track_id == get_video_trak_id()) // 是 video traf
			{
				if(find_box(traf_array[j], "trun") != NULL)
					Vtrun_num ++ ;
				
			}

			if(track_id == get_audio_trak_id()) // 是 audio traf
			{
				if(find_box(traf_array[j], "trun") != NULL)
					Atrun_num ++ ;
			}
			
		}

		HLS_FREE(traf_array);
		traf_array = NULL;
	}
	HLS_FREE(moof_array);
	moof_array = NULL;
		
	if(Vtrun_num != Atrun_num ||moof_num != Vtrun_num)
	{
		ERROR_LOG("moof_num(%d) != Vtrun_num(%d) != Atrun_num(%d) !\n",moof_num,Vtrun_num,Atrun_num);
		return -1;
	}

	/*--------------------------------------------------------------------------------------------*/

	//分配数组的内存
	Atrun_sample_array = (unsigned int *)HLS_MALLOC(context, Atrun_num * sizeof(int));
	if(NULL == Atrun_sample_array)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;;
	}
	memset(Atrun_sample_array,0,Atrun_num * sizeof(int));

	Vtrun_sample_array = (unsigned int *)HLS_MALLOC(context, Vtrun_num * sizeof(int));
	if(NULL == Vtrun_sample_array)
	{
		ERROR_LOG("malloc failed !\n");
		HLS_FREE(Atrun_sample_array);
		return -1;
	}
	memset(Vtrun_sample_array,0,Vtrun_num * sizeof(int));
	
	/*----初始化 Vtrun_sample_array/Atrun_sample_array 数组------------------------------------*/
	/*
		确定mdat box里边的samples数据，video/audio         samples 谁填充在前边，谁在后边
		即确定video/audio trak 的先后顺序
	*/
	//在 traf[0]下解析 tfhd 确定traf[0] 的 trak_id 是 哪个，即能确定哪个trak在前边
	int track_id;
	int first_track = -1; //video/audio 轨道的先后顺序（mdat box里边谁的数据排在前边）
	
	moof_num = find_box_one_level(context,root,"moof",&moof_array);
	unsigned int Atrun_index = 0;  
	unsigned int Vtrun_index = 0;
	
	for(i=0 ; i<moof_num ; i++)
	{
		int traf_num = find_box_one_level(context,moof_array[i],"traf",&traf_array);
		int j = 0;
		for(j=0 ; j<traf_num ; j++)
		{
			if(0 == i)//确定排在前边的轨道是video 还是 audio 轨道
			{
				MP4_BOX* tfhd =  find_box(traf_array[0],"tfhd");
				if(NULL == tfhd)
				{
					ERROR_LOG("find_box failed!\n");
					return -1;
				}
				else
				{
					source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
					track_id = t_ntohl(track_id);
					first_track = track_id;
				}
			
			}

	
			/*---区分开 video/audio traf 的根节点---*/	
			//先解析tfhd确定当前traf的trak_id
			int track_id;
			MP4_BOX* tfhd =  find_box(traf_array[j],"tfhd");
			if(NULL == tfhd)
			{
				ERROR_LOG("find_box failed!\n");
				HLS_FREE(traf_array);
				HLS_FREE(moof_array);
				return -1;
			}
			else
			{
				source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
				track_id = t_ntohl(track_id);
				
			}

			MP4_BOX* trun =  find_box(traf_array[j],"trun");
			if(NULL == tfhd)
			{
				ERROR_LOG("find_box failed!\n");
				HLS_FREE(traf_array);
				HLS_FREE(moof_array);
				return -1;
			}

			if(track_id == get_video_trak_id()) // 是 video traf
			{
		
				unsigned int sample_count = 0;
				source->read(mp4,&sample_count,4,trun->box_first_byte+12,0);
				sample_count = t_ntohl(sample_count);
				
				Vtrun_sample_array[Vtrun_index] = sample_count;
				Vtrun_index ++;
				
			}

			if(track_id == get_audio_trak_id()) // 是 audio traf
			{
				unsigned int sample_count = 0;
				source->read(mp4,&sample_count,4,trun->box_first_byte+12,0);
				sample_count = t_ntohl(sample_count);
				
				Atrun_sample_array[Atrun_index] = sample_count;
				Atrun_index ++;
				
			}
				
		}
		HLS_FREE(traf_array);
		traf_array = NULL;
			
	}
	HLS_FREE(moof_array);
	moof_array = NULL;
	
	if(Vtrun_index != Vtrun_num ||Atrun_index != Atrun_num)
	{
		ERROR_LOG("Vtrun_index(%d) != Vtrun_num(%d) ||Atrun_index(%d) != Atrun_num(%d) !\n",Vtrun_index,Vtrun_num,Atrun_index,Atrun_num);
		return -1;
	}
	
	/*---初始化全局参数sample_offset------------------------------------------------------------------*/
	m_stat_ptr->frame_info.video_offset_array = HLS_MALLOC(context, m_stat_ptr->frame_info.video_frames * sizeof(int));
	if(NULL == m_stat_ptr->frame_info.video_offset_array)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
	memset(m_stat_ptr->frame_info.video_offset_array,0,m_stat_ptr->frame_info.video_frames * sizeof(int));

	m_stat_ptr->frame_info.audio_offset_array = HLS_MALLOC(context, m_stat_ptr->frame_info.audio_frames * sizeof(int));
	if(NULL == m_stat_ptr->frame_info.audio_offset_array)
	{
		ERROR_LOG("malloc failed !\n");
		return -1;
	}
	memset(m_stat_ptr->frame_info.audio_offset_array,0,m_stat_ptr->frame_info.audio_frames * sizeof(int));
	/*-----------------------------------------------------------------------------------------------*/

	
	/*===获取每个音视频帧的 偏移信息数组==============================================================*/
	
	MP4_BOX** mdat_array = NULL;
	int mdat_num = find_box_one_level(context,root,"mdat",&mdat_array);
	DEBUG_LOG("mdat_num = %d\n",mdat_num);
	if(mdat_num != moof_num )
	{
		ERROR_LOG("moof num != mdat num !\n");
		HLS_FREE(mdat_array);
		return -1;
	}

	if(mdat_num != Atrun_index || mdat_num != Vtrun_index )
	{
		ERROR_LOG("mdat_num(%d) Atrun_index(%d) Vtrun_index(%d)\n",mdat_num,Atrun_index,Vtrun_index);
		return -1;
	}


	unsigned int*	V_size_array = m_stat_ptr->frame_info.video_size_array;
	unsigned int*	A_size_array = m_stat_ptr->frame_info.audio_size_array;
	unsigned int*	V_offset_array =  m_stat_ptr->frame_info.video_offset_array;
	unsigned int*	A_offset_array =  m_stat_ptr->frame_info.audio_offset_array;

	unsigned int  V_offset_index = 0; 	//V_offset_array 的下标值
	unsigned int  A_offset_index = 0; 	//A_offset_array 的下标值
	unsigned int  V_size_index = 0; 	//V_size_array的下标值
	unsigned int  A_size_index = 0; 	//A_size_array的下标值
	
	for(i=0 ; i<mdat_num ; i++)  
	{
		if(first_track == get_video_trak_id()) //在前边的是video帧
		{
			unsigned int V_start_pos = mdat_array[i]->box_first_byte + 8;//记录 mdat box video 数据部分开始位置
			unsigned int A_start_pos = mdat_array[i]->box_first_byte + 8;	//记录 mdat box audio 数据部分开始位置
			unsigned int j = 0; 
			unsigned int count_V_sample_size = 0;
			unsigned int count_A_sample_size = 0;
			for(j=0 ; j < Vtrun_sample_array[i] ; j++) //记录视频帧的偏移
			{
				V_offset_array[V_offset_index] = 	V_start_pos + count_V_sample_size;
				V_offset_index ++;
				count_V_sample_size += V_size_array[V_size_index];
				V_size_index ++;
				A_start_pos += count_V_sample_size; //音频sample开始位置偏移到视频sample的后边	
			}

			for(j=0 ; j < Atrun_sample_array[i] ; j++) //记录音频帧的偏移
			{
			
				A_offset_array[A_offset_index] = 	A_start_pos + count_A_sample_size;
				A_offset_index ++;
				count_A_sample_size +=  A_size_array[A_size_index];
				A_size_index ++;	
			}
			
		}
		else if(first_track == get_audio_trak_id())//在前边的是audio帧
		{
			unsigned int V_start_pos = mdat_array[i]->box_first_byte + 8;//记录 mdat box video 数据部分开始位置
			unsigned int A_start_pos = mdat_array[i]->box_first_byte + 8;	//记录 mdat box audio 数据部分开始位置
			unsigned int j = 0; 
			unsigned int count_V_sample_size = 0;
			unsigned int count_A_sample_size = 0;
			for(j=0 ; j < Atrun_sample_array[i] ; j++) //记录音频帧的偏移
			{
			
				A_offset_array[A_offset_index] = 	A_start_pos + count_A_sample_size;
				A_offset_index ++;
				count_A_sample_size +=  A_size_array[A_size_index];
				A_size_index ++;	
				V_start_pos += count_A_sample_size; //视频sample开始位置偏移到音频sample的后边	
			}
						
			for(j=0 ; j < Vtrun_sample_array[i] ; j++) //记录视频帧的偏移
			{
				V_offset_array[V_offset_index] = 	V_start_pos + count_V_sample_size;
				V_offset_index ++;
				count_V_sample_size += V_size_array[V_size_index];
				V_size_index ++;
	
			}


		}
		else
		{
			ERROR_LOG("first_track is not audio or video!\n");
			return -1;
		}
		
			
	}
	HLS_FREE(mdat_array);
	mdat_array = NULL;
	/*================================================================================================*/

	return 0;
	
}



/*
@stsz：box信息链表的头指针
@stsz_sample_count：文件中的sample（帧）个数（Sample count字段）
返回值：
	每个sample的大小信息数组指针（数组有sample_count个元素，每个元素的值是对应sample的大小）
注意：该函数只适合 普通mp4文件，fmp4文件请直接使用 sample_size 结构体变量
*/
int* read_stsz(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* stsz, int* stsz_sample_count) 
{
	int sample_count=0;
	int sample_size=0;
	int how_much=0;
//	fseek(mp4, stsz->box_first_byte+12, SEEK_SET);
//	fread(&sample_size,4,1,mp4);
	source->read(mp4, &sample_size, 4, stsz->box_first_byte+12, 0);
	sample_size=t_ntohl(sample_size);
//	fread(&sample_count, 4,1,mp4);
	source->read(mp4, &sample_count, 4, stsz->box_first_byte+12+4, 0);
	sample_count=t_ntohl(sample_count);
	*stsz_sample_count=sample_count;
	
	int* stsz_data;
	stsz_data=(int*) HLS_MALLOC (context, sizeof(int)*sample_count);
	how_much = stsz->box_first_byte+12+4+4;
	/*
	有部分编码器生成的MP4文件的stsz box里边的Sample size字段是没有设置的，考虑兼容性，
	直接从stsz的数据部分读取每个sample的大小信息，做累加
	*/
	if (sample_size==0) 
	{
		for(int i=0; i<sample_count; i++) 
		{
			//fread(&stsz_data[i],4,1,mp4);
			how_much += source->read(mp4, &stsz_data[i], 4,  how_much, 0);
			stsz_data[i]=t_ntohl(stsz_data[i]);
		}
	}
	else 
	{
		for (int i=0; i<sample_count; i++)
			stsz_data[i]=sample_size;
	}
	return stsz_data;
}

int tag_size(file_handle_t* mp4, file_source_t* source, int offset_from_file_start) 
{
	int tagsize=0;
	int tagsize_bytes=0;
	int temp;
	int how_much = offset_from_file_start;
	do {
		//fread(&temp,1,1,mp4);
		how_much += source->read(mp4, &temp, 1, how_much, 0);
		tagsize = tagsize <<7;
		tagsize |= (temp & 127);
		tagsize_bytes += 1;
	} while ((temp&128)>0);
	return tagsize_bytes;
}

//return count of bytes depends of flag
int read_esds_flag(file_handle_t* mp4, file_source_t* source, int offset_from_file_start) 
{ 
	int flags;
	int count=1;
	//fread(&flags,1,1,mp4);
	source->read(mp4, &flags, 1, offset_from_file_start, 0);
	//streamDependenceflag
	if ((flags & 128) > 0)
		count+=2;
	//URL_Flag
	if ((flags & 64) > 0)
		count+=2;
	//OCR_Streamflag
	if ((flags & 32) > 0)
		count+=2;
	return count;
}

/*
esds box 音频解码参数的解析
返回：是esds box里边Audio Decoder Specific Info下的两个字节解码信息（主要是 Audio_object_type，带有采样率等信息）
*/
int get_DecoderSpecificInfo(file_handle_t* mp4, file_source_t* source, MP4_BOX* stsd) 
{
	/*
	 * stsd:
	 * 		Fullbox - 12 byte
	 * 		entry_count - 4 byte
	 * AudioSampleEntry
	 * 		Box - 8 byte
	 * 		reserverd - 6 byte
	 * 		data_references_index - 2 byte
	 * 		reserved - 8 byte
	 * 		channel_count - 2 byte
	 * 		samplesize - 2 byte
	 * 		pre_defined - 2 byte
	 * 		rezerved - 2 byte
	 * 		samplerate - 4 byte
	 * 	esds
	 * 		Fullbox - 12 byte
	 * 		tag  - 1 byte
	 * 		size - (#1)1*N byte (if size <128  N==1), else
													int sizeOfInstance = 0;
													bit(1) nextByte;
													bit(7) sizeOfInstance;
													while(nextByte) {
													bit(1) nextByte;
													bit(7) sizeByte;
													sizeOfInstance = sizeOfInstance<<7 | sizeByte;
													}
	 * 		ES_ID - 2 byte
	 * 		streamDependenceFlag - 1 bit
	 * 		URL_Flag - 1 bit
	 * 		OCR_Streamflag - 1 bit
	 * 		streamPriority - 5 bit
	 * 		if (streamDependenceFlag)
	 * 			dependsOn_ES_ID - 2 byte
	 * 		if (URL_Flag) {
	 *			URLlength - 1 byte
	 * 			URLstring[URLlength] - 1 byte
	 *			}
	 *		if (OCRstreamFlag)
	 *			OCR_ES_Id - 2 byte
	 *	DecoderConfigDescriptor
	 * 		tag - 1 byte
	 * 		size - 1*N byte (#1)
	 * 		ObjectTypeIndication - 1 byte
	 * 		streamtype - 6 bit
	 * 		upstream - 1 bit
	 * 		reserved - 1 bit
	 * 		buffersizeDB - 3 byte
	 * 		maxBitrate - 4 byte
	 * 		avgBitrate - 4 byte
	 * 	DecoderSpecificInfo
	 * 		tag - 1 byte
	 * 		size - 1*N byte (#1)
	 * 		Data[0] - 1 byte
	 * 		Data[1] - 1 byte
	 *
	 *
	 * 	Data[0] & Data [1] - is necessary;
	 */
	 //-----这一段只是在计算skip_bytes-------------------------------------------------------------------------------
	int skip_bytes=0; // byte counter before Data[0];
	skip_bytes = 65; //before tag_size;
	//fseek(mp4, stsd->box_first_byte+skip_bytes, SEEK_SET);
	skip_bytes += (tag_size(mp4, source, stsd->box_first_byte+skip_bytes) + 2); //2 - ES_ID
	//fseek(mp4,2,SEEK_CUR); // 2 - ES_ID
	skip_bytes += (read_esds_flag(mp4, source, stsd->box_first_byte+skip_bytes) +1); // 1 - DecoderConfigDescriptor tag
	//fseek(mp4,stsd->box_first_byte+skip_bytes,SEEK_SET);
	skip_bytes += (tag_size(mp4, source, stsd->box_first_byte+skip_bytes) + 13 + 1); // 13 - Sum(ObjectTypeIndication .. avgBitrate); 1 - DecoderSpecificInfo tag
	//fseek(mp4,stsd->box_first_byte+skip_bytes,SEEK_SET);
	skip_bytes += tag_size(mp4, source, stsd->box_first_byte+skip_bytes);
	//---------------------------------------------------------------------------------------------------------------
	int data0=0;
	int data1=0;
	//fread(&data0,1,1,mp4);
	//fread(&data1,1,1,mp4);
	source->read(mp4, &data0, 1, stsd->box_first_byte+skip_bytes, 0);
	source->read(mp4, &data1, 1, stsd->box_first_byte+skip_bytes+1, 0);
	data0 = (data0<<8) | data1;
	// Temporary ignored 5 bytes info;
	return data0;
}

/*
avc --> avcC box 视频解码参数的解析
@avc_dec_info_size：数据 sps + pps的大小
@NALUnitLengthSize: avcC的NALUnitLengthSize字段
返回：成功：sps pps的信息指针 	失败：NULL
*/
char* get_AVCDecoderSpecificInfo(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* stsd, int* avc_dec_info_size, int* NALUnitLengthSize) 
{
	/*
	 * stsd:
	 * 		Fullbox - 12 byte
	 * 		entry_count - 4 byte
	 * VisualSampleEntry
	 * 		Box - 8 byte
	 * 		reserverd - 6 byte
	 * 		data_references_index - 2 byte
	 *
	 *		pre_defined - 2 byte
	 *		reserved - 2 byte
	 *		pre_defined - 12 byte
	 *		width - 2 byte
	 *		height - 2 byte
	 *		horizresolution - 4 byte
	 *		vertresolution - 4 byte
	 *		reserved - 4 byte
	 *		frame_count - 2 byte
	 *		compressorname - 32 byte
	 *		depth - 2 byte
	 *		pre_defined - 2 byte
	 *
	 * AVCSampleEntry
	 * 		AVCConfigurationBox
	 * 			Box - 8 byte ("avcC")
	 * 			AVCDecoderConfigurationRecord {
					configurationVersion  - 1 byte
					AVCProfileIndication - 1 byte
					profile_compatibility - 1 byte
					AVCLevelIndication - 1 byte
					reserved = ‘111111’b - 6 bit
					lengthSizeMinusOne - 2 bit
					reserved = ‘111’b - 3 bit
					numOfSequenceParameterSets - 5 bit
					for (i=0; i< numOfSequenceParameterSets; i++) {
						sequenceParameterSetLength - 2 byte
						sequenceParameterSetNALUnit - 1*(sequenceParameterSetLength) byte
					}
					numOfPictureParameterSets - 1 byte
					for (i=0; i< numOfPictureParameterSets; i++) {
						pictureParameterSetLength - 2 byte
						pictureParameterSetNALUnit - 1*(pictureParameterSetLength) byte
					}
				}
	 *
	 *
	 */

	DEBUG_LOG("into get SPS PPS func! \n");
	int skip_bytes=stsd->box_first_byte+16;
	//int entry_count=0;
	//skip_bytes+=source->read(mp4, &entry_count, 4, skip_bytes, 0);
	//entry_count=ntohl(entry_count);
	skip_bytes+=86;
//	if (entry_count>1) {
		int box_size=0;
		char box_name[4]={0,0,0,0};
		while (memcmp("avcC", box_name, 4)!=0) 
		{
			source->read(mp4, &box_size, 4, skip_bytes, 0);
			source->read(mp4, box_name, 4, skip_bytes+4, 0);
			box_size=t_ntohl(box_size);
			if (memcmp("avcC",box_name, 4)!=0)
				skip_bytes+=box_size;
		}
		
	//}
		//NALUnitLengthSize
		int offset=skip_bytes+12;
		int NALULS=0;
		source->read(mp4,&NALULS, 1, offset, 0);
		NALULS&=3;
		if(NALUnitLengthSize!=NULL)
			*NALUnitLengthSize=NALULS+1;
	skip_bytes+=13; // avcC before reserved 3 bits
	int numofSPS=0;
	skip_bytes+=source->read(mp4, &numofSPS, 1, skip_bytes, 0);
	numofSPS&=31;
	int* SPSLength;
	SPSLength = (int*) HLS_MALLOC (context, sizeof(int)*numofSPS);
	if (SPSLength==NULL) 
	{
		ERROR_LOG("Couldn't allocate memory for SPSLength");
		return NULL;
	}
	
	for (int v=0; v<numofSPS; v++)
		SPSLength[v]=0;
	int sps_length_summ=0;
	int sps_count=skip_bytes;
	for (int i=0; i<numofSPS; i++) 
	{
		source->read(mp4, &SPSLength[i], 2, sps_count, 0);
		SPSLength[i]=t_ntohs(SPSLength[i]);
		sps_length_summ+=SPSLength[i];
		sps_count+=2+SPSLength[i];
	}
	char* SPS=NULL;
	SPS = (char*) HLS_MALLOC (context, sizeof(char)*sps_length_summ+sizeof(char)*4*numofSPS);
	if (SPS==NULL) 
	{
			ERROR_LOG("Couldn't allocate memory for SPS");
			return NULL;
	}
	sps_count=skip_bytes+2;
	char* temp_ptr=SPS;
	char annexb_syncword[4]={0,0,0,1};
	for (int i=0; i<numofSPS; i++) 
	{
		memcpy(temp_ptr, annexb_syncword, 4);
		source->read(mp4, temp_ptr+4, SPSLength[i], sps_count, 0);
		sps_count+=SPSLength[i]+2;
		temp_ptr+=4+SPSLength[i];
	}
	
	skip_bytes=sps_count-2;

	int numofPPS=0;
	skip_bytes+=source->read(mp4, &numofPPS, 1, skip_bytes, 0);
	int* PPSLength;
	PPSLength = (int*) HLS_MALLOC (context, sizeof(int)*numofPPS);
	if (PPSLength==NULL) 
	{
			ERROR_LOG("Couldn't allocate memory for PPSLength");
			return NULL;
	}
	
	for (int v=0; v<numofPPS; v++)
			PPSLength[v]=0;
	int pps_length_summ=0;
	int pps_count=skip_bytes;
	for (int i=0; i<numofPPS; i++) 
	{
		source->read(mp4, &PPSLength[i], 2, pps_count, 0);
		PPSLength[i]=t_ntohs(PPSLength[i]);
		pps_length_summ+=PPSLength[i];
		pps_count+=2+PPSLength[i];
	}
	
	char* PPS;
	PPS = (char*) HLS_MALLOC (context, sizeof(char)*pps_length_summ+sizeof(char)*4*numofPPS);
	if (PPS==NULL) 
	{
		ERROR_LOG("Couldn't allocate memory for PPS");
		return NULL;
	}
	
	pps_count=skip_bytes+2;
	temp_ptr=PPS;
	for (int i=0; i<numofPPS; i++) 
	{
		memcpy(temp_ptr, annexb_syncword, 4);
		source->read(mp4, temp_ptr+4, PPSLength[i], pps_count, 0);
		pps_count+=PPSLength[i]+2;
		temp_ptr+=4+PPSLength[i];
	}

	char* avc_decoder_info=NULL; //SPS+PPS
	int SPS_size=sizeof(char)*sps_length_summ+sizeof(char)*4*numofSPS;
	int PPS_size=sizeof(char)*pps_length_summ+sizeof(char)*4*numofPPS;
	//avc_decoder_info= (char*) malloc (sizeof(char)*(sps_length_summ+pps_length_summ)+sizeof(char)*4*(numofSPS+numofPPS));
	avc_decoder_info= (char*) HLS_MALLOC (context, SPS_size+PPS_size);
	if (avc_decoder_info==NULL) 
	{
		ERROR_LOG("Couldn't allocate memory for avc_decoder_info");
		return NULL;
	}
	
	*avc_dec_info_size=SPS_size+PPS_size;
	memcpy(avc_decoder_info, SPS, SPS_size);
	memcpy(avc_decoder_info+SPS_size, PPS, PPS_size);
	HLS_FREE(SPS);
	SPS = NULL;
	
	HLS_FREE(SPSLength);
	SPSLength = NULL;
	
	HLS_FREE(PPS);
	PPS = NULL;
	
	HLS_FREE(PPSLength);
	PPSLength = NULL;
	//OUTPUT TESTING
		/*FILE* mp4_sound;
			mp4_sound=fopen("avc_dec_info.h264","wb");
			if(mp4_sound==NULL) {
				printf("\nCould'n create testfile.h264 file\n");
				exit(1);
			}
			fwrite(avc_decoder_info, *avc_dec_info_size, 1, mp4_sound);
			fclose(mp4_sound);
			*/
	//
	DEBUG_LOG("out get SPS PPS func ! \n");
	return avc_decoder_info;
}


int  get_sounddata_to_file(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* root) {
	FILE* mp4_sound;
	mp4_sound=fopen("mp4_sound.aac","wb");
	if(mp4_sound==NULL) 
	{
		ERROR_LOG("\nCould'n create mp4_sound.aac file\n");
		return -1;
	}
	MP4_BOX* find_trak;
	MP4_BOX* find_stsc;
	MP4_BOX* find_stco;
	MP4_BOX* find_stsz;
	MP4_BOX* find_stsd;

	find_trak=find_necessary_trak(mp4, source, root, "soun");
	find_stsc=(find_box(find_trak, "stsc"));
	find_stco=(find_box(find_trak, "stco"));
	find_stsz=(find_box(find_trak, "stsz"));
	find_stsd=(find_box(find_trak, "stsd"));

	int stsc_entry_count=0;
	int stco_entry_count=0;
	int stsz_entry_count=0;

	sample_to_chunk** stsc_data;
	stsc_data = read_stsc(context, mp4, source, find_stsc, &stsc_entry_count);

	int* stco_data;
	stco_data = read_stco(context, mp4, source, find_stco, &stco_entry_count);

	int* stsz_data;
	stsz_data = read_stsz(context, mp4, source, find_stsz, &stsz_entry_count);

	//Test getDecoderSpecificInfo
	int DecSpecInfo=0;
	long long BaseMask=0xFFF10200001FFC;
	DecSpecInfo = get_DecoderSpecificInfo(mp4, source, find_stsd);
	long long objecttype=0;
	long long frequency_index=0;
	long long channel_count=0;
	long long TempMask=0;
	objecttype = (DecSpecInfo >> 11) - 1;
	frequency_index = (DecSpecInfo >> 7) & 15;
	channel_count = (DecSpecInfo >> 3) & 15;

	objecttype = objecttype << 38;
	frequency_index = frequency_index << 34;
	channel_count = channel_count << 30;
	BaseMask = BaseMask | objecttype | frequency_index | channel_count;

	//read and write chunks
	char* buffer;
	int samples_in_chunk;
	int last_used_stsz_sample_size=0;
	for (int i=0; i<stco_entry_count; i++) {
	//	fseek(mp4,stco_data[i],SEEK_SET);
		int how_much=stco_data[i];
		//get sample count from sample_to_chunk
		for(int j=0; j<stsc_entry_count; j++) {
			if (j<stsc_entry_count-1) {
				if ((i+1 >= stsc_data[j]->first_chunk) && (i+1 < stsc_data[j+1]->first_chunk)) {
					samples_in_chunk = stsc_data[j]->samples_per_chunk;
					break;
				}
			}
			else {
				samples_in_chunk = stsc_data[j]->samples_per_chunk;
			}
		}
		//read data from mp4 and write to mp4_sound
		for (int k=last_used_stsz_sample_size; k<last_used_stsz_sample_size+samples_in_chunk; k++) {
			buffer = HLS_MALLOC (context, stsz_data[k]);
			if(buffer==NULL) {
				printf("\nCouldn't allocate memory for buffer[%d]",k);
				exit(1);
			}
		//	fread(buffer,stsz_data[k],1,mp4);
			how_much+=source->read(mp4, buffer,stsz_data[k],how_much, 0);
			TempMask = BaseMask | ((stsz_data[k] + 7) << 13);
			char for_write=0;
			if(k==0)
				printf("\nTempMask[%d] = %lld\n",k,TempMask);
			for (int g=6; g>=0; g--) {
				for_write=TempMask >> g*8;

				fwrite(&for_write,1,1,mp4_sound);
			}
			fwrite(buffer,stsz_data[k],1,mp4_sound);
			HLS_FREE(buffer);
			if(k==last_used_stsz_sample_size+samples_in_chunk-1) {
				last_used_stsz_sample_size+=samples_in_chunk;
				break;
			}

		}
	}
	//free used memory
	HLS_FREE(stco_data);
	HLS_FREE(stsz_data);
	for (int z=0; z<stsc_entry_count; z++)
		HLS_FREE(stsc_data[z]);
	HLS_FREE(stsc_data);
	fclose(mp4_sound);

	return 0;
}

/*
解析tkhd box的id号(video/audio trak所对应的id号)
返回：
	成功：id号		失败：-1;
*/
int parse_trak_id(file_handle_t* mp4, file_source_t* source, MP4_BOX* tkhd)
{
	if(NULL== mp4||NULL == source||NULL == tkhd)
	{
		printf("parameter error !\n");
		return -1;
	}

	int tkhd_id;
	source->read(mp4, &tkhd_id, 4, tkhd->box_first_byte+ 20, 0);//tkhd box中的trak_id字段相对于box头偏移了20个字节
	tkhd_id = t_ntohl(tkhd_id);
	return tkhd_id;
	
}


/*
获取moof box的总数量，并返回每个moof box节点的指针数组
*/
int get_count_of_moof(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* root,MP4_BOX**moof_box)
{
	//
	if(NULL == mp4 || NULL == source || NULL == moof_box)
	{
		printf("Illegal parameter!\n");
		return -1;
	}
	
	unsigned int moof_count = 0;
	MP4_BOX* box = NULL;
	if (root->child_count > 0) 
	{
		for (int i=0; i<root->child_count; i++) 
		{
			box = find_box(root->child_ptr[i], "moof");
			if (box!=NULL)
			{
				moof_count ++;
			}
				
		}
	}
}


/*
返回：
	成功：trak的数量（音视频轨道）;并将（video/audio）trak的首节点指针放到参数 moov_traks 里边,构成trak节点数组
	失败：-1
	注意：用完后需要释放 moov_traks指针
*/
int get_count_of_traks(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* root, MP4_BOX*** moov_traks) 
{
	MP4_BOX* moov;
	*moov_traks = (MP4_BOX**) HLS_MALLOC (context, sizeof(MP4_BOX*)*2);
	if(*moov_traks==NULL) 
	{
		printf("Couldn't allocate memory for moov_traks");
		return -1;
	}
	moov = find_box(root, "moov");
	int trak_counter=0;
	for(int i=0; i<moov->child_count; i++) 
	{
		/* Old Code
		if (trak_counter<2 && compare_box_type(moov->child_ptr[i],"trak") &&
			(handlerType(mp4, source, find_box(moov->child_ptr[i],"hdlr"), "soun") || handlerType(mp4, source, find_box(moov->child_ptr[i],"hdlr"), "vide"))) 
		{
			(*moov_traks)[trak_counter]=moov->child_ptr[i];
			trak_counter++;
		}
		*/

		if (trak_counter<2 && compare_box_type(moov->child_ptr[i],"trak"))
		{
			/*---记录video/audio trak对应的id号-----------------------------------------*/
			if(handlerType(mp4, source, find_box(moov->child_ptr[i],"hdlr"), "soun"))//是音频trak
			{
				int audio_id = parse_trak_id(mp4,source,find_box(moov->child_ptr[i],"tkhd"));
				set_audio_trak_id(audio_id);
			}
			else if(handlerType(mp4, source, find_box(moov->child_ptr[i],"hdlr"), "vide"))//是视频trak
			{
				int video_id = parse_trak_id(mp4,source,find_box(moov->child_ptr[i],"tkhd"));
				set_video_trak_id(video_id);
			}
			else
			{
				ERROR_LOG("error : unknow trak!\n");
				return -1;
			}
			/*----------------------------------------------------------------------------*/

			(*moov_traks)[trak_counter]=moov->child_ptr[i];
			trak_counter++;			
				
		}

	}
			 
	return trak_counter;
}
/*
int get_count_of_audio_traks(file_handle_t* mp4, file_source_t* source, MP4_BOX* root, MP4_BOX*** moov_traks) {
	MP4_BOX* moov;
	*moov_traks = (MP4_BOX**) malloc (sizeof(MP4_BOX*)*2);
	if(*moov_traks==NULL) {
		printf("Couldn't allocate memory for moov_traks");
		exit(1);
	}
	moov = find_box(root, "moov");
	int trak_counter=0;
	for(int i=0; i<moov->child_count; i++) {
		if (compare_box_type(moov->child_ptr[i],"trak") && handlerType(mp4,source, find_box(moov->child_ptr[i],"hdlr"),"soun")) {
			(*moov_traks)[trak_counter]=moov->child_ptr[i];
			trak_counter++;
		}
	}
	return trak_counter;
}
*/

/*
获取MP4文件中的编解码信息（寻找MP4a 和 avc1 box）
返回：
	0x0F：mp4a（ AAC_VIDEO ）		0x1B：avc1（ H264_VIDEO ）  		0：未找到
*/
int get_codec(file_handle_t* mp4, file_source_t* source, MP4_BOX* trak) 
{
	if(NULL == mp4 || NULL == source || NULL == trak)
	{
		ERROR_LOG("Illegal parameter !\n");
		return 0;
	}
	
	MP4_BOX* stsd;
	stsd = find_box(trak, "stsd");
	if(NULL == stsd)
	{
		ERROR_LOG("not find stsd box in  %.4s box !\n",trak->box_type);
		return 0;
	}
	char box_type[4];
//	fseek(mp4, stsd->box_first_byte+20, SEEK_SET);
//	fread(box_type, 1, 4, mp4);
	source->read(mp4, box_type, 4, stsd->box_first_byte+20, 0);
	char codec[4]="mp4a";
	char codec2[4]="avc1";
	/*for(int i=0; i<4; i++) {
		if (box_type[i]!=codec[i])
			return 0;
	}
	return 0x0F;
	*/
	DEBUG_LOG("box_type = %.4s\n",box_type);
	if(memcmp(codec,box_type,4)==0)
		return 0x0F;//mp4a
	if(memcmp(codec2,box_type,4)==0)
		return 0x1B;//avc1
	return 0;
}

/*
@trak ：trak分支的根节点
@root ：box信息节点链表的根节点
注意：该接口只适用于普通MP4文件
返回：帧数目

*/
int get_nframes(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* trak) 
{
	
		MP4_BOX* stsz;
		stsz = find_box(trak, "stsz");
		int* data;
		int nframes=0;
		data = read_stsz(context, mp4, source, stsz, &nframes);
		HLS_FREE(data);
		return nframes;
	
}

/*
获取音频帧的pts,计算每一个帧的pts值，放到参数pts所指向的数组中
返回： 成功：0     	失败：-1
*/
int  get_pts(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* audio_trak, int nframes, int DecoderSpecificInfo,float* pts) 
{
	int* delta;
	delta = (int*) HLS_MALLOC (context, sizeof(int)*nframes);
	if (delta==NULL) 
	{
			ERROR_LOG("\nCouldn't allocate memory for pts!\n");
			return -1;
	}

	/*----注意，解析stts的方式只适合于普通的mp4文件-------------------------------------------------------------*/
	if(MP4 == get_mp4_file_type())//该部分代码只是为了构造delta[]数组，里边值为每个帧的大小（采样点）信息
	{
		MP4_BOX* stts;
		stts = find_box(audio_trak, "stts");
		int stts_entry_count=0;
		//fseek(mp4, stts->box_first_byte+12, SEEK_SET);
		//fread(&stts_entry_count, 4, 1, mp4);
		source->read(mp4, &stts_entry_count, 4, stts->box_first_byte+12, 0);
		stts_entry_count=t_ntohl(stts_entry_count);

		//TEST stts_entry_count
		//printf("\nstts_entry_count = %d\n",stts_entry_count);
		//fflush(stdout);
		//
		int sample_count=0;
		int sample_size=0;//Sample delta
		int delta_counter=0;
		int how_much=stts->box_first_byte+12+4;
		int i;
		for(i = 0; i < stts_entry_count ; i++) 
		{
			//fread(&sample_count,4,1,mp4);
			//fread(&sample_size,4,1,mp4);
			how_much += source->read(mp4, &sample_count, 4, how_much, 0);//读取 stts box的sample_count
			how_much += source->read(mp4, &sample_size, 4, how_much, 0);//读取 stts box的Sample delta
			sample_count=t_ntohl(sample_count);
			sample_size=t_ntohl(sample_size);
			for(int k=0; k < sample_count; k++) //存放每一个帧的大小信息（实际是采样点信息，AAC 为 1024）
			{
				delta[delta_counter]=sample_size;
				delta_counter++;
			}

		}
	}
	/*----------------------------------------------------------------------------------------------*/
	else if(FMP4 == get_mp4_file_type())
	{
		 // audio_frames 即为上方的  sample_count
		int i = 0;
		for (i=0 ; i<frame_info.audio_frames ; i++)
		{
			delta[i] = 1024;	//AAC数据帧的 sample_size 采样点数固定为 1024
		}
	}
	else
	{
		ERROR_LOG("unknown file type!\n");
		return -1;
	}
	

	
	pts[0]=0;
	double real_pts=0;
	double real_pts_minus_one=0;
	double tmp=0;
	DecoderSpecificInfo = (DecoderSpecificInfo>>7) & 15;//&5：保留低5位
	/*----计算每一个帧的 PTS值---------------------------------------------------------*/
	for(int i=1; i<nframes; i++) 
	{
//		pts[i]=pts[i-1]+(float)1024/SamplingFrequencies[DecoderSpecificInfo];
		real_pts = real_pts_minus_one + (double)delta[i-1]/SamplingFrequencies[DecoderSpecificInfo];//一帧的实际时长
		float cmpr=fabsf(real_pts - (real_pts_minus_one+(double)1024/SamplingFrequencies[DecoderSpecificInfo]));
//		if(cmpr>0.2) {
//			printf("\ncmpr[%d] = %f",i,cmpr);
//			fflush(stdout);
//		}
		tmp=(cmpr<0.2) ? (tmp+(double)1024/SamplingFrequencies[DecoderSpecificInfo]) : (real_pts);
		pts[i]=tmp;
		real_pts_minus_one=real_pts;
	}
	/*---------------------------------------------------------------------------------*/
	HLS_FREE(delta);

	return 0;

}

/*
功能：获取fmp4文件 一个video traf-->trun 视频  	   帧的持续时间（内部时间，非实际时间）数组指针
参数：
	@duration_array:每个帧的持续时间（内部时间，非实际时间）数组指针
返回：成功：数组元素个数		   		失败：-1
注意：使用后需要 free(duration_array)  
*/
int get_fmp4_video_sample_duration(void* context, file_handle_t* mp4, file_source_t* source,MP4_BOX* traf_video,int**duration_array)
{
	if(NULL == mp4 || NULL == source || NULL == traf_video)
	{
		ERROR_LOG("Illegal parameter !\n");
		return -1;
	}

	
	/*---读取 tfhd-->Default sample duration，以防trun-->duration没有填充，就用该值-------*/
	unsigned int 	default_sample_duration = 0; 	
	MP4_BOX* tfhd =  find_box(traf_video, "tfhd");
	if(tfhd)
	{
		source->read(mp4, &default_sample_duration, 4, tfhd->box_first_byte + 24, 0);
		default_sample_duration = t_ntohl(default_sample_duration);
		if(0 == default_sample_duration)
		{
			//文件中该项没有填充，以trun-->duration为准
			//do noting
		}
		
	}

	unsigned int flag = 0;
	unsigned int Sample_count = 0;//当前trun下的 samples（帧） 个数
	unsigned int first_duration_offset = 0;//第一个duration数据开始的位置偏移
	unsigned int Duration_offset = 0; //统计 两个sample_duration 字段之间的距离
	MP4_BOX* trun =  find_box(traf_video, "trun");
	if(trun)
	{
		//sample_count
		source->read(mp4, &Sample_count, 4, trun->box_first_byte + 12, 0);
		Sample_count = t_ntohl(Sample_count);
		int* duration_array_tmp = HLS_MALLOC(context, sizeof(int) * Sample_count);
		if(NULL == duration_array_tmp)
		{
			ERROR_LOG("malloc failed !\n");
			return -1;
		}

		//flag  
		source->read(mp4, &flag, 4, trun->box_first_byte + 8, 0);
		flag = t_ntohl(flag);
		flag = flag & 0x00FFFFFF;//flag只占三字节

		/*---计算读取第一个 sample_duration 的偏移位置----------------------*/
		first_duration_offset = trun->box_first_byte + 16 ; //计算到sample_count字段
		//以下两个字段为可选字段，不一定每个mp4文件都填充，所以需判断flag来动态变换 first_duration_offset 的偏移长度
		if(flag & E_data_offset)
			first_duration_offset += 4;
		if(flag & E_first_sample_flags)
			first_duration_offset += 4;
		/*------------------------------------------------------------------*/

		/*---统计两个sample_duration 字段之间的间隔字节数-------------------*/
		if(flag & E_sample_duration)
			Duration_offset += 4;
		if(flag & E_sample_size)
			Duration_offset += 4;
		if(flag & E_sample_flags)
			Duration_offset += 4;
		if(flag & E_sample_composition_time_offset)
			Duration_offset += 4;
		/*------------------------------------------------------------------*/

		//开始填充 duration_array 信息数组
		int i = 0;
		for (i=0 ; i<Sample_count ; i++)
		{
			source->read(mp4, &duration_array_tmp[i], 4 , first_duration_offset + i*Duration_offset , 0);
			duration_array_tmp[i] = t_ntohl(duration_array_tmp[i]);
			DEBUG_LOG("duration_array_tmp[%d] = %d\n",i,duration_array_tmp[i]);
			
		}

		*duration_array = duration_array_tmp;
		return Sample_count;
		
	}

	return -1;
	

}


/*
获取视频帧的dts,计算每一个帧的dts值，放到参数dts所指向的数组中
返回：成功 ：0 	失败：-1
*/
int get_video_dts(void* context, file_handle_t* mp4, file_source_t* source,  MP4_BOX* root , MP4_BOX* video_trak, int nframes, float* dts) 
{
	if(NULL == mp4 || NULL == source || NULL == root || NULL == video_trak || NULL == dts)
	{
		ERROR_LOG("Illegal parameter !\n");
		return -1;
	}
	
	int* delta= NULL;   //数组指针，用于存储 所有视频帧的 sample_duration 参数
	int i = 0;
	delta = (int*) HLS_MALLOC (context, sizeof(int)*nframes);
	if (delta==NULL) 
	{
		ERROR_LOG("Couldn't allocate memory for TMP_video_dts\n");
		return -1;
	}

	/*----注意，解析stts的方式只适合于普通的mp4文件-------------------------------------------------------------*/
	if(MP4 == get_mp4_file_type())//获取每个视频帧的 所占的时间单元数数组
	{
		MP4_BOX* stts=NULL;
		stts = find_box(video_trak, "stts");
		int stts_entry_count=0;
	//	fseek(mp4, stts->box_first_byte+12, SEEK_SET);
	//	fread(&stts_entry_count, 4, 1, mp4);
		source->read(mp4, &stts_entry_count, 4, stts->box_first_byte+12, 0);
		stts_entry_count=t_ntohl(stts_entry_count);
		
		int sample_count=0;
		int sample_size=0; //每一帧所占的时间单元数 （Time scale）
		int delta_counter=0;
		int how_much=stts->box_first_byte+12+4;
		for(i=0; i<stts_entry_count; i++) 
		{
			//fread(&sample_count,4,1,mp4);
			//fread(&sample_size,4,1,mp4);
			how_much += source->read(mp4, &sample_count, 4, how_much, 0);
			how_much += source->read(mp4, &sample_size, 4, how_much, 0);
			sample_count = t_ntohl(sample_count);
			sample_size = t_ntohl(sample_size);
			for(int k=0; k<sample_count; k++) 
			{
				delta[delta_counter]=sample_size;
				delta_counter++;
			}
		}

	}
	else if(FMP4 == get_mp4_file_type())
	{
		/*---获取每个视频帧的 所占的时间单元数数组----------------------*/
		//1.找出所有moof box节点，构成moof节点数组
		MP4_BOX** moof_array = NULL;
		MP4_BOX** traf_array = NULL;
		unsigned int delta_counter = 0;
		
		int moof_num = find_box_one_level(context,root,"moof",&moof_array);
		DEBUG_LOG("moof_num = %d\n",moof_num);
		for(i=0 ; i<moof_num ; i++)
		{
			int traf_num = find_box_one_level(context,moof_array[i],"traf",&traf_array);
			DEBUG_LOG("traf_num = %d\n",traf_num);
			int j = 0;
			for(j=0 ; j<traf_num ; j++)
			{
				/*---确保传进去的是 video traf 的根节点-----*/	
				//先解析tfhd确定当前traf的trak_id
				int track_id;
				MP4_BOX* tfhd =  find_box(traf_array[j],"tfhd");
				if(NULL == tfhd)
				{
					ERROR_LOG("find_box failed!\n");
					continue;
				}
				else
				{
					source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
					track_id = t_ntohl(track_id);
					
				}
				/*------------------------------------------*/

				if(track_id == get_video_trak_id()) // 是 video traf
				{
					int* duration_array = NULL;
					int array_num = 0;
					array_num = get_fmp4_video_sample_duration(context , mp4 , source ,traf_array[j], &duration_array);
					if(array_num > 0 && NULL != duration_array)
					{
						//将该分支的 sample_duration 数组拷贝到整个大的 sample_duration数组
						memcpy(&delta[delta_counter],duration_array,sizeof(int)*array_num);
						delta_counter += array_num;
						
					}
					HLS_FREE(duration_array);
				}
				
			}
			
		}
		
	
		DEBUG_LOG("delta[0] = %d delta[1] = %d delta[2] = %d\n",delta[0],delta[1],delta[2]);
		HLS_FREE(moof_array);
		HLS_FREE(traf_array);
		
	}
	else
	{
	
		ERROR_LOG("unknown file type!\n");
		return -1;
	}

	/*----构建视频帧的dts------------------------------------------------------------------*/
	MP4_BOX* mdhd=NULL;
	mdhd = find_box(video_trak, "mdhd");
	int mdhd_version=0;
	source->read(mp4, &mdhd_version, 1, mdhd->box_first_byte+8, 0);
	//printf("\nmdhd version = %d",mdhd_version);
	//fflush(stdout);
	int need_to_skip=0;
	if(mdhd_version==1) 
	{
		need_to_skip=16;
	}
	else if(mdhd_version==0) 
	{
		need_to_skip=8;
	}
	else if(mdhd_version!=0 && mdhd_version!=1) 
	{
		ERROR_LOG("trak's mdhd!=0 && mdhd!=1\n");
		return -1;
	}
	int timescale=0;
	source->read(mp4, &timescale, 4, mdhd->box_first_byte+12+need_to_skip, 0);
	timescale = t_ntohl(timescale);
	DEBUG_LOG("timescale = %d \n",timescale);
	
	dts[0]=0;
	double real_dts=0;
//	real_pts+=(double)delta[i-1]/SamplingFrequencies[DecoderSpecificInfo];
	DEBUG_LOG("nframes = %d real_dts = %f\n",nframes,real_dts);
	for(int i=1; i<nframes; i++) 
	{
		real_dts += (double)delta[i-1]/timescale;
		dts[i]=(float)real_dts;
		DEBUG_LOG("real_dts = %f dts[%d] = %f\n",real_dts,i,dts[i]);
	}
	HLS_FREE(delta);
	DEBUG_LOG("dts[0] = %f  dts[nframes-1] = %f\n",dts[0],dts[nframes-1]);
	return 0;
}

/*
返回： 成功：0 	失败：-1
*/
int  get_video_pts(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* video_trak, int nframes, float* pts, float* dts) 
{
	int* delta;
	delta = (int*) HLS_MALLOC (context, sizeof(int)*nframes);
	if (delta==NULL) 
	{
		printf("\nCouldn't allocate memory for TMP_video_dts");
		return -1;
	}

	/*---ctts只有普通mp4文件才有，fmp4文件需要特殊处理-----------------------*/
	if(MP4 == get_mp4_file_type())
	{
		MP4_BOX* ctts=NULL;
		ctts = find_box(video_trak, "ctts");
		if(ctts==NULL) 
		{
			memcpy(pts,dts,nframes*sizeof(float));
			return 0;
		}
		int ctts_entry_count=0; //entry_count 字段
		source->read(mp4, &ctts_entry_count, 4, ctts->box_first_byte+12, 0);
		ctts_entry_count = t_ntohl(ctts_entry_count);
		
		int sample_count=0;
		int sample_size=0;
		int delta_counter=0;
		int how_much = ctts->box_first_byte+12+4;
		for(int i=0; i<ctts_entry_count; i++) 
		{
			//fread(&sample_count,4,1,mp4);
			//fread(&sample_size,4,1,mp4);
			how_much += source->read(mp4, &sample_count, 4, how_much, 0);
			how_much += source->read(mp4, &sample_size, 4, how_much, 0);
			sample_count = t_ntohl(sample_count);
			sample_size = t_ntohl(sample_size);
			for(int k=0; k<sample_count; k++) 
			{
				delta[delta_counter]=sample_size;
				//printf("\nctts[%d] = %d",delta_counter, delta[delta_counter]);
				//fflush(stdout);
				delta_counter++;
			}
		}
	}
	else if(FMP4 == get_mp4_file_type())
	{
		for(int i=0; i<nframes; i++) 
		{
			delta[i] = 0;  //暂时置为0，后边如若有问题再修改(如果一个视频没有B帧，则pts永远和dts相同)
		}
	
	}
	else
	{
		ERROR_LOG("unknown file type!\n");
		return -1;
	}
	

	/*---------------------------------------------------------------------*/
	
	////////////////
	//printf("\nDelta counter (ctts sample counter) = %d",delta_counter);
	///////////////
	MP4_BOX* mdhd=NULL;
	mdhd = find_box(video_trak, "mdhd");
	int mdhd_version=0;
	source->read(mp4, &mdhd_version, 1, mdhd->box_first_byte+8, 0);
	int need_to_skip=0;
	if(mdhd_version==1) 
	{
		need_to_skip=16;
	}
	else if(mdhd_version==0) 
	{
		need_to_skip=8;
	}
	else if(mdhd_version!=0 && mdhd_version!=1) 
	{
		printf("\ntrak's mdhd!=0 && mdhd!=1");
		return -1;
	}
	int timescale=0;
	source->read(mp4, &timescale, 4, mdhd->box_first_byte+12+need_to_skip, 0);
	timescale = t_ntohl(timescale);
	//
	//printf("\nTimescale = %d",timescale);
	//


	pts[0]=0;
	double real_pts=0;
	for(int i=0; i<nframes; i++) 
	{
		real_pts = dts[i]+(double)delta[i]/timescale;
		pts[i]=(float)real_pts;//dts[i]+(float)delta[i]/timescale;
	}
	HLS_FREE(delta);

	return 0;

}

/*
获取stss 数据(确定media中的关键帧) ，返回数组指针，数组长度在参数 entry_count 返回
注意：使用完后需要释放 返回的指针
返回：成功 数组指针 失败：NULL;
*/
int* get_stss(void* context, file_handle_t* mp4, file_source_t* source, MP4_BOX* video_trak, int* entry_count) 
{
	MP4_BOX* stss_ptr;
	stss_ptr = find_box(video_trak, "stss");
	if(NULL == stss_ptr)
	{
		ERROR_LOG("not have stss box!\n"); //fmp4文件的关键帧信息不放在该box里边，可能没有该box
		return NULL;
		
	}
	int stss_entry_count=0;
	source->read(mp4, &stss_entry_count, 4, stss_ptr->box_first_byte+12, 0);
	stss_entry_count = t_ntohl(stss_entry_count);
	*entry_count=stss_entry_count;
	int how_much=stss_ptr->box_first_byte+16;
	int* stss;
	stss = (int*) HLS_MALLOC (context, sizeof(int)*stss_entry_count);
	if (stss==NULL) 
	{
			ERROR_LOG("Couldn't allocate memory for stss !\n");
			return NULL;
	}
	for (int i=0; i<stss_entry_count; i++) 
	{
		how_much += source->read(mp4, &stss[i], 4, how_much, 0);
		stss[i]=t_ntohl(stss[i]);
	}
	return stss;
}

/*
获取音频的 采样率 + 通道数
*/
void get_samplerate_and_nch(int DecoderSpecificInfo, int* sample_rate, int* n_ch) 
{
	int sample_rate_index = (DecoderSpecificInfo>>7) & 15;
	*sample_rate = SamplingFrequencies[sample_rate_index];
	int n_ch_index = (DecoderSpecificInfo>>3) & 15;
	*n_ch = ChannelConfigurations[n_ch_index]; 
}



typedef enum _mp4_file_type
{
	MP4 = 1,		//普通mp4文件
	FMP4 			//fmp4文件
}mp4_file_type_e;




typedef struct _mp4_file_status_t
{
	enum _mp4_file_type 	MP4_file_type;		//文件类型（MP4 or fmp4）
	MP4_BOX* 				root ;				//文件的box根结点指针
	media_stats_t			track_info;			//文件中tracks信息
	MP4_BOX**				moov_traks;			//moov box下的traks数组指针
	unsigned int 			moof_num;			//文件中moof box的数量
	MP4_BOX** 				moof_array;			//文件中moof box节点数组指针
	unsigned int 			traf_num;			//文件中traf box的数量
	MP4_BOX** 				traf_array;			//文件中traf box节点数组指针
	frame_info_t 			frame_info;			//文件中音视频帧（数量）信息
}mp4_file_status_t;

/*
@mp4：MP4文件的句柄
@source：文件操作句柄（系列函数）
返回： 
	失败：-1
	成功：传入的m_stat_ptr==NULL，则返回该指针指向空间应分配内存大小值
		  传入的m_stat_ptr!=NULL，则返回output_buffer_size的值
*/
#if 1
int mp4_media_get_stats(void* context, file_handle_t* mp4, file_source_t* source, mp4_file_status_t* m_stat_ptr) 
{
	if(NULL == mp4 || NULL == source || NULL == m_stat_ptr)
	{
		ERROR_LOG("Illegal parameter");
		return -1;
	}
	



	//创建MP4文件box的信息描述节点链表
	m_stat_ptr->root = mp4_looking(context, mp4,source);
	if(NULL == m_stat_ptr->root)
	{
		ERROR_LOG("call  mp4_looking failed!\n ");
		return -1;
	}	

	/*---区分mp4文件类别(是普通MP4文件还是fmp4文件)-*/
	MP4_BOX* moof = find_box(root, "moof");
	if(NULL == moof)
	{
		m_stat_ptr->MP4_file_type = MP4;
	}
	else
	{
		m_stat_ptr->MP4_file_type = FMP4;
	}
	
	
	//获取MP4源文件中的trak数量（音视频轨道是否都有还是只有一个）
	m_stat_ptr->track_info.n_tracks = get_count_of_traks(context, mp4, source, m_stat_ptr->root, m_stat_ptr->moov_traks);
	if(m_stat_ptr->track_info.n_tracks < 0)
	{
		ERROR_LOG("call get_count_of_traks failed!\n");
		return -1;
	}
	
	DEBUG_LOG("track num = %d\n",m_stat_ptr->track_info.n_tracks);
	
	
	/*---计算 及分配 video/audio 帧数据所需要分配缓存大小--------------------------------------------------
		fmp4文件和普通的MP4文件frames(samples)的计算方式不一样
		MP4：在trak下的stsz box 里边就能够获取samples总数
		fmp4:需要将每个moof-->traf-->trun下的 Sample count字段求和才能计算出总数
	------------------------------------------------------------------------------------------------------*/
	unsigned int video_buf_size = 0;
	unsigned int audio_buf_size = 0;
	//MediaStatsT=sizeof(media_stats_t)+sizeof(track_t)*n_tracks; 无该部分

	int i,j;
	if(MP4 == m_stat_ptr->MP4_file_type)//普通MP4文件
	{
		for (i=0; i<m_stat_ptr->track_info.n_tracks ; i++) 
		{
			if(handlerType(mp4, source, find_box(m_stat_ptr->moov_traks[i],"hdlr"),"soun"))
				audio_buf_size += ((sizeof(float)+sizeof(int))*get_nframes(context, mp4, source, m_stat_ptr->moov_traks[i]));//sizeof(float) 代表的是 pts数组
			if(handlerType(mp4, source, find_box(m_stat_ptr->moov_traks[i],"hdlr"),"vide"))
				video_buf_size += (2*(sizeof(float)+sizeof(int))*get_nframes(context, mp4, source, m_stat_ptr->moov_traks[i]));
		}
				
	}
	else if(FMP4 == m_stat_ptr->MP4_file_type)//FMP4文件
	{
		//1.找出所有moof box节点，构成moof节点数组	
		m_stat_ptr->moof_num = find_box_one_level(context,m_stat_ptr->root,"moof",&m_stat_ptr->moof_array);
		DEBUG_LOG("moof_num = %d\n",m_stat_ptr->moof_num);
		
		//2.遍历每一个moof节点，找出里边的traf节点，构成traf指针数组
		int track_id = -1;
		int sample_count = 0;
		for (i = 0 ; i < m_stat_ptr->moof_num;i++)
		{
			m_stat_ptr->traf_num = find_box_one_level(context,m_stat_ptr->moof_array[i],"traf",&m_stat_ptr->traf_array);
			DEBUG_LOG("moof_array[%d] --> traf_num = %d\n",i,m_stat_ptr->traf_num);
			if(m_stat_ptr->traf_num < 0)
			{
				ERROR_LOG("find_box_one_level error !\n");
				return -1;
			}
			
			//3.循环每一个traf box节点，按照video/audio ,读取每个moof-->traf-->trun下的 Sample count字段求和 ,统计video/audio帧总数
			for(j = 0 ; j < m_stat_ptr->traf_num ; j++)
			{
				//先解析tfhd确定当前traf的trak_id
				MP4_BOX* tfhd =  find_box(m_stat_ptr->traf_array[j],"tfhd");
				if(NULL == tfhd)
				{
					ERROR_LOG("find_box failed!\n");
					return -1;
				}
				else
				{
					source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
					track_id = t_ntohl(track_id);
					
					DEBUG_LOG("track_id = %d\n",track_id);
					
					//解析trun下的 Sample count字段
					MP4_BOX* trun =  find_box(m_stat_ptr->traf_array[j],"trun");
					if(NULL == trun)
					{
						ERROR_LOG("find_box failed!\n");
						return -1;
					}
					
					source->read(mp4,&sample_count,4,trun->box_first_byte+12,0);
					sample_count = t_ntohl(sample_count);
	
					if(track_id == get_video_trak_id())//video
					{
						m_stat_ptr->frame_info.video_frames += sample_count;
						video_buf_size +=(2*(sizeof(float)+sizeof(int))*sample_count);
						
					}
					else if(track_id == get_audio_trak_id())//audio
					{
						m_stat_ptr->frame_info.audio_frames += sample_count;
						audio_buf_size += ((sizeof(float)+sizeof(int))*sample_count);
					}
					else
					{
						ERROR_LOG("unknown track_id !\n");
						return -1;
					}
						
				}

			}
			
		}
		
		DEBUG_LOG("video_frames = %d\n",m_stat_ptr->frame_info.video_frames);
		DEBUG_LOG("audio_frames = %d\n",m_stat_ptr->frame_info.audio_frames);
		
	}
	else
	{
		ERROR_LOG("<error!> unknow file type!\n");
		return -1;
	}
	
	//分配video/audio缓存空间
	m_stat_ptr->frame_info.video_buf = (char*)HLS_MALLOC(context, video_buf_size);
	if(NULL == m_stat_ptr->frame_info.video_buf)
	{
		ERROR_LOG("malloc failed!\n");
		return -1;
	}
	memset(m_stat_ptr->frame_info.video_buf,0,video_buf_size);

	m_stat_ptr->frame_info.audio_buf = (char*)HLS_MALLOC(context, audio_buf_size);
	if(NULL == m_stat_ptr->frame_info.audio_buf)
	{
		ERROR_LOG("malloc failed!\n");
		return -1;
	}
	memset(m_stat_ptr->frame_info.audio_buf,0,audio_buf_size);
	
	
	/*------------------------------------------------------------------------------------------------------*/


	
	/******如果传入的指针不为空（外部已经分配好内存），则直接进行trak信息填充**********/
	/*----增加初始化 video/audio 帧（sample）大小信息的逻辑----------------------*/
		if(init_sample_size_array(context,mp4,source,m_stat_ptr->root,m_stat_ptr) < 0)
		{
			ERROR_LOG("init_sample_size_offset_array failed !\n");
			return -1;
		}

		if(init_sample_offset_array(context,mp4,source,m_stat_ptr->root,m_stat_ptr) < 0)
		{
			ERROR_LOG("init_sample_offset_array failed !\n");
			return -1;
		}
		DEBUG_LOG("into position A , n_tracks = %d\n",n_tracks);
	/*---------------------------------------------------------------------------*/
		
	m_stat_ptr->n_tracks = n_tracks;
	for (int i=0; i<n_tracks && i<2; i++) //最大不能超过 2 条轨道
	{
		
		//对trak的起始位置（m_stat_ptr->track[i]）进行初始化
		//--????????????????????????????????????????????????????????????????????????????
		if (i==0) //video
		{
			m_stat_ptr->track[i]=(track_t*)((char*)m_stat_ptr + sizeof(media_stats_t));
		}
		else //i = 1 audio
		{
			if(handlerType(mp4, source, find_box(moov_traks[i-1],"hdlr"),"soun"))
				m_stat_ptr->track[i]=(track_t*)((char*)m_stat_ptr->track[i-1]+sizeof(track_t)+((sizeof(float)+sizeof(int))*track[i-1].n_frames));
			if(handlerType(mp4, source, find_box(moov_traks[i-1],"hdlr"),"vide"))
				m_stat_ptr->track[i]=(track_t*)((char*)m_stat_ptr->track[i-1]+sizeof(track_t)+((2*sizeof(float)+sizeof(int))*track[i-1].n_frames));
		}
		//--??????????????????????????????????????????????????????????????????????????????
		
		//对trak的详细信息进行解析填充。
		track = m_stat_ptr->track[i];
		track->codec = get_codec(mp4, source, moov_traks[i]);
		DEBUG_LOG("track->codec = %x\n",track->codec);
		
		if(MP4 == get_mp4_file_type())//普通MP4文件
		{
			track->n_frames=get_nframes(context, mp4, source, moov_traks[i]);
		}	
		else if(FMP4 == get_mp4_file_type())
		{
		DEBUG_LOG("into position C\n");
			if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun"))
				track->n_frames = frame_info.audio_frames;
			else if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide"))
				track->n_frames = frame_info.video_frames;
			else
			{
				ERROR_LOG("unknown trak!\n"); 
				continue;
			}
				
		}
			
		DEBUG_LOG("into position B\n"); 
		track->bitrate=0;
		
		//音频轨道的相关参数解析
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) 
		{
			DEBUG_LOG("into position D\n"); 
			track->pts = (float*)((char*)track+sizeof(track_t));//将pts数组（每个音频帧的pts构成的数组）放到了整个track描述信息的后边
			track->dts = track->pts;//音频帧的dts = pts
			track->flags = (int *)((char*)track->pts + sizeof(float) * track->n_frames);//flags初始位置指向 pts 末尾 
			for(int z=0; z < track->n_frames; z++)	//音频帧每一帧都为关键帧
				track->flags[z]=1;

			int ret = 0;
			ret = get_pts(context, mp4, source, moov_traks[i],track->n_frames,get_DecoderSpecificInfo(mp4, source, find_box(moov_traks[i],"stsd")),track->pts);
			if(ret < 0)
			{
				ERROR_LOG("get_pts failed !\n");
				return -1;
			}
			get_samplerate_and_nch(get_DecoderSpecificInfo(mp4,source,find_box(moov_traks[i],"stsd")), &track->sample_rate, &track->n_ch);
			track->sample_size=16;
			//PTS Test PRINTF
			/*
			for(int vvv=0; vvv<track->n_frames; vvv++)
				printf("\nSOUND DTS = %f",track->dts[vvv]);
			*/


		}
		
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) 
		{
			DEBUG_LOG("into position E\n"); 
			track->pts=(float*)((char*)track + sizeof(track_t));//pts 实际数据放到 trak 参数结构的后边
			track->dts=(float*)((char*)track->pts + sizeof(float)*track->n_frames);//dts 实际数据放到 pts 的后边
			track->flags=(int*)((char*)track->dts + sizeof(float)*track->n_frames); // flags 实际数据放到 dts 后边
			get_video_dts(context, mp4, source, root , moov_traks[i], track->n_frames, track->dts);
			DEBUG_LOG("test free 001!\n");
			get_video_pts(context, mp4, source, moov_traks[i], track->n_frames, track->pts, track->dts);
			DEBUG_LOG("test free 002!\n");
			for(int z=0; z<track->n_frames; z++)
				track->flags[z]=0;
			int stss_entry_count=0;
			int* stss=NULL;
			
			/*---stss确定media中的关键帧----------------------------------------------------------------*/
			if(MP4 == get_mp4_file_type())//普通MP4文件
			{
				stss=get_stss(context, mp4, source, moov_traks[i], &stss_entry_count);
				for(int i=0; i<stss_entry_count; i++)
					track->flags[stss[i]-1]=1;
			}
			else if( FMP4 == get_mp4_file_type())//不存在stss box（一般是fmp4文件）
			{
				//do nothing ？？？？？？？？？暂时无法确定fmp4文件中的关键帧
				for(int i=0; i<track->n_frames ; i++)
				{
					if(i % 15 == 0)
					track->flags[i] = 1;	
				}
					//track->flags[0] = 1;	//默认stss_entry_count = 1;stss[0] = 1
			}
			else
			{
				DEBUG_LOG("unknown file type!\n");
				return -1;
			}

			track->n_ch=0;
			track->sample_rate=0;
			track->sample_size=0;


			DEBUG_LOG("test free 003!\n");
			//PTS Test PRINTF
			/*for(int vvv=0; vvv<track->n_frames; vvv++)
				printf("\nVIDEO DTS = %f",track->dts[vvv]);
			*/
			if(MP4 == get_mp4_file_type())
				HLS_FREE(stss);
			
			DEBUG_LOG("test free 004!\n");
			
		}
		
		track->repeat_for_every_segment=0;
		track->data_start_offset=0;
		track->type=0;
		
	}

	DEBUG_LOG("into position F\n"); 
	HLS_FREE(moov_traks);
	i_want_to_break_free(root);
	return output_buffer_size;
}

#else
int mp4_media_get_stats(void* context, file_handle_t* mp4, file_source_t* source, media_stats_t* m_stat_ptr, int output_buffer_size) 
{
	//创建MP4文件box的信息描述节点链表
	MP4_BOX* root=mp4_looking(context, mp4,source);
	int MediaStatsT=0;
	MP4_BOX** moov_traks=NULL;
	

	/*---区分mp4文件类别(是普通MP4文件还是fmp4文件)------------*/
	MP4_BOX*moof = NULL;
	moof = find_box(root, "moof");
	if(NULL == moof)
	{
		set_mp4_file_type(MP4);
	}
	else
	{
		set_mp4_file_type(FMP4);
	}
	/*--------------------------------------------------------*/
	
	//获取MP4源文件中的trak数量（音视频轨道是否都有还是只有一个）
	int n_tracks=get_count_of_traks(context, mp4, source, root, &moov_traks);
	DEBUG_LOG("track num = %d\n",n_tracks);
	track_t* track = NULL;

	if (m_stat_ptr==NULL) //如果传入的指针为空（外部不知道应该分配多大的内存），则计算该指针所指向内存理应大小，返回该值
	{
		MediaStatsT=sizeof(media_stats_t)+sizeof(track_t)*n_tracks;

		/*
			fmp4文件和普通的MP4文件frames(samples)的计算方式不一样
			MP4：在trak下的stsz box 里边就能够获取samples总数
			fmp4:需要将每个moof-->traf-->trun下的 Sample count字段求和才能计算出总数
		*/
		int i,j;
		if(MP4 == get_mp4_file_type())//普通MP4文件
		{
			for (i=0; i<n_tracks; i++) 
			{
				if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun"))
					MediaStatsT+=((sizeof(float)+sizeof(int))*get_nframes(context, mp4, source, moov_traks[i]));//sizeof(float) 代表的是 pts数组
				if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide"))
					MediaStatsT+=(2*(sizeof(float)+sizeof(int))*get_nframes(context, mp4, source, moov_traks[i]));
			}
			
					
		}
		else if(FMP4 == get_mp4_file_type())//FMP4文件
		{
	
			//1.找出所有moof box节点，构成moof节点数组
			MP4_BOX** moof_array = NULL;
			MP4_BOX** traf_array = NULL;
			
			int moof_num = find_box_one_level(context,root,"moof",&moof_array);
			DEBUG_LOG("moof_num = %d\n",moof_num);
			
			/*for (i = 0 ; i < moof_num;i++)
			{
				printf("moof_array[i] = %x\n",moof_array[i]);
			}
			*/

			//2.遍历每一个moof节点，找出里边的traf节点，构成traf指针数组
			for (i = 0 ; i < moof_num;i++)
			{
				
				int traf_num = find_box_one_level(context,moof_array[i],"traf",&traf_array);
				DEBUG_LOG("moof_array[%d] --> traf_num = %d\n",i,traf_num);
				if(traf_num < 0)
				{
					ERROR_LOG("find_box_one_level error !\n");
					HLS_FREE(traf_array);
					HLS_FREE(moof_array);
					return -1;
				}
				
				//3.循环每一个traf box节点，按照video/audio ,读取每个moof-->traf-->trun下的 Sample count字段求和 = frames
				for(j = 0 ; j < traf_num ; j++)
				{
					//先解析tfhd确定当前traf的trak_id
					MP4_BOX* tfhd =  find_box(traf_array[j],"tfhd");
					if(NULL == tfhd)
					{
						ERROR_LOG("find_box failed!\n");
						continue;
					}
					else
					{
						int track_id;
						source->read(mp4,&track_id,4,tfhd->box_first_byte+12,0);
						track_id = t_ntohl(track_id);
						//DEBUG_LOG("track_id = %d\n",track_id);
						
						//解析trun下的 Sample count字段
						MP4_BOX* trun =  find_box(traf_array[j],"trun");
						if(NULL == trun)
						{
							ERROR_LOG("find_box failed!\n");
							continue;
						}
						int sample_count = 0;
						source->read(mp4,&sample_count,4,trun->box_first_byte+12,0);
						sample_count = t_ntohl(sample_count);
		
						if(track_id == get_video_trak_id())//video
						{
							if(frame_info.inited !=1 )
								frame_info.video_frames += sample_count;
							MediaStatsT +=(2*(sizeof(float)+sizeof(int))*sample_count);
							
						}
						else if(track_id == get_audio_trak_id())//audio
						{
							if(frame_info.inited !=1 )
								frame_info.audio_frames += sample_count;
							MediaStatsT+=((sizeof(float)+sizeof(int))*sample_count);
						}
						else
						{
							ERROR_LOG("unknown track_id !\n");
							continue;
						}
							
					
					}

				}

				
			}

			frame_info.inited = 1;
			
			DEBUG_LOG("video_frames = %d\n",frame_info.video_frames);
			DEBUG_LOG("audio_frames = %d\n",frame_info.audio_frames);
			
			HLS_FREE(traf_array);
			HLS_FREE(moof_array);
		
		}
		else
		{
			ERROR_LOG("<error!> unknow file type!\n");
			return -1;
		}
		
		HLS_FREE(moov_traks);
		return MediaStatsT;
	}
	
	/******如果传入的指针不为空（外部已经分配好内存），则直接进行trak信息填充**********/
	/*----增加初始化 video/audio 帧（sample）大小信息的逻辑----------------------*/
		if(init_sample_size_array(context,mp4,source,root) < 0)
		{
			ERROR_LOG("init_sample_size_offset_array failed !\n");
			return -1;
		}

		if(init_sample_offset_array(context,mp4,source,root) < 0)
		{
			ERROR_LOG("init_sample_offset_array failed !\n");
			return -1;
		}
		DEBUG_LOG("into position A , n_tracks = %d\n",n_tracks);
	/*---------------------------------------------------------------------------*/
		
	m_stat_ptr->n_tracks = n_tracks;
	for (int i=0; i<n_tracks && i<2; i++) //最大不能超过 2 条轨道
	{
		
		//对trak的起始位置（m_stat_ptr->track[i]）进行初始化
		//--????????????????????????????????????????????????????????????????????????????
		if (i==0) //video
		{
			m_stat_ptr->track[i]=(track_t*)((char*)m_stat_ptr + sizeof(media_stats_t));
		}
		else //i = 1 audio
		{
			if(handlerType(mp4, source, find_box(moov_traks[i-1],"hdlr"),"soun"))
				m_stat_ptr->track[i]=(track_t*)((char*)m_stat_ptr->track[i-1]+sizeof(track_t)+((sizeof(float)+sizeof(int))*track[i-1].n_frames));
			if(handlerType(mp4, source, find_box(moov_traks[i-1],"hdlr"),"vide"))
				m_stat_ptr->track[i]=(track_t*)((char*)m_stat_ptr->track[i-1]+sizeof(track_t)+((2*sizeof(float)+sizeof(int))*track[i-1].n_frames));
		}
		//--??????????????????????????????????????????????????????????????????????????????
		
		//对trak的详细信息进行解析填充。
		track = m_stat_ptr->track[i];
		track->codec = get_codec(mp4, source, moov_traks[i]);
		DEBUG_LOG("track->codec = %x\n",track->codec);
		
		if(MP4 == get_mp4_file_type())//普通MP4文件
		{
			track->n_frames=get_nframes(context, mp4, source, moov_traks[i]);
		}	
		else if(FMP4 == get_mp4_file_type())
		{
		DEBUG_LOG("into position C\n");
			if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun"))
				track->n_frames = frame_info.audio_frames;
			else if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide"))
				track->n_frames = frame_info.video_frames;
			else
			{
				ERROR_LOG("unknown trak!\n"); 
				continue;
			}
				
		}
			
		DEBUG_LOG("into position B\n");	
		track->bitrate=0;
		
		//音频轨道的相关参数解析
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) 
		{
			DEBUG_LOG("into position D\n");	
			track->pts = (float*)((char*)track+sizeof(track_t));//将pts数组（每个音频帧的pts构成的数组）放到了整个track描述信息的后边
			track->dts = track->pts;//音频帧的dts = pts
			track->flags = (int *)((char*)track->pts + sizeof(float) * track->n_frames);//flags初始位置指向 pts 末尾 
			for(int z=0; z < track->n_frames; z++)	//音频帧每一帧都为关键帧
				track->flags[z]=1;

			int ret = 0;
			ret = get_pts(context, mp4, source, moov_traks[i],track->n_frames,get_DecoderSpecificInfo(mp4, source, find_box(moov_traks[i],"stsd")),track->pts);
			if(ret < 0)
			{
				ERROR_LOG("get_pts failed !\n");
				return -1;
			}
			get_samplerate_and_nch(get_DecoderSpecificInfo(mp4,source,find_box(moov_traks[i],"stsd")), &track->sample_rate, &track->n_ch);
			track->sample_size=16;
			//PTS Test PRINTF
			/*
			for(int vvv=0; vvv<track->n_frames; vvv++)
				printf("\nSOUND DTS = %f",track->dts[vvv]);
			*/


		}
		
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) 
		{
			DEBUG_LOG("into position E\n");	
			track->pts=(float*)((char*)track + sizeof(track_t));//pts 实际数据放到 trak 参数结构的后边
			track->dts=(float*)((char*)track->pts + sizeof(float)*track->n_frames);//dts 实际数据放到 pts 的后边
			track->flags=(int*)((char*)track->dts + sizeof(float)*track->n_frames); // flags 实际数据放到 dts 后边
			get_video_dts(context, mp4, source, root , moov_traks[i], track->n_frames, track->dts);
			DEBUG_LOG("test free 001!\n");
			get_video_pts(context, mp4, source, moov_traks[i], track->n_frames, track->pts, track->dts);
			DEBUG_LOG("test free 002!\n");
			for(int z=0; z<track->n_frames; z++)
				track->flags[z]=0;
			int stss_entry_count=0;
			int* stss=NULL;
			
			/*---stss确定media中的关键帧----------------------------------------------------------------*/
			if(MP4 == get_mp4_file_type())//普通MP4文件
			{
				stss=get_stss(context, mp4, source, moov_traks[i], &stss_entry_count);
				for(int i=0; i<stss_entry_count; i++)
					track->flags[stss[i]-1]=1;
			}
			else if( FMP4 == get_mp4_file_type())//不存在stss box（一般是fmp4文件）
			{
				//do nothing ？？？？？？？？？暂时无法确定fmp4文件中的关键帧
				for(int i=0; i<track->n_frames ; i++)
				{
					if(i % 15 == 0)
					track->flags[i] = 1;	
				}
					//track->flags[0] = 1;  //默认stss_entry_count = 1;stss[0] = 1
			}
			else
			{
				DEBUG_LOG("unknown file type!\n");
				return -1;
			}

			track->n_ch=0;
			track->sample_rate=0;
			track->sample_size=0;


			DEBUG_LOG("test free 003!\n");
			//PTS Test PRINTF
			/*for(int vvv=0; vvv<track->n_frames; vvv++)
				printf("\nVIDEO DTS = %f",track->dts[vvv]);
			*/
			if(MP4 == get_mp4_file_type())
				HLS_FREE(stss);
			
			DEBUG_LOG("test free 004!\n");
			
		}
		
		track->repeat_for_every_segment=0;
		track->data_start_offset=0;
		track->type=0;
		
	}

	DEBUG_LOG("into position F\n");	
	HLS_FREE(moov_traks);
	i_want_to_break_free(root);
	return output_buffer_size;
}


#endif
/*
功能：（只处理一个帧）？？？？？？？？？？？？？？？？？？？？？？？？？？？？
参数：
	@ptr：当前帧的开始位置
	@NALlensize：avcC box 的NALUnitLengthSize字段
	@frame_size: 帧的大小信息的指针（理解为数组的指针）
	@frame_offset：帧的偏移量信息的指针（理解为数组的指针）（相较于 buffer开始位置 ）
	@index: 当前帧的下标（相较于 buffer 中的第一个帧 ，下标从 0 开始）
返回： 
	成功：0 
	失败：-1

*/
int  convert_to_annexb(void* context, char* ptr, int NALlensize, int* frame_size, int* frame_offset, int index) 
{

	DEBUG_LOG("into convert_to_annexb!\n");
	char annexb[4]={0,0,0,1};
	int nal_size=0;
	//申请 “当前帧大小 + 保留大小” 的内存空间
	char* temp_buffer=(char*) HLS_MALLOC (context, sizeof(char)*(frame_size[index] + RESERVED_SPACE));
	if (temp_buffer==NULL) 
	{
		ERROR_LOG("Couldn't allocate memory for temp_buffer !\n");
		return -1;
	}
	
	int j=0;
	char nal_type = 0;
	int i = 0;//在temp_buffer中偏移的字节数
	
	for(i = 0 ; i < frame_size[index]; ) //frame_size[index]:当前帧的大小
	{
		//解析NALU的大小和类型
		memcpy((char*)&nal_size+(4-NALlensize), ptr  + i, NALlensize);
		nal_size = t_ntohl(nal_size);
		memcpy(&nal_type, ptr + i + NALlensize, 1);
		nal_type &= 31;  // 31 : 0x1F : 0001 1111 :即保留低5位
		
 		if(nal_type==7 || nal_type==8 || nal_type==9) //SPS PPS 。。 即是关键帧
		{
			i += nal_size + NALlensize;
			nal_size = 0;
		}
		else 		//非关键帧，直接拷贝 头部 + 数据，无SPS PPS部分
		{
			memcpy(temp_buffer + j , annexb , 4);  //H264帧公共头
			memcpy(temp_buffer + j + 4 , ptr + i + NALlensize , nal_size);
			i += nal_size + NALlensize;
			j += 4 + nal_size;
			nal_size = 0;
		}
	}
	DEBUG_LOG("into convert_to_annexb 01!\n");
	frame_size[index] = j + 6;
//	if(index!=0)
//		frame_offset[index+1]=frame_offset[index]+frame_size[index];
	memcpy(ptr,temp_buffer, frame_size[index]-6);
	HLS_FREE(temp_buffer);
	DEBUG_LOG("out convert_to_annexb!\n");
	return 0;
}

void print_V_size_array(int NO)
{
	DEBUG_LOG("%d sample_size.V_size_array[0] = %d sample_size.V_size_array[1] = %d sample_size.V_size_array[2] = %d\n",
								NO,sample_size.V_size_array[0],sample_size.V_size_array[1],sample_size.V_size_array[2]);
}

/* 
功能：将音视频trak下的帧数据都按顺序读取到内存中，
	  H264帧加上 AUD头（兼容ios系统） + 公共头 + （关键帧加上SPS PPS）+ 帧数据；
	  AAC数据帧 加上 ADTS帧头
参数：
	@piece:当前TS片的序号
	@output_buffer：帧数据的输出buf
	@output_buffer_size：buf的大小
返回：
	失败：-1  
	成功：
		  传入output_buffer= NULL，则返回应分配的内存空间大小
		  传入output_buffer！= NULL，则返回。。。
*/
int mp4_media_get_data(void* context, file_handle_t* mp4, file_source_t* source, media_stats_t* stats, int piece, media_data_t* output_buffer, int output_buffer_size) 
{

	MP4_BOX* root = mp4_looking(context, mp4,source);
	int MediaDataT=0; //最终要返回的内存空间大小

	MP4_BOX** moov_traks=NULL;

	int n_tracks = get_count_of_traks(context, mp4, source, root, &moov_traks);
	
	int lenght = get_segment_length(); // recommended_lenght for test （我们设置的TS文件切片时长）

	/*----output_buffer == NULL-------计算  应该分配的内存大小--------------------------------------------------------------------------------------*/
	/*=============总 文件缓冲buf组成结构========================================================================
		
		Audio帧：7字节adts + 数据帧

		Video帧：帧数据 + 160字节的RESERVED_SPACE
				 额外保存一份 AUD + SPS + PPS
		
		n_frames：代表的是整个文件 video/audio 的帧数。
		-----总头------|---------video部分--------------------------------------------------------|------audio部分--------
		| media_data_t | track_data_t(video) | size * n_frames | offset * n_frames | frames data  | ...same as video ... |
		------------------------------------------------------------------------------------------------------------------
	============================================================================================================*/
	if(output_buffer==NULL) //统计
	{
		MediaDataT = sizeof(media_data_t)+sizeof(track_data_t)*n_tracks;
		DEBUG_LOG("n_tracks(%d) MediaDataT(%d)\n",n_tracks,MediaDataT);
		for (int i=0; i<n_tracks; i++) 
		{
			int tmp_sf=0;	//当前序号的TS片，帧区间开始位置（下标）
			int tmp_ef=0;	//当前序号的TS片，帧区间结束位置（下标）
			int sample_count=0;
			int* stsz_data = NULL; //sample(帧)的大小信息数组指针
			if(MP4 == get_mp4_file_type())
			{
				stsz_data=read_stsz(context, mp4, source, find_box(moov_traks[i],"stsz"), &sample_count);
			}
			else if(FMP4 == get_mp4_file_type())//fmp4文件没有stsz box
			{
				if(sample_size.init_done)
				{
					if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) 
					{
						sample_count = sample_size.A_sample_num;
						stsz_data = HLS_MALLOC(context, sample_count * sizeof(int));
						if(NULL == stsz_data)
						{	
							HLS_FREE(moov_traks);
							i_want_to_break_free(root);
							ERROR_LOG("malloc faield !\n");
							return -1;
						}
						memcpy(stsz_data,sample_size.A_size_array,sample_count * sizeof(int));
						DEBUG_LOG("soun stsz_data[0] = %d stsz_data[1] = %d\n",stsz_data[0],stsz_data[1]);

					}
						
					if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) 
					{
						sample_count = sample_size.V_sample_num;
						stsz_data = HLS_MALLOC(context, sample_count * sizeof(int));
						if(NULL == stsz_data)
						{	
							HLS_FREE(moov_traks);
							i_want_to_break_free(root);
							ERROR_LOG("malloc faield !\n");
							return -1;
						}
						memcpy(stsz_data,sample_size.V_size_array,sample_count * sizeof(int));
						
						DEBUG_LOG("vide stsz_data[0] = %d stsz_data[1] = %d\n",stsz_data[0],stsz_data[1]);

					}
						
				}
				else
				{
					ERROR_LOG("sample_size not init !\n");
					return -1;
				}
			}

			
			int tmp_buffer_size=0;  //当前TS文件对应的帧的总大小
			//当前TS切片文件对应数据帧的总数量
			DEBUG_LOG("into get_frames_in_piece...\n");
			int temp_nframes = get_frames_in_piece(stats, piece, i, &tmp_sf, &tmp_ef, lenght);
			DEBUG_LOG("out get_frames_in_piece...\n");
			
			int j = 0;
			if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) 
			{
				DEBUG_LOG("soun : tmp_sf(%d)---tmp_ef(%d)  total audio frame(%d)\n",tmp_sf,tmp_ef,sample_offset.A_sample_num);
				for(j=tmp_sf; j<tmp_ef; j++)
				{
					tmp_buffer_size += stsz_data[j]+7; // 7 - adts header size
					DEBUG_LOG("stsz_data[%d] = %d  tmp_buffer_size(%d) \n",j,stsz_data[j],tmp_buffer_size);

				}
					
			}
			if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) 
			{
				DEBUG_LOG("vide : tmp_sf(%d)---tmp_ef(%d)   total video frame(%d)\n",tmp_sf,tmp_ef,sample_offset.V_sample_num);
				for(j = tmp_sf ; j < tmp_ef ; j++)
				{
					tmp_buffer_size += stsz_data[j]; 
					DEBUG_LOG("stsz_data[%d] = %d  tmp_buffer_size(%d) \n",j,stsz_data[j],tmp_buffer_size);

				}
					
				char* temp_AVCDecInfo=NULL;
				int avc_decinfo_size=0; //SPS + PPS的数据大小
				temp_AVCDecInfo = get_AVCDecoderSpecificInfo(context, mp4, source, find_box(moov_traks[i],"stsd"), &avc_decinfo_size, NULL);
				DEBUG_LOG("avc_decinfo_size(%d) temp_nframes(%d)\n",avc_decinfo_size,temp_nframes);
				tmp_buffer_size += avc_decinfo_size + RESERVED_SPACE * temp_nframes;
				HLS_FREE(temp_AVCDecInfo);
			}
			//size:当前TS文件对应的video/audio帧的大小信息
			//offset:当前TS文件对应的video/audio帧的偏移信息
			DEBUG_LOG("MediaDataT(%d) temp_nframes(%d) tmp_buffer_size(%d)\n",MediaDataT,temp_nframes,tmp_buffer_size);
			MediaDataT += (sizeof(int/*int* size*/) + sizeof(int/*int* offset*/))*temp_nframes + sizeof(char/*char* buffer*/)*tmp_buffer_size;

			HLS_FREE(stsz_data);

		}
		HLS_FREE(moov_traks);

		DEBUG_LOG("into position I1 MediaDataT = %d\n",MediaDataT);	
		
		return MediaDataT;
	}
	
	/*---output_buffer不为空，正常业务逻辑部分---------------------------------------------------------------------------------------------------*/
		
	media_data_t* data = NULL;
	track_data_t* cur_track_data = NULL;
	output_buffer->n_tracks = n_tracks;

	
	for(int i=0; i<n_tracks; i++) 
	{
		if (i==0)  //video
		{	//将track_data[0]指向的trak信息数据media_data_t结构后
			output_buffer->track_data[i] = (track_data_t*)((char*)output_buffer+sizeof(media_data_t));
		}
		else 	//audio
		{
			//计算track_data[1]指向的起始位置
			
			output_buffer->track_data[i] = (track_data_t*)((char*)output_buffer->track_data[i-1] + sizeof(track_data_t) + sizeof(char) * output_buffer->track_data[i-1]->buffer_size + (sizeof(int) * output_buffer->track_data[i-1]->n_frames) * 2);//*2是因为每个帧都有 size 和 offset都是四字节，两倍关系

			unsigned int offset  = (char*)output_buffer->track_data[i] - (char*)output_buffer;
			if(offset >= output_buffer_size)
			{
				ERROR_LOG("buffer overflow!\n");
				i_want_to_break_free(root);
				HLS_FREE(moov_traks);
				return -1;
			}
			DEBUG_LOG("audio output_buffer->track_data[i] = %p  offset(%d)\n",&output_buffer->track_data[i]->first_frame,offset);
		}
		
		cur_track_data = output_buffer->track_data[i];
		int tmp_ef=0; //当前TS分片对应帧区间的结束位置（下标）
		int tmp_sample_count=0; //stsz 的样本个数（Sample count字段）
		DEBUG_LOG("2944 into get_frames_in_piece... track = %d &track_data->first_frame = %p\n",i,&cur_track_data->first_frame);
		cur_track_data->first_frame = 0; 
		DEBUG_LOG("back---- track_data->first_frame = 0\n");
		cur_track_data->n_frames = get_frames_in_piece(stats, piece, i, &cur_track_data->first_frame, &tmp_ef, lenght);
		DEBUG_LOG("2944 out get_frames_in_piece...\n");
		
#if 1		
		int* stsz_data = NULL; //sample(帧)的大小信息数组指针
			
		if(MP4 == get_mp4_file_type())
		{
			stsz_data =	read_stsz(context, mp4, source, find_box(moov_traks[i],"stsz"), &tmp_sample_count);
		}
		else if(FMP4 == get_mp4_file_type())
		{
			if(sample_size.init_done)
			{
				DEBUG_LOG("into position I3 \n");
				if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) 
				{
					tmp_sample_count = frame_info.audio_frames;
					
					stsz_data = (int *)HLS_MALLOC(context, tmp_sample_count*sizeof(int));
					if(NULL == stsz_data)
					{
						ERROR_LOG("malloc failed !\n");
						HLS_FREE(moov_traks);
						i_want_to_break_free(root);
						return -1;
					}
					memcpy(stsz_data,sample_size.A_size_array,tmp_sample_count*sizeof(int));
					
					DEBUG_LOG("frame_info.audio_frames = %d\n",frame_info.audio_frames);
					DEBUG_LOG("audio : stsz_data[0]= %d stsz_data[1]= %d stsz_data[2]= %d\n",stsz_data[0],stsz_data[1],stsz_data[2]);
				}
					
				if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) 
				{
					tmp_sample_count = frame_info.video_frames;
					stsz_data = (int*)HLS_MALLOC(context, tmp_sample_count*sizeof(int));
					if(NULL == stsz_data)
					{
						ERROR_LOG("malloc failed !\n");
						HLS_FREE(moov_traks);
						i_want_to_break_free(root);
						return -1;
					}
					memcpy(stsz_data,sample_size.V_size_array,tmp_sample_count*sizeof(int));
					
					DEBUG_LOG("frame_info.video_frames = %d\n",frame_info.video_frames);
					DEBUG_LOG("sample_size.V_size_array[0] = %d sample_size.V_size_array[1] = %d sample_size.V_size_array[2] = %d\n",
								sample_size.V_size_array[0],sample_size.V_size_array[1],sample_size.V_size_array[2]);
					DEBUG_LOG("video : stsz_data[0]= %d stsz_data[1]= %d stsz_data[2]= %d\n",stsz_data[0],stsz_data[1],stsz_data[2]);
				}
					
			}
			else
			{
				ERROR_LOG("sample_size not init !\n");
				return -1;
			}
			
		}
		else
		{
			ERROR_LOG("unknown file type !\n");
			return -1;
		}
		
		print_V_size_array(3);

		cur_track_data->buffer_size=0;
		int k=0;
		cur_track_data->size = (int *)((char*)cur_track_data + sizeof(track_data_t));
		cur_track_data->offset=(int *)((char*)cur_track_data->size + sizeof(int)*cur_track_data->n_frames);
		cur_track_data->buffer=(char*)cur_track_data->offset + sizeof(int)*cur_track_data->n_frames;
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) 
		{
			DEBUG_LOG("into position I4 \n");
			int j = 0;
			for(j=cur_track_data->first_frame; j<tmp_ef; j++) 
			{
				cur_track_data->size[k]		=stsz_data[j]+7;// 7 - adts header size
				cur_track_data->offset[k]	=cur_track_data->buffer_size;
				cur_track_data->buffer_size += cur_track_data->size[k];
				++k;
			}
		}
		
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) 
		{
			DEBUG_LOG("into position I5 \n");
			int j = 0;
			for(j = cur_track_data->first_frame ; j < tmp_ef; j++) 
			{
				if(k!=0)
				{
					cur_track_data->size[k]	=stsz_data[j];
				}
				else //（k == 0）第一帧，需要添加 SPS PPS 的大小
				{
					char* temp_AVCDecInfo=NULL;
					int avc_decinfo_size=0;
					temp_AVCDecInfo=get_AVCDecoderSpecificInfo(context, mp4, source, find_box(moov_traks[i],"stsd"), &avc_decinfo_size, NULL);
					if(NULL == temp_AVCDecInfo)
					{
						ERROR_LOG("get_AVCDecoderSpecificInfo error!\n");
						return -1;
					}
					HLS_FREE(temp_AVCDecInfo);
				

					cur_track_data->size[k]	= stsz_data[j] + avc_decinfo_size;//第一帧视频帧要加上 SPS + PPS的大小					
				}
				
				DEBUG_LOG("video track_data->size[%d] = %d\n",k,cur_track_data->size[k]);
				
				cur_track_data->offset[k] = cur_track_data->buffer_size;
				cur_track_data->buffer_size += cur_track_data->size[k];
				++k;
			}
			cur_track_data->buffer_size += RESERVED_SPACE;
		}
		
		cur_track_data->frames_written=0;
		cur_track_data->data_start_offset=0;
		cur_track_data->cc=0;

		
		MP4_BOX* find_stsd=(find_box(moov_traks[i], "stsd"));
		int* mp4_sample_offset; //描述在文件中 video 或者 audio trak 下的每一个 sample（帧）的偏移量（数组指针）
		mp4_sample_offset = (int*) HLS_MALLOC (context, sizeof(int)*tmp_sample_count);

		if (mp4_sample_offset == NULL) 
		{
			ERROR_LOG("Couldn't allocate memory for mp4_sample_offset malloc_size(%d)\n",tmp_sample_count);
			return -1;
		}
		DEBUG_LOG("into position I6 \n");
		
		//===================该部分区分普通MP4文件和fmp4文件=========================================
		if(MP4 == get_mp4_file_type())
		{
			MP4_BOX* find_stsc=(find_box(moov_traks[i], "stsc"));
			MP4_BOX* find_stco=(find_box(moov_traks[i], "stco"));
			//MP4_BOX* find_stsz=(find_box(moov_traks[i], "stsz"));

			int stsc_entry_count = 0;		//stsc 的入口个数(每个入口包含一个或多个chunk)
			int stco_entry_count = 0;		//stco 的入口数 （每个入口记录一个 chunk 的偏移）
			int stsz_entry_count = tmp_sample_count;//stsz 的sample（帧）个数（Sample count字段）

			sample_to_chunk** stsc_dat; //stsc box 的入口描述信息指针数组（的）指针 
			stsc_dat = read_stsc(context, mp4, source, find_stsc, &stsc_entry_count);

			int* stco_dat;		//每个chunk相对于文件头的偏移位置数组（的）指针
			stco_dat = read_stco(context, mp4, source, find_stco, &stco_entry_count);

			//int* stsz_dat;
			//stsz_dat = stsz_data;//read_stsz(mp4, source, find_stsz, &stsz_entry_count);
			
			/*------构造文件中当前 trak 帧的偏移量数组（相较于整个文件）---------------------------------------------------------------*/
			int mp4_sample_offset_counter=0;  //描述整个文件每一个sample（帧）的偏移量数组的下标
			int stc = 0;
			int chunk_offset = 0;//chunk的序号（在每个入口来说的）
			int b = 0;
			for (stc=0; stc < stsc_entry_count; stc++) 
			{	//循环该入口的每个chunk（下标是从1开始的）-------    -----（没有到最后一个入口）？（没到下一个入口就继续循环，到了就停）：（没到最后一个sample就继续循环，到了就停）-----------------
				for (chunk_offset=stsc_dat[stc]->first_chunk; (stc < (stsc_entry_count-1) ? (chunk_offset < stsc_dat[stc+1]->first_chunk) : (chunk_offset<=stco_entry_count)); chunk_offset++) 
				{
					//循环该chunk的每个sample
					for (b=0; b<stsc_dat[stc]->samples_per_chunk; b++) 
					{
						if(b==0) //该chunk的第一个sample
						{
							mp4_sample_offset[mp4_sample_offset_counter]=stco_dat[chunk_offset-1]; 
							mp4_sample_offset_counter++;
						}
						else //该chunk的其他sample
						{
							//为（从整个文件的samples来看）前一个的samlple的起始位置+前一个的samlple的大小
							mp4_sample_offset[mp4_sample_offset_counter]=mp4_sample_offset[mp4_sample_offset_counter-1]+stsz_data[mp4_sample_offset_counter-1];
							mp4_sample_offset_counter++;
						}
					}
				}
			}
			
			int zvc=0;
			for (zvc=0; zvc<stsc_entry_count; zvc++)
				HLS_FREE(stsc_dat[zvc]);
			HLS_FREE(stsc_dat);
			HLS_FREE(stco_dat);
		}
		else if(FMP4 == get_mp4_file_type())
		{
			DEBUG_LOG("into position I7 \n");
			
			if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun"))
			{
				DEBUG_LOG("into position I8 \n");
				if(sample_offset.A_sample_num != tmp_sample_count)
				{
					ERROR_LOG("sample_offset.A_sample_num(%d) != tmp_sample_count(%d)\n",sample_offset.A_sample_num,tmp_sample_count);
					return -1;
				}
				//mp4_sample_offset = sample_offset.A_offset_array;
				memcpy(mp4_sample_offset , sample_offset.A_offset_array , sizeof(int)*tmp_sample_count);
				DEBUG_LOG("mp4_sample_offset[0] = %d mp4_sample_offset[1] = %d mp4_sample_offset[2] = %d\n",
						mp4_sample_offset[0],mp4_sample_offset[1],mp4_sample_offset[2]);
			}
			
			if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide"))
			{
				DEBUG_LOG("into position I9 \n");
				
				if(sample_offset.V_sample_num != tmp_sample_count)
				{
					ERROR_LOG("sample_offset.V_sample_num(%d) != tmp_sample_count(%d)\n",sample_offset.V_sample_num,tmp_sample_count);
					return -1;
				}
				//mp4_sample_offset = sample_offset.V_offset_array;
				memcpy(mp4_sample_offset , sample_offset.V_offset_array , sizeof(int)*tmp_sample_count);
				DEBUG_LOG("mp4_sample_offset[0] = %d mp4_sample_offset[1] = %d mp4_sample_offset[2] = %d\n",
						mp4_sample_offset[0],mp4_sample_offset[1],mp4_sample_offset[2]);

			}
			
		}
		else
		{
			ERROR_LOG("unknown file type!\n");
			return -1;
		}
		
		/*-----------------------------------------------------------------------------------------------------------------------------*/

		/*
	 	for (int kkk=0; kkk<30; kkk++) 
	 	{
			printf("\nmp4_sample_offset[%d] = %d",kkk, mp4_sample_offset[kkk]);
		}
		printf("\nmp4_sample_offset_counter = %d", mp4_sample_offset_counter);
		fflush(stdout);
		*/
		
		/***START** 获取当前TS分片对应音频帧的帧数据********************************************************************************************/
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"soun")) //如果当前trak（moov_traks[i]）是 audio trak
		{
		DEBUG_LOG("into position I10 \n");
			/*---------解析esds box，获取音频解码参数-------------------------------------------------------------------------*/
			int DecSpecInfo=0;
			long long BaseMask=0xFFF10200001FFC;
			DecSpecInfo = get_DecoderSpecificInfo(mp4, source, find_stsd);
			long long objecttype=0;
			long long frequency_index=0;
			long long channel_count=0;
			long long TempMask=0;
			objecttype = 0;//(DecSpecInfo >> 11) - 1;
			frequency_index = (DecSpecInfo >> 7) & 15;
			channel_count = (DecSpecInfo >> 3) & 15;

			objecttype = objecttype << 38;
			frequency_index = frequency_index << 34;
			channel_count = channel_count << 30;
			BaseMask = BaseMask | objecttype | frequency_index | channel_count;

			/*-------遍历当前TS分片需要的每一个帧,将帧数据放到缓存中（并加上7个字节的 adts 头数据）----------------------------*/
			int z = 0;//记录当前TS分片对应帧的下标（区间值）
			for (z = cur_track_data->first_frame ;  z < cur_track_data->first_frame + cur_track_data->n_frames; z++) 
			{
				TempMask = BaseMask | ((stsz_data[z] + 7) << 13);
				char for_write=0;
				int g=6;
				int t=0;
				for (g=6, t=0 ; g>=0 ;  g--, t++) 
				{
					for_write = TempMask >> g*8;
					cur_track_data->buffer[cur_track_data->offset[z-cur_track_data->first_frame]+t] = for_write;
				}
				source->read(	mp4, 
								(char*)cur_track_data->buffer + cur_track_data->offset[z - cur_track_data->first_frame] + 7, //7----adts头
								cur_track_data->size[z - cur_track_data->first_frame] - 7, //前边还要多读7字节的 adts 头数据 
								mp4_sample_offset[z], 
								0
							);
			}

		}
		/***END******************************************************************************************************************************/

		/***START** 获取当前TS分片对应视频帧的帧数据********************************************************************************************/
		if(handlerType(mp4, source, find_box(moov_traks[i],"hdlr"),"vide")) //如果当前trak（moov_traks[i]）是 video trak
		{
		DEBUG_LOG("into position I11 \n");
			char* AVCDecInfo=NULL;
			int avc_decinfo_size=0; 		//SPS + PPS 数据部分的大小
			int NALlengthsize=0; 			//avcC box 的NALUnitLengthSize字段
			char AUD[6]={0,0,0,1,9,240}; 	// AUD for ios support
			AVCDecInfo = get_AVCDecoderSpecificInfo(context, mp4, source, find_box(moov_traks[i],"stsd"), &avc_decinfo_size, &NALlengthsize);
			if(NULL == AVCDecInfo)
			{
				ERROR_LOG("get_AVCDecoderSpecificInfo error !\n");
				return -1;
			}
			
//			if(NALlengthsize==4) {
				int z = 0; //记录当前TS分片对应帧的下标（区间值）
				for (z = cur_track_data->first_frame; z < cur_track_data->first_frame + cur_track_data->n_frames ; z++) 
				{
					if (z == cur_track_data->first_frame) //第一个视频帧（当前的TS文件来说），特殊处理
					{
						DEBUG_LOG("into position I11--A \n");
						int nal_size = 0;
						int c = 0;
						int m = 0;
						for(c = 0 ; c < 6; c++)//放 AUD 头（6字节）
							cur_track_data->buffer[c] = AUD[c];
						for(m = 0 ; m < avc_decinfo_size ; m++)//放 SPS + PPS 数据
							cur_track_data->buffer[m+6] = AVCDecInfo[m]; //6 - AUD size;
							
						//直接从文件将帧的数据部分读到 buf 中
						//读帧的数据部分（跳过 AUD + SPS + PPS 部分，这里之前填充的size字段时就没有算进去AUD，所以只 - avc_decinfo_size）
						DEBUG_LOG("track_data->size[ z -track_data->first_frame] = %d mp4_sample_offset[%d] = %d\n",
									cur_track_data->size[ z -cur_track_data->first_frame],z,mp4_sample_offset[z]);
						
						cur_track_data->size[z - cur_track_data->first_frame] -= avc_decinfo_size; 
						source->read(	mp4,
										(char*)cur_track_data->buffer + avc_decinfo_size + 6, /*一次读一帧，保留SPS PPS空间 + 6字节的AUD头空间*/
										cur_track_data->size[ z -cur_track_data->first_frame],
										mp4_sample_offset[z],
										0
									);
						//convert_to_annexb((char*)track_data->buffer+avc_decinfo_size, track_data->size[z-track_data->first_frame]-avc_decinfo_size, NALlengthsize);
						//鬼知道在干什么
						
						int ret = convert_to_annexb(	context, 
														(char*)cur_track_data->buffer + avc_decinfo_size + 6, /*传入的是当前所读帧的数据部分*/
														NALlengthsize, 
														cur_track_data->size, 
														cur_track_data->offset, 
														z - cur_track_data->first_frame
										 			);
						if(ret < 0)
						{
							ERROR_LOG("convert_to_annexb error !\n");
							return -1;
						}
						
						cur_track_data->size[z-cur_track_data->first_frame]+=avc_decinfo_size;
						cur_track_data->offset[z-cur_track_data->first_frame+1]=cur_track_data->size[z-cur_track_data->first_frame];
						DEBUG_LOG("into position I11--A--out \n");

					}
					else 
					{
						DEBUG_LOG("into position I11--B \n");
						int c = 0;
						for(c = 0 ; c < 6 ; c++)
							cur_track_data->buffer[cur_track_data->offset[ z - cur_track_data->first_frame] + c] = AUD[c];
						
						
						source->read(	mp4,
										(char*)cur_track_data->buffer + cur_track_data->offset[ z - cur_track_data->first_frame] + 6, 
										cur_track_data->size[z - cur_track_data->first_frame] , 
										mp4_sample_offset[z] ,
										0 
									);
						
						int ret = convert_to_annexb(	context, 
														(char*)cur_track_data->buffer + cur_track_data->offset[ z - cur_track_data->first_frame] + 6, 
														NALlengthsize, 
														cur_track_data->size, 
														cur_track_data->offset, 
														z - cur_track_data->first_frame
													);
						if(ret < 0)
						{
							ERROR_LOG("convert_to_annexb error !\n");
							return -1;
						}
						
						
						if(z - cur_track_data->first_frame + 1 < cur_track_data->n_frames)//帧还没有读完（当前TS分片所对应的帧），计算后一个帧的偏移量 offset
							cur_track_data->offset[z - cur_track_data->first_frame + 1] = cur_track_data->offset[ z - cur_track_data->first_frame] + cur_track_data->size[ z - cur_track_data->first_frame];
						DEBUG_LOG("into position I11--B--out \n");
					}
				}
				HLS_FREE(AVCDecInfo);


		}
		
		/***END******************************************************************************************************************************/
		
		HLS_FREE(stsz_data);
		HLS_FREE(mp4_sample_offset);

	#endif

		DEBUG_LOG("at end of for i<n_traks\n");

	}

	
	HLS_FREE(moov_traks);
	i_want_to_break_free(root);
	DEBUG_LOG("out of mp4_media_get_data !\n");
	return output_buffer_size;
}


void hls_exit(void)
{
	if(NULL != sample_offset.V_offset_array)
		HLS_FREE(sample_offset.V_offset_array);
	if(NULL != sample_offset.A_offset_array)
		HLS_FREE(sample_offset.A_offset_array);
	if(NULL != sample_size.V_size_array)
		HLS_FREE(sample_size.V_size_array);
	if(NULL != sample_size.A_size_array)
		HLS_FREE(sample_size.A_size_array);
	
}


media_handler_t mp4_file_handler = {
										.get_media_stats = mp4_media_get_stats,
										.get_media_data  = mp4_media_get_data
									};






