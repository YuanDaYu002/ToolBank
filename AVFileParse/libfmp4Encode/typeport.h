
/* 
 * File:   typeport.h
 * Author: ChenMin
 *
 * Created on June 30, 2018, 10:36 AM
 */

#ifndef TYPEPORT_H
#define TYPEPORT_H
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char HLE_U8;
typedef unsigned short HLE_U16;
typedef unsigned int HLE_U32;
typedef signed char HLE_S8;
typedef signed short HLE_S16;
typedef signed int HLE_S32;
typedef unsigned long long  HLE_U64;
typedef signed long long HLE_S64;


#define HLE_RET_OK               (0)     //正常
#define HLE_RET_ERROR            (-1)    //未知错误
#define HLE_RET_EINVAL           (-2)    //参数错误
#define HLE_RET_ENOTINIT         (-3)    //模块或设备未初始化
#define HLE_RET_ENOTSUPPORTED    (-4)    //不支持该功能
#define HLE_RET_EBUSY            (-5)    //设备或资源忙
#define HLE_RET_ENORESOURCE      (-6)    //设备资源不足
#define HLE_RET_EIO              (-7)    //I/O错误
#define HLE_RET_ENETWORK         (-8)    //网络错误
#define HLE_RET_EPASSWORD        (-9)    //用户名密码校验错误
#define HLE_RET_USER_EXISTS      (-10)   //该用户名已存在
#define HLE_RET_ENORIGHT         (-11)   //没有操作权限


#define HLE_DEBUG
#define HLE_ERROR
#define HLE_FATAR

#ifdef HLE_DEBUG
#define DEBUG_LOG(args...)  \
do { \
    printf("<DEBUG %s @%s:%d>: ", __FUNCTION__, __FILE__, __LINE__);   \
    printf(args);\
} while(0)
#else
#define DEBUG_LOG(args...)
#endif  /*HLE_DEBUG*/


#ifdef HLE_ERROR
#define ERROR_LOG(args...)  \
do { \
    printf("\033[1;31m<ERROR! %s @%s:%d>: ", __FUNCTION__, __FILE__, __LINE__);  \
    printf(args);\
    printf("\033[0m");\
} while(0)
#else
#define ERROR_LOG(args...)
#endif  /*HLE_ERROR*/


#ifdef HLE_FATAR
#define FATAR_LOG(args...)  \
do { \
    printf("\033[1;31m<FATAR!!! %s @%s:%d>: ", __FUNCTION__, __FILE__, __LINE__);    \
    printf(args);\
    printf("\033[0m");\
} while(0)
#else
#define FATAR_LOG(args...)
#endif  /*HLE_FATAR*/

typedef struct
{
    HLE_U32 left;
    HLE_U32 top;
    HLE_U32 right;
    HLE_U32 bottom;
} HLE_RECT;

typedef struct
{
    HLE_U32 tm_msec; /*milliseconds in day, [0,86400000)*/
    HLE_U8 tm_mday; /* day of the month, [1,31]*/
    HLE_U8 tm_mon; /* month, [1,12]*/
    HLE_U16 tm_year; /* year [1970,2037]*/
} HLE_SYS_TIME;

typedef enum
{
    DDF_YYYYMMDD, //年/月/日
    DDF_MMDDYYYY, //月/日/年
    DDF_DDMMYYYY, //日/月/年

    DDF_NR,
} E_DATE_DISPLAY_FMT;

//视频制式

typedef enum
{
    VIDEO_STD_PAL = 0,
    VIDEO_STD_NTSC,

    VIDEO_STD_NR,
} VIDEO_STD_E;



#ifdef __cplusplus
}
#endif

#endif /* TYPEPORT_H */


