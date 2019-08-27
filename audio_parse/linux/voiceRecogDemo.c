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
			printf("imei:%s, phoneName:%s", phone.imei, phone.phoneName);
		}
		else if(infoType == IT_SSID_WIFI)
		{
			vr_decodeSSIDWiFi(_recogStatus, _data, _dataLen, &wifi);
			printf("ssid:%s, pwd:%s\n", wifi.ssid, wifi.pwd);
		}
		else if(infoType == IT_STRING)
		{
			vr_decodeString(_recogStatus, _data, _dataLen, s, sizeof(s));
			printf("string:%s\n", s);

		}
		else if(infoType == IT_WIFI)
		{
			vr_decodeWiFi(_recogStatus, _data, _dataLen, &macWifi);
			printf("mac wifi:");
			for (i = 0; i < macWifi.macLen; i ++)
			{
				printf("0x%.2x ", macWifi.mac[i] & 0xff);
			}
			printf(", %s\n", macWifi.pwd);
		}
		else
		{
			printf("------------------recognized data:%s\n", _data);
		}

	}
	else
	{
		printf("------------------recognize invalid data, errorCode:%d, error:%s\n", _recogStatus, recorderRecogErrorMsg(_recogStatus));
		//vr_decodeString(3, _data, _dataLen, s, sizeof(s));
		printf("string:%s\n", _data);
	}
}

void runRecorderVoiceRecognize( void * _recognizer) 
{
	vr_runRecognizer(_recognizer);
}

//录音机回调函数
int recorderShortWrite(void *_writer, const void *_data, unsigned long _sampleCout)
{
	char *data = (char *)_data;
	void *recognizer = _writer;
	return vr_writeData(recognizer, data, ((int)_sampleCout)*2);
}

int freqs[]= {15000,15200,15400,15600,15800,16000,16200,16400,16600,16800,17000,17200,17400,17600,17800,18000,18200,18400,18600};;

void init_()
{
	void *recognizer=NULL ;	
	void *recorder = NULL;
	int baseFreq = 16000;
	int i, r, ccc;
	//创建识别器，并设置监听器
	recognizer = vr_createVoiceRecognizer2(MemoryUsePriority,44100);
	for(i = 0; i < sizeof(freqs)/sizeof(int); i ++)
	{
		freqs[i] = baseFreq + i * 150;
	}

	vr_setRecognizeFreqs(recognizer, freqs, sizeof(freqs)/sizeof(int));
	//设置识别监听器
	vr_setRecognizerListener(recognizer,NULL,recognizerStart,recognizerEnd);

	//创建录音机	
	r = initRecorder(44100, 1, 16, 512, &recorder);//要求录取short数据
	if(r != 0)
	{
		printf("recorder init error:%d", r);
		return;
	}
	//开始录音
	r = startRecord(recorder, recognizer, recorderShortWrite);//short数据
	if(r != 0)
	{
		printf("recorder record error:%d", r);
		return;
	}
	//开始识别
#ifdef WIN32
	//CreateThread( NULL, 0, runRecorderVoiceRecognize, recognizer, 0, 0 );
	_beginthread(runRecorderVoiceRecognize, 0, recognizer);
#else
	pthread_t ntid;
	pthread_create(&ntid, NULL, runRecorderVoiceRecognize, recognizer);
#endif
	printf("recognizer start\n");

	scanf_s("%c", &ccc);

	//停止录音
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
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif
	} while (!vr_isRecognizerStopped(recognizer));

	//销毁识别器
	vr_destroyVoiceRecognizer(recognizer);
}

int main()
{
	init_();
	return 0;
}


