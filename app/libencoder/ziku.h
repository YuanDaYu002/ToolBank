#ifndef HAL_ZIKU_H
#define HAL_ZIKU_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "typeport.h"

typedef struct _tag_GLYPH_LATTICE
{
    int width; /*width of glyph*/
    int height; /*width of glyph*/
    int bpl; /*bytes per line*/
    const char *bits; /*start address of glyph lattice*/
} GLYPH_LATTICE;


/*
 * function: int zk_init()
 * description:
 *              init zk
 * return:
 *              0, success
 *              -1, fail
 * */
int zk_init(void);


/*
 * function: char *zk_get_lattice(unsigned char *str, GLYPH_LATTICE *lattice)
 * description:
 *              找到str字符串中第一个字(单字节ASCII或双字节GBK字符)的点阵
 * arguments:
 *              str, 需要解析的字符串
 *              lattice, str中已处理字节的字形点阵信息
 * return:
 *              返回该次调用已处理的str中的字节数
 *              0, 未处理任何字节
 *              1, 处理了一个字节(ASCII)
 *              2, 处理了两个字节(GBK)
 *              -1, fail
 * */
int zk_get_lattice(const unsigned char *str, GLYPH_LATTICE *lattice);

/*
 * function: int str2matrix(const HLE_U8 *str, HLE_U8 *matrix_data)
 * description:
 *              将str字符串转换成整片的点阵信息
 * arguments:
 *              str, 需要转换的字符串
 *              matrix_data 转换后的点阵信息数据
 * return:		void
 *
 * */
void str2matrix(const HLE_U8 *str, HLE_U8 *matrix_data);

/*
 * function: int get_matrix_width(const HLE_U8 *str)
 * description:
 *              将str字符串转换成整片的点阵后，点阵的宽度，单位为像素
 * arguments:
 *              str, 需要转换的字符串
 * return:		点阵的宽度
 *
 * */
int get_matrix_width(const HLE_U8 *str);

#endif /*HAL_ZIKU_H*/

