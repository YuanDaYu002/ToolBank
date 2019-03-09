/*********************************************************************************
  *FileName: media_server_p2p.c
  *Create Date: 2018/09/06
  *Description: 本文件是p2p函数定义文件，基于shangyun demo的ListenTester.cpp进行修改。 
  *Others:  
  *History:  
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdint.h>  // uintptr_t


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
#include <time.h>

#include "PPCS_API.h"
#include "PPCS_Error.h"
#include "PPCS_Type.h"
#include "media_server_p2p.h"
#include "media_server_interface.h"
#include "media_server_signal_def.h"
#include "media_server_signal_parse.h"



#include "typeport.h"


#define 	PKT_TEST


typedef 	pthread_t				my_threadid;
typedef		pthread_mutex_t  		my_Thread_mutex_t;
#define		my_Mutex_Lock(mutex)	pthread_mutex_lock(&mutex)
#define		my_Mutex_UnLock(mutex)	pthread_mutex_unlock(&mutex)
#define		my_Mutex_Close(mutex) 	pthread_mutex_destroy(&mutex)
#define		my_SocketClose(skt)		close(skt)


#define		my_Thread_exit(a)		pthread_exit(a) 



typedef uintptr_t UINTp;

// show info 开关 -> 终端打印调试信息
static HLE_S32 g_ST_INFO_OPEN = ST_INFO_OPEN;
// debug log 开关 -> 输出到本地log文件
static HLE_S32 g_DEBUG_LOG_FILE_OPEN = 0;
const HLE_S8 LogFileName[] = "./ListenTester.log";

volatile HLE_S8 P2P_status;			//P2Pmedia server的状态


//// liteOS TCP packet Send to Device: 
////#define WAKEUP_CODE {0x98,0x3b,0x16,0xf8,0xf3,0x9c}

HLE_S32 gSessionID = -99;
// LoginStatus_Check Thread exit flags: 0->keep running,1->exit			 
HLE_S8 gThread_Exit = 1;		
// LoginStatus_Check Thread Running flags: thread exit->0, thread running->1	
HLE_S8 gThread_bRunning = 0;		

// define for Read/Write test mode
#define TEST_WRITE_SIZE 		1004  // (251 * 4)
#define TOTAL_WRITE_SIZE 		(4*1024*TEST_WRITE_SIZE)
#define TEST_NUMBER_OF_CHANNEL 	8
typedef struct 
{
	unsigned long TotalSize_Read;
	unsigned long TotalSize_Write;
	HLE_U32 Tick_Used_Read;
	HLE_U32 Tick_Used_Write;
} st_RW_Test_Info;
st_RW_Test_Info g_RW_Test_Info[TEST_NUMBER_OF_CHANNEL];

typedef struct 
{
	HLE_S32 Year;
	HLE_S32 Mon;
	HLE_S32 Day;
	HLE_S32 Week;
	HLE_S32 Hour;
	HLE_S32 Min;
	HLE_S32 Sec;
	HLE_S32 mSec;
	unsigned long TimeTick_mSec;
} st_Time_Info;

#define ST_TIME_USED	(HLE_S32)(TimeEnd.TimeTick_mSec-TimeBegin.TimeTick_mSec)

#define ST_TIME_YEAR 	(HLE_S32)(TimeEnd.Year-TimeBegin.Year)
#define ST_TIME_MON 	(HLE_S32)(TimeEnd.Mon-TimeBegin.Mon)
#define ST_TIME_DAY 	(HLE_S32)(TimeEnd.Day-TimeBegin.Day)
#define ST_TIME_HOUR 	(HLE_S32)(TimeEnd.Hour-TimeBegin.Hour)
#define ST_TIME_MIN		(HLE_S32)(TimeEnd.Min-TimeBegin.Min)
#define ST_TIME_SEC  	ST_TIME_USED



void st_info(const HLE_S8 *format, ...) 
{
	va_list ap;
	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);
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

void st_debug(const HLE_S8 *format, ...) 
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

void my_GetCurrentTime(st_Time_Info *Time)
{

	struct timeval mTime;
	HLE_S32 ret = gettimeofday(&mTime, NULL);
	if (0 != ret)
	{
		st_info("gettimeofday failed!! errno=%d\n", errno);
		memset(Time, 0, sizeof(st_Time_Info));
		return ;
	}
	//struct tm *ptm = localtime((const time_t *)&mTime.tv_sec); 
	struct tm st_tm;
	memset(&st_tm,0,sizeof(st_tm));
	struct tm *ptm = localtime_r((const time_t *)&mTime.tv_sec, &st_tm); 
	if (!ptm)
	{
		st_info("localtime_r failed!!\n");
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
		Time->mSec = (HLE_S32)(mTime.tv_usec/1000);
		Time->TimeTick_mSec = mTime.tv_sec*1000 + mTime.tv_usec/1000;
	}

}

/*
unsigned long getTickCount(void)
{

	struct timeval current;
	gettimeofday(&current, NULL);
	return current.tv_sec*1000 + current.tv_usec/1000;

}
*/

void mSleep(UINT32 ms)
{

	usleep(ms * 1000);
}

void error(const HLE_S8 *msg) 
{
	st_info(msg);
    perror(msg);
    exit(0);
}

const HLE_S8 *getP2PErrorCodeInfo(HLE_S32 err)
{
    if (0 < err) 
	{
		return "NoError";
	}
    switch (err)
    {
        case 0: return "ERROR_P2P_SUCCESSFUL";
        case -1: return "ERROR_P2P_NOT_INITIALIZED";
        case -2: return "ERROR_P2P_ALREADY_INITIALIZED";
        case -3: return "ERROR_P2P_TIME_OUT";
        case -4: return "ERROR_P2P_INVALID_ID";
        case -5: return "ERROR_P2P_INVALID_PARAMETER";
        case -6: return "ERROR_P2P_DEVICE_NOT_ONLINE";
        case -7: return "ERROR_P2P_FAIL_TO_RESOLVE_NAME";
        case -8: return "ERROR_P2P_INVALID_PREFIX";
        case -9: return "ERROR_P2P_ID_OUT_OF_DATE";
        case -10: return "ERROR_P2P_NO_RELAY_SERVER_AVAILABLE";
        case -11: return "ERROR_P2P_INVALID_SESSION_HANDLE";
        case -12: return "ERROR_P2P_SESSION_CLOSED_REMOTE";
        case -13: return "ERROR_P2P_SESSION_CLOSED_TIMEOUT";
        case -14: return "ERROR_P2P_SESSION_CLOSED_CALLED";
        case -15: return "ERROR_P2P_REMOTE_SITE_BUFFER_FULL";
        case -16: return "ERROR_P2P_USER_LISTEN_BREAK";
        case -17: return "ERROR_P2P_MAX_SESSION";
        case -18: return "ERROR_P2P_UDP_PORT_BIND_FAILED";
        case -19: return "ERROR_P2P_USER_CONNECT_BREAK";
        case -20: return "ERROR_P2P_SESSION_CLOSED_INSUFFICIENT_MEMORY";
        case -21: return "ERROR_P2P_INVALID_APILICENSE";
        case -22: return "ERROR_P2P_FAIL_TO_CREATE_THREAD";
        default: return "Unknown, something is wrong!";
    }
}

void showErrorInfo(HLE_S32 ret)
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
		st_info("Listen time out, No client connect me !!\n"); 
		break;			
	case ERROR_PPCS_INVALID_ID:
		st_info("Invalid Device ID !!\n"); 
		break;
	case ERROR_PPCS_INVALID_PREFIX:
		st_info("Prefix of Device ID is not accepted by Server !!\n"); 
		break;
	case ERROR_PPCS_UDP_PORT_BIND_FAILED:
		st_info("The specified UDP port can not be binded !!\n"); 
		break;
	case ERROR_PPCS_MAX_SESSION:
		st_info("Exceed max session !!\n"); 
		break;
	case ERROR_PPCS_USER_LISTEN_BREAK:
		st_info("Listen break is called !!\n"); 
		break;
	case ERROR_PPCS_INVALID_APILICENSE:
		st_info("API License code is not correct !!\n");
		break;
	case ERROR_PPCS_FAIL_TO_CREATE_THREAD:
		st_info("Fail tO Create Thread !!\n");
		break;
	default: st_info("%s\n", getP2PErrorCodeInfo(ret));
		break;
	}
}

void showNetwork(st_PPCS_NetInfo NetInfo)
{
	st_info("-------------- NetInfo: -------------------\n");
	st_info("Internet Reachable     : %s\n", (NetInfo.bFlagInternet == 1) ? "YES":"NO");
	st_info("P2P Server IP resolved : %s\n", (NetInfo.bFlagHostResolved == 1) ? "YES":"NO");
	st_info("P2P Server Hello Ack   : %s\n", (NetInfo.bFlagServerHello == 1) ? "YES":"NO");
	switch(NetInfo.NAT_Type)
	{
	case 0: st_info("Local NAT Type         : Unknow\n"); break;
	case 1: st_info("Local NAT Type         : IP-Restricted Cone\n"); break;
	case 2: st_info("Local NAT Type         : Port-Restricted Cone\n"); break;
	case 3: st_info("Local NAT Type         : Symmetric\n"); break;
	}
	st_info("My Wan IP : %s\n", NetInfo.MyWanIP);
	st_info("My Lan IP : %s\n", NetInfo.MyLanIP);
	st_info("-------------------------------------------\n");
}

HLE_S32 iPN_StringEnc(const HLE_S8 *keystr, const HLE_S8 *src, HLE_S8 *dest, HLE_U32 maxsize)
{
	HLE_S32 Key[17] = {0};
	HLE_U32 i;
	HLE_U32 s, v;
	if (maxsize < strlen(src) * 2 + 3)
	{
		return -1;
	}
	for (i = 0 ; i < 16; i++)
	{
		Key[i] = keystr[i];
	}
	srand((HLE_U32)time(NULL));
	s = abs(rand() % 256);
	memset(dest, 0, maxsize);
	dest[0] = 'A' + ((s & 0xF0) >> 4);
	dest[1] = 'a' + (s & 0x0F);
	for (i = 0; i < strlen(src); i++)
	{
		v = s ^ Key[(i + s * (s % 23))% 16] ^ src[i];
		dest[2 * i + 2] = 'A' + ((v & 0xF0) >> 4);
		dest[2 * i + 3] = 'a' + (v & 0x0F);
		s = v;
	}
	return 0;
}

HLE_S32 iPN_StringDnc(const HLE_S8 *keystr, const HLE_S8 *src, HLE_S8 *dest, HLE_U32 maxsize)
{
	HLE_S32 Key[17] = {0};
	HLE_U32 i;
	HLE_U32 s, v;
	if ((maxsize < strlen(src) / 2) || (strlen(src) % 2 == 1))
	{
		return -1;
	}
	for (i = 0 ; i < 16; i++)
	{
		Key[i] = keystr[i];
	}	
	memset(dest, 0, maxsize);
	s = ((src[0] - 'A') << 4) + (src[1] - 'a');
	for (i = 0; i < strlen(src) / 2 - 1; i++)
	{
		v = ((src[i * 2 + 2] - 'A') << 4) + (src[i * 2 + 3] - 'a');
		dest[i] = v ^ Key[(i + s * (s % 23))% 16] ^ s;
		if (dest[i] > 127 || dest[i] < 32) 
		{
			return -1; // not a valid character string
		}	
		s = v;
	}
	return 0;
}

//检查设备的登录状态（是否登录服务器）
void *Thread_LoginStatus_Check(void *arg)
{

	pthread_detach(pthread_self());	
	gThread_bRunning = 1;
	DEBUG_LOG("Thread_LoginStatus_Check start...\n");
	HLE_S32 i = 0;
	HLE_S8 LoginStatus = 0;
	HLE_S8 StatusFlags = -99;
	while (0 == gThread_Exit)
	{
		mSleep(1000);
		
		if (++i % 60 != 10) //起来后第10秒检查一次设备的登录状态，后边是每一分钟才检查一次状态
		{
			
			continue;
		}	
		DEBUG_LOG("PPCS_LoginStatus_Check!\n");
		if (0 == gThread_Exit)
		{
			if (ERROR_PPCS_SUCCESSFUL == PPCS_LoginStatus_Check(&LoginStatus))
			{
				if (StatusFlags != LoginStatus)
				{
					StatusFlags = LoginStatus;
					if (1 == LoginStatus) 
					{
						st_info("Got Server Response!!\n");
					}						
					else 
					{
						P2P_status = offline;
						st_info("No Server Response!!!\n");
					}						
				}
			}
		}			
	}
	gThread_bRunning = 0;
	DEBUG_LOG("Thread_LoginStatus_Check exit!\n");
	my_Thread_exit(0);
}

void CreateThread_LoginStatus_Check(void)
{
	gThread_Exit = 0;


	pthread_t threadID_LoginStatus_Check;
	HLE_S32 err = pthread_create(&threadID_LoginStatus_Check, NULL, &Thread_LoginStatus_Check, NULL);
	if (0 != err) 
	{
		error("create Thread LoginStatus_Check failed");
	}
	

}


void *ThreadWrite(void *arg)
{	
	INT32 Channel = (INT32)((UINTp)arg);
	INT32 i = 0;
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
	for (i = 0 ; i < TEST_WRITE_SIZE; i++) 
	{
		Buffer[i] = i%251; //0~250
	}
	Buffer[TEST_WRITE_SIZE] = '\0';
	st_info("ThreadWrite Channel %d running... \n", Channel);
	
	INT32 ret = 0;
	INT32 Check_ret = 0;
	ULONG TotalSize = 0;
	UINT32 WriteSize = 0;
	UINT32 tick = getTickCount();
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
	tick = getTickCount() - tick;
	g_RW_Test_Info[Channel].Tick_Used_Write = tick;
	g_RW_Test_Info[Channel].TotalSize_Write = TotalSize;
		
	if (Buffer) free(Buffer);
	DEBUG_LOG("ThreadWrite Channel %d Exit. TotalSize: %lu Byte (%.2f MByte), Time:%3d.%03d sec, %.3f KByte/sec\n", Channel, TotalSize, (double)TotalSize/(1024*1024), tick/1000, tick%1000, (0==tick)?0:((double)TotalSize/tick));
	
	my_Thread_exit(0);
}


void *ThreadRead(void *arg)
{
	INT32 Channel = (INT32)((UINTp)arg);
	if (0 > Channel || 7 < Channel)
	{
		st_info("ThreadRead - Channel=%d !!\n", Channel);
		my_Thread_exit(0);
	}
	ULONG TotalSize = 0;
	HLE_S32 timeout_ms = 200;
	st_info("ThreadRead  Channel %d running... \n", Channel);
	
	UINT32 tick = getTickCount();
	while (1)
	{
		UCHAR zz = 0;
		INT32 ReadSize = 1;
		INT32 ret = PPCS_Read(gSessionID, Channel, (HLE_S8 *)&zz, &ReadSize, timeout_ms);
		//st_info("PPCS_Read ret=%d, CH=%d, ReadSize=%d Byte, TotalSize=%lu Byte, zz=%d\n", ret, Channel, ReadSize, TotalSize, zz);
		
		if ((ret < 0) && (ret != ERROR_PPCS_TIME_OUT))
		{
			if (TOTAL_WRITE_SIZE == TotalSize) 
			{
				break;
			}		
			st_info("PPCS_Read ret=%d, CH=%d, ReadSize=%d Byte, TotalSize=%lu Byte\n", ret, Channel, ReadSize, TotalSize);
			break;
		}
		if (ReadSize && (TotalSize%251) != zz)//TotalSize%251: 0~250, zz: 0~250
		{
			st_info("\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ret=%d, CH=%d Error!! ReadSize=%d, TotalSize=%lu, zz=%d\n", ret, Channel, ReadSize, TotalSize, zz);
			break;
		} 
		else if (TotalSize%(1*1024*1024) == 1*1024*1024-1) 
		{	
			st_info("%d", Channel);
			setbuf(stdout, NULL);
		}
		TotalSize += ReadSize;
		if (TOTAL_WRITE_SIZE == TotalSize) 
		{
			break;
		}			
	}
	tick = getTickCount() - tick;
	g_RW_Test_Info[Channel].Tick_Used_Read = tick;
	g_RW_Test_Info[Channel].TotalSize_Read = TotalSize;
	
	DEBUG_LOG("ThreadRead  Channel %d Exit - TotalSize: %lu Byte (%.2f MByte), Time:%3d.%03d sec, %.3f KByte/sec\n", Channel, TotalSize, (double)TotalSize/(1024*1024), tick/1000, tick%1000, (0==tick)?0:((double)TotalSize/tick));
	
	my_Thread_exit(0);
}

HLE_S32 RW_Test(void)
{	
	INT32 i = 0;
	pthread_t ThreadWriteID[TEST_NUMBER_OF_CHANNEL];
	pthread_t ThreadReadID[TEST_NUMBER_OF_CHANNEL];
	
	memset(g_RW_Test_Info, 0, sizeof(g_RW_Test_Info));
	for (i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
	{

		if (0 != pthread_create(&ThreadWriteID[i], NULL, &ThreadWrite, (void *)&i)) 
		{
			error("create ThreadWrite failed");
		}			
		if (0 != pthread_create(&ThreadReadID[i], NULL, &ThreadRead, (void *)&i)) 
		{
			error("create ThreadRead failed");
		}			


		mSleep(10);
	}

	
	for (i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
	{	
	
		pthread_join(ThreadReadID[i], NULL);
		pthread_join(ThreadWriteID[i], NULL);

	}

	printf("\n");
	
	// show transmission information for each channel
	UINT32 tick = 0;
	ULONG TotalSize = 0;
	float speed_Write = 0;
    float speed_Read = 0;
    float Total_Speed_Read = 0;
    float Total_Speed_Write = 0;
	for (i = 0; i < TEST_NUMBER_OF_CHANNEL; i++)
	{
		// read info
		tick = g_RW_Test_Info[i].Tick_Used_Read;
		TotalSize = g_RW_Test_Info[i].TotalSize_Read;
		speed_Read = (0==tick)?0:((double)TotalSize/tick);
		Total_Speed_Read += speed_Read;
		st_info("ThreadRead  Channel %d Done - TotalSize: %lu Byte (%.2f MByte), Time:%3d.%03d sec, %.3f KByte/sec\n", i, TotalSize, (double)TotalSize/(1024*1024), tick/1000, tick%1000, speed_Read);
		
		// write info
		tick = g_RW_Test_Info[i].Tick_Used_Write;
		TotalSize = g_RW_Test_Info[i].TotalSize_Write;
		speed_Write = (0==tick)?0:((double)TotalSize/tick);
		Total_Speed_Write += speed_Write; 
		st_info("ThreadWrite Channel %d Done - TotalSize: %lu Byte (%.2f MByte), Time:%3d.%03d sec, %.3f KByte/sec\n", i, TotalSize, (double)TotalSize/(1024*1024), tick/1000, tick%1000, speed_Write);
		
		setbuf(stdout, NULL);
	}
	st_info("Total_Speed_Read  %.3f KByte/sec\n", Total_Speed_Read);
    st_info("Total_Speed_Write %.3f KByte/sec\n", Total_Speed_Write);
	
	return 0;
}

HLE_S32 ft_Test(void)
{
	const HLE_S8 fileName[] = "1.7z";
	FILE *fp = fopen(fileName, "rb");
	if (!fp)
	{
		st_info("***Error: failed to open file: %s\n", fileName);
		return -1;
	}
	
	INT32 ret = -1;
	UINT32 wsize = 0;
	unsigned long SizeCounter = 0;
	HLE_U32 tick = getTickCount();
	while (!feof(fp))
	{				
		ret = PPCS_Check_Buffer(gSessionID, CH_DATA, &wsize, NULL);
		if (0 > ret)
		{
			st_info("PPCS_Check_Buffer ret=%d %s\n", ret, getP2PErrorCodeInfo(ret));
			break;
		}
		if (wsize > 256*1024) 
		{
			mSleep(5);
			continue;
		}
		if (wsize > 128*1024) 
		{
			mSleep(2);
			continue;
		}
		
		CHAR buf[1024];
		memset(buf, 0, sizeof(buf));
		INT32 DataSize = (INT32)fread(buf, 1, sizeof(buf), fp);
		if (0 == DataSize) 
		{
			st_info("ft_Test - fread failed: file DataSize=0, No Data!!\n");
			continue;
		}
		
		ret = PPCS_Write(gSessionID, CH_DATA, buf, DataSize);
		if (0 > ret)
		{
			if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
			{
				st_info("Session Closed TimeOUT!!\n");
			}				
			else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)
			{
				st_info("Session Remote Close!!\n");
			}				
			else 
			{
				st_info("PPCS_Write ret=%d %s\n", ret, getP2PErrorCodeInfo(ret));
			}				
			break;
		}
		SizeCounter += ret;
		if (SizeCounter % (1024*1024) == 0) 
		{
			st_info("*");
			setbuf(stdout, NULL);
		}	
	}
	while (ERROR_PPCS_SUCCESSFUL == PPCS_Check_Buffer(gSessionID, CH_DATA, &wsize, NULL))
	{
		//st_info("gSessionID=%d, CH=%d, wsize=%d\n", gSessionID, CH_DATA, wsize);
		//setbuf(stdout, NULL);
		if (0 == wsize) 
		{
			break;
		} 			
		else 
		{
			mSleep(1);
		}			
	}
	tick = getTickCount() - tick;
	
	st_info("\nFile Transfer done!! Send Size=%lu Byte, Time:%3d.%03d sec, %4lu KByte/sec\n", SizeCounter, tick/1000,tick%1000, (0==tick)?0:(SizeCounter/tick));
	
	fclose(fp);
	return 0;	
}

#ifdef PKT_TEST
HLE_S32 pkt_Test(void)
{
	INT32 i = 0;

	for (i = 0 ; i < 5000; i++)
	{
		HLE_S8 PktBuf[1024];
		memset(PktBuf, (UCHAR)(i % 100), sizeof(PktBuf));// data: 0~99
		
		HLE_S32 ret = PPCS_PktSend(gSessionID, CH_DATA, PktBuf, sizeof(PktBuf));
		
		DEBUG_LOG("PPCS_PktSend: ret=%d, session=%d, channel=%d, data=%d..., size=%lu\n", ret, gSessionID, CH_DATA, PktBuf[0], sizeof(PktBuf));
		
		if (0 > ret)
		{
			if (ERROR_PPCS_SESSION_CLOSED_TIMEOUT == ret)
			{
				st_info("Session Closed TimeOUT!!\n");
			}				
			else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret)
			{
				st_info("Session Remote Close!!\n");
			}			
			else 
			{
				st_info("PPCS_PktSend ret=%d %s\n", ret, getP2PErrorCodeInfo(ret));
			}				
			break;
		}
		if (i%100 == 99) 
		{
			st_info("----->Send %d packets. (1 packets=%u Byte)\n", i+1, (unsigned)sizeof(PktBuf));
		}		
		mSleep(5); 
	}
	return 0;
}
#endif

HLE_S32 Call_SendCMD(const HLE_S32 skt, const HLE_S8 *CMD, const HLE_U16 SizeOfCMD, struct sockaddr_in Addr, const HLE_S32 index)
{
	if (0 > skt)
	{
		ERROR_LOG("Invalid skt!!\n");
		return skt;
	}
	if (!CMD || 0 == SizeOfCMD)
	{
		ERROR_LOG("Invalid CMD!!\n");
		return -1;
	}
	//HLE_S8 dest[30];
	//memset(dest, 0, sizeof(dest));
	//st_debug("write CMD to %s ...", inet_ntop(Addr.sin_family, (HLE_S8 *)&Addr.sin_addr.s_addr, dest, sizeof(dest)));
	DEBUG_LOG("write CMD to %s ...\n", inet_ntoa(Addr.sin_addr));

	
	HLE_S32 size = write(skt, CMD, SizeOfCMD);

	if (size <= 0) 
	{
		ERROR_LOG("ERROR, writing to socket ");
		my_SocketClose(skt);
		DEBUG_LOG("close socket: %d\n", skt);
	}
	else 
	{
		st_info("#");
		setbuf(stdout, NULL);
		st_debug("write data(%u Byte):%s\n", size, CMD);
	}
	return size;
}


/***********************************************************************************************
*函数名 ：	   Call_P2P_Listen 
*功能描述 ：监听等待客户端连接。
*			此函数是个阻塞的函数，正常返回是唤醒服务器唤醒或者超时，此外除非异常情况发生
*			listen超时时间：10min
*参数 ： 	   Did:设备id（尚云定义的，非本公司设备id，属于P2P初始化参数）
*		   APILicense：连接密匙（尚云提供）
*返回值 ：
*		成功：>= 0的 SessionID
*		失败：<     0的值
*	
***********************************************************************************************/

HLE_S32 Call_P2P_Listen(const HLE_S8 *Did, const HLE_S8 *APILicense)
{

	st_Time_Info TimeBegin, TimeEnd;

	DEBUG_LOG("PPCS_Listen('%s', 600, 0, 1, '%s')...\n", _DID_, APILicense);
	my_GetCurrentTime(&TimeBegin);	
	gSessionID = PPCS_Listen(Did, 600, 0, 1, APILicense);
	my_GetCurrentTime(&TimeEnd);
	
	if (gSessionID < 0)
	{
		ERROR_LOG("Listen failed (%d): ", gSessionID);
		showErrorInfo(gSessionID);
		return gSessionID;
	}
	// Success!! gSessionID>=0
	HLE_S32 ret = -1;
	st_PPCS_Session Sinfo;	
	if (ERROR_PPCS_SUCCESSFUL == (ret = PPCS_Check(gSessionID, &Sinfo)))//检查通过，有客户端成功接入。
	{
		DEBUG_LOG("RemoteAddr=%s:%d, Mode=%s, Time=%d.%03d (Sec)\n", 
				inet_ntoa(Sinfo.RemoteAddr.sin_addr), 
				ntohs(Sinfo.RemoteAddr.sin_port), 
				(Sinfo.bMode == 0)? "P2P":"RLY",
				ST_TIME_USED/1000,
				ST_TIME_USED%1000);
		return gSessionID;
	}
	else // connect success, but remote session closed
	{
		DEBUG_LOG("RemoteAddr=Unknown (remote closed), Mode=Unknown, Time=%d.%03d (Sec)\n", ST_TIME_USED/1000, ST_TIME_USED%1000);
		PPCS_Close(gSessionID);
		DEBUG_LOG("--PPCS_Close(%d).\n", gSessionID);
		return ret;
	}

}


 /***********************************************************************************************
 *函数名 ： 	p2p_init
 *功能描述 ：初始化P2P参数
 *参数 ： 		P2P_handle
 *返回值 ： 	成功：0 失败：-1
 ***********************************************************************************************/

HLE_S32 p2p_init(p2p_handle_t *P2P_handle )
{
	if(NULL == P2P_handle)
	{
		ERROR_LOG("illegal argument!!\n");
		return -1;
	}
	
	//初始化p2p连接参数句柄
	memset(P2P_handle,0,sizeof(p2p_handle_t));
	P2P_handle->Did = (HLE_S8*)_DID_;
	
	strcat((HLE_S8*)&P2P_handle->APILicense ,APILICENSE);
	if(strlen(CRCKEY)>0)//CRCKEY 字符串长度不为零才进行拼接
	{
		strcat((HLE_S8*)&P2P_handle->APILicense,":");
		strcat((HLE_S8*)&P2P_handle->APILicense,CRCKEY); 
		//P2P_handle->CRCKey = CRCKEY;
	}

	P2P_handle->InitString = (HLE_S8*)INITSTRING;
#ifdef P2P_SUPORT_WAKEUP
	P2P_handle->WakeupKey = (HLE_S8*)WAKEUPKEY;
	//memcpy(P2P_handle->IP[0],&g_WakeUpServerIP[0],sizeof(g_WakeUpServerIP));
	strcpy((HLE_S8*)&P2P_handle->IP[0],SERVER_IP1);
	strcpy((HLE_S8*)&P2P_handle->IP[1],SERVER_IP2);
	strcpy((HLE_S8*)&P2P_handle->IP[2],SERVER_IP3);

	P2P_handle->skt = -1;
	P2P_handle->server_index = -1;
#endif
	P2P_handle->Session_num = 0;
	pthread_mutex_init(&P2P_handle->lock , NULL);

	P2P_status = wakeup;
	
	
	// 1. get P2P API Version
	UINT32 APIVersion = PPCS_GetAPIVersion();
	st_info("P2P API Version: %d.%d.%d.%d\n",
							(APIVersion & 0xFF000000)>>24, 
							(APIVersion & 0x00FF0000)>>16, 
							(APIVersion & 0x0000FF00)>>8, 
							(APIVersion & 0x000000FF)>>0);
	//参数合法性检查
#ifdef P2P_SUPORT_WAKEUP
	HLE_S32 useless_ip = 0;  //无效的ip数
	if(P2P_handle->WakeupKey) //支持唤醒服务器唤醒
	{ 
	
		HLE_S32 server_num = 0;
		HLE_S32 ip_string  = 0;	
		HLE_S32 ip_addr_ok = 0;	 //ip参数项是否检验通过
		

		/*3个服务器IP都为空则直接退出，至少需要一个IP不为空。*/
		for(server_num = 0;server_num<SERVER_NUM;server_num++)
		{
			for(ip_string = 0;ip_string<IP_LENGTH;ip_string++)
			{
				if(0 != P2P_handle->IP[server_num][ip_string])
				{
					ip_addr_ok = 1;
					break;
					break;
				}
				
			}
			
		}

		if(0 == ip_addr_ok)
		{
			perror("All wakeupServer ip[1-3] is NULL！\n ]");
			return -1;
		}

		DEBUG_LOG("gethostbyname...\n");
		for ( server_num = 0; server_num < SERVER_NUM; server_num++)
		{
			P2P_handle->serveraddr[server_num].sin_family = AF_INET; 
			P2P_handle->serveraddr[server_num].sin_port = htons(TCP_PORT);
			P2P_handle->serveraddr[server_num].sin_addr.s_addr = inet_addr(P2P_handle->IP[server_num]);
			
			/*gethostbyname: get the server's DNS entry*/
			struct hostent *Host = gethostbyname(P2P_handle->IP[server_num]);
			if (!Host)
			{
				fprintf(stderr, "ERROR, no such host as %s\n", P2P_handle->IP[server_num]);
				//perror("gethostbyname failed");
				useless_ip++;
			}
			else
			{
				// build the server's Internet address
				fprintf(stdout, "%s is ok！\n", P2P_handle->IP[server_num]);
				P2P_handle->serveraddr[server_num].sin_family = Host->h_addrtype;
				P2P_handle->serveraddr[server_num].sin_port = htons(TCP_PORT);
				P2P_handle->serveraddr[server_num].sin_addr.s_addr = *((HLE_U32*)Host->h_addr_list[0]);
				//bcopy((HLE_S8 *)Host.h_addr, (HLE_S8 *)&serveraddr[i].sin_addr.s_addr, Host.h_length);
			}
		
		}

		for (HLE_S32 i = 0; i < SERVER_NUM; i++)
		{
			//st_debug("Host[%d]:%s\n", i, inet_ntop(serveraddr[i].sin_family, (HLE_S8 *)&serveraddr[i].sin_addr.s_addr, dest, sizeof(dest)));
			DEBUG_LOG("Host[%d]:%s\n", i, inet_ntoa(P2P_handle->serveraddr[i].sin_addr));
		}
				
		if(useless_ip == SERVER_NUM)
		{
			error("ERROR, gethostbyname failed! all wakeupServer ip is bad!!");
			return -1;
		}
	
	}
#endif

	DEBUG_LOG("DID = %s\n", P2P_handle->Did);
	DEBUG_LOG("APILicense = %s\n", P2P_handle->APILicense);
	DEBUG_LOG("InitString = %s\n", P2P_handle->InitString);
#ifdef P2P_SUPORT_WAKEUP
	DEBUG_LOG("WakeupKey = %s\n", P2P_handle->WakeupKey);
	DEBUG_LOG("useful NumberOfServer = %d\n\n", SERVER_NUM - useless_ip);
#endif

	// 2. P2P Initialize
	INT32 ret = PPCS_Initialize((HLE_S8 *)P2P_handle->InitString);
	DEBUG_LOG("PPCS_Initialize done! ret=%d\n", ret);
	if (ERROR_PPCS_SUCCESSFUL != ret && ERROR_PPCS_ALREADY_INITIALIZED != ret)
	{
		ERROR_LOG("PPCS_Initialize failed!! ret=%d: %s\n", ret, getP2PErrorCodeInfo(ret));
		return -1;
	}

	return 0;

}


/***********************************************************************************************
*函数名 ：	   p2p_conect
*功能描述 ：网络侦测 + 连接唤醒服务器
*参数 ： 	   P2P_handle
*返回值 ：	   成功：0 失败：-1
***********************************************************************************************/
st_Time_Info TimeBegin;				//与唤醒服务器建立连接的时间点
st_Time_Info TimeEnd;				//接收到唤醒服务器唤醒包的时间点
HLE_S32 p2p_conect(p2p_handle_t *P2P_handle)
{
	if(NULL == P2P_handle)
	{
		ERROR_LOG("illegal argument!!\n");
		return -1;
	}
	
	// 3. Network Detect
	st_PPCS_NetInfo NetInfo;
	HLE_S32 ret = PPCS_NetworkDetect(&NetInfo, 0);
	if (0 > ret) 
	{
		st_info("PPCS_NetworkDetect failed: ret=%d\n", ret);
		return -1;
	}
	showNetwork(NetInfo);


#ifdef P2P_SUPORT_WAKEUP
	HLE_S8 CMD[64];
	HLE_U16 SizeOfCMD = 0;
	
	if(P2P_handle->WakeupKey) //支持唤醒的逻辑部分（Advanced P2P）
	{
		// 加密DID，设备与唤醒服务器建立TCP连接后第一次发封包必须要发送加密的DID
		if (0 > iPN_StringEnc(WAKEUPKEY, _DID_, CMD, sizeof(CMD))) 
		{
			st_info("StringEncode failed.\n");
			ret = PPCS_DeInitialize();
			DEBUG_LOG("PPCS_DeInitialize() done!\n");
			return -1;
		}
		SizeOfCMD = strlen(CMD);
		DEBUG_LOG("DID[%s] %u Byte --encryption--> CMD[%s] %u Byte\n", _DID_, (unsigned)strlen(_DID_), CMD, SizeOfCMD);

		/*随机抽取一台唤醒服务器建立连接*/
		HLE_S8 dest[30]={0};
		
		HLE_S32 size_W = 0;
		//HLE_S32 size_R = 0;

		if(P2P_handle->skt < 0)
		{
			srand((UINT32)time(NULL));
			HLE_S32 i = abs(rand() % SERVER_NUM);
			for (HLE_S32 x = 0; x < SERVER_NUM; x++)//X只是控制循环的次数，真正决定连接哪一台服务器的变量是i
			{
					if (0 > P2P_handle->skt)
						{
							DEBUG_LOG("Create TCP Socket...\n");
							P2P_handle->skt = socket(AF_INET, SOCK_STREAM, 0);
							if (0 > P2P_handle->skt) 
								error("ERROR, create TCP socket ");
						}
					
						i = (i + 1) % SERVER_NUM;
						memset(dest, 0, sizeof(dest));

						const HLE_S8 *pAddr = inet_ntoa(P2P_handle->serveraddr[i].sin_addr);
						memcpy(dest, pAddr, strlen(pAddr));
						DEBUG_LOG("%s:%d connecting ...\n", dest, TCP_PORT);
						my_GetCurrentTime(&TimeBegin);

						//connect
						if (0 > connect(P2P_handle->skt, (struct sockaddr *)&P2P_handle->serveraddr[i], sizeof(struct sockaddr)))
						{
							st_info("%s - ", dest);
							setbuf(stdout, NULL);
							perror("ERROR, connecting ");
							my_SocketClose(P2P_handle->skt);
							DEBUG_LOG("close socket: %d\n", P2P_handle->skt);
							P2P_handle->skt = -1;
							continue;
						}
						else //connect wakeup server success
						{
							DEBUG_LOG("%s  connected \n", dest);
							P2P_handle->server_index = i;
							break;
						}
						
			}

			if(0 > P2P_handle->skt)
			{
				error("ERROR, all server IP is connect failed !\n");
				return -1;
			}

		}

		//发加密的DID--->CMD，设备与唤醒服务器建立TCP连接后第一次发封包必须要发送加密的DID
		if (0 >= (size_W = Call_SendCMD(P2P_handle->skt, CMD, SizeOfCMD, P2P_handle->serveraddr[P2P_handle->server_index], P2P_handle->server_index)))
		{
			//skt = -1;
			error("ERROR, Call_SendCMD error !");
			return -1;
			//continue;
		}
		
			
	}
#endif

	return 0;

}


/***********************************************************************************************
*函数名 ：	   P2P_wait_for_wakeup
*功能描述 ：进入睡眠状态，等待唤醒。
*			此函数是个阻塞的函数，正常返回是唤醒服务器唤醒或者select超时，此外是异常情况发生
*参数 ： 	   P2P_handle
*返回值 ：	
*		成功：
*			WAKEUP_SUCC (0): 唤醒成功
*		失败：
*			WAKEUP_ERR (-1)：唤醒出错(失败)
*			NOT_SUPPORT_WAKEUP (-2)：当前参数不支持唤醒
*			HAVE_CLIENT_CONNECTED (-3)：还有客户端连接，不能进入睡眠状态
***********************************************************************************************/
HLE_S32 P2P_wait_for_wakeup(p2p_handle_t *P2P_handle)
{

#ifdef P2P_SUPORT_WAKEUP

	if(NULL == P2P_handle->WakeupKey)
	{
		return WAKEUP_ERR;
	}
	
	if(P2P_handle->skt < 0) 
	{
		ERROR_LOG("TCP socket not init!\n");
		return WAKEUP_ERR;
	}
	
	if(P2P_handle->server_index < 0||P2P_handle->server_index > SERVER_NUM)
	{
		ERROR_LOG("server_index error!\n");
		return WAKEUP_ERR;
	}
	
	/*
	//检查服务器地址是否为空（至少一个不为空）
	struct sockaddr_in zero[SERVER_NUM] = {0};
	if(0 == memcmp((const void*)&P2P_handle->serveraddr ,(const void *)&zero ,sizeof(struct sockaddr_in)*(SERVER_NUM)))
	{
		ERROR_LOG("server_index error!\n");
		return WAKEUP_ERR;
	}
	*/
	
	setbuf(stdout, NULL);
	HLE_U32 timeout_ms = 10*1000;//10 秒
	struct timeval timeout;
	HLE_S8 readBuf[1024];
	
	HLE_S32 i = 0;
	HLE_S8 CMD[64] = {0};//发送给唤醒服务器的随机数据，值可随机。
	HLE_S32 size_W;
	HLE_S8 LoginStatus;
	
	/*进入休眠--------所有用户都退出了才能进入到休眠状态*/
	if(0 == P2P_handle->Session_num)
	{
		
		while (0 <= P2P_handle->skt) //P2P_handle里边的skt创建成功后就会一直循环，等待唤醒才能退出。
		{
			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(P2P_handle->skt, &readfds);
			timeout.tv_sec = (timeout_ms-(timeout_ms%1000))/1000;
			timeout.tv_usec = (timeout_ms%1000)*1000;
			P2P_status = sleeping;
			
			gThread_Exit = 1; // Exit the LoginStatus_Check thread
			
			DEBUG_LOG("Enter select, Waiting to be wakeup ...\n");
			HLE_S32 activity = select(P2P_handle->skt+1, &readfds, NULL , NULL , &timeout);
			
			switch (activity)
			{
				case 0: // time out，没有唤醒就会一直进入到 case 0中
					{
						DEBUG_LOG("-------------------(select timeout!)\n");
						printf(".");
						setbuf(stdout, NULL);
						if (0 == ++i%6)  //select 10S超时，6*10 = 60s 发送一包保活封包
						{
							if (0 >= (size_W = Call_SendCMD(P2P_handle->skt, CMD,sizeof(CMD),P2P_handle->serveraddr[P2P_handle->server_index], P2P_handle->server_index)))
							{
								//发送失败，一次发送失败就要退出吗？？？
								perror("ERROR, Keep aliving package send error!! \n");
								P2P_handle->skt = -1;
								return WAKEUP_ERR;
							}
						}
						//没有1min则继续下一次循环（select超时休眠状态）
						continue;
					}
				
				case -1://select error
					ERROR_LOG("\n-------------------(select return -1, errno=%d)\n", errno);
					if (EINTR != errno)
					{
						my_SocketClose(P2P_handle->skt);
						st_debug("close skt: %d\n", P2P_handle->skt);
						P2P_handle->skt = -1;
						return WAKEUP_ERR;
					}
					break;
					
				default:
					if (FD_ISSET(P2P_handle->skt, &readfds)) //如果socket有数据可读说明唤醒服务器进行了唤醒操作
					{
						memset(readBuf, 0, sizeof(readBuf));
						DEBUG_LOG("\nrecv data ...\n");	
						
						HLE_S32 size_R = read(P2P_handle->skt, readBuf, sizeof(readBuf));
						if (0 >= size_R) 
						{
							ERROR_LOG("\nTCP read failed(%d)\n", size_R);
							if (ERROR_PPCS_SUCCESSFUL == PPCS_LoginStatus_Check(&LoginStatus))
							{
							
								if (1 == LoginStatus) 
								{
									st_info("Got Server Response!!\n");
								}						
								else 
								{
									P2P_status = offline;
									PPCS_ForceClose(P2P_handle->SessionID);
									st_info("No Server Response!!!\n");
									
									return SERVERS_OFFLINE;
								}						
							}
							return WAKEUP_ERR;
						} 
						else //成功接收唤醒包
						{
							my_GetCurrentTime(&TimeEnd);
							DEBUG_LOG("Wakeup[%d.%03d (sec)]\n",ST_TIME_USED/1000, ST_TIME_USED%1000);
							DEBUG_LOG("\n------Got Wakeup Cmd(%d Byte):", size_R);
							for(HLE_S32 i = 0; i < size_R; i++) 
							{
								st_debug("0x%02X%c", readBuf[i]&0xFF, (i==size_R-1)?'\n':' ');
							}
							P2P_status = wakeup;
							return WAKEUP_SUCC;
						
						}
				
				}
			} // switch
					
		}//while(1)
		
		return WAKEUP_ERR;
		
	}
	else
	{
		//还有客户端连接，直接返回。
		usleep(1000);
		return HAVE_CLIENT_CONNECTED;		
	}

#else //不支持唤醒模式

	return NOT_SUPPORT_WAKEUP;
#endif
}


/***********************************************************************************************
*函数名 ：	   p2p_listen
*功能描述 ：监听等待客户端连接。
*			此函数是个阻塞的函数，正常返回是唤醒服务器唤醒或者超时，此外除非异常情况发生
*			listen超时时间：10min
*参数 ： 	   P2P_handle
*返回值 ：	
*		成功：>= 0的 SessionID
*		失败：<     0的值
*	
***********************************************************************************************/
HLE_S32 p2p_listen(p2p_handle_t *P2P_handle)
{
	if( P2P_status != sleeping)
	{
		if (0 == gThread_bRunning) 
		{
			//DEBUG_LOG("into CreateThread_LoginStatus_Check\n");
			CreateThread_LoginStatus_Check();
		}
		if(P2P_handle->Session_num < MAX_CLIENT_NUM)//同时登录的客户端数量不能超过限制
		{
			
			
			HLE_S32 SessionID = Call_P2P_Listen(P2P_handle->Did , P2P_handle->APILicense);//阻塞
			if(SessionID >= 0)//listen success
			{
				DEBUG_LOG("Call_P2P_Listen success! gSessionID = %d\n",SessionID);
				P2P_status = connected;
				return SessionID;
			}
			else
			{
				ERROR_LOG("Call_P2P_Listen error! gSessionID = %d\n ",SessionID);
				return SessionID;
			}

		}
		else
		{
			ERROR_LOG("too many client connect!\n ");
			return -1;
		}
	
	}
	else
	{ 
		ERROR_LOG("Call_P2P_Listen error!! P2P_status is sleeping!\n ");
		return -1; 
	}
			
}

/***********************************************************************************************
*函数名 ：	   P2P_client_task_create
*功能描述 ：创建专门的task线程去和客户端交互，针对接入的客户端。
*参数 ： 	   P2P_handle
*返回值 ：	
*		成功： 0
*		失败：-1
*	
***********************************************************************************************/
HLE_S32 P2P_client_task_create(p2p_handle_t *P2P_handle)
{

	pthread_t taskid;
	
	HLE_S32 ret = pthread_create(&taskid,NULL,P2P_client_task_func,(void*)P2P_handle);  
    if(ret != 0)  
    {  
        ERROR_LOG("Create pthread error!\n");  
    	return -1;  
    }
	
	
	return 0;
}

/***********************************************************************************************
*函数名 ：	   P2P_client_task_func
*功能描述 ：task线程的线程响应函数。
*参数 ： 	   P2P_handle
*返回值 ：void*	
*	
*	
***********************************************************************************************/
void * P2P_client_task_func(void*P2P_handle)
{
	pthread_detach(pthread_self());
	
	if(NULL == P2P_handle)
	{
		ERROR_LOG("illegal argument!!\n");
		return NULL;
	}
	DEBUG_LOG("Thread create: %s :[tid:%lu]\n", __FUNCTION__, pthread_self());

	p2p_handle_t *Session_handle = P2P_handle;
	
	pthread_mutex_lock(&Session_handle->lock);
	Session_handle->Session_num ++;
	pthread_mutex_unlock(&Session_handle->lock);
	
	//对当前客户端的SessionID进行备份,并加入状态数组。
	HLE_S32 SessionID = Session_handle->SessionID;	
	HLE_S32 index = add_one_session_to_arr(SessionID);
	if(index < 0)
	{
		ERROR_LOG("too many client connected !\n");
		pthread_exit(NULL);
	}

	session_status_t status;
	memset(&status,0,sizeof(session_status_t));
	get_session_status(SessionID,&status);
	status.Session_status = ONLINE;
	set_session_status(SessionID,&status);
	
	cmd_header_t cmd_header;
	HLE_S32 ReadSize;
	
	st_PPCS_Session Sinfo;
	memset(&Sinfo,0,sizeof(st_PPCS_Session));
	
	while(ONLINE == is_session_online(SessionID) )//客户端没退出
	{
		memset(&Sinfo,0,sizeof(st_PPCS_Session));
		memset(&cmd_header,0,sizeof(cmd_header_t));
		//读取信令头
		ReadSize = sizeof(cmd_header_t);
	
		//PPCS_Read 每次返回后，ReadSize 会被填入实际读到的字节数。
		HLE_S32 ret = PPCS_Read(SessionID, CH_CMD, (CHAR*)&cmd_header, &ReadSize, 2000);
		if(ret < 0)//读取出错
		{
			if (ERROR_PPCS_TIME_OUT == ret) 
			{
				DEBUG_LOG("Read CMD Header timeout !!\n");
				usleep(50*1000);
				continue;
			}	
			else if (ERROR_PPCS_SESSION_CLOSED_REMOTE == ret) //远程会话关闭（客户端退出）
			{
			
				ERROR_LOG("Remote site call close!!\n");
				break;
				
			}
			else 
			{
				ERROR_LOG("PPCS_Read: Channel=%d, ret=%d\n", CH_CMD, ret);
				
				break;
			}
			
	

		}

		DEBUG_LOG("PPCS_Read CMD Header success! SessionID(%d) read(%d)bytes!\n",SessionID,ReadSize);
		
		//信令头读取成功！对信令进行解析
		med_ser_signal_parse(SessionID,&cmd_header,ReadSize);
		usleep(100*1000);
	

	}

	mSleep(50);
	
	
	del_one_session_from_arr(SessionID);
	PPCS_Close(SessionID);

	pthread_mutex_lock(&Session_handle->lock);
	Session_handle->Session_num --;
	if(Session_handle->Session_num < 0)
		Session_handle->Session_num = 0;
	
	pthread_mutex_unlock(&Session_handle->lock);
	
	DEBUG_LOG("--PPCS_Close(%d)\n", SessionID);
		
	pthread_exit(0);

	return NULL;
}

//recv
HLE_S32 p2p_recv(void *handle,void*buf,HLE_S32 length)
{

	return 0;
}

//send
HLE_S32 p2p_send(void *handle,const void*data,HLE_S32 length)
{
	return 0;
}

//close
HLE_S32 p2p_close(void *handle)
{
	return 0;
}





