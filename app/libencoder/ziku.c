#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "ziku.h"
#include "hal.h"

/*
GBK的编码范围
                范围		第1字节		第2字节			编码数		字数
水准 GBK/1 		A1–A9		A1–FE			846			717
水准 GBK/2		B0–F7		A1–FE			6,768		6,763
水准 GBK/3		81–A0		40–FE (7F除外) 6,080		6,080
水准 GBK/4 		AA–FE 		40–A0 (7F除外) 8,160 		8,160
水准 GBK/5 		A8–A9 		40–A0 (7F除外) 192 		166
用户定义 		AA–AF 		A1–FE 			564
用户定义 		F8–FE 		A1–FE 			658
用户定义 		A1–A7 		40–A0 (7F除外) 672
合计: 										23,940 	    21,886
 */

#define ZK_USE_GBK3
#define ZK_USE_GBK4
#define ZK_USE_GBK5

/*点阵字库描述头结构*/
struct ZK_HEAD
{
#define ZK_MAGIC  0x544E4F46
    int magic;
    HLE_U8 q_start;  	//字库内部字符的编码区间，开始值（ASCII码）
    HLE_U8 q_end;		//字库内部字符的编码区间，结束值（ASCII码）
    HLE_U8 w_start;
    HLE_U8 w_end;
    HLE_U8 width;		//单个字符的宽度
    HLE_U8 height;		//单个字符的高度
    HLE_U8 pad[2];
};

/*字库描述结构*/
struct ZK
{
    struct ZK_HEAD head;
    int bpl; 	/*bytes per line*/
    int size; 	/*size of the buf*/
    char *buf; 	/*buf to hold the zk lattice，字库点阵数据的缓存起始位置*/
    char *file; /*zk file name*/
};


static struct ZK zks[] = {
    {
        {0}, 0, 0, NULL,
        "fonts/ASC16",
    },

    {
        {0}, 0, 0, NULL,
        "fonts/HZK16",
    },
};

/*字库的（区）区间*/
#define ZK_QU_COUNT(pzk) (pzk->head.q_end - pzk->head.q_start + 1)
/*字库的（位）区间*/
#define ZK_WEI_COUNT(pzk)   (pzk->head.w_end - pzk->head.w_start + 1)

#define ARRAY_COUNT(array)     (sizeof(array)/sizeof(array[0]))

/*
功能：
	将字库文件缓冲到ZK内部指向的内存，并初始化内部参数
返回：
	成功： 0;
	失败：-1;
*/
static int load_zk(struct ZK *zk)
{
    DEBUG_LOG("loading %s ...\n", zk->file);

    int fd = open(zk->file, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    /*calc size from head*/
    if (hal_readn(fd, &zk->head, sizeof (zk->head)) != sizeof (zk->head)) {
        ERROR_LOG("read head failed!\n");
        goto closefile;
    }
    zk->bpl = (zk->head.width + 7) / 8;//向上 8 bit对齐
    zk->size = zk->bpl * zk->head.height * ZK_QU_COUNT(zk) * ZK_WEI_COUNT(zk);
    DEBUG_LOG("width=%d, height=%d, zk size %d\n", zk->head.width, zk->head.height, zk->size);

    /*calc file size*/
    int file_size = lseek(fd, 0, SEEK_END);
    DEBUG_LOG("file size %d\n", file_size);

    /*compare two size, check if them match*/
    if (file_size != zk->size + sizeof (zk->head)) {
        ERROR_LOG("file size and font size mismatch, this file is broken!\n");
        goto closefile;
    }

    /*malloc zk buf and fill it*/
    zk->buf = malloc(zk->size);
    if (zk->buf == NULL) {
        ERROR_LOG("malloc failed!\n");
        goto closefile;
    }

    lseek(fd, sizeof (zk->head), SEEK_SET);
    if (hal_readn(fd, zk->buf, zk->size) != zk->size) {
        perror("read");
        goto freebuf;
    }

    return 0;

freebuf:
    free(zk->buf);
closefile:
    close(fd);
    return -1;
}

/*
 * function: int zk_init()
 * description:
 *              init zk
 * return:
 *              0, success
 *              -1, fail
 * */
int zk_init()
{
    int i;
    for (i = 0; i < ARRAY_COUNT(zks); i++) {
        if (load_zk(zks + i) < 0) {
            ERROR_LOG("load zk %s failed\n", zks[i].file);
            return -1;
        }
    }

    return 0;
}

/*
 * function: char *zk_get_lattice(HLE_U8 *str, GLYPH_LATTICE *lattice)
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
int zk_get_lattice(const HLE_U8 *str, GLYPH_LATTICE *lattice)
{
	/*如果字符属于ASCII码表的范围*/
    if (*str >= zks[0].head.q_start && *str <= zks[0].head.q_end) 
	{
        lattice->width = zks[0].head.width;
        lattice->height = zks[0].head.height;
        lattice->bpl = zks[0].bpl;
		
		/*
		计算目标字型（点阵数据）的所在位置：
		*/
        lattice->bits = zks[0].buf +
        		(*str - zks[0].head.q_start)/*字库起始字形往后偏移多少个字形才是目标字形的位置*/
                * zks[0].bpl * zks[0].head.height/*每个字形的字节数*/;

        return 1;
    }
	
	/*字符不属于ASCII码表范围的情况（查找其他字库）*/
    int i;
    for (i = 1; i < ARRAY_COUNT(zks); i++) 
	{
        if ((*str >= zks[i].head.q_start && *str <= zks[i].head.q_end)
            && (*(str + 1) >= zks[i].head.w_start && *(str + 1) <= zks[i].head.w_end)) 
        {
            struct ZK *pzk = zks + i;
            lattice->width = pzk->head.width;
            lattice->height = pzk->head.height;
            lattice->bpl = pzk->bpl;
            lattice->bits = pzk->buf + ((*str - pzk->head.q_start) * ZK_WEI_COUNT(pzk)
                    + (*(str + 1) - pzk->head.w_start)) * pzk->bpl * pzk->head.height;

            return 2;
        }
    }

    /*如果我们的字库中没有该字符的点阵，使用#符号来代替*/
    lattice->width = zks[0].head.width;
    lattice->height = zks[0].head.height;
    lattice->bpl = zks[0].bpl;
    lattice->bits = zks[0].buf + ('#' - zks[0].head.q_start)
            * zks[0].bpl * zks[0].head.height;
    return 1;
}

/* 计算字符串的（宽度）像素点个数（bit位数） */
int get_matrix_width(const HLE_U8 *str)
{
    int str_len = 0;

    while (*str != '\0') 
	{
        GLYPH_LATTICE ltc;
        str += zk_get_lattice(str, &ltc);
        str_len += ltc.bpl;
    }
    return (str_len * 8); //返回的是字符串的（宽度）像素点个数（bit位数）
}

/* 
功能：
	将字符串信息转换成BMP点阵数据 
参数：
	@str：（入）要转换的字符串
	@matrix_data：（返）转换后的BMP点阵数据
*/
void str2matrix(const HLE_U8 *str, HLE_U8 *matrix_data)
{
    int matrix_width = get_matrix_width(str);//字符串BMP数据（宽度）像素点个数（bit位数）
    int matrix_bytes = matrix_width / 8;	 //字符串BMP数据（宽度）字节数
    HLE_U8 *tmp_data = matrix_data;

    int i = 0;
    while (*str != '\0') 
	{
        GLYPH_LATTICE ltc;
        str += zk_get_lattice(str, &ltc);//一次获取一个字符的BMP数据描述信息

        /*
        完整打印一个字符的BMP数据(到线性数组缓存)：
        整个字符串的BMP数据当做一个整体存储到一维线性数组里边，假设 OSD字符串为 ABC，
        A0、B0、C0代表各个字符点阵第0行的数据，依次类推A1、B1、C1代表第1行的数据......
        则，线性数组存储的数据为：A0 B0 C0 A1 B1 C1 A2 B2 C2 ........ 
        如此去理解i的作用就显而易见了
		*/
        int j;
        for (j = 0; j < ltc.height; j++) //单个字符逐行打印
        {
            tmp_data = matrix_data + ((j * matrix_bytes) + i);
            int k;
            for (k = 0; k < ltc.bpl; k++) //拷贝一行的字节数
			{
                *tmp_data++ = *ltc.bits++;
            }
        }

        /* i = 已经打印字符的宽度之和 */
        i += ltc.bpl;
    }
}

#if 0

/* 测试用函数,用在让串口打印出点阵, str 参数要和str2matrix()函数的str 一致 */
int draw_pic(const HLE_U8 *str, const HLE_U8 *matrix_data)
{
    int matrix_width = get_matrix_width(str);
    int matrix_bytes = matrix_width / 8;

    int j;
    for (j = 0; j < 32; j++) {
        int k;
        for (k = 0; k < matrix_bytes; k++) {
            int i;
            for (i = 7; i >= 0; i--) {
                if (*matrix_data & (1 << i))
                    printf("*");
                else
                    printf("-");
            }
            matrix_data++;
        }
        printf("\n");
    }
    printf("\n");

    return 0;
}

/* 测试用函数，用来产生一个 32x32 的点阵 */
void make_text(HLE_U8 *str_data)
{
    int i, j;

    for (i = 0; i < 32; i++) {
        for (j = 0; j < 1; j++) {
            *str_data++ = 0x5F;
        }
        for (j = 0; j < 3; j++) {
            *str_data++ = 0x00;
        }
    }
}

/* 用来打印单个字符的点阵 */
int draw_text(const HLE_U8 *str)
{
    while (*str != 0) {
        GLYPH_LATTICE ltc;
        str += zk_get_lattice(str, &ltc);

        const char *start = ltc.bits;
        int j;
        for (j = 0; j < ltc.height; j++) {
            int k;
            for (k = 0; k < ltc.width; k++) {
                if (start[k / 8] & (1 << (7 - k % 8)))
                    printf("*");
                else
                    printf("-");
            }
            start += ltc.bpl;
            printf("\n");
        }
        printf("\n");
    }

    return 0;
}

int main(int argc, char *argv[])
{
    HLE_U8 test_str[] = "hhhh";
    HLE_U8 data[8096];
    HLE_U8 m_text[1024];
    zk_init();
    //make_text(m_text);
    //draw_pic(test_str, m_text);

    str2matrix(test_str, data);
    draw_pic(test_str, data);

    return 0;
}

#endif

