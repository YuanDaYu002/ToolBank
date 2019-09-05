
#include <stdlib.h>
#include "audioRecorder.h"
#include "typeport.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>

//#include <los_task.h>



typedef struct 
{
	int 	sampleRateInHz;
	int 	channel;
	int 	audioFormat;
	int 	bufferSize;
	void 	*buffer;
	unsigned long 	sample_count; //buffer 中的audio样本数
	pthread_mutex_t lock;
    pthread_cond_t cond;
}Recorder_t;

static Recorder_t Recoder_attr = {0};


/************************************************************************/
/* 创建录音机
/* _sampleRateInHz为44100
/* _channel为单声道，1为单声道，2为立体声
/* _audioFormat为一个信号的bit数，单声道为16
/*返回值：
/*	成功：0 失败：-1
/************************************************************************/
VOICERECOGNIZEDLL_API int initRecorder(int _sampleRateInHz, int _channel, int _audioFormat, int _bufferSize, void **_precorder)
{	
	Recoder_attr.sampleRateInHz = _sampleRateInHz;  
	Recoder_attr.channel = _channel;
	Recoder_attr.audioFormat = _audioFormat;
	Recoder_attr.bufferSize = _bufferSize;
	Recoder_attr.buffer = (void*)malloc(_bufferSize);
	if(NULL == Recoder_attr.buffer)
	{
		ERROR_LOG("malloc failed!\n");
		return -1;
	}
	memset(Recoder_attr.buffer,0,_bufferSize);
	
	pthread_mutex_init(&Recoder_attr.lock, NULL);
    pthread_cond_init(&Recoder_attr.cond, NULL);

	*_precorder = &Recoder_attr;
	return 0;
}


/*******************************************************************************
*@ Description    :音频源数据发送函数，由 voice_src_data_func 线程函数 接收
*@ Input          :
*@ Output         :
*@ Return         :成功：0 ； 失败：-1
*@ attention      :
*******************************************************************************/
int send_pcm_audio_data(const void *_data, unsigned long _sampleCout) 
{
	if(NULL == _data || _sampleCout < 0)
	{
		ERROR_LOG("Illegal parameter!\n");
		return -1; 
	}

	int all_data_len = _sampleCout * 2;//总数据长度，*2代表一个采样点2个字节
	int writed_data_len = 0 ; //已经写入的数据长度
	int remain_data_len = all_data_len; //剩余数据长度
	
	if(all_data_len > Recoder_attr.bufferSize)//传入数据长度超过了buffer长度
	{
		/*---#按照每次最多512字节进行循环写入，直到写完------------------------------------------------------------*/
		while(writed_data_len < all_data_len)
		{
			pthread_mutex_lock(&Recoder_attr.lock);
			int copy_len = (remain_data_len > Recoder_attr.bufferSize) ? Recoder_attr.bufferSize : remain_data_len;
			memcpy(Recoder_attr.buffer , (char*)_data + writed_data_len , copy_len);
		
			Recoder_attr.sample_count = copy_len/2;
			//DEBUG_LOG("memcpy audio data!... copy_len(%d) Recoder_attr.sample_count(%d)\n",copy_len,Recoder_attr.sample_count);
			writed_data_len += copy_len;
			remain_data_len -= copy_len;
			
			pthread_cond_signal(&Recoder_attr.cond);
			pthread_mutex_unlock(&Recoder_attr.lock);
			usleep(20);//必须休眠，不然锁又会被该循环占用
		}
	}
	else //传入数据长度没有超过buffer长度
	{
		pthread_mutex_lock(&Recoder_attr.lock);
		//DEBUG_LOG("memcpy audio data! size = 512 ...\n");
		memcpy(Recoder_attr.buffer , _data , _sampleCout*(16/8));
		Recoder_attr.sample_count = _sampleCout;
		
		pthread_cond_signal(&Recoder_attr.cond);
	 	pthread_mutex_unlock(&Recoder_attr.lock);
	}

	 return 0;

}

/*******************************************************************************
*@ Description    :将PCM源数据处放入到音频解析系统
*@ Input          :
*@ Output         :
*@ Return         :
*@ attention      :
*******************************************************************************/
void *g_writer = NULL;
void* voice_src_data_func ( void * attr) 
{
	pthread_detach(pthread_self());
	r_pwrite write_func = (r_pwrite)attr;

	while(1/*设备没有连上网*/)
	{
		/*---#获取原始音频数据，按照 Recoder_attr 要求进行拼凑------------------------------------------*/
		pthread_mutex_lock(&Recoder_attr.lock);
			pthread_cond_wait(&Recoder_attr.cond, &Recoder_attr.lock);

		/*---#回调函数将原始数据传回识别系统------------------------------------------------------------*/
		if(NULL == g_writer)
		{
			ERROR_LOG("NULL == g_writer");
			pthread_exit(NULL);
		}
		
		int ret = write_func(g_writer,Recoder_attr.buffer,Recoder_attr.sample_count);
		if(ret < 0)
		{
			ERROR_LOG("write_func failed!\n");
			pthread_mutex_unlock(&Recoder_attr.lock);
			pthread_exit(NULL);
		}
		//DEBUG_LOG("write_func success! Recoder_attr.sample_count = %d\n",Recoder_attr.sample_count);

		pthread_mutex_unlock(&Recoder_attr.lock);
		


	}

	pthread_exit(NULL);

	
}


/************************************************************************/
/* 开始录音
/************************************************************************/
VOICERECOGNIZEDLL_API int startRecord(void *_recorder, void *_writer, r_pwrite _pwrite)
{
//	Recorder_t* Recoder_attr = (Recorder_t*)_recorder;
//	struct VoiceRecognizer *recognizer = (struct VoiceRecognizer *)_writer;

	g_writer = _writer;
	pthread_t ntid;
	int ret = pthread_create(&ntid, NULL, voice_src_data_func, (void*)_pwrite);
	if (ret) 
	{
		ERROR_LOG("pthread_create fail: %d\n", ret);
		return -1;
	}
	DEBUG_LOG("***** TID = %d *****\n",ntid);

	
	return 0;
}

/************************************************************************/
/* 停止录音，该函数需同步返回（销毁函数是真正停止后才能释放内存）
/************************************************************************/
VOICERECOGNIZEDLL_API int stopRecord(void *_recorder)
{
	return 0;
}

/************************************************************************/
/* 释放录音器的资源
/************************************************************************/
VOICERECOGNIZEDLL_API int releaseRecorder(void *_recorder)
{
	Recorder_t* Recoder_attr = (Recorder_t*)_recorder;
	if(Recoder_attr->buffer)
	{
		free(Recoder_attr->buffer);
		Recoder_attr->buffer = NULL;
	}
	
	return 0;
}
