//// ReadWriteTester.cpp 
//// Author: Zheng.B.C
////	To test PPCS connection with a specified DID device, from an Internet Host
////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>  // uintptr_t

#ifdef LINUX
#include <unistd.h> 
#include <fcntl.h>
#include <pthread.h> 
#include <sys/types.h> 
#include <signal.h> 
#include <netinet/in.h>
#include <netdb.h> 
#include <net/if.h>
#include <sched.h>
#include <stdarg.h>
#include <dirent.h>
#include <arpa/inet.h> 
#endif
#if defined(WIN32DLL) || defined(WINDOWS)
#include <windows.h>
#include <direct.h>
#endif

#include "PPCS_API.h"
#include "PPCS_Error.h"
#include "media_server_signal_def.h"
#include "P2P_ReadWriteTester.h"





#define 	PKT_TEST

#if defined(WIN32DLL) || defined(WINDOWS)
typedef	 	DWORD					my_threadid;
typedef		HANDLE	 				my_Thread_mutex_t;
#define		my_Mutex_Lock(mutex) 	WaitForSingleObject(mutex, INFINITE) 
#define		my_Mutex_UnLock(mutex) 	ReleaseMutex(mutex) 
#define		my_Thread_exit(a)  		return(a)
#elif defined(LINUX) 
typedef 	pthread_t				my_threadid;
typedef		pthread_mutex_t  		my_Thread_mutex_t;
#define		my_Mutex_Lock(mutex)	pthread_mutex_lock(&mutex)
#define		my_Mutex_UnLock(mutex)	pthread_mutex_unlock(&mutex)
#define		my_Thread_exit(a)		pthread_exit(a) 		
#endif

typedef uintptr_t UINTp;

// show info 开关 -> 终端打印调试信息
static int g_ST_INFO_OPEN = 0;
// debug log 开关 -> 输出到本地log文件
static int g_DEBUG_LOG_FILE_OPEN = 0;
const char *LogFileName = "./ReadWriteTester.log";

#define CH_CMD				0	//信令传输通道（P2P）
#define CH_STREAM			1	//编码流传输通道（P2P）
#define CH_DATA				2	//其他数据传输通道（P2P）


#define SIZE_DID 			30
#define SIZE_INITSTRING 	256
int gSessionID = -99;

#define TEST_WRITE_SIZE 		1004  // (251 * 4)
#define TOTAL_WRITE_SIZE 		(4*1024*TEST_WRITE_SIZE)
#define TEST_NUMBER_OF_CHANNEL 	8
typedef struct 
{
	unsigned long TotalSize_Read;
	unsigned long TotalSize_Write;
	unsigned int Tick_Used_Read;
	unsigned int Tick_Used_Write;
} st_RW_Test_Info;
st_RW_Test_Info g_RW_Test_Info[TEST_NUMBER_OF_CHANNEL];

#define ST_TIME_USED	(int)(TimeEnd.TimeTick_mSec-TimeBegin.TimeTick_mSec)
typedef struct
{
	int Year;
	int Mon;
	int Day;
	int Week;
	int Hour;
	int Min;
	int Sec;
	int mSec;
	unsigned long TimeTick_mSec;
} st_Time_Info;

unsigned long my_getTickCount(void)
{
#if defined(WIN32DLL) || defined(WINDOWS) 
	return my_getTickCount();
#elif defined(LINUX)
	struct timeval current;
	gettimeofday(&current, NULL);
	return current.tv_sec*1000 + current.tv_usec/1000;
#endif
}

void my_GetCurrentTime(st_Time_Info *Time)
{
#if  defined(WINDOWS) || defined(WIN32DLL)
	SYSTEMTIME mTime = {0};
	GetLocalTime(&mTime);
	Time->Year = mTime.wYear;
	Time->Mon = mTime.wMonth;
	Time->Day = mTime.wDay;
	Time->Week = mTime.wDayOfWeek;
	Time->Hour = mTime.wHour;
	Time->Min = mTime.wMinute;
	Time->Sec = mTime.wSecond;
	Time->mSec = mTime.wMilliseconds;
	Time->TimeTick_mSec = my_getTickCount();
#elif defined(LINUX)
	struct timeval mTime;
	int ret = gettimeofday(&mTime, NULL);
	if (0 != ret)
	{
		printf("gettimeofday failed!! errno=%d\n", errno);
		memset(Time, 0, sizeof(st_Time_Info));
		return ;
	}
	//struct tm *ptm = localtime((const time_t *)&mTime.tv_sec); 
	struct tm st_tm = {0};
	struct tm *ptm = localtime_r((const time_t *)&mTime.tv_sec, &st_tm); 
	if (!ptm)
	{
		printf("localtime_r failed!!\n");
		memset(Time, 0, sizeof(st_Time_Info));
		Time->TimeTick_mSec = mTime.tv_sec*1000 + mTime.tv_usec/1000;
	}
	else
	{
		Time->Year = st_tm.tm_year+1900;
		Time->Mon = st_tm.tm_mon+1;
		Time->Day = st_tm.tm_mday;
		Time->Week = st_tm.tm_wday;
		Time->Hour = st_tm.tm_hour;
		Time->Min = st_tm.tm_min;
		Time->Sec = st_tm.tm_sec;
		Time->mSec = (int)(mTime.tv_usec/1000);
		Time->TimeTick_mSec = mTime.tv_sec*1000 + mTime.tv_usec/1000;
	}
#endif
}



void st_info(const char *format, ...) 
{
	//if (1 == g_ST_INFO_OPEN) 
	//{
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	//}
	if (1 == g_DEBUG_LOG_FILE_OPEN) 
	{
		FILE *pFile = fopen(LogFileName, "a");
		if (!pFile)
		{
			fprintf(stderr, "Error: Can not Open %s file!\n", LogFileName);
			return ;
		}
		va_list ap;
		va_start(ap, format);
		vfprintf(pFile, format, ap);
		va_end(ap);
		fclose(pFile);
	}
}
void st_debug(const char *format, ...) 
{
	if (1 == g_ST_INFO_OPEN) 
	{
		va_list ap;
		va_start(ap, format);
		vfprintf(stderr, format, ap);
		va_end(ap);
	}
	if (1 == g_DEBUG_LOG_FILE_OPEN) 
	{
		FILE *pFile = fopen(LogFileName, "a");
		if (!pFile)
		{
			fprintf(stderr, "Error: Can not Open %s file!\n", LogFileName);
			return ;
		}
		va_list ap;
		va_start(ap, format);
		vfprintf(pFile, format, ap);
		va_end(ap);
		fclose(pFile);
	}
}

void mSleep(UINT32 ms)
{
#if defined(WIN32DLL) || defined(WINDOWS)
	Sleep(ms);
#elif defined LINUX
	usleep(ms * 1000);
#endif
}

void error(const char *msg) 
{
	st_info(msg);
    perror(msg);
    exit(0);
}

const char *getP2PErrorCodeInfo(int err)
{
    if (0 < err) 
		return "NoError";
    switch (err)
    {
	case 0: return "ERROR_PPCS_SUCCESSFUL";
	case -1: return "ERROR_PPCS_NOT_INITIALIZED";
	case -2: return "ERROR_PPCS_ALREADY_INITIALIZED";
	case -3: return "ERROR_PPCS_TIME_OUT";
	case -4: return "ERROR_PPCS_INVALID_ID";
	case -5: return "ERROR_PPCS_INVALID_PARAMETER";
	case -6: return "ERROR_PPCS_DEVICE_NOT_ONLINE";
	case -7: return "ERROR_PPCS_FAIL_TO_RESOLVE_NAME";
	case -8: return "ERROR_PPCS_INVALID_PREFIX";
	case -9: return "ERROR_PPCS_ID_OUT_OF_DATE";
	case -10: return "ERROR_PPCS_NO_RELAY_SERVER_AVAILABLE";
	case -11: return "ERROR_PPCS_INVALID_SESSION_HANDLE";
	case -12: return "ERROR_PPCS_SESSION_CLOSED_REMOTE";
	case -13: return "ERROR_PPCS_SESSION_CLOSED_TIMEOUT";
	case -14: return "ERROR_PPCS_SESSION_CLOSED_CALLED";
	case -15: return "ERROR_PPCS_REMOTE_SITE_BUFFER_FULL";
	case -16: return "ERROR_PPCS_USER_LISTEN_BREAK";
	case -17: return "ERROR_PPCS_MAX_SESSION";
	case -18: return "ERROR_PPCS_UDP_PORT_BIND_FAILED";
	case -19: return "ERROR_PPCS_USER_CONNECT_BREAK";
	case -20: return "ERROR_PPCS_SESSION_CLOSED_INSUFFICIENT_MEMORY";
	case -21: return "ERROR_PPCS_INVALID_APILICENSE";
	case -22: return "ERROR_PPCS_FAIL_TO_CREATE_THREAD";
	default: return "Unknown, something is wrong!";
    }
}

void showErrorInfo(int ret)
{
	if (0 <= ret) 
	{
		return ;
	}
	switch (ret)
	{
	case ERROR_PPCS_NOT_INITIALIZED:
		st_info("API didn't initialized\n"); 
		break;		
	case ERROR_PPCS_TIME_OUT:
		st_info("Connection time out, probably the device is off line now\n"); 
		break;			
	case ERROR_PPCS_INVALID_ID:
		st_info("Invalid Device ID\n"); 
		break;
	case ERROR_PPCS_INVALID_PREFIX:
		st_info("Prefix of Device ID is not accepted by Server\n"); 
		break;
	case ERROR_PPCS_DEVICE_NOT_ONLINE:
		st_info("Device is not on line now, nor login in the past 5 minutes\n"); 
		break;
	case ERROR_PPCS_NO_RELAY_SERVER_AVAILABLE:
		st_info("No relay server can provide service\n"); 
		break;
	case ERROR_PPCS_SESSION_CLOSED_REMOTE:
		st_info("Session close remote.\n"); 
		break;
	case ERROR_PPCS_SESSION_CLOSED_TIMEOUT:
		st_info("Session close timeout.\n"); 
		break;
	case ERROR_PPCS_MAX_SESSION:
		st_info("Exceed max Session\n"); 
		break;
	case ERROR_PPCS_UDP_PORT_BIND_FAILED:
		st_info("The specified UDP port can not be binded\n"); 
		break;
	case ERROR_PPCS_USER_CONNECT_BREAK:
		st_info("connect break is called\n"); 
		break;
	default: st_info("%s\n", getP2PErrorCodeInfo(ret));
		break;
	} // switch	
}

#ifdef WIN32DLL
DWORD WINAPI ThreadWrite(void* arg)
#elif defined(LINUX)
void *ThreadWrite(void *arg)
#endif
{	
	//INT32 Channel = (INT32)((UINTp)arg);
	INT32 Channel = *(int*)arg;
	if (0 > Channel || 7 < Channel)
	{
		st_info("ThreadWrite - Channel=%d !!\n", Channel);
		my_Thread_exit(0);
	}
	UCHAR *Buffer = (UCHAR *)malloc(TEST_WRITE_SIZE+1);
	if (!Buffer) 
	{
		st_info("ThreadWrite Channel %d - Malloc failed!!\n", Channel);
		my_Thread_exit(0);
	}
	for (INT32 i = 0 ; i < TEST_WRITE_SIZE; i++) 
	{
		Buffer[i] = i%251; //0~250
	}
	Buffer[TEST_WRITE_SIZE] = '\0';
	st_info("ThreadWrite Channel %d running... \n", Channel);
	
	INT32 ret = 0;
	INT32 Check_ret = 0;
	ULONG TotalSize = 0;
	UINT32 WriteSize = 0;
	UINT32 tick = my_getTickCount();
	while (ERROR_PPCS_SUCCESSFUL == (Check_ret = PPCS_Check_Buffer(gSessionID, Channel, &WriteSize, NULL)))
	{
		if ((WriteSize < 256*1024) && (TotalSize < TOTAL_WRITE_SIZE))
		{
			ret = PPCS_Write(gSessionID, Channel, (CHAR*)Buffer, TEST_WRITE_SIZE);
			if (0 > ret)
			{
				if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
				{
					st_info("ThreadWrite CH=%d, ret=%d, Session Closed TimeOUT!!\n", Channel, ret);
				}				
				else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)
				{
					st_info("ThreadWrite CH=%d, ret=%d, Session Remote Close!!\n", Channel, ret);
				}				
				else 
				{
					st_info("ThreadWrite CH=%d, ret=%d %s\n", ret, getP2PErrorCodeInfo(ret));
				}				
				continue;
			}
			TotalSize += ret; // PPCS_Write return ret >=0: Number of byte wirten.
		}
		//When PPCS_Check_Buffer return WriteSize equals 0, all the data in this channel is sent out
		else if (0 == WriteSize) 
		{
			break;
		}			
		else 
		{
			mSleep(2);
		}			
	}
	tick = my_getTickCount() - tick;
	g_RW_Test_Info[Channel].Tick_Used_Write = tick;
	g_RW_Test_Info[Channel].TotalSize_Write = TotalSize;
		
	if (Buffer) free(Buffer);
	st_debug("ThreadWrite Channel %d Exit. TotalSize: %lu Byte (%.2f MByte), Time:%3d.%03d sec, %.3f KByte/sec\n", Channel, TotalSize, (double)TotalSize/(1024*1024), tick/1000, tick%1000, (0==tick)?0:((double)TotalSize/tick));
	
	my_Thread_exit(0);
}










#define _DID_ 				"RTOS-000236-STWDB"
#define APILICENSE 			"ATTWPQ"
#define CRCKEY 				"EasyView"
#define INITSTRING 			"EFGBFFBJKEJKGGJJEEGFFHELHHNNHONHGLFNBHCCAEJDLNLPDDAGCIOFGDLGJMLAAOMOKCDLOONOBICJJIMM "
#define WAKEUPKEY 			"1234567890ABCDEF"			
#define IP_LENGTH			16
#define TCP_PORT			12306
#define UDP_PORT			12305
#define SERVER_IP1			"112.74.108.149"
#define SERVER_IP2			"112.74.108.149"
#define SERVER_IP3			"112.74.108.149"

#if 0
INT32 main(INT32 argc, CHAR **argv)
{

	// 1. get P2P API Version
	UINT32 APIVersion = PPCS_GetAPIVersion();
	st_info("P2P API Version: %d.%d.%d.%d\n",
				(APIVersion & 0xFF000000)>>24, 
				(APIVersion & 0x00FF0000)>>16, 
				(APIVersion & 0x0000FF00)>>8, 
				(APIVersion & 0x000000FF)>>0);


	
	char DID[SIZE_DID] = {};
	char InitString[SIZE_INITSTRING];
	memset(DID, 0, sizeof(DID));
	memset(InitString, 0, sizeof(InitString));
	
	strcpy((char*)&DID, _DID_);
	strcpy((char*)&InitString,INITSTRING);

	st_debug("DID = %s\n", DID);
	st_debug("InitString = %s\n", InitString);


	// 2. P2P Initialize
	st_debug("PPCS_Initialize(%s) ...\n", InitString);
	INT32 ret = PPCS_Initialize((char *)InitString);
	st_debug("PPCS_Initialize done! ret=%d\n", ret);
	if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
	{
		printf("PPCS_Initialize failed!! ret=%d: %s\n", ret, getP2PErrorCodeInfo(ret));
		return 0;
	}
	
	// 3. Network Detect
	st_PPCS_NetInfo NetInfo;
	ret = PPCS_NetworkDetect(&NetInfo, 0);
	if (0 > ret) 
	{
		st_info("PPCS_NetworkDetect failed: ret=%d\n", ret);
	}	
	st_info("-------------- NetInfo: -------------------\n");
	st_info("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
	st_info("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
	st_info("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");
	switch (NetInfo.NAT_Type)
	{
	case 0: st_info("Local NAT Type         : Unknow\n"); break;
	case 1: st_info("Local NAT Type         : IP-Restricted Cone\n"); break;
	case 2: st_info("Local NAT Type         : Port-Restricted Cone\n"); break;
	case 3: st_info("Local NAT Type         : Symmetric\n"); break;
	}
	st_info("My Wan IP : %s\n", NetInfo.MyWanIP);
	st_info("My Lan IP : %s\n", NetInfo.MyLanIP);
	st_info("-------------------------------------------\n");
	
	// 4. Connect to Device. 
	st_info("PPCS_Connect(%s, 0x7E, 0)...\n", DID);
	gSessionID = PPCS_Connect(DID, 0x7E, 0);
	
	if (0 > gSessionID)
	{
		st_info("Connect failed: %d [%s]\n", gSessionID, getP2PErrorCodeInfo(gSessionID));
		ret = PPCS_DeInitialize();
		st_debug("PPCS_DeInitialize() done!!\n");
		return 0;
	}
	else
	{
		st_info("Connect Success!! gSessionID= %d.\n", gSessionID);
		st_PPCS_Session Sinfo;	
		if (ERROR_PPCS_SUCCESSFUL == PPCS_Check(gSessionID, &Sinfo))
		{
			st_info("----------- Session Ready: -%s -----------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			st_info("Socket : %d\n", Sinfo.Skt);
			st_info("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			st_info("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			st_info("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			st_info("Connection time : %d second before\n", Sinfo.ConnectTime);
			st_info("DID : %s\n", Sinfo.DID);
			st_info("I am %s\n", (Sinfo.bCorD == 0)? "Client":"Device");
			st_info("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			st_info("---------- End of Session info : ----------\n");
		}
	}


	
	// 5. do job ...
	// 填充信令数据结构
	cmd_header_t cmd_header = {0};
	S_GET_LIVING_OPEN_REQUEST cmd_open_living = {0};
	cmd_header.command = (HLE_U32)CMD_GET_LIVING_OPEN;
	cmd_header.head = (HLE_S32)HLE_MAGIC;
	cmd_header.type = 1;
	cmd_header.length = sizeof(S_GET_LIVING_OPEN_REQUEST)-sizeof(cmd_header_t);
	memcpy(&cmd_open_living,&cmd_header,sizeof(cmd_header));
	cmd_open_living.openAudio = 0;
	cmd_open_living.videoType = (HLE_U32)MAIN_STREAM;
		
	
	/*
	发送命令
	PPCS_write 发送数据可以区分不同channel，有专门的命令channel，有专门的码流channel，这样不容易混淆，造成数据错乱
	*/
	ret = PPCS_Write(gSessionID, CH_CMD, (char*)&cmd_open_living, sizeof(S_GET_LIVING_OPEN_REQUEST));
	if (0 > ret)
	{
		st_info("PPCS_Write: gSessionID=%d, CH=%d, ret=%d [%s]\n", gSessionID, CH_CMD, ret, getP2PErrorCodeInfo(ret));
		return -1;
	}
	else // Select the test options according to the Mode
	{	
		printf("PPCS_Write ok !\n");

	}

	//发送成功，等待接收码流数据
	int ReadSize = 1024*200;
	char* video_buffer = (char*)malloc(ReadSize);
	FILE* video_file = fopen("./video.h264","wb+");
	if(NULL == video_file)
	{
		printf("open file error!\n");
		return -1;
		
	}

	
	while(1)
	{
		memset(video_buffer,0,ReadSize);
		ret = PPCS_Read(gSessionID, CH_STREAM, (CHAR*)video_buffer, &ReadSize, 2000);//注意实际督导的字节数会放到ReadSize
		if(ret < 0)
		{
			printf("PPCS_Read error !\n");
			continue;
		}
		else //返回零为成功
		{
			printf("read frame size(%d)kb\n",ReadSize/1024);
			
		
		}
		
	}
		
	free(video_buffer);
	//退出
	fclose(video_file);
	ret = PPCS_Close(gSessionID);
	st_debug("PPCS_Close(%d) done!! ret=%d\n", gSessionID, ret);
	
	ret = PPCS_DeInitialize();
	st_debug("PPCS_DeInitialize() done!!\n");
	return 0;
}

#endif


/*******************************************************************************
*@ Description    :初始化实时流传输（P2P连接）
*@ Input          :
*@ Output         :
*@ Return         :成功：0；失败：-1
*@ attention      :
*******************************************************************************/
char* video_buffer = NULL; //编码帧缓存空间
unsigned int video_buffer_size = 1024*300;  

int living_stream_init(void)
{
	video_buffer = (char*)malloc(video_buffer_size);
	if(NULL == video_buffer)
	{
		printf("malloc failed !\n");
		return -1;
	}
	
// 1. get P2P API Version
	UINT32 APIVersion = PPCS_GetAPIVersion();
	st_info("P2P API Version: %d.%d.%d.%d\n",
				(APIVersion & 0xFF000000)>>24, 
				(APIVersion & 0x00FF0000)>>16, 
				(APIVersion & 0x0000FF00)>>8, 
				(APIVersion & 0x000000FF)>>0);


	
	char DID[SIZE_DID] = {};
	char InitString[SIZE_INITSTRING];
	memset(DID, 0, sizeof(DID));
	memset(InitString, 0, sizeof(InitString));
	
	strcpy((char*)&DID, _DID_);
	strcpy((char*)&InitString,INITSTRING);

	st_debug("DID = %s\n", DID);
	st_debug("InitString = %s\n", InitString);


	// 2. P2P Initialize
	st_debug("PPCS_Initialize(%s) ...\n", InitString);
	INT32 ret = PPCS_Initialize((char *)InitString);
	st_debug("PPCS_Initialize done! ret=%d\n", ret);
	if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
	{
		printf("PPCS_Initialize failed!! ret=%d: %s\n", ret, getP2PErrorCodeInfo(ret));
		return -1;
	}
	
	// 3. Network Detect
	st_PPCS_NetInfo NetInfo;
	ret = PPCS_NetworkDetect(&NetInfo, 0);
	if (0 > ret) 
	{
		st_info("PPCS_NetworkDetect failed: ret=%d\n", ret);
	}	
	st_info("-------------- NetInfo: -------------------\n");
	st_info("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
	st_info("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
	st_info("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");
	switch (NetInfo.NAT_Type)
	{
		case 0: st_info("Local NAT Type         : Unknow\n"); break;
		case 1: st_info("Local NAT Type         : IP-Restricted Cone\n"); break;
		case 2: st_info("Local NAT Type         : Port-Restricted Cone\n"); break;
		case 3: st_info("Local NAT Type         : Symmetric\n"); break;
	}
	
	st_info("My Wan IP : %s\n", NetInfo.MyWanIP);
	st_info("My Lan IP : %s\n", NetInfo.MyLanIP);
	st_info("-------------------------------------------\n");
	
	// 4. Connect to Device. 
	st_info("PPCS_Connect(%s, 0x7E, 0)...\n", DID);
	gSessionID = PPCS_Connect(DID, 0x7E, 0);
	if (0 > gSessionID)
	{
		st_info("Connect failed: %d [%s]\n", gSessionID, getP2PErrorCodeInfo(gSessionID));
		ret = PPCS_DeInitialize();
		st_debug("PPCS_DeInitialize() done!!\n");
		return -1;
	}
	else
	{
		st_info("Connect Success!! gSessionID= %d.\n", gSessionID);
		st_PPCS_Session Sinfo;	
		if (ERROR_PPCS_SUCCESSFUL == PPCS_Check(gSessionID, &Sinfo))
		{
			st_info("----------- Session Ready: -%s -----------\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			st_info("Socket : %d\n", Sinfo.Skt);
			st_info("Remote Addr : %s:%d\n", inet_ntoa(Sinfo.RemoteAddr.sin_addr),ntohs(Sinfo.RemoteAddr.sin_port));
			st_info("My Lan Addr : %s:%d\n", inet_ntoa(Sinfo.MyLocalAddr.sin_addr),ntohs(Sinfo.MyLocalAddr.sin_port));
			st_info("My Wan Addr : %s:%d\n", inet_ntoa(Sinfo.MyWanAddr.sin_addr),ntohs(Sinfo.MyWanAddr.sin_port));
			st_info("Connection time : %d second before\n", Sinfo.ConnectTime);
			st_info("DID : %s\n", Sinfo.DID);
			st_info("I am %s\n", (Sinfo.bCorD == 0)? "Client":"Device");
			st_info("Connection mode: %s\n", (Sinfo.bMode ==0)? "P2P":"RLY");
			st_info("---------- End of Session info : ----------\n");
		}
		else
		{
			printf("PPCS_Check failed !\n");
			return -1;
		}
		
	}


	return 0;

}

/*******************************************************************************
*@ Description    :开始实时流传输（发送命令获取实时编码帧数据）
*@ Input          :
*@ Output         :
*@ Return         :成功：0 ； 失败：-1
*@ attention      :
*******************************************************************************/
int living_stream_satrt(void)
{
	int ret;

	// 5. do job ...
	// 填充信令数据结构
	cmd_header_t cmd_header = {0};
	S_GET_LIVING_OPEN_REQUEST cmd_open_living = {0};
	cmd_header.command = (HLE_U32)CMD_GET_LIVING_OPEN;
	cmd_header.head = (HLE_S32)HLE_MAGIC;
	cmd_header.type = 1;
	cmd_header.length = sizeof(S_GET_LIVING_OPEN_REQUEST)-sizeof(cmd_header_t);
	memcpy(&cmd_open_living,&cmd_header,sizeof(cmd_header));
	cmd_open_living.openAudio = 0;
	cmd_open_living.videoType = (HLE_U32)MAIN_STREAM;
		
	
	/*
	发送命令
	PPCS_write 发送数据可以区分不同channel，有专门的命令channel，有专门的码流channel，这样不容易混淆，造成数据错乱
	*/
	ret = PPCS_Write(gSessionID, CH_CMD, (char*)&cmd_open_living, sizeof(S_GET_LIVING_OPEN_REQUEST));
	if (0 > ret)
	{
		st_info("PPCS_Write: gSessionID=%d, CH=%d, ret=%d [%s]\n", gSessionID, CH_CMD, ret, getP2PErrorCodeInfo(ret));
		return -1;
	}
	else // Select the test options according to the Mode
	{	
		printf("living_stream_satrt ok !\n");
		return 0;
	}

}

/*******************************************************************************
*@ Description    :获取一帧编码帧（音视频编码帧通用接口）
*@ Input          :
*@ Output         :<data>帧数据
					<len> 数据长度
*@ Return         :成功：读到的字节数 ； 失败：-1
*@ attention      :返回的数据是除去 HDR+INFO 的数据（ FRAME_HDR + IFRAME_INFO + DATA）
					只有 DATA 部分
*******************************************************************************/
int living_stream_get_frame_data(void**data,unsigned int* len)
{
	if(NULL == video_buffer)
	{
		printf("Illegal parameter!\n");
		return -1;
	}
		
	int ret;
	char*	read_buf = NULL;
	INT32 read_size;
	
	//unsigned int skip_len = 0;
	FRAME_HDR header;
	IFRAME_INFO I_info;
	PFRAME_INFO P_info;
	AFRAME_INFO A_info;
	
	/*---#step 1:先接收 header 部分------------------------------------------------------------*/
	read_buf = (char*)&header;
	read_size = sizeof(FRAME_HDR);
	ret = PPCS_Read(gSessionID, CH_STREAM, (CHAR*)read_buf, &read_size, 2000);//注意实际督导的字节数会放到ReadSize
	if (ERROR_PPCS_TIME_OUT == ret) 
	{
		ERROR_LOG("PPCS_Read timeout !!\n");
		return -1;
	}	
	else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) 
	{
		ERROR_LOG("Remote site call close!!\n");
		return -1;
	}
	else //返回零为成功
	{
		if(header.sync_code[0] == 0x00 && header.sync_code[1] == 0x00 && header.sync_code[2] == 0x01)
		{
			if(header.type == 0xF8) //关键帧
			{
				//skip_len = sizeof (FRAME_HDR) + sizeof (IFRAME_INFO);
				read_size = sizeof(IFRAME_INFO);
				read_buf = (char*)&I_info;

				
			}
			else if(header.type == 0xF9) //非关键帧
			{
				//skip_len = sizeof (FRAME_HDR) + sizeof (PFRAME_INFO);
				read_size = sizeof(PFRAME_INFO);
				read_buf = (char*)&P_info;
			}
			else if(header.type == 0xFA)//音频帧
			{
				//skip_len = sizeof (FRAME_HDR) + sizeof (AFRAME_INFO);
				read_size = sizeof(AFRAME_INFO);
				read_buf = (char*)&A_info;
			}
			else
			{
				perror("unknown header->type!\n");
				return -1;
			}

			//DEBUG_LOG("skip_len = %d\n",skip_len);

		}
		else
		{
			printf("error sync_code! header.sync_code[0] = %#x header.sync_code[1] = %#x header.sync_code[2] = %#x\n",
										header.sync_code[0] ,header.sync_code[1],header.sync_code[2]);
			//同步码出了问题，说明没有对应上帧头，异常机制,后续完善
			//往后边找，直到找到下一个 header 为止
			return -1;
		}
		
	}

	
	/*---#step 2:接收 FRAME_INFO   部分------------------------------------------------------------*/
	ret = PPCS_Read(gSessionID, CH_STREAM, (CHAR*)read_buf, &read_size, 2000);//注意实际督导的字节数会放到ReadSize
	if (ERROR_PPCS_TIME_OUT == ret) 
	{
		ERROR_LOG("PPCS_Read timeout !!\n");
		return -1;
	}	
	else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) 
	{
		ERROR_LOG("Remote site call close!!\n");
		return -1;
	}
	else //返回零为成功
	{
			read_buf = video_buffer;
			if(header.type == 0xF8) //关键帧
			{
				read_size = I_info.length;
			}
			else if(header.type == 0xF9) //非关键帧
			{
				read_size = P_info.length;
			}
			else if(header.type == 0xFA)//音频帧
			{
				read_size = A_info.length;
			}
			else
			{
				perror("unknown header->type!\n");
				return -1;
			}
	
	}

//		DEBUG_LOG("sizeof (FRAME_HDR) = %d sizeof (IFRAME_INFO)=%d sizeof (PFRAME_INFO) = %d\n",
//				sizeof (FRAME_HDR),sizeof (IFRAME_INFO),sizeof (PFRAME_INFO));

	/*---#step 3:接收 FAME DATA	 部分------------------------------------------------------------*/
	//DEBUG_LOG("FAME DATA read_size = %d\n",read_size);
	ret = PPCS_Read(gSessionID, CH_STREAM, (CHAR*)read_buf, &read_size, 3000);//注意实际督导的字节数会放到ReadSize
	if (ERROR_PPCS_TIME_OUT == ret) 
	{
		ERROR_LOG("PPCS_Read timeout !!\n");
		return -1;
	}	
	else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) 
	{
		ERROR_LOG("Remote site call close!!\n");
		return -1;
	}
	else //返回零为成功
	{
		/*---#查看接收通道里边还剩的数据量------------------------------------------------------------*/
		unsigned int Rsize = 0;
		ret = PPCS_Check_Buffer(gSessionID, CH_STREAM, NULL, &Rsize);
		if (ret < 0)
		{
			st_info("PPCS_Check_Buffer ret=%d %s\n", ret, getP2PErrorCodeInfo(ret));
			return -1;
		}
	
		DEBUG_LOG("read frame size(%d)(%d)KB  Rsize (%d)\n",read_size,read_size/1024,Rsize/1024);
		*data = read_buf;
		*len = read_size;
#if 0
		int m = 0;
		printf("read_buf: ");
		for(m = 0;m<20;m++)
		{
			printf("%#x ",read_buf[m]);
		}
		printf("\n");
#endif 	
		return read_size;
	}
	

}



int living_stream_exit(void)
{
	if(video_buffer) free(video_buffer);

	int ret = PPCS_Close(gSessionID);
	st_debug("PPCS_Close(%d) done!! ret=%d\n", gSessionID, ret);
	
	ret = PPCS_DeInitialize();
	st_debug("PPCS_DeInitialize() done!!\n");
	return 0;
}



