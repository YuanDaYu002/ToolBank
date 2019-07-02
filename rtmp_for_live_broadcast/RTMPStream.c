 
/***************************************************************************
 * * @file: RTMPStream.c
 * * @author:   
 * * @date:  6,29,2019
 * * @brief:  
 * * @attention:发送H264视频到RTMP Server，使用libRtmp库 
 * ***************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "rtmp.h"  
#include "rtmp_sys.h"  
#include "amf.h"  

 
#include "RTMPStream.h"  
#include "SpsDecode.h"
#include "rtmp_typeport.h"
 

RTMP* m_pRtmp;		//RTMP句柄	
unsigned char* 	m_pFileBuf; 	//H264文件缓存buffer 
unsigned int	m_nFileBufSize; //H264文件缓存buffer的“占用”（有效数据）大小 
unsigned int	m_nCurPos;		//当前读的位置
  

  
enum  
{  
    FLV_CODECID_H264 = 7,  
};  
  
int InitSockets()    
{    
    return TRUE;    
}    
  
inline void CleanupSockets()    
{    
 
}    
  
char * put_byte( char *output, uint8_t nVal )    
{    
    output[0] = nVal;    
    return output+1;    
}    
char * put_be16(char *output, uint16_t nVal )    
{    
    output[1] = nVal & 0xff;    
    output[0] = nVal >> 8;    
    return output+2;    
}    
char * put_be24(char *output,uint32_t nVal )    
{    
    output[2] = nVal & 0xff;    
    output[1] = nVal >> 8;    
    output[0] = nVal >> 16;    
    return output+3;    
}    
char * put_be32(char *output, uint32_t nVal )    
{    
    output[3] = nVal & 0xff;    
    output[2] = nVal >> 8;    
    output[1] = nVal >> 16;    
    output[0] = nVal >> 24;    
    return output+4;    
}    
char *  put_be64( char *output, uint64_t nVal )    
{    
    output=put_be32( output, nVal >> 32 );    
    output=put_be32( output, nVal );    
    return output;    
}    
char * put_amf_string( char *c, const char *str )    
{    
    uint16_t len = strlen( str );    
    c=put_be16( c, len );    
    memcpy(c,str,len);    
    return c+len;    
}    
char * put_amf_double( char *c, double d )    
{    
    *c++ = AMF_NUMBER;  /* type: Number */    
    {    
        unsigned char *ci, *co;    
        ci = (unsigned char *)&d;    
        co = (unsigned char *)c;    
        co[0] = ci[7];    
        co[1] = ci[6];    
        co[2] = ci[5];    
        co[3] = ci[4];    
        co[4] = ci[3];    
        co[5] = ci[2];    
        co[6] = ci[1];    
        co[7] = ci[0];    
    }    
    return c+8;    
}  

/*******************************************************************************
 * *@ Description    :RTMP传输模块初始化
 * *@ Input          :
 * *@ Output         :
 * *@ Return         :成功：0 ； 失败：-1
 * *@ attention      :
 * *******************************************************************************/
int CRTMPStream_init(void)   
{
	m_pRtmp = NULL;  
	m_nFileBufSize = 0;  
	m_nCurPos = 0; 
  
	m_pFileBuf = (unsigned char*)malloc(FILEBUFSIZE);
	if(NULL == m_pFileBuf)
	{
		RTMP_ERROR_LOG("malloc failed!\n");
		return -1;
	}
    memset(m_pFileBuf,0,FILEBUFSIZE);  
    InitSockets();  
    m_pRtmp = RTMP_Alloc();
	if(NULL == m_pRtmp)
	{
		RTMP_ERROR_LOG("RTMP_Alloc failed!\n");
		return -1;	
	}
    RTMP_Init(m_pRtmp);   
	return 0;
}  
  
void CRTMPStream_exit(void)  
{  
	//Close();  
	//WSACleanup(); 
	RTMP_Free(m_pRtmp);
	if(m_pFileBuf) free(m_pFileBuf);  
}  

/*******************************************************************************
*@ Description    :建立传输连接
*@ Input          :<url>
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
char CRTMPStream_Connect(char* url)  
{ 
	if(NULL == url)
	{
		return FALSE;
	}
	if(!RTMP_SetupURL(m_pRtmp, (char*)url))  
	{  
		return FALSE;  
	}  
	RTMP_EnableWrite(m_pRtmp);  
	if(!RTMP_Connect(m_pRtmp, NULL))  
	{  
		RTMP_ERROR_LOG("RTMP_Connect failed!\n");
		return FALSE;  
	}  
	if(!RTMP_ConnectStream(m_pRtmp,0))  
	{  
		return FALSE;  
	} 
	RTMP_DEBUG_LOG("RTMP_ConnectStream success!\n");
	return TRUE;  
}  

/*******************************************************************************
*@ Description    :关闭传输流
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
void CRTMPStream_Close(void)  
{  
	if(m_pRtmp)  
	{  
		RTMP_Close(m_pRtmp);  
		RTMP_Free(m_pRtmp);  
		m_pRtmp = NULL;  
	}	  
}  

/*******************************************************************************
*@ Description    :RTMP发送数据包
*@ Input          :
*@ Output         :
*@ Return         :成功：0 ； 失败：-1
*@ attention      :
*******************************************************************************/ 
int CRTMPStream_SendPacket(unsigned int nPacketType,unsigned char *data,unsigned int size,unsigned int nTimestamp)  
{  
	if(m_pRtmp == NULL)  
	{  
		return -1;  
	}  

	RTMPPacket packet;  
	RTMPPacket_Reset(&packet);  
	RTMPPacket_Alloc(&packet,size);  

	packet.m_packetType = nPacketType;  
	packet.m_nChannel = 0x04;    
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;    
	packet.m_nTimeStamp = nTimestamp;    
	packet.m_nInfoField2 = m_pRtmp->m_stream_id;  
	packet.m_nBodySize = size;  
	memcpy(packet.m_body,data,size);  

	int nRet = RTMP_SendPacket(m_pRtmp,&packet,0);  

	RTMPPacket_Free(&packet);  

	return 0;  
}  

/*******************************************************************************
*@ Description    :RTMP发送媒体数据
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/  
char CRTMPStream_SendMetadata(LPRTMPMetadata lpMetaData)  
{  
	if(lpMetaData == NULL)  
	{  
		return -1;  
	}  
	char body[1024] = {0};;  

	char * p = (char *)body;    
	p = put_byte(p, AMF_STRING );  
	p = put_amf_string(p , "@setDataFrame" );  

	p = put_byte( p, AMF_STRING );  
	p = put_amf_string( p, "onMetaData" );  

	p = put_byte(p, AMF_OBJECT );    
	p = put_amf_string( p, "copyright" );    
	p = put_byte(p, AMF_STRING );    
	p = put_amf_string( p, "firehood" );    

	p =put_amf_string( p, "width");  
	p =put_amf_double( p, lpMetaData->nWidth);  

	p =put_amf_string( p, "height");  
	p =put_amf_double( p, lpMetaData->nHeight);  

	p =put_amf_string( p, "framerate" );  
	p =put_amf_double( p, lpMetaData->nFrameRate);   

	p =put_amf_string( p, "videocodecid" );  
	p =put_amf_double( p, FLV_CODECID_H264 );  

	p =put_amf_string( p, "" );  
	p =put_byte( p, AMF_OBJECT_END  );  

	int index = p-body;  

	CRTMPStream_SendPacket(RTMP_PACKET_TYPE_INFO,(unsigned char*)body,p-body,0);  

	int i = 0;  
	body[i++] = 0x17; // 1:keyframe  7:AVC  
	body[i++] = 0x00; // AVC sequence header  

	body[i++] = 0x00;  
	body[i++] = 0x00;  
	body[i++] = 0x00; // fill in 0;  

	// AVCDecoderConfigurationRecord.  
	body[i++] = 0x01; // configurationVersion  
	body[i++] = lpMetaData->Sps[1]; // AVCProfileIndication  
	body[i++] = lpMetaData->Sps[2]; // profile_compatibility  
	body[i++] = lpMetaData->Sps[3]; // AVCLevelIndication   
	body[i++] = 0xff; // lengthSizeMinusOne    

	// sps nums  
	body[i++] = 0xE1; //&0x1f  
	// sps data length  
	body[i++] = lpMetaData->nSpsLen>>8;  
	body[i++] = lpMetaData->nSpsLen&0xff;  
	// sps data  
	memcpy(&body[i],lpMetaData->Sps,lpMetaData->nSpsLen);  
	i= i+lpMetaData->nSpsLen;  

	// pps nums  
	body[i++] = 0x01; //&0x1f  
	// pps data length   
	body[i++] = lpMetaData->nPpsLen>>8;  
	body[i++] = lpMetaData->nPpsLen&0xff;  
	// sps data  
	memcpy(&body[i],lpMetaData->Pps,lpMetaData->nPpsLen);  
	i= i+lpMetaData->nPpsLen;  

	return CRTMPStream_SendPacket(RTMP_PACKET_TYPE_VIDEO,(unsigned char*)body,i,0);  

}  

/*******************************************************************************
*@ Description    :发送h264包
*@ Input          :
*@ Output         :
*@ Return         :成功：0 ； 失败：-1
*@ attention      :
*******************************************************************************/
char CRTMPStream_SendH264Packet(unsigned char *data,unsigned int size,char bIsKeyFrame,unsigned int nTimeStamp)  
{  
	if(data == NULL && size<11)  
	{  
		return -1;  
	}  

	unsigned char *body = (unsigned char *)malloc(size+9);
	if(NULL == body)
	{
		RTMP_ERROR_LOG("malloc failed!\n");
		return -1;
	}

	int i = 0;  
	if(bIsKeyFrame)  
	{  
		body[i++] = 0x17;// 1:Iframe  7:AVC  
	}  
	else  
	{  
		body[i++] = 0x27;// 2:Pframe  7:AVC  
	}  
	body[i++] = 0x01;// AVC NALU  
	body[i++] = 0x00;  
	body[i++] = 0x00;  
	body[i++] = 0x00;  

	// NALU size  
	body[i++] = size>>24;  
	body[i++] = size>>16;  
	body[i++] = size>>8;  
	body[i++] = size&0xff;;  

	// NALU data  
	memcpy(&body[i],data,size);  

	char bRet = CRTMPStream_SendPacket(RTMP_PACKET_TYPE_VIDEO,body,i+size,nTimeStamp);  

	free(body);  

	return 0;  
}  


/*******************************************************************************
*@ Description    :发送一个H264文件
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
char CRTMPStream_SendH264File(const char *pFileName)  
{  
	if(pFileName == NULL)  
	{  
		return FALSE;  
	}  
	FILE *fp = fopen(pFileName, "rb");    
	if(!fp)    
	{    
		RTMP_ERROR_LOG("open file %s failed!",pFileName);
		return FALSE;  
	}    
	fseek(fp, 0, SEEK_SET);

	m_nFileBufSize = fread(m_pFileBuf, sizeof(unsigned char), FILEBUFSIZE, fp);  
	if(m_nFileBufSize >= FILEBUFSIZE)  
	{  
		RTMP_ERROR_LOG("File size is larger than BUFSIZE\n");  
		return FALSE;  
	}  
	fclose(fp);    

	RTMPMetadata metaData;  
	memset(&metaData,0,sizeof(RTMPMetadata));  

	NaluUnit naluUnit;  
	// 读取SPS帧  
	CRTMPStream_ReadOneNaluFromBuf(naluUnit);  
	metaData.nSpsLen = naluUnit.size;  
	memcpy(metaData.Sps,naluUnit.data,naluUnit.size);  

	// 读取PPS帧  
	CRTMPStream_ReadOneNaluFromBuf(naluUnit);  
	metaData.nPpsLen = naluUnit.size;  
	memcpy(metaData.Pps,naluUnit.data,naluUnit.size);  

	// 解码SPS,获取视频图像宽、高信息  
	int width = 0,height = 0;  
	h264_decode_sps(metaData.Sps,metaData.nSpsLen,width,height);  
	metaData.nWidth = width;  
	metaData.nHeight = height;  
	metaData.nFrameRate = 15;  

	// 发送MetaData  
	CRTMPStream_SendMetadata(&metaData);

	unsigned int tick = 0;  
	while(CRTMPStream_ReadOneNaluFromBuf(naluUnit))  
	{  
		char bKeyframe  = (naluUnit.type == 0x05) ? TRUE : FALSE;  
		// 发送H264数据帧  
		CRTMPStream_SendH264Packet(naluUnit.data,naluUnit.size,bKeyframe,tick);  
		msleep(40);  
		tick +=40;  
	}  

	return TRUE;  
}  

/*******************************************************************************
*@ Description    :读取一个NALU(SPS 	 PPS)
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
char CRTMPStream_ReadOneNaluFromBuf(NaluUnit nalu)  
{  
	int i = m_nCurPos;  
	while(i<m_nFileBufSize)  
	{  
		if(m_pFileBuf[i++] == 0x00 &&  
		m_pFileBuf[i++] == 0x00 &&  
		m_pFileBuf[i++] == 0x00 &&  
		m_pFileBuf[i++] == 0x01  
		)  
		{  
			int pos = i;  
			while (pos<m_nFileBufSize)  
			{  
				if(m_pFileBuf[pos++] == 0x00 &&  
				m_pFileBuf[pos++] == 0x00 &&  
				m_pFileBuf[pos++] == 0x00 &&  
				m_pFileBuf[pos++] == 0x01  
				)  
				{  
					break;  
				}  
			}  
			if(pos == m_nFileBufSize)  
			{  
				nalu.size = pos-i;    
			}  
			else  
			{  
				nalu.size = (pos-4)-i;  
			}  
			nalu.type = m_pFileBuf[i]&0x1f;  
			nalu.data = &m_pFileBuf[i];  

			m_nCurPos = pos-4;  
			return TRUE;  
		}  
	}  
	return FALSE;  
}  







