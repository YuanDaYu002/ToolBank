/*********************************************************************************
  *FileName: media_server_signal_parse.h
  *Create Date: 2018/09/19
  *Description: 客户端信令解析的头文件，用户事件处理函数 
  *Others:  
  *History:  
**********************************************************************************/
#ifndef MEDIA_SERVER_SIGNAL_PARSE_H
#define MEDIA_SERVER_SIGNAL_PARSE_H

#include "typeport.h"

enum stream_status_e
{
	CLOSE = 0,
	OPEN = 1,
};

enum session_status_e
{
	OFFLINE = 0,
	ONLINE = 1,
};



//会话状态结构体，目前只描述各个会话的各码流状态
typedef struct _session_status_t
{
	HLE_S32 SessionID;
	HLE_S8  Session_status;		//当前会话状态 OFFLINE ：掉线  	 	 	ONLINE：在线 
	HLE_S8 	current_stream;		//当前请求的流，	MAIN_STREAM、SECONDRY_STREAM、LOWER_STREAM
	HLE_S8  stream_status;		//流的状态 CLOSE:关闭  	 	OPEN：打开
	HLE_S8  node_is_used;		//当前节点是否已使用，0：未使用	  			 1：已使用 ; 客户端退出后将会清零
}session_status_t;



HLE_S32 med_ser_signal_parse(HLE_S32 SessionID,cmd_header_t*data,HLE_S32 length);

HLE_S32 cmd_Get_Mac(HLE_S32 SessionID);						//获取设备MAC地址
HLE_S32 cmd_read_dev_info(HLE_S32 SessionID);				//读设备信息	
HLE_S32 cmd_set_dev_para(HLE_S32 SessionID);  				//设置设备系统参数	
HLE_S32 cmd_read_dev_para(HLE_S32 SessionID); 				//读取设备系统参数	
HLE_S32 cmd_alarm_update(HLE_S32 SessionID); 				//报警通知
void * cmd_open_living(void* args); 						//打开实时流传输
HLE_S32 cmd_close_living(HLE_S32 SessionID); 				//关闭实时流传输
HLE_S32 cmd_set_reboot(HLE_S32 SessionID); 					//重启命令
HLE_S32 cmd_set_update(HLE_S32 SessionID,HLE_S32 readsize); 					//升级命令
HLE_S32 cmd_set_wifi_connect(HLE_S32 SessionID); 			//连接WIFI热点		
HLE_S32 cmd_get_wifi_status(HLE_S32 SessionID); 			//获取wifi连接状态
HLE_S32 cmd_request_login(HLE_S32 SessionID); 				// 登陆请求命令
HLE_S32 cmd_request_logout(HLE_S32 SessionID); 				// 退出登陆请求命令
HLE_S32 cmd_set_audio_vol(HLE_S32 SessionID); 				//设置AUdio音量参数
HLE_S32 cmd_set_time_zone(HLE_S32 SessionID);				//时区设置（校时）

HLE_S32 add_one_session_to_arr(HLE_S32 SessionID);	//添加一个会话到会话状态数组
HLE_S32 get_session_status(HLE_S32 SessionID,session_status_t* status);
HLE_S32 set_session_status(HLE_S32 SessionID,session_status_t* status); //修改某个会话的状态信息
HLE_S32 is_session_online(HLE_S32 SessionID);
HLE_S32 del_one_session_from_arr(HLE_S32 SessionID);	//从会话状态数组删除一个会话





#endif








