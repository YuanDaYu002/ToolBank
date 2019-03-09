/*********************************************************************************
  *FileName: media_server_signal_parse.c
  *Create Date: 2018/09/18
  *Description: 客户端信令解析的头文件，用户事件处理函数 。 
  *Others:  
  *History:  
**********************************************************************************/

#include <string.h>
#include <stdio.h>
#include <time.h>


#include "media_server_signal_def.h"
#include "media_server_interface.h"
//#include "media_server_curl.h"
#include "media_server_signal_parse.h"

#include "media_server_p2p.h"

#include "PPCS_API.h"
#include "PPCS_Error.h"
#include "PPCS_Type.h"
#include "opt.h"
#include "timezone.h"




extern med_ser_init_info_t g_med_ser_envir;



//会话状态数组，描述每个会话的状态信息
static session_status_t SessionStatus[MAX_CLIENT_NUM] = {0};

//实时视频流线程创建时传入的参数结构体
typedef struct _open_living_args_t
{
	STRUCT_OPEN_LIVING_REQUEST cmd_data;
	HLE_S32 SessionID;
}open_living_args_t;

/*
添加一个会话到会话状态数组
返回：
	成功：sessionID所在的数组下标 i
	失败：< 0 的错误码
*/

HLE_S32 add_one_session_to_arr(HLE_S32 SessionID)
{
	HLE_S8 i;
	//查找状态数组的空闲元素下标，填入会话ID，初始化状态。
	for(i = 0;i<MAX_CLIENT_NUM;i++)
	{
		if(0 == SessionStatus[i].node_is_used)
		{
			SessionStatus[i].SessionID = SessionID;
			SessionStatus[i].node_is_used = 1;
			
			return i;
		}
	}
	DEBUG_LOG("SessionStatus array is full!\n");
	return HLE_RET_ENORESOURCE;
	
}

/*
判断某个会话是否退出（客户端退出）
返回：
	在线： ONLINE
	退出：	CLOSED
*/
HLE_S32 is_session_online(HLE_S32 SessionID)
{
	HLE_S8 i;

	for(i = 0;i<MAX_CLIENT_NUM;i++)
	{
		if(SessionID == SessionStatus[i].SessionID)
		{
			if(ONLINE == SessionStatus[i].Session_status)
			return ONLINE;
		}
	}
	return OFFLINE;
}


/*
获取某个会话的状态信息
参数：
	SessionID（传入）:会话ID
	status（传出）：会话的状态信息
返回：
	成功：sessionID所在的数组下标 i
	失败：< 0 的错误码

*/
HLE_S32 get_session_status(HLE_S32 SessionID,session_status_t* status)
{
	if(NULL == status)
		return HLE_RET_EINVAL;
	HLE_S8 i;
	for(i = 0;i<MAX_CLIENT_NUM;i++)
	{
		if(SessionID == SessionStatus[i].SessionID)
		{
			memcpy(status,&SessionStatus[i],sizeof(session_status_t));
			return i;
		}
	}
	
	DEBUG_LOG("SessionStatus array not have a item named %d !\n",SessionID);
	return HLE_RET_ERROR;

}


/*
修改某个会话的状态信息 
参数：
	SessionID（传入）:会话ID
	status（传入）：会话的状态信息

返回：
	成功：sessionID所在的数组下标 i
	失败：< 0 的错误码
	
注意：一定要先get 然后修改，之后再 set
*/
HLE_S32 set_session_status(HLE_S32 SessionID,session_status_t* status)
{
	if(NULL == status)
		return HLE_RET_EINVAL;
	HLE_S8 i;
	for(i = 0;i<MAX_CLIENT_NUM;i++)
	{
		if(SessionID == SessionStatus[i].SessionID)
		{
			memcpy(&SessionStatus[i],status,sizeof(session_status_t));
			return i;
		}
	}
	
	DEBUG_LOG("SessionStatus array not have a item named %d !\n",SessionID);
	return HLE_RET_ERROR;

}


/*
从会话状态数组删除一个会话
返回：
	成功：sessionID所在的数组下标 i
	失败：< 0 的错误码

*/
HLE_S32 del_one_session_from_arr(HLE_S32 SessionID)
{

	HLE_S8 i;
	//找到该会话在会话数组中的下标位置，将该下标的数组元素清0
	for(i = 0;i<MAX_CLIENT_NUM;i++)
	{
		if(SessionID == SessionStatus[i].SessionID)
		{
			memset(&SessionStatus[i],0,sizeof(session_status_t));
			return i;
		}
	}
	DEBUG_LOG("SessionStatus array not have a item named %d !\n",SessionID);
	return HLE_RET_ERROR;
}





//获取设备MAC地址
HLE_S32 cmd_Get_Mac(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_Get_Mac sucess!\n");
	/*需要知道获取设备参的接口和类型*/
	return HLE_RET_OK;
}

// 读设备信息	
HLE_S32 cmd_read_dev_info(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_read_dev_info sucess!\n");
	/*需要知道获取设备参的接口和类型*/
	return HLE_RET_OK;
}

//设置设备系统参数	
HLE_S32 cmd_set_dev_para(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_set_dev_para sucess!\n");
	/*需要知道获取设备参的接口和类型*/
	return HLE_RET_OK;
}

//读取设备系统参数	
HLE_S32 cmd_read_dev_para(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_read_dev_para sucess!\n");
	/*需要知道获取设备参的接口和类型*/
	return HLE_RET_OK;
}

//报警通知
HLE_S32 cmd_alarm_update(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_alarm_update sucess!\n");
	return HLE_RET_OK;
}


//音视频帧头标志

typedef struct
{
    HLE_U8 sync_code[3]; /*帧头同步码，固定为0x00,0x00,0x01*/
    HLE_U8 type; /*帧类型, 0xF8-视频关键帧，0xF9-视频非关键帧，0xFA-音频帧*/
} FRAME_HDR;

//非关键帧描述信息
typedef struct
{
    HLE_U64 pts_msec; //毫秒级时间戳，一直累加，溢出后自动回绕
    HLE_U32 length;
} PFRAME_INFO;

//关键帧描述信息
typedef struct
{
    HLE_U8 enc_std; //编码标准，具体见E_VENC_STANDARD
    HLE_U8 framerate; //帧率
    HLE_U16 reserved;
    HLE_U16 pic_width;
    HLE_U16 pic_height;
    HLE_SYS_TIME rtc_time; //当前帧时间戳，精确到秒，非关键帧时间戳需根据帧率来计算
    HLE_U32 length;
    HLE_U64 pts_msec; //毫秒级时间戳，一直累加，溢出后自动回绕
} IFRAME_INFO;

//打开实时流传输
#define MAX_WRITE_ERR_NUM  30  //(15+25)*1 ,连续发送编码帧失败的最大容忍限度，超出后认为客户端异常断线
void* cmd_open_living(void* args)
{
	if(NULL == args)
		return NULL;
	
	 pthread_detach(pthread_self());

	 //参数数据备份
	 open_living_args_t  stream_args;
	 memset(&stream_args,0,sizeof(open_living_args_t));
	 memcpy(&stream_args,args,sizeof(open_living_args_t));

	 printf("videoType(%d)  openAudio(%d)!\n",stream_args.cmd_data.videoType,stream_args.cmd_data.openAudio);
	

	//解析 cmd_body，回传对应码流,目前只有 MAIN_STREAM 和 LOWER_STREAM 两道流
	if(MAIN_STREAM == stream_args.cmd_data.videoType ||LOWER_STREAM == stream_args.cmd_data.videoType)
	{
		void* pack_addr = NULL;
		void* frame_addr = NULL;
		HLE_S32 length = 0;
		st_PPCS_Session Sinfo;
		int ret;
		DEBUG_LOG("SessionID(%d): real time stream(%d) transmission start!\n",stream_args.SessionID ,stream_args.cmd_data.videoType);	

		//改变会话状态信息
		session_status_t  status;
		memset(&status,0,sizeof(status));
		int index = get_session_status(stream_args.SessionID,&status);
		if(index < 0)
		{
			ERROR_LOG("get_session_status failed !\n");
			pthread_exit(NULL) ;
		}
		status.current_stream = stream_args.cmd_data.videoType;
		status.stream_status = OPEN;
		
		index = set_session_status(stream_args.SessionID ,&status);
		if(index < 0)
		{
			ERROR_LOG("change_session_status failed !\n");
			pthread_exit(NULL) ;
		}
		

		int first_is_iframe = 0;//初次进入码流发送状态需要强制I帧
		g_med_ser_envir.encoder_force_iframe(0,stream_args.cmd_data.videoType);

		HLE_S8 write_err_count = 0;
				
		while(OPEN == status.stream_status)//如若客户端退出，自然 stream_status会是0,所以这里不再判断客户端是否在线。
		{
			frame_addr = NULL;
			length = 0;

			memset(&status,0,sizeof(status));
			get_session_status(stream_args.SessionID,&status);
			
			g_med_ser_envir.get_one_encoded_frame_callback(stream_args.cmd_data.videoType,\
														   stream_args.cmd_data.openAudio,\
															&pack_addr,\
															&frame_addr,&length);
 

			//寻找视频关键帧。
			if(0 == first_is_iframe)
			{
				 FRAME_HDR *header = (FRAME_HDR *) frame_addr;
				if(header->type == 0xF8)//找到视频关键帧,进入传输模式
				{
					first_is_iframe = 1;
					DEBUG_LOG("find the I frame!\n");
				}
				else
				{
					g_med_ser_envir.dec_frame_ref_callback(pack_addr);
					//spm_pack_print_ref(pack_addr);
					continue;
				}
					
				
			}

			//进入传输状态
			if(1 == first_is_iframe)
			{
				ret = PPCS_Write(stream_args.SessionID,CH_STREAM,frame_addr,length);
				if (ret < 0)//发送失败
				{
					if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
					{
						ERROR_LOG("PPCS_Write  CH=%d, ret=%d, Session Closed TimeOUT!!\n", CH_STREAM, ret);
					}				
					else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)//远程会话关闭（客户端退出）
					{
						ERROR_LOG("PPCS_Write CH=%d, ret=%d, Session Remote Close!!\n", CH_STREAM, ret);
					}				
					else 
					{
						ERROR_LOG("PPCS_Write CH=%d, ret=%d  [%s]\n", CH_STREAM,ret, getP2PErrorCodeInfo(ret));
						
					}	
					
					g_med_ser_envir.dec_frame_ref_callback(pack_addr);

					memset(&status,0,sizeof(status));
					get_session_status(stream_args.SessionID,&status);
					status.stream_status = CLOSE;
					status.Session_status = OFFLINE;
					set_session_status(stream_args.SessionID,&status);				
	
					break;

					
					#if 0
					g_med_ser_envir.dec_frame_ref_callback(pack_addr);
		
					write_err_count++; 
					if(write_err_count > MAX_WRITE_ERR_NUM)//认为异常断线
					{
						
						get_session_status(stream_args.SessionID,&status);
						status.stream_status = CLOSE;
						status.Session_status = OFFLINE;
						set_session_status(stream_args.SessionID,&status);	
						ERROR_LOG("Session Closed abnormally !\n");
						break;
					}
					#endif
					
					//msleep(50);
					//continue ;
					
				}
				else
				{
					//DEBUG_LOG("send success! pack_addr(%p) frame_addr(%p)  length(%d) write(%d)bytes!\n",pack_addr,frame_addr,length,ret);
					g_med_ser_envir.dec_frame_ref_callback(pack_addr);	
				}
			}
			
		
		}
		
	}
	else
	{
		ERROR_LOG("stream resolution(%d) not support ! \n",stream_args.cmd_data.videoType);
		pthread_exit(NULL) ;
		
	}

	DEBUG_LOG("SessionID(%d): real time stream(%d) closed !\n",stream_args.SessionID,stream_args.cmd_data.videoType);
	pthread_exit(NULL) ;
	return NULL;
}

/*
关闭实时流传输
注意：不要关闭了SessionID，SessionID只能由收到“退出登陆请求命令”接口关闭
*/
HLE_S32 cmd_close_living(HLE_S32 SessionID)
{
	session_status_t  status;
	memset(&status,0,sizeof(status));

	int index = get_session_status(SessionID,&status);
	if(index < 0)
	{
		ERROR_LOG("get_session_status failed !\n");
	}
	
	status.stream_status = CLOSE;
	
	index = set_session_status(SessionID,&status);
	if(index < 0)
	{
		ERROR_LOG("change_session_status failed !\n");
	}
	
	DEBUG_LOG("cmd_close_living sucess!\n");
	return HLE_RET_OK;
}

//重启命令
HLE_S32 cmd_set_reboot(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_set_reboot sucess!\n");
	return HLE_RET_OK;
}

//升级命令
#include <sys/types.h>
#include <pwd.h>

HLE_S32 cmd_set_update(HLE_S32 SessionID,HLE_S32 readsize)
{

	char * url = NULL;

	STRUCT_SET_UPDATE_REQUEST cmd_body;
	//接收信令的 body 部分,注意cmd_body 的 DEF_CMD_HEADER 部分在之前已经接收过了，这里需要跳过该部分
	//所以，这里的 cmd_body.DEF_CMD_HEADER 是空的，不要使用。
	memset(&cmd_body,0,sizeof(STRUCT_SET_UPDATE_REQUEST));
	HLE_S32 ReadSize = readsize;
	
	
	printf("cmd body ReadSize = %d\n",ReadSize);
	//注意：PPCS_Read 的返回值成功为0 ，实际读到的字节数会返回在传进去的变量ReadSize中。
	HLE_S32 ret = PPCS_Read(SessionID, CH_CMD, (CHAR*)&cmd_body + sizeof(cmd_header_t) , &ReadSize, 2000);
	if(ret < 0)//读取出错
	{
		if (ERROR_PPCS_TIME_OUT == ret) 
		{
			ERROR_LOG("PPCS_Read timeout !!\n");					
		}	
		else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) 
		{
			ERROR_LOG("Remote site call close!!\n");
		}
		else 
		{
			ERROR_LOG("PPCS_Read: Channel=%d, ret=%d\n", CH_CMD, ret);
		}
		return HLE_RET_ERROR;
	}
		
	//body长度数据校验
	if(ReadSize  != readsize)
	{
		ERROR_LOG("check cmd body length error! ret(%d)\n",ret);
		return HLE_RET_ERROR;
	}

	/*
		解析 出URL 对升级包的版本号等进行校验,待定。。。。。
	*/
	url = (char*)cmd_body.URL;


	#if 0 //liteos 不支持 curl库中调用的函数：geteuid 、 getpwuid_r
	//url = "http://soft1.xzstatic.com/KMSpico_Install_DownZa.Cn.rar";
	if(NULL != url)
	{
		printf("url : %s\n",url);
		url_download_file(url);

	}
		
	#endif

	/*
		对升级包再进行防错校验，待定。。。。。。
		写Flash 等  
	*/

	
	DEBUG_LOG("cmd_set_update sucess!\n");
	return HLE_RET_OK;
}

//连接WIFI热点		
HLE_S32 cmd_set_wifi_connect(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_set_wifi_connect sucess!\n");
	return HLE_RET_OK;
}

//获取wifi连接状态
HLE_S32 cmd_get_wifi_status(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_get_wifi_status sucess!\n");
	return HLE_RET_OK;
}

// 登陆请求命令
HLE_S32 cmd_request_login(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_request_login sucess!\n");
	return HLE_RET_OK;
}

// 退出登陆请求命令
HLE_S32 cmd_request_logout(HLE_S32 SessionID)
{

	del_one_session_from_arr(SessionID);
	PPCS_Close(SessionID);
		
	DEBUG_LOG("cmd_request_logout sucess!\n");
	return HLE_RET_OK;
}


//设置AUdio音量参数
HLE_S32 cmd_set_audio_vol(HLE_S32 SessionID)
{
	DEBUG_LOG("cmd_set_audio_vol sucess!\n");
	return HLE_RET_OK;
}

//设置时区(校时)命令
HLE_S32 cmd_set_time_zone(HLE_S32 SessionID)
{
	
	STRUCT_SET_TIME_ZONE_REQUEST data_buf;
	HLE_S32  BufSize = sizeof(STRUCT_SET_TIME_ZONE_REQUEST);

	HLE_S32 ret = PPCS_Read(SessionID, CH_CMD, (CHAR*)&data_buf, &BufSize, 2000);
	if(ret < 0)//读取出错
	{
		if (ERROR_PPCS_TIME_OUT == ret) 
		{
			ERROR_LOG("PPCS_Read timeout!!\n");
			
		}	
		else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) 
		{
			ERROR_LOG("Remote site call close!!\n");
			
		}
		else 
		{
			ERROR_LOG("PPCS_Read unknow error!ret=%d\n", ret);
			
			
		}
		return HLE_RET_ERROR;

	}
	
	DEBUG_LOG("Read TimeZone data success! BufSize(%d) ReadSize(%d)\n",BufSize,ret);

	
	time_t settime, difftime;		//time_t实际上就是一个long int型
	settime = time(NULL);
	
	/*1.计算新时区应设置的时间秒数*/
	if(data_buf.tzindex < 0||data_buf.tzindex > 142)
	{
		ERROR_LOG("tzindex out of range!\n");
		return HLE_RET_EINVAL;
	}
	city_time_zone_t timezone_buf =  tz_diff_table[data_buf.tzindex];
	difftime = timezone_buf.diff_timezone; //新设置的时区与UTC时间差值查询
	settime = settime + difftime;	

	/*2.进行夏令时调整*/
	if(data_buf.daylight)
	{
		/*南北半球国家的夏令时调整时间不统一，且还存在法律调整等情况，
		所以不方便用程序将夏令时的调整时间固定下来，因此夏令时调整时，
		需要用户通过手机app进行一次校时操作(打开夏令时开关)*/
		settime = settime + 3600;
	}

	/*3.重新设置系统rtc时钟*/
	SNTP_SET_SYSTEM_TIME(settime);/** SNTP macro to change system time and/or the update the RTC clock */

	return HLE_RET_OK;
}


/*
	传入接收到的信令数据，该函数解析接收到客户端发送的信令，交给响应函数处理
*/

HLE_S32 med_ser_signal_parse(HLE_S32 SessionID,cmd_header_t *data,HLE_S32 length)
{

	if(NULL == data)
	{
		ERROR_LOG("illegal argument!!\n");
		return HLE_RET_EINVAL;
	}

	
	cmd_header_t  cmd_header;
	memset(&cmd_header,0,sizeof(cmd_header_t));

	memcpy(&cmd_header,data,sizeof(cmd_header_t));

	
	if(cmd_header.head != HLE_MAGIC)
	{
		ERROR_LOG("cmd header is illegal!\n");
		return HLE_RET_ERROR;
	}

	/*数据长度校验,此时只接收到“命令头”部分*/
	if(sizeof(cmd_header_t) != length)
	{
		ERROR_LOG("package data length error!\n");
		return HLE_RET_ERROR;
	}
	
	/*cmd_header->type暂时没使用*/

	pthread_t living_threadID;
	HLE_S32 ret;



	//cmd_body_length_t cmd_body_arg;
	
	switch(cmd_header.command)
	{
		open_living_args_t open_living_args;
		STRUCT_OPEN_LIVING_REQUEST cmd_body;
	
		case CMD_GET_MAC :    		//获取设备MAC地址
			/*响应函数*/
			cmd_Get_Mac(SessionID);	
			break;

	
		case CMD_READ_DEV_INFO:		// 读设备信息	
				/*响应函数*/
			cmd_read_dev_info(SessionID);
			break;
		case CMD_SET_DEV_PARA:		//设置设备系统参数
				/*响应函数*/
			cmd_set_dev_para(SessionID);
			break;
		
		
		case CMD_READ_DEV_PARA:		// 读取设备系统参数	
			/*响应函数*/
			cmd_read_dev_para(SessionID);
			break;
		
		case CMD_ALARM_UPDATE:		//报警通知
			/*响应函数*/
			cmd_alarm_update(SessionID);
			break;

		case CMD_OPEN_LIVING:		//打开实时流传输

			//接收信令的 body 部分,注意cmd_body 的 DEF_CMD_HEADER 部分在之前已经接收过了，这里需要跳过该部分
			//所以，这里的 cmd_body.DEF_CMD_HEADER 是空的，不要使用。
			memset(&cmd_body,0,sizeof(STRUCT_OPEN_LIVING_REQUEST));
			HLE_S32 ReadSize = cmd_header.length;

			
			printf("cmd body ReadSize = %d\n",ReadSize);
			//注意：PPCS_Read 的返回值成功为0 ，实际读到的字节数会返回在传进去的变量ReadSize中。
			HLE_S32 ret = PPCS_Read(SessionID, CH_CMD, (CHAR*)&cmd_body + sizeof(cmd_header_t) , &ReadSize, 2000);
			if(ret < 0)//读取出错
			{
				if (ERROR_PPCS_TIME_OUT == ret) 
				{
					ERROR_LOG("PPCS_Read timeout !!\n");					
				}	
				else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) 
				{
					ERROR_LOG("Remote site call close!!\n");
				}
				else 
				{
					ERROR_LOG("PPCS_Read: Channel=%d, ret=%d\n", CH_CMD, ret);
				}
				break;
			}

			//body长度数据校验
			if(ReadSize  != cmd_header.length)
			{
				ERROR_LOG("check cmd body length error! ret(%d)\n",ret);
				break;
			}
	
			//创建线程专门负责传流
			memset(&open_living_args,0,sizeof(open_living_args));
			memcpy(&open_living_args,&cmd_body,sizeof(cmd_body));
			open_living_args.SessionID = SessionID;
			
			ret = pthread_create(&living_threadID, NULL, &cmd_open_living, &open_living_args);
			if (0 != ret) 
			{
				ERROR_LOG("pthread_create cmd_open_living failed!\n");
				break;
			}
			DEBUG_LOG("pthread_create cmd_open_living success,tid(%d)\n",living_threadID);
			break;
				
		case CMD_CLOSE_LIVING:		//关闭实时流传输
				/*响应函数*/
			cmd_close_living(SessionID);
			break;
		case CMD_SET_REBOOT:			// 重启命令
				/*响应函数*/
			cmd_set_reboot(SessionID);
			break;
		
		case CMD_SET_UPDATE:			//升级命令
				/*响应函数*/
			cmd_set_update(SessionID,cmd_header.length);
			break;
		
		case CMD_SET_WIFI_CONNECT:		//连接WIFI热点
				/*响应函数*/
			cmd_set_wifi_connect(SessionID);
			break;
				
		case CMD_GET_WIFI_STATUS:		//获取wifi连接状态
				/*响应函数*/
			cmd_get_wifi_status(SessionID);
			break;
				
		case CMD_REQUEST_LOGIN:			// 登陆请求命令
				/*响应函数*/
			cmd_request_login(SessionID); 
			break;

		case CMD_REQUEST_LOGOUT:		// 退出登录请求命令
				/*响应函数*/
			cmd_request_logout(SessionID); 
			break;
				
		case CMD_SET_AUDIO_VOL:			//设置AUdio音量参数
				/*响应函数*/
			cmd_set_audio_vol(SessionID);
			break;
		case CMD_SET_TIME_ZONE:			//设置时区(校时)命令
			cmd_set_time_zone(SessionID);
			break;
		default:
				/*响应函数*/
			DEBUG_LOG("default command!\n");
			break;
				
	}
	

	return HLE_RET_OK;
}












