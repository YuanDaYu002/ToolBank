#include <stdio.h>
#ifdef WIN32
#include <Windows.h>
#include <process.h>
#else
#include<pthread.h>
#include <unistd.h>
#define scanf_s scanf 
#endif
#include "voiceRecog.h"
#include "audioRecorder.h"
#include "typeport.h"

static void * recognizer = NULL; //句柄 




const char *recorderRecogErrorMsg(int _recogStatus)
{
	char *r = (char *)"unknow error";
	switch(_recogStatus)
	{
	case VR_ECCError:
		r = (char *)"ecc error";
		break;
	case VR_NotEnoughSignal:
		r = (char *)"not enough signal";
		break;
	case VR_NotHeaderOrTail:
		r = (char *)"signal no header or tail";
		break;
	case VR_RecogCountZero:
		r = (char *)"trial has expires, please try again";
		break;
	}
	return r;
}

//识别开始回调函数
void recognizerStart(void *_listener, float _soundTime)
{
	printf("------------------recognize start\n");
}

//识别结束回调函数
void recognizerEnd(void *_listener, float _soundTime, int _recogStatus, char *_data, int _dataLen)
{
	struct SSIDWiFiInfo wifi;
	struct WiFiInfo macWifi;
	int i;
	struct PhoneInfo phone;
	char s[100];
	if (_recogStatus == VR_SUCCESS)
	{		
		enum InfoType infoType = vr_decodeInfoType(_data, _dataLen);
		if(infoType == IT_PHONE)
		{
			vr_decodePhone(_recogStatus, _data, _dataLen, &phone);
			ERROR_LOG("imei:%s, phoneName:%s", phone.imei, phone.phoneName);
		}
		else if(infoType == IT_SSID_WIFI)
		{
			vr_decodeSSIDWiFi(_recogStatus, _data, _dataLen, &wifi);
			ERROR_LOG("ssid:%s, pwd:%s\n", wifi.ssid, wifi.pwd);
			
		}
		else if(infoType == IT_STRING)
		{
			vr_decodeString(_recogStatus, _data, _dataLen, s, sizeof(s));
			ERROR_LOG("string:%s\n", s);

		}
		else if(infoType == IT_WIFI)
		{
			vr_decodeWiFi(_recogStatus, _data, _dataLen, &macWifi);
			ERROR_LOG("mac wifi:");
			for (i = 0; i < macWifi.macLen; i ++)
			{
				printf("0x%.2x ", macWifi.mac[i] & 0xff);
			}
			printf(", %s\n", macWifi.pwd);
		}
		else
		{
			ERROR_LOG("------------------recognized data:%s\n", _data);
		}

	}
	else
	{
		ERROR_LOG("------------------recognize invalid data, errorCode:%d, error:%s\n", _recogStatus, recorderRecogErrorMsg(_recogStatus));
		//vr_decodeString(3, _data, _dataLen, s, sizeof(s));
		ERROR_LOG("string:%s\n", _data);
	}
}

void *runRecorderVoiceRecognize( void * _recognizer) 
{
	vr_runRecognizer(_recognizer);
}

//录音机回调函数(AI视音频数据的传入口)
int recorderShortWrite(const void *_data, unsigned long _sampleCout)
{
	char *data = (char *)_data;
	//char* p = _data;
	//printf("((int)_sampleCout)*2 = %d  data: %x %x %x %x %x\n",((int)_sampleCout)*2,p[0],p[1],p[2],p[3],p[4]);
	return vr_writeData(recognizer, data, ((int)_sampleCout)*2);
}

int freqs[]= {15000,15200,15400,15600,15800,16000,16200,16400,16600,16800,17000,17200,17400,17600,17800,18000,18200,18400,18600};;

/*******************************************************************************
*@ Description    :声波识别业务启动
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
#define VOICE_SAMPLERATE  44100  //音频采样率
void* voice_recog_init(void*args)
{
	void *recorder = NULL;
	int baseFreq = 8000;//8000; //载波必须8K
	int i, r, ccc;
	//创建识别器，并设置监听器
	recognizer = vr_createVoiceRecognizer2(MemoryUsePriority,VOICE_SAMPLERATE);
	for(i = 0; i < sizeof(freqs)/sizeof(int); i ++)
	{
		freqs[i] = baseFreq + i * 150;
	}

	vr_setRecognizeFreqs(recognizer, freqs, sizeof(freqs)/sizeof(int));
	//设置识别监听器
	vr_setRecognizerListener(recognizer,NULL,recognizerStart,recognizerEnd);

	/*创建录音机	，传入的音频包长度为1024字节/包  = 512采样/包，
		这里不执行如下两个函数（创建接收线程），而是直接用 AI线程直接写入
	*/
	/*
	r = initRecorder(VOICE_SAMPLERATE, 1, 16,1024, &recorder);//要求录取short数据
	if(r != 0)
	{
		printf("recorder init error:%d", r);
		return NULL;
	}

	
	//开始录音  （用 AI线程直接写入）
	r = startRecord(recorder, recognizer, recorderShortWrite);//short数据
	if(r != 0)
	{
		printf("recorder record error:%d", r);
		return NULL;
	}
	*/
	
	//启动算法识别线程	
	pthread_t ntid;
	pthread_create(&ntid, NULL, runRecorderVoiceRecognize, recognizer);
	DEBUG_LOG("recognizer start ***** TID = %d *****\n",ntid);
	
	while(1/*网络没有连上*/)
	{
		sleep(3);
		printf("voiceRecog sleeping...\n");
	}

	printf("voiceRecog exit! #########################\n");

	
	//退出,停止录音
	r = stopRecord(recorder);
	if(r != 0)
	{
		printf("recorder stop record error:%d", r);
	}
	r = releaseRecorder(recorder);
	if(r != 0)
	{
		printf("recorder release error:%d", r);
	}

	//通知识别器停止，并等待识别器真正退出
	vr_stopRecognize(recognizer);
	do 
	{		
		printf("recognizer is quiting\n");
		sleep(1);
	} while (!vr_isRecognizerStopped(recognizer));

	//销毁识别器
	vr_destroyVoiceRecognizer(recognizer);

	return NULL;
}

/*******************************************************************************
*@ Description    :开始声波识别线程
*@ Input          :
*@ Output         :
*@ Return         :成功：线程的tid ；失败：-1
*@ attention      :
*******************************************************************************/
int voice_recog_thread_start (void)
{
	pthread_t ntid;
	int ret = pthread_create(&ntid, NULL, voice_recog_init, NULL);
	if (ret) 
	{
		ERROR_LOG("pthread_create fail: %d\n", ret);
		return -1;
	}
	DEBUG_LOG("voice_recog_thread_start ***** TID = %d *****\n",ntid);

	return ntid;
}

//int main()
//{
//	init_();
//	return 0;
//}


