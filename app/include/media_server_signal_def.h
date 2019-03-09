/*********************************************************************************
  *FileName: media_server_signal_def.h
  *Create Date: 2018/09/05
  *Description: 本文件为设备 media server 和服务器或手机APP端交互的信令协议文件。 
  *Others:  
  *History:  
**********************************************************************************/
#ifndef MEDIA_SERVER_SIGNAL_DEF_H
#define MEDIA_SERVER_SIGNAL_DEF_H

#include "typeport.h"

/*
待定：
TYPE_DEVICE_INFO	deviceInfo ;	//设备信息
SYSTEM_PARAMETER	param ;			//系统参数
TYPE_WIFI_LOGIN		wifiAP;			// wifi热点信息结构体
Audio_Coder_Param	audioParam ;	//音频编码参数
*/

//==================================
//用户名长度定义，所有与用户名相关的操作均
//使用该宏来定义用户名buf长度。方便后期调整
#define USER_NAME_LEN		16
//==================================
//用户名密码长度定义，同上
#define USER_PASS_LEN		16
//==================================


#ifdef __cplusplus
extern "C" {
#endif
#pragma pack( 4 )

#define HLE_MAGIC 0xB1A75   //HLE ASCII : 76 72 69-->767269-->0xB1A75

typedef struct
{
	HLE_S32		head;	//命令头标志，统一填充HLE_MAGIC
	HLE_S32		length;	//数据长度,去除head
	HLE_U16	type;
	HLE_U16	command;
}cmd_header_t ;

//定义消息头的宏，之后定义的每条消息的消息头必须填充该头。
#define DEF_CMD_HEADER	cmd_header_t	header


//========================================================
// 以下是具体信令定义:
// 新增定义请保持相同格式，变长数据也请用注释说明成员情况。
// 命令请求结构命名格式: STRUCT_XXX_XXX_REQUEST
// 回应结构命名格式: STRUCT_XXX_XXX_ECHO
//========================================================

#define CMD_GET_MAC 			0x1001 		//获取设备MAC地址
typedef struct
{
	DEF_CMD_HEADER ;

}STRUCT_GET_MAC_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER;
	HLE_S8 macAddr[6]; //mac地址
}STRUCT_GET_MAC_ECHO ; 

//------------------------------------------------------------------------
#define CMD_READ_DEV_INFO		0x1019		// 读设备信息	
typedef struct
{
	DEF_CMD_HEADER	;
}STRUCT_READ_DEVINFO_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	//TYPE_DEVICE_INFO	devInfo ;			//设备信息,待定

}STRUCT_READ_DEVINFO_ECHO ;


//------------------------------------------------------------------------
#define CMD_SET_DEV_PARA		0x100f		//设置设备系统参数	
typedef struct
{
	DEF_CMD_HEADER	;
//	SYSTEM_PARAMETER	param ;				//系统参数,待定义
}STRUCT_SET_DEV_PARAM_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		   echo ;						//回应结果(0:成功,-1:失败)
}STRUCT_SET_DEV_PARAM_ECHO ;

//------------------------------------------------------------------------
#define CMD_READ_DEV_PARA		0x1018		// 读取设备系统参数	
typedef struct
{
	DEF_CMD_HEADER	;
}STRUCT_READ_DEV_PARAM_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
//	SYSTEM_PARAMETER	param ;				//系统参数
}STRUCT_READ_DEV_PARAM_ECHO ;


//------------------------------------------------------------------------	
#define CMD_ALARM_UPDATE		0x100B		//报警通知
	
typedef struct
{
	DEF_CMD_HEADER ;
	HLE_S32	alarmType;							//报警类型0:移动侦测1:IO报警
	HLE_S32	alarmChl;							//报警通道号或者报警IO口,从0开始
	HLE_S32	reserved[2];						//保留
}STRUCT_ALARM_UPDATE_ECHO ;

//------------------------------------------------------------------------
#define CMD_OPEN_LIVING			0x100C		//打开实时流传输
enum Stream_Resolution
{
	MAIN_STREAM = 0,
	SECONDRY_STREAM = 1,
	LOWER_STREAM = 2,
};

typedef struct
{
	DEF_CMD_HEADER ;
	HLE_S32 videoType;							//视频码流类型：0：(1920*1088)，
											//1：(960*544)，2：(480*272)	,填入上方的枚举变量。
	HLE_S32 openAudio;							//音频码流开关：0:关闭, 1:打开
	
}STRUCT_OPEN_LIVING_REQUEST;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		result ;						//成功：0
											//失败：
											//-1：请求视频流失败
											//-2: 请求音频流失败
}STRUCT_OPEN_LIVING_ECHO ;


//------------------------------------------------------------------------
#define CMD_CLOSE_LIVING		0x100D		//关闭实时流传输
typedef struct
{
	DEF_CMD_HEADER ;

}STRUCT_CLOSE_LIVING_REQUEST;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		result ;						//成功：0
											//失败：-1
		
}STRUCT_CLOSE_LIVING_ECHO ;


//------------------------------------------------------------------------
#define CMD_SET_REBOOT			0x1010		// 重启命令
typedef struct
{
	DEF_CMD_HEADER;
	HLE_U32	headFlag1	;			//0x55555555
	HLE_U32	headFlag2	;			//0xaaaaaaaa
}STRUCT_SET_REBOOT_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32 echo ;								//回应结果(0:成功,-1:失败)
}STRUCT_SET_REBOOT_ECHO ;

//------------------------------------------------------------------------
#define CMD_SET_UPDATE			0x1012		//升级命令
typedef struct
{
	DEF_CMD_HEADER;
	HLE_S8 packageVersion[16];				//线上升级包的最新版本号
	HLE_S8 URL[256];							//升级文件在网络中的位置信息（URL）
	

}STRUCT_SET_UPDATE_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		   echo ;						/*   0:succeed,
												-1:check version failed
												-2:package version same
												-3:download package failed
												-4:checksum incorrect
												-5:erase flash failed
												-6:write flash failed
												-7:malloc failed
												-8:file length incorrect
												-9:malloc failed
												-10:unknow error
											*/
	HLE_U16	percent;				//进度百分比：0-100
	HLE_U16	status ;				/*升级状态：
												0：版本校验状态
												1：文件下载状态
												2: 文件校验状态
												3：擦除flash状态
												4：写flash状态
											*/
}STRUCT_SET_UPDATE_ECHO ;

//------------------------------------------------------------------------
#define	CMD_SET_RESTORE	 		0x1014		// 恢复出厂设置
typedef struct
{
	DEF_CMD_HEADER;
	HLE_U32	headFlag1	;			//0x55555555
	HLE_U32	headFlag2	;			//0xaaaaaaaa
	
}STRUCT_SET_RESTORE_REQUEST;

typedef struct
{
	DEF_CMD_HEADER;
	HLE_S32       echo;							//0:成功,-1:失败
	
}STRUCT_SET_RESTORE_ECHO ;

//------------------------------------------------------------------------
#define CMD_SET_WIFI_CONNECT	0x1016		//连接WIFI热点		
typedef struct
{
	DEF_CMD_HEADER	;
//	TYPE_WIFI_LOGIN		wifiAP;				//要连接的wifi热点的信息
}STRUCT_SET_WIFICONNECT_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32			echo ;						//-1：未连接，0：已连接
}STRUCT_SET_WIFICONNECT_ECHO ;

//------------------------------------------------------------------------
#define CMD_GET_WIFI_STATUS		0x1017		//获取wifi连接状态
typedef struct
{
	DEF_CMD_HEADER	;
//	TYPE_WIFI_LOGIN		wifiAP ;			//所要获取wifi连接状态的热点信息
}STRUCT_GET_WIFISTATUS_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
//	TYPE_WIFI_LOGIN		wifiAP ;			//返回对应wifi热点的连接状态信息
}STRUCT_GET_WIFISTATUS_ECHO ;
//------------------------------------------------------------------------

#define CMD_REQUEST_LOGIN		0x101A		// 登陆请求命令
typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S8			name[USER_NAME_LEN] ;	//用户名
	HLE_S8			pwd[USER_PASS_LEN] ;	//密码
}STRUCT_REQ_LOGIN_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32	permit  ;							//权限：0：超级用户，1：普通用户
	HLE_S32 echo ;								//成功：0 ,
											//失败：-1:密码错误，-2：用户名错误

}STRUCT_REQ_LOGIN_ECHO ;

//------------------------------------------------------------------------

#define CMD_REQUEST_LOGOUT		0x101B		// 退出登录请求命令
typedef struct
{
	DEF_CMD_HEADER	;
}STRUCT_REQ_LOGOUT_REQUEST ;


//------------------------------------------------------------------------
#define CMD_SET_AUDIO_VOL	0x1026			//设置AUdio音量参数
typedef struct
{
	DEF_CMD_HEADER ;
	HLE_U16		micVol ;			//设备麦克音量,0-100；
	HLE_U16		spkVol ;			//设备喇叭音量,0-100;
											/*（设备端音频API接口的音量设置区间可能不是0-100，
											客户端按0-100设置，设备端需要重新映射）*/
}STRUCT_SET_AUDIO_VOL_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER ;
	HLE_S32					echo ;				//-1：失败，0：成功
}STRUCT_SET_AUDIO_VOL_ECHO ;

//------------------------------------------------------------------------
#define CMD_SET_TIME_ZONE	0x1027			//设置时区(校时)命令
typedef struct
{
	float    timezone;			//时区(有个别城市的时间使用时区不是整数)
	HLE_S32  tzindex;        	//当前时区在时区表(保存所有时区)的索引[0-141]
	HLE_S8   daylight;			//此时区是否启用夏令时，0：不启用，1：启用
	HLE_S8   reserved[11];
}STRUCT_SET_TIME_ZONE_REQUEST;



#pragma pack()

#ifdef __cplusplus
}
#endif
#endif







