/*********************************************************************************
  *FileName: media_server.h
  *Create Date: 2018/09/18
  *Description: 设备软件端Media Server程序业务逻辑处理的主头文件
  *Others:  
  *History:  
**********************************************************************************/
#ifndef MEDIA_SERVER_H
#define MEDIA_SERVER_H
#include "typeport.h"


/*********************************************************************
 *函数名：media_server_start
 *功能：启动媒体服务器
 *参数：
 *返回值：
 *	成功：HLE_RET_OK
 *	失败：HLE_RET_ERROR
 * ******************************************************************/
static void* media_server_start(void*arg);

static HLE_S32 media_server_exit(void);


#endif














