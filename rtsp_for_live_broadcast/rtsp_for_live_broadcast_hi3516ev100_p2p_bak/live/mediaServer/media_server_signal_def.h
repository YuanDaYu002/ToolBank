/*********************************************************************************
  *FileName: media_server_signal_def.h
  *Create Date: 2018/09/05
  *Description: 本文件为设备 media server 和服务器或手机APP端交互的信令协议文件。 
  *Others:  
  *History:  
**********************************************************************************/
#ifndef MEDIA_SERVER_SIGNAL_DEF_H
#define MEDIA_SERVER_SIGNAL_DEF_H

#ifdef __cplusplus
extern "C" {
#endif


#include "typeport.h"


#pragma pack( 4 )


/*
用户名长度定义，所有与用户名相关的操作均
使用该宏来定义用户名buf长度。方便后期调整
*/
#define USER_NAME_LEN		16

/*
用户名密码长度定义，同上
*/
#define USER_PASS_LEN		16

#define HLE_MAGIC 0xB1A75   //HLE ASCII : 76 72 69-->767269-->0xB1A75

/*=================================================================
					命令类型+状态枚举变量定义
==================================================================*/
typedef enum
{
	CMD_SET_CONT_NET_INFO 	= 0x1008,		//设置网络连接属性
	CMD_GET_NET_STATUS 		= 0x1018,		//获取NET连接状态
	CMD_SET_LOGIN 			= 0x1028,		//登陆请求
	CMD_SET_LOGOUT 			= 0x1038,		//退出登录
	CMD_SET_PASSWD 			= 0x1048,		//修改设备(用户)密码
	CMD_GET_DEV_INFO 		= 0x1058,		//读设备信息
	CMD_SET_DEV_PARA 		= 0x1068,		//设置系统参数
	CMD_GET_DEV_PARA 		= 0x1078,		//读取系统参数
	CMD_GET_SUPPORT_STREAM 	= 0x1187,		//获取支持的码流（不同分辨率）
	CMD_GET_LIVING_OPEN 	= 0x1088,		//打开实时流传输
	CMD_SET_LIVING_CLOSE 	= 0x1098,		//关闭实时流传输
	CMD_SET_REBOOT 			= 0x10A8,		//设备重启
	CMD_SET_UPDATE 			= 0x10B8,		//设备升级
	CMD_SET_RESTORE 		= 0x10C8,		//恢复出厂设置
	CMD_SET_TIME_ZONE 		= 0x10D8,		//设置时区(校时)
	CMD_SET_LIGHT 			= 0x10E8,		//设置LED灯参数
	CMD_SET_PIC_ATTRIBUTE 	= 0x1108,		//设置图像属性
	CMD_SET_AUDIO_VOL 		= 0x1118,		//设置AUdio音量参数
	CMD_SET_MD_SENSITIVE 	= 0x1128,		//设置移动侦测的灵敏度
	CMD_SET_MEG_PUSH_LEVEL 	= 0x1158,		//设置消息推送时间间隔
	//添加：
	CMD_SET_SPEAKER_START	= 0x1168,		//设置对讲开始
	CMD_SET_SPEAKER_STOP	= 0x1178		//设置对讲结束

	
}E_CMD_TYPE;

typedef enum
{
	AVIOTC_WIFIAPMODE_NULL				= 0x00,
	AVIOTC_WIFIAPMODE_MANAGED			= 0x01,
	AVIOTC_WIFIAPMODE_ADHOC				= 0x02,
}E_AP_MODE;

typedef enum
{
	AVIOTC_WIFIAPENC_INVALID			= 0x00, 
	AVIOTC_WIFIAPENC_NONE				= 0x01, //
	AVIOTC_WIFIAPENC_WEP				= 0x02, //WEP, for no password
	AVIOTC_WIFIAPENC_WPA_TKIP			= 0x03, 
	AVIOTC_WIFIAPENC_WPA_AES			= 0x04, 
	AVIOTC_WIFIAPENC_WPA2_TKIP			= 0x05, 
	AVIOTC_WIFIAPENC_WPA2_AES			= 0x06,

	AVIOTC_WIFIAPENC_WPA_PSK_TKIP  		= 0x07,
	AVIOTC_WIFIAPENC_WPA_PSK_AES   		= 0x08,
	AVIOTC_WIFIAPENC_WPA2_PSK_TKIP 		= 0x09,
	AVIOTC_WIFIAPENC_WPA2_PSK_AES  		= 0x0A,
}E_AP_ENCTYPE;

typedef enum 
{	
	MAIN_STREAM = 0,						//主码流(1920*1080)
	SECONDRY_STREAM = 1,					//次码流(960*544)
	LOWER_STREAM = 2,						//低清码流(480*272)
}E_Stream_Resolution;


/*=================================================================
					设备信息
==================================================================*/
typedef struct _device_info_t
{
	char 			name[64];				//设备名称
	char 			deveice_id[32];			//设备id(序列号)
	char 			firmware_version[64];   //固件版本号
	char 			firmware_type[32];		//固件类型
	char 			manufactur[64];			//制造商 
	char 			MAC[32];				//MAC地址（字符串形式）
	char 			ipv4[32];				//IP地址（字符串形式）	
}device_info_t;

/*=================================================================
					参数部分
==================================================================*/
typedef struct _Net_parameter_t
{
	int  	current_flag;					//当前使用的哪部分参数 0:无线,1:有线自动ip,2:有线手动ip
	/*---#无线部分------------------*/
	char 	ssid[32];						//WiFi ssid
	char 	password[32];					//if exist, WiFi password
	int  	mode;							//refer to E_AP_MODE
	int 	enctype;						//refer to E_AP_ENCTYPE
	/*---#有线部分------------------*/
	char 	ip[16];							//有线ip
	char 	mask[16];						//子网掩码
	char 	gateway[16];					//默认网关
	char 	dns_server[16];					//DNS服务器
}Net_parameter_t;

typedef struct _P2P_parameter_t
{

}P2P_parameter_t;

typedef struct _MD_parameter_t
{

}MD_parameter_t;

typedef struct _Acodec_parameter_t
{

}Acodec_parameter_t;

typedef struct _Vcodec_parameter_t
{

}Vcodec_parameter_t;


typedef struct _Light_parameter_t
{
	int onff;							//灯开关0关/1开
	int	brightness;						//亮度 0--100
	int mode;							//工作模式 0/1/2/3    //定时/自动/微亮/手动           //常亮/自动/自动+微亮
	int lux;							//lux光感值1--100
	int pir_time;						//感应时间 5--300秒
	int pir_len;						//感应长度20--60米
	int pir_alarm;						//pir报警开关 0/1
	int auto_start;						//自动模式---开始时间  
	int auto_end;							
	int shimmer_start;					//微亮模式---开始时间
	int shimmer_end;
	int timing_start;					//定时模式---开始时间
	int timing_end;
} Light_parameter_t;

typedef struct _system_parameter_t
{
	Net_parameter_t		Net_param;		//网络参数
	P2P_parameter_t 	P2P_param;		//P2P参数
	MD_parameter_t		MD_param;		//MD（移动侦测）参数（不接受修改）
	Acodec_parameter_t	Acodec_param; 	//音频编码参数（不接受修改）	
	Vcodec_parameter_t	Vcodec_param; 	//视频编码参数（不接受修改）
	Light_parameter_t	Light_param;	//LED灯参数
}system_parameter_t;


/*=================================================================
					CMD标准头（每个CMD都得添加）
==================================================================*/
typedef struct
{
	HLE_S32		head;					//命令头标志，统一填充HLE_MAGIC
	HLE_S32		length;					//数据长度,去除head
	HLE_S32		type;					//1:手机--->设备；2：设备--->手机
	HLE_U32		command;				//参考 E_CMD_TYPE
}cmd_header_t ;

//定义命令头的宏，之后定义的每条消息的消息头必须填充该头。
#define DEF_CMD_HEADER	cmd_header_t	header 


/*=================================================================
					信令部分---(手机--->设备)
新增定义请保持相同格式，变长数据也请用注释说明成员情况。
命令请求结构命名格式: S_XXX_XXX_REQUEST
回应结构命名格式: S_XXX_XXX_ECHO
==================================================================*/

/*---# 设置网络连接属性 ------------------------------------------------------------*/
//command = CMD_SET_CONT_NET_INFO;
typedef struct
{
	DEF_CMD_HEADER	;
	int 			flag;					//0:无线,1:有线自动ip,2:有线手动ip
	Net_parameter_t net_parameter;

}S_SET_CONT_NET_INFO_REQUEST;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32			echo ;					//-1：未连接，0：已连接
}S_SET_CONT_NET_INFO_ECHO ;

/*---# 获取NET连接状态 ------------------------------------------------------------*/
//command = CMD_GET_NET_STATUS;			
typedef struct
{
	DEF_CMD_HEADER	;
}S_GET_WIFISTATUS_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	Net_parameter_t	net_parameter ;				//获取设备net连接信息
}S_GET_WIFISTATUS_ECHO ;

/*---# 登陆请求 ------------------------------------------------------------*/
//command = CMD_SET_LOGIN;		
typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S8			name[USER_NAME_LEN] ;	//用户名
	HLE_S8			pwd[USER_PASS_LEN] ;	//密码
}S_REQ_LOGIN_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32	permit  ;						//权限：0：超级用户，1：普通用户
	HLE_S32 echo ;							//成功：0 ,
											//失败：-1:密码错误，-2：用户名错误
}S_REQ_LOGIN_ECHO ;

/*---# 修改设备(用户)密码 ------------------------------------------------------------*/
//command = CMD_SET_PASSWD;		
typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S8		name[USER_NAME_LEN] ;		//用户名
	HLE_S8		oldpwd[USER_PASS_LEN] ;		//旧密码
	HLE_S8		newpwd[USER_PASS_LEN] ;		//新密码
}S_REQ_SET_PASSWD_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32 echo ;							//成功：0 ,
											//失败：-1:旧密码错误，-2：用户名错误 ，-3：未知错误
}S_REQ_SET_PASSWD_ECHO;


/*---# 退出登录 ------------------------------------------------------------*/
//command =  CMD_SET_LOGOUT;	
typedef struct
{
	DEF_CMD_HEADER	;
}S_REQ_LOGOUT_REQUEST ;

/*---# 读设备信息 ------------------------------------------------------------*/
//command = CMD_GET_DEV_INFO;	
typedef struct
{
	DEF_CMD_HEADER	;
}S_READ_DEVINFO_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	device_info_t	devInfo ;				//返回设备信息
}S_READ_DEVINFO_ECHO ;

/*---# 设置系统参数	 ------------------------------------------------------------*/
//command = CMD_SET_DEV_PARA;		
typedef struct
{
	DEF_CMD_HEADER	;
	system_parameter_t	param ;				//系统参数
}S_SET_DEV_PARAM_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		   echo ;					//回应结果(0:成功,-1:失败)
}S_SET_DEV_PARAM_ECHO ;

/*---# 读取系统参数 ------------------------------------------------------------*/
//command = CMD_GET_DEV_PARA;			
typedef struct
{
	DEF_CMD_HEADER	;
}S_READ_DEV_PARAM_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	system_parameter_t	param ;				//系统参数
}S_READ_DEV_PARAM_ECHO ;

/*---#获取支持的码流（不同分辨率）------------------------------------------------*/
//command = CMD_GET_SUPPORT_STREAM;
typedef struct
{
	DEF_CMD_HEADER ;
}S_GET_SUPPORT_STREAM_REQUEST;

typedef struct
{
	DEF_CMD_HEADER ;
	char support_stream[3][12]; /*数组的第0、1、2个元素分别代表：主码流、次码流、低清码流
								eg:{{1,9,2,0,*,1,0,8,0},{0},{4,8,0,*,2,7,2}}
								如果某个一维数组元素都为0，则代表不支持*/
	char reserved;
}S_GET_SUPPORT_STREAM_ECHO;


/*---# 打开实时流传输 ------------------------------------------------------------*/
//command = CMD_GET_LIVING_OPEN;			
typedef struct
{
	DEF_CMD_HEADER ;
	HLE_U32 videoType;			//video 分辨率 参考 E_Stream_Resolution
	HLE_U32 openAudio;			//音频码流开关：0:关闭, 1:打开 
}S_GET_LIVING_OPEN_REQUEST;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		result ;					//成功：0
											//失败：
											//-1：请求视频流失败
											//-2: 请求音频流失败
}S_GET_LIVING_OPEN_ECHO ;

/*
码流帧格式:
    视频关键帧: FRAME_HDR + IFRAME_INFO + DATA
    视频非关键帧: FRAME_HDR + PFRAME_INFO + DATA
    音频帧: FRAME_HDR + AFRAME_INFO + DATA
关于帧类型和帧同步标志选取:
    前三个字节帧头同步码和H.264 NALU分割码相同，均为0x00 0x00 0x01
    第四个字节使用了H.264中不会使用到的0xF8-0xFF范围
 */


//音视频帧头标志

typedef struct
{
    HLE_U8 sync_code[3]; /*帧头同步码，固定为0x00,0x00,0x01*/
    HLE_U8 type; /*帧类型, 0xF8-视频关键帧，0xF9-视频非关键帧，0xFA-音频帧*/
} FRAME_HDR;

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

//非关键帧描述信息
typedef struct
{
    HLE_U64 pts_msec; //毫秒级时间戳，一直累加，溢出后自动回绕
    HLE_U32 length;
} PFRAME_INFO;

//音频帧描述信息
typedef struct
{
    HLE_U8 enc_type; //音频编码类型，具体见E_AENC_STANDARD
    HLE_U8 sample_rate; //采样频率，具体见E_AUDIO_SAMPLE_RATE
    HLE_U8 bit_width; //采样位宽，具体见E_AUDIO_BIT_WIDTH
    HLE_U8 sound_mode; //单声道还是立体声，具体见E_AUDIO_SOUND_MODE
    HLE_U32 length;
    HLE_U64 pts_msec;
} AFRAME_INFO;


/*---# 关闭实时流传输 ------------------------------------------------------------*/
//command = CMD_SET_LIVING_CLOSE;		
typedef struct
{
	DEF_CMD_HEADER ;

}S_CLOSE_LIVING_REQUEST;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32		result ;					//成功：0
											//失败：-1
		
}S_CLOSE_LIVING_ECHO ;


/*---# 设备重启 ------------------------------------------------------------*/
//command = CMD_SET_REBOOT;		
typedef struct
{
	DEF_CMD_HEADER;
	HLE_U32	headFlag1	;					//0x55555555
	HLE_U32	headFlag2	;					//0xaaaaaaaa
}S_SET_REBOOT_REQUEST ;

typedef struct
{
	DEF_CMD_HEADER	;
	HLE_S32 echo ;							//回应结果(0:成功,-1:失败)
}S_SET_REBOOT_ECHO ;

/*---# 设备升级 ------------------------------------------------------------*/
//command = CMD_SET_UPDATE;		
typedef struct
{
	DEF_CMD_HEADER;
	HLE_S8 packageVersion[16];				//线上升级包的最新版本号
	HLE_S8 URL[256];						//升级文件在网络中的位置信息（URL）
}S_SET_UPDATE_REQUEST ;

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
	HLE_U16	percent;						//进度百分比：0-100
	HLE_U16	status ;						/*升级状态：
												0：版本校验状态
												1：文件下载状态
												2: 文件校验状态
												3：擦除flash状态
												4：写flash状态
											*/
}S_SET_UPDATE_ECHO ;

/*---# 恢复出厂设置 ------------------------------------------------------------*/
//command = CMD_SET_RESTORE;		
typedef struct
{
	DEF_CMD_HEADER;
	HLE_U32	headFlag1	;					//0x55555555
	HLE_U32	headFlag2	;					//0xaaaaaaaa
	
}S_SET_RESTORE_REQUEST;

typedef struct
{
	DEF_CMD_HEADER;
	HLE_S32       echo;						//0:成功,-1:失败
}S_SET_RESTORE_ECHO ;

/*---# 设置时区(校时) ------------------------------------------------------------*/
//command = CMD_SET_TIME_ZONE;			
typedef struct
{
	DEF_CMD_HEADER;
	float    timezone;						//时区(有个别城市的时间使用时区不是整数)
	HLE_S32  tzindex;        				//当前时区在时区表(保存所有时区)的索引[0-141]
	HLE_S8   daylight;						//此时区是否启用夏令时，0：不启用，1：启用
	HLE_S8   reserved[11];
}S_SET_TIME_ZONE_REQUEST;

/*---# 设置图像属性 ------------------------------------------------------------*/
//command = CMD_SET_PIC_ATTRIBUTE;			
typedef struct 
{
	DEF_CMD_HEADER;
	int 	mirror; 						//镜像 0:enable,1:disable
	int 	flip; 							//翻转 0:enable,1:disable
} S_SET_PIC_ATTRIBUTE_REQUEST;

/*---# 设置AUdio音量参数 ------------------------------------------------------------*/
//command = CMD_SET_AUDIO_VOL;			
typedef struct
{
	DEF_CMD_HEADER ;
	HLE_U16		micVol ;					//设备麦克音量,0-100；
	HLE_U16		spkVol ;					//设备喇叭音量,0-100;
											/*（设备端音频API接口的音量设置区间可能不是0-100，
											app客户端按0-100设置，设备端需要重新映射）*/
}S_SET_AUDIO_VOL_REQUEST;

typedef struct
{
	DEF_CMD_HEADER ;
	HLE_S32					echo ;			//-1：失败，0：成功
}S_SET_AUDIO_VOL_ECHO ;

/*---# 设置移动侦测的灵敏度 ------------------------------------------------------------*/
//command = CMD_SET_MD_SENSITIVE;		
typedef struct
{
	DEF_CMD_HEADER;
	unsigned int sensitivity; 				// 0(Disabled) ~ 100(MAX):
											// index		sensitivity value
											// 0			0
											// 1			25
											// 2			50
											// 3			75
											// 4			100
}S_SET_MD_SENSITIVE_REQUEST;

typedef struct
{
	DEF_CMD_HEADER;
	int echo;								//0:设置成功，1：设置失败
}S_SET_MD_SENSITIVE_ECHO;


/*---# 设置LED灯参数 ------------------------------------------------------------*/
//command = CMD_SET_LIGHT;	
typedef struct 
{
	DEF_CMD_HEADER;
	Light_parameter_t  Light_parameter;  //可考虑只开放一部分数据
}S_SET_LIGHT_REQUEST;
typedef struct 
{
	DEF_CMD_HEADER;
	int echo;								//0:设置成功，1：设置失败
}S_SET_LIGHT_ECHO;

/*---# 设置消息推送时间间隔 ------------------------------------------------------------*/
//command = CMD_SET_MEG_PUSH_LEVEL;		
typedef struct
{
	DEF_CMD_HEADER;
	int time_interval; //0 关闭（不推送），1  没有限制，2 -1分钟，3- 3分钟，3- 5分钟，4- 10分钟，5- 30分钟
}S_SET_MSG_PUSH_LEVEL_REQUEST;

typedef struct
{
	DEF_CMD_HEADER;
	int ret;		//0:设置成功，1：设置失败
}S_SET_MSG_PUSH_LEVEL_ECHO;






/*=================================================================
					信令部分---(设备--->手机)
新增定义请保持相同格式，变长数据也请用注释说明成员情况。
 命令请求结构命名格式: S_XXX_XXX_INFO
 回应结构命名格式: 推送无回应
==================================================================*/
typedef struct
{
	DEF_CMD_HEADER;
	char index_file_path[128];		//索引文件(在云端)路径（包含视频文件）
	char pic_file_path[128];		//抓拍图片(在云端)路径
}S_PUSH_MD_MSG_INFO;






#pragma pack()

#ifdef __cplusplus
}
#endif
#endif











