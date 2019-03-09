



/*********************************************************************************
  *FileName: media_server.c
  *Create Date: 2018/09/18
  *Description: P2P media server 的主干业务文件，也是P2P media server的入口文件。 
  *Others:  
  *History:  
**********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#include "media_server_p2p.h"
#include "media_server_signal_def.h"
#include "media_server_interface.h"
#include "media_server.h"
#include "opt.h"
#include "sntp.h"
#include "unistd.h"
#include "pwd.h"
#include "PPCS_API.h"



p2p_handle_t P2P_handle = {0};


/*****************************************************************
*函数名 ：	   media_server_init
*功能描述 ：P2PMediaServer初始化函数,负责注册设备app传过来的初始化信息
*参数 ： 	   info：媒体服务程序初始化信息结构体指针
		   info_len 
*返回值 ：	   成功：HLE_RET_OK 失败：HLE_RET_EINVAL
*****************************************************************/
med_ser_init_info_t g_med_ser_envir;
HLE_S8 g_is_inited = 0;

HLE_S32 media_server_init(med_ser_init_info_t* info, HLE_U32 info_len)
{
	if(NULL == info||info_len != sizeof(med_ser_init_info_t))
	{
		ERROR_LOG("media server init failed!\n");
		return HLE_RET_EINVAL;
	}

	  memcpy(&g_med_ser_envir, info, info_len);
	
	/*g_med_ser_envir 对象中有一部分成员因为不方便提前限定长度，所以类型定为char*，
	采取如下方式拷贝字符串，进行灵活赋值*/
#define COPY_ENVIR_STRING(EVIR_NAME)\
    do{\
        if(info->EVIR_NAME) g_med_ser_envir.EVIR_NAME = strdup(info->EVIR_NAME);\
        else g_med_ser_envir.EVIR_NAME = strdup("");\
    }while(0)

	COPY_ENVIR_STRING(device_id);
	COPY_ENVIR_STRING(device_name);
	COPY_ENVIR_STRING(config_dir);
	COPY_ENVIR_STRING(uboot_version);
	COPY_ENVIR_STRING(kernel_version);
	COPY_ENVIR_STRING(rootfs_version);
	COPY_ENVIR_STRING(app_version);
	COPY_ENVIR_STRING(network_interface);
	COPY_ENVIR_STRING(high_resolution);
	COPY_ENVIR_STRING(secondary_resolution);
	COPY_ENVIR_STRING(low_resolution);

#undef COPY_ENVIR_STRING
	
	g_is_inited = 1;
	return HLE_RET_OK;
}

static HLE_S32 media_server_exit(void)
{
	/*结束p2p medai server其他线程*/

	//释放申请的内存
#if 0
	free(g_med_ser_envir.device_id);
	free(g_med_ser_envir.device_name);
	free(g_med_ser_envir.config_dir);
	free(g_med_ser_envir.uboot_version);
	free(g_med_ser_envir.kernel_version);
	free(g_med_ser_envir.rootfs_version);
	free(g_med_ser_envir.app_version);
	free(g_med_ser_envir.network_interface);
	free(g_med_ser_envir.high_resolution);
	free(g_med_ser_envir.secondary_resolution);
	free(g_med_ser_envir.low_resolution);
#endif	
	PPCS_DeInitialize();
	return HLE_RET_OK ;
}

/******************************************************************
*函数名 ：	   media_server_Idle
*功能描述 ：闲置线程入口函数，负责一些比较杂的业务，其中就包括了校时功能
*参数 ： 	   arg： 
*返回值 ：	   NULL
*****************************************************************/
HLE_S32 connected_client_num = 0;

static void* media_server_Idle(void*arg)
{

	int ret;

	/*函数 lwIP_sntp_start 在 liteos 中默认有ntp服务器域名，这里就不手动再输入*/
	/* After doing this it will preiodically sends SNTP request message
	 for every 1 hour (SNTP_UPDATE_DELAY) and keeps updating system time
	 by calling the function configured in macro SNTP_SET_SYSTEM_TIME(t) */
	
	/* after startding SNTP, application code can continue its other task
	 once application decides to stop the application need to call
	 the API lwIP_sntp_stop() */
	#if 0
	ret = lwIP_sntp_start(NULL);
	if (ret == ERR_OK) 
	{
		DEBUG_LOG("sntp started successfully\n");
	} 
	else 
	{
		ERROR_LOG("sntp start failed\n");
	}
	#endif
	

	while(1)
	{
		
		//do something
		connected_client_num = P2P_handle.Session_num;
		//printf("connected_client_num = %d\n",connected_client_num);
		usleep(50*1000);//该延迟不宜太长，因为connected_client_num的值不能实时更新，将导致编码队列的编码数据包累积过多。
		
	
		

	}
	return NULL;

	
}
	

/*****************************************************************
*函数名 ：	   Idle_thread_create
*功能描述 ：创建闲置线程
*参数 ： 	  
*返回值 ：	   成功：0    失败：<0的值
*****************************************************************/
static HLE_S32 Idle_thread_create(void)
{
	pthread_t threadID_Idle;
	HLE_S32 err = pthread_create(&threadID_Idle, NULL, &media_server_Idle, NULL);
	if (0 != err) 
	{
		ERROR_LOG("create Thread Idle failed!\n");
	}
	
	return err;
}

/*****************************************************************
*函数名 ：	   media_server_start_thread
*功能描述 ：创建媒体服务线程
*参数 ： 	  
*返回值 ：	   成功：HLE_RET_OK     失败：返回错误码
*****************************************************************/
 HLE_S32 media_server_start_thread(void)
{ 
	if(0 == g_is_inited)//没有初始化
		return HLE_RET_ENOTINIT;
		
	pthread_t threadID_Idle;
	HLE_S32 err = pthread_create(&threadID_Idle, NULL, &media_server_start, NULL);
	if (0 != err) 
	{
		ERROR_LOG("create media_server_start failed!\n");
		return HLE_RET_ERROR;
	}
	
	return HLE_RET_OK;
}


/*****************************************************************
*函数名 ：	   media_server_start
*功能描述 ：启动 meidia server 业务
*参数 ： 	  
*返回值 ：	   成功：HLE_RET_OK     失败：HLE_RET_ERROR
*****************************************************************/
HLE_S8 is_first_run = 0;
static void* media_server_start(void*arg)
{
	
	is_first_run = 1; 
	HLE_S32 ret;
	
	/*创建IDLE线程*/
	ret = Idle_thread_create();
	if(ret < 0)
	{
		ERROR_LOG("Idle_thread_create error !\n");
		return NULL;	
	}
	DEBUG_LOG("Idle_thread_create success!\n");
	
P2P_restart:	
	//p2p_handle_t P2P_handle;
	memset(&P2P_handle,0,sizeof(p2p_handle_t));


	ret = p2p_init(&P2P_handle);
	if(ret < 0)
	{
		ERROR_LOG("p2p_init error !\n");
		return NULL;
	}
	DEBUG_LOG("p2p_init success!\n");
	
	//注意无线没连上或者不通外网的情况
	ret = p2p_conect(&P2P_handle);
	if(ret < 0)
	{
		ERROR_LOG("p2p_conect error !\n");
		return NULL;	
	}
	DEBUG_LOG("p2p_conect success!\n");


	while(1)//后边要有条件，不能是死循环。
	{
		do
		{
			ret = P2P_wait_for_wakeup(&P2P_handle);
			if(SERVERS_OFFLINE == ret)
				goto P2P_restart;
			
		}while(WAKEUP_ERR == ret);	
		DEBUG_LOG("media server is wakeup !\n");
		
		do
		{
			printf("into p2p_listen!\n");
			ret = p2p_listen(&P2P_handle);
			//if()什么条件下需要重新进入到睡眠状态，或者3min没有客户端连接成功，自动break再进入睡眠
			
			
		}while(ret < 0);
		
		
		if(ret >= 0 )//成功连接进客户端
		{
			DEBUG_LOG("one client connect,SessionID:%d\n",ret);
			P2P_handle.SessionID = ret;
			P2P_client_task_create(&P2P_handle);//创建与客户端交互的业务线程。
			
			
		}

		
		
	}

		
		
    return NULL;
}


















