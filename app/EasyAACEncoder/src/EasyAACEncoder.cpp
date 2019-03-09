

/*
	Copyright (c) 2013-2016 EasyDarwin.ORG.  All rights reserved.
	Github: https://github.com/EasyDarwin
	WEChat: EasyDarwin
	Website: http://www.easydarwin.org
*/
/* 
 * File:   EasyAACEncoder.cpp
 * Author: Wellsen@easydarwin.org
 * 
 * Created on 2015年4月11日, 上午11:44
 */

#include "EasyAACEncoder.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "g711.h"

#include "EasyAACEncoderAPI.h"

#include "outDebug.h"
#include "G711AToPcm.h"
#include "G726ToPcm.h"
#include "condef.h"



G7ToAac::G7ToAac()
{
    m_pbPCMBuffer = NULL;
    m_pbAACBuffer = NULL;
	m_pbG7FrameBuffer = NULL;
    m_pbPCMTmpBuffer = NULL;

    m_audio_buffer_ = NULL;

	m_pDecodeToPcm = NULL;

	m_pPCMToAAC = NULL;
}

G7ToAac::~G7ToAac()
{

    /*free the source of malloc*/
	SAFE_FREE_BUF(m_pbPCMBuffer);
	SAFE_FREE_BUF(m_pbAACBuffer);
	SAFE_FREE_BUF(m_pbG7FrameBuffer);
	SAFE_FREE_BUF(m_pbPCMTmpBuffer);

	SAFE_DELETE_OBJ(m_audio_buffer_);
	SAFE_DELETE_OBJ(m_pDecodeToPcm);
	SAFE_DELETE_OBJ(m_pPCMToAAC);

}
bool G7ToAac::init()
{
	nRet = 0;
	nTmp = 0;
	nCount = 0;
	nStatus = 0;
	nPCMRead = 0;



	CreateBuffer();

	return true;
}

bool G7ToAac::init(InAudioInfo info)
{
	m_inAudioInfo = info;

	bool ret=false;
	ret = CreateDecodePcm();

	ret = CreateEncodeAac();
	if (!ret)
	{
		return false;
	}
	return init();
}
bool G7ToAac::CreateDecodePcm()
{
	if ( Law_ALaw == m_inAudioInfo.CodecType())
	{
		m_pDecodeToPcm = new G711AToPcm();
	}else if ( Law_ULaw == m_inAudioInfo.CodecType() )
	{
		m_pDecodeToPcm = new G711UToPcm();
	}else if ( Law_G726 == m_inAudioInfo.CodecType())
	{
		m_pDecodeToPcm = new G726ToPcm();
	}else
	{
		m_pDecodeToPcm = new G711AToPcm();
	}
	m_pDecodeToPcm->Init(m_inAudioInfo);

	return true;
}
bool G7ToAac::CreateEncodeAac()
{
	m_pPCMToAAC = new PcmToAac();
	printf("Easy into 000 \n");
	bool ret = m_pPCMToAAC->Init(&m_inAudioInfo);

	return ret;
}
bool G7ToAac::CreateBuffer()
{
	m_nPCMBufferSize = m_pPCMToAAC->GetPCMBufferSize();
	m_pbPCMBuffer = (unsigned char*) malloc(m_nPCMBufferSize * sizeof (unsigned char));
	memset(m_pbPCMBuffer, 0, m_nPCMBufferSize);

	m_nMaxOutputBytes = m_pPCMToAAC->GetMaxOutputBytes();
	m_pbAACBuffer = (unsigned char*) malloc(m_nMaxOutputBytes * sizeof (unsigned char));
	memset(m_pbAACBuffer, 0, m_nMaxOutputBytes);

	m_nG7FrameBufferSize = m_pDecodeToPcm->G7FrameSize();
	m_pbG7FrameBuffer = (unsigned char *) malloc(m_nG7FrameBufferSize * sizeof (unsigned char));
	memset(m_pbG7FrameBuffer, 0, m_nG7FrameBufferSize);

	m_nPCMSize = m_pDecodeToPcm->PCMSize();
	m_pbPCMTmpBuffer = (unsigned char *) malloc(m_nPCMSize * sizeof (unsigned char));
	memset(m_pbPCMTmpBuffer, 0, m_nPCMSize);

	m_audio_buffer_ = new audio_buffer();

	return true;
}
int G7ToAac::aac_encode(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen)
{
	int encodeLen = 0;

	if (NULL != m_pDecodeToPcm)
	{
		encodeLen = aac_encode_obj(inbuf , inlen , outbuf , outlen);
	}
	if(encodeLen < 0)
		printf("<ERROR> %s %d encodeLen = %d\n",__FILE__,__LINE__,encodeLen);
	return encodeLen;
}

int G7ToAac::aac_encode_obj(unsigned char* inbuf, unsigned int inlen, unsigned char* outbuf, unsigned int* outlen )
{
	m_audio_buffer_->write_data(inbuf, inlen); //将数据放入缓存区
	int buffer_len = 0;
	*outlen = 0;
	int nPCMSize = 0;
	//while ((buffer_len = audio_buffer_->get_data(pbG711ABuffer, /*164*/G711_ONE_LEN)) > 0)
	//while ((buffer_len = m_audio_buffer_->get_data(m_pbG7FrameBuffer, m_nG7FrameBufferSize)) > 0)
	while ((buffer_len = m_audio_buffer_->get_data(m_pbPCMTmpBuffer, m_nPCMSize)) > 0)//直接获取到PCM帧缓存
	{

	
		if (buffer_len <= 0)
		{
			if(AAC_DEBUG) printf("%s:[%d] G711A -> PCM  no G711 data !\n", __FUNCTION__, __LINE__);
			//Sleep(100);
			return -1;
		}

		nStatus = 0;        
		//memset(m_pbPCMTmpBuffer, 0, m_nPCMSize);
		//nPCMSize = m_nPCMSize;
		//if ((nPCMRead = m_pDecodeToPcm->Decode(pbPCMTmpBuffer, (unsigned int*)&PCMSize, pbG711ABuffer+/*4*/G711_ONE_OFFSET, buffer_len-/*4*/G711_ONE_OFFSET )) < 0) // TODO: skip 4 byte?

	#if 1 HLE_MODIFY  //传入的直接是PCM数据，不需要做转码
		//memcpy(m_pbPCMTmpBuffer,m_pbG7FrameBuffer,buffer_len); //PCM--->G711 压缩比 2:1 ， 1字节G711的数据解压会得到2字节的PCM数据
		nPCMSize = buffer_len;
		nPCMRead = buffer_len;
		//printf("nPCMRead = %d  \n",buffer_len);//320 bytes 
	#else
		if ((nPCMRead = m_pDecodeToPcm->Decode(m_pbPCMTmpBuffer, (unsigned int*)&nPCMSize, m_pbG7FrameBuffer, buffer_len )) < 0) // TODO: skip 4 byte?
		{
			if(AAC_DEBUG) printf("%s:[%d] G711A -> PCM  failed buffer_len = %d !\n", __FUNCTION__, __LINE__, buffer_len);            
			return -1;
		}
	#endif
		//if(AAC_DEBUG) printf("nPCMRead = %d, PCMSize = %d\n", nPCMRead, PCMSize);

		if ((m_nPCMBufferSize - nCount) < nPCMRead)//缓存放不下了,把能放下的放入，进行编码之后，再把没放完的放进去。
		{
			//if(AAC_DEBUG) printf("nPCMBufferSize = %d, nCount = %d, nPCMRead = %d\n", nPCMBufferSize, nCount, nPCMRead);
			nStatus = 1;//标志： 进行AAC编码
			memset(m_pbAACBuffer, 0, m_nMaxOutputBytes);
			
			//将buf还剩余的部分填充满
			memcpy(m_pbPCMBuffer + nCount, m_pbPCMTmpBuffer, (m_nPCMBufferSize - nCount));
 
			//进行AAC编码,AAC数据放到 m_pbAACBuffer 中。
			nRet = m_pPCMToAAC->Encode( (int32_t*)m_pbPCMBuffer , m_nPCMBufferSize , m_pbAACBuffer, m_nMaxOutputBytes);
			
			//printf("<ERROR>file:%s line:%d m_pPCMToAAC->Encode--->nRet (%d) read PCM_len(%d) m_nMaxOutputBytes(%d)\n",__FILE__,__LINE__,nRet,buffer_len,m_nMaxOutputBytes);

			//将编码好的AAC数据返回给上层调用（返回编码数据）
			memcpy(outbuf + *outlen, m_pbAACBuffer, nRet); 
			*outlen += nRet;
			

			//计算读到的PCM帧还剩余多少字节没有放入（PCM）缓存（进行AAC编码）的。
			nTmp = nPCMRead - (m_nPCMBufferSize - nCount);
			
			memset(m_pbPCMBuffer, 0, m_nPCMBufferSize);
			//拷贝剩余的部分帧数据
			memcpy(m_pbPCMBuffer, m_pbPCMTmpBuffer + (m_nPCMBufferSize - nCount), nTmp);
			if(AAC_DEBUG) printf("%s:[%d] G711A -> PCM (nPCMBufferSize - nCount) < nPCMRead\n",  __FUNCTION__, __LINE__);
			nCount = 0;
			nCount += nTmp;
		}

		if (nStatus == 0)//PCM buf缓存还没满的情况，继续放入PCM数据
		{
			if(AAC_DEBUG) printf("%s:[%d] G711A -> PCM nStatus = 0...\n",  __FUNCTION__, __LINE__);
			memcpy(m_pbPCMBuffer + nCount, m_pbPCMTmpBuffer, nPCMRead);
			if(AAC_DEBUG) printf("%s:[%d] G711A -> PCM nStatus = 0\n",  __FUNCTION__, __LINE__);
			nCount += nPCMRead;
		}

		if (nPCMRead < /*320*/CON_PCM_SIZE) //解码出来的PCM数据小于320字节（低于160采样点/1帧）,我们默认传入的PCM数据都有320字节
		{
			printf("<ERROR>!!! into nPCMRead < 320 !!! nPCMRead(%d)\n",nPCMRead);
			if(AAC_DEBUG) printf("nPCMRead = %d\n", nPCMRead);

			nRet = m_pPCMToAAC->Encode((int*) m_pbPCMBuffer, nCount , m_pbAACBuffer, m_nMaxOutputBytes);


			memcpy(outbuf + *outlen, m_pbAACBuffer, nRet);
			*outlen += nRet;

			INFO_D(AAC_DEBUG , "G711A -> PCM nPCMRead < 320");
		}
	}

	if(nRet < 0||nRet >2048)
		printf("aac_encode_obj ---> nRet (%d) m_nPCMBufferSize(%d) m_nMaxOutputBytes(%d)\n",nRet,m_nPCMBufferSize,m_nMaxOutputBytes);
	
	return *outlen;
}














