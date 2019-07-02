 
/***************************************************************************
* @file: rtmp_typeport.h
* @author:   
* @date:  7,1,2019
* @brief:  
* @attention:
***************************************************************************/


#ifndef RTMP_TYPEPORT_H
#define RTMP_TYPEPORT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

typedef unsigned char RTMP_U8;
typedef unsigned short RTMP_U16;
typedef unsigned int RTMP_U32;
typedef signed char RTMP_S8;
typedef signed short RTMP_S16;
typedef signed int RTMP_S32;
typedef unsigned long long  RTMP_U64;
typedef signed long long HLE_S64;


#define RTMP_RET_OK               (0)     //正常
#define RTMP_RET_ERROR            (-1)    //未知错误
#define RTMP_RET_EINVAL           (-2)    //参数错误
#define RTMP_RET_ENOTINIT         (-3)    //模块或设备未初始化
#define RTMP_RET_ENOTSUPPORTED    (-4)    //不支持该功能
#define RTMP_RET_EBUSY            (-5)    //设备或资源忙
#define RTMP_RET_ENORESOURCE      (-6)    //设备资源不足
#define RTMP_RET_EIO              (-7)    //I/O错误
#define RTMP_RET_ENETWORK         (-8)    //网络错误
#define RTMP_RET_EPASSWORD        (-9)    //用户名密码校验错误
#define RTMP_RET_USER_EXISTS      (-10)   //该用户名已存在
#define RTMP_RET_ENORIGHT         (-11)   //没有操作权限


#define RTMP_DEBUG
#define RTMP_ERROR
#define RTMP_FATAR

#ifdef RTMP_DEBUG
#define RTMP_DEBUG_LOG(args...)  \
do { \
    printf("<RTMP_DEBUG %s @%s:%d>: ", __FUNCTION__, __FILE__, __LINE__);   \
    printf(args);\
} while(0)
#else
#define RTMP_DEBUG_LOG(args...)
#endif  /*RTMP_DEBUG*/


#ifdef RTMP_ERROR
#define RTMP_ERROR_LOG(args...)  \
do { \
    printf("\033[1;31m<RTMP_ERROR! %s @%s:%d>: ", __FUNCTION__, __FILE__, __LINE__);  \
    printf(args);\
    printf("\033[0m");\
} while(0)
#else
#define RTMP_ERROR_LOG(args...)
#endif  /*RTMP_ERROR*/


#ifdef RTMP_FATAR
#define RTMP_FATAR_LOG(args...)  \
do { \
    printf("\033[1;31m<FATAR!!! %s @%s:%d>: ", __FUNCTION__, __FILE__, __LINE__);    \
    printf(args);\
    printf("\033[0m");\
} while(0)
#else
#define RTMP_FATAR_LOG(args...)
#endif  /*RTMP_FATAR*/


#ifdef __cplusplus
}
#endif

#endif /* RTMP_TYPEPORT_H */




