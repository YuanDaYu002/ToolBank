 
/***************************************************************************
* @file: RTMPStream.h
* @author:   
* @date:  6,29,2019
* @brief:  
* @attention:发送H264视频到RTMP Server，使用libRtmp库 
***************************************************************************/


#ifndef RTMP_STREAM_H
#define RTMP_STREAM_H

#define FILEBUFSIZE (1024 * 1024 * 10)       //  10M  
  
// NALU单元  
typedef struct _NaluUnit  
{  
    int type;  
    int size;  
    unsigned char *data;  
}NaluUnit;  
  
typedef struct _RTMPMetadata  
{  
    // video, must be h264 type  
    unsigned int    nWidth;  
    unsigned int    nHeight;  
    unsigned int    nFrameRate;     // fps  
    unsigned int    nVideoDataRate; // bps  
    unsigned int    nSpsLen;  
    unsigned char   Sps[1024];  
    unsigned int    nPpsLen;  
    unsigned char   Pps[1024];  
  
    // audio, must be aac type  
    char            bHasAudio;  
	char            pAudioSpecCfg;
	char 			reserved[2];
	unsigned int    nAudioSampleRate;  
    unsigned int    nAudioSampleSize;  
    unsigned int    nAudioChannels;    
    unsigned int    nAudioSpecCfgLen;  
  
} RTMPMetadata,*LPRTMPMetadata;  
  
  
 
int CRTMPStream_init(void);  
void CRTMPStream_exit(void);  
 
// 连接到RTMP Server  
char CRTMPStream_Connect(char* url);  
// 断开连接  
void CRTMPStream_Close();  
// 发送MetaData  
char CRTMPStream_SendMetadata(LPRTMPMetadata lpMetaData);  
// 发送H264数据帧  
char CRTMPStream_SendH264Packet(unsigned char *data,unsigned int size,char bIsKeyFrame,unsigned int nTimeStamp);  
// 发送H264文件  
char CRTMPStream_SendH264File(const char *pFileName);  

// 送缓存中读取一个NALU包  
char CRTMPStream_ReadOneNaluFromBuf(NaluUnit nalu);  
// 发送数据  
int CRTMPStream_SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp);  

#endif


