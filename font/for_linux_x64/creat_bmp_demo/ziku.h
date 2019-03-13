#ifndef _ZIKU_H
#define _ZIKU_H

typedef unsigned char HLE_U8;
typedef unsigned short HLE_U16;
typedef unsigned int HLE_U32;
typedef signed char HLE_S8;
typedef signed short HLE_S16;
typedef signed int HLE_S32;
typedef unsigned long long  HLE_U64;
typedef signed long long HLE_S64;



/*矢量转点阵字库，单字符描述结构*/
struct CHARACTER_INFO
{
	  HLE_U16	width;			//当前字符的宽度
      HLE_U16 	height;			//当前字符的高度
      HLE_U32	matrix_size;	//当前字符点阵数据的字节数（不包括该 CHARACTER_INFO） 
};

/*矢量点阵字库描述头结构*/
struct ZK_HEAD_VECTOR
{
#define ZK_MAGIC  0x544E4F46
    int 		magic;
    HLE_U8 		q_start;  	//字库内部字符的编码区间，开始值（英文字符指ASCII码）
    HLE_U8 		q_end;		//字库内部字符的编码区间，结束值（英文字符指ASCII码）
    HLE_U8 		w_start;
    HLE_U8 		w_end;
	HLE_U32		data_size;	//后边数据部分的大小（所有的 CHARACTER_INFO + 点阵数据部分）
	HLE_U32		ASCII_offset[256];	//专门描述ASCII字符的偏移量（距离文件头的偏移字节数），下标与ASCII码相对应（如果为0则代表不支持该ASCII码）
   
};

/*矢量字库描述结构*/
struct ZK_VECTOR
{
    struct ZK_HEAD_VECTOR head;
    int size; 	/*size of the buf*/
    char *buf; 	/*buf to hold the zk lattice，字库点阵数据的缓存起始位置*/
    char *file; /*zk file name*/
};


#endif



