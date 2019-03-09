

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>  
#include <sys/un.h> 
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>

#include "in.h"


#include "mpi_sys.h"

//#include "hi_comm_aio.h"
#include "mpi_ai.h"
#include "hi_comm_aenc.h"
#include "mpi_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_adec.h"
#include "mpi_ao.h"
#include "acodec.h"

#include "hal_def.h"
//#include "hal.h"
#include "audio.h"
#include "fifo.h"
#include "encoder.h"
#include "shine/layer3.h"
#include "comm_sys.h"
#include "EasyAACEncoderAPI.h"

 




#define TALK_BUF_SIZE (8*1024)


static int audio_init = 0;

typedef struct
{
    int getdata_enable;
    int putdata_enable;
    FIFO_HANDLE fifo;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} TALKBACK_CONTEXT;

static TALKBACK_CONTEXT tb_ctx;

#if USE_HISI_AENC
	AIO_ATTR_S g_aio_attr[2] = {
		{AUDIO_SAMPLE_RATE_8000, AUDIO_BIT_WIDTH_16, AIO_MODE_I2S_MASTER, AUDIO_SOUND_MODE_MONO, 0, 20, AUDIO_PTNUMPERFRM, 1, 0},
	    {AUDIO_SAMPLE_RATE_8000, AUDIO_BIT_WIDTH_16, AIO_MODE_I2S_MASTER, AUDIO_SOUND_MODE_MONO, 0, 20, AUDIO_PTNUMPERFRM, 1, 0}
	};
#else //使用AAC编码 16K采样率 ,注意AUDIO_PTNUMPERFRM 宏的值
	AIO_ATTR_S g_aio_attr[2] = {
		{AUDIO_SAMPLE_RATE_16000, AUDIO_BIT_WIDTH_16, AIO_MODE_I2S_MASTER, AUDIO_SOUND_MODE_MONO, 0, 20, AUDIO_PTNUMPERFRM, 1, 0},
	    {AUDIO_SAMPLE_RATE_16000, AUDIO_BIT_WIDTH_16, AIO_MODE_I2S_MASTER, AUDIO_SOUND_MODE_MONO, 0, 20, AUDIO_PTNUMPERFRM, 1, 0}
	};
#endif

static AENC_ATTR_ADPCM_S lAencAdpcm = {
	ADPCM_TYPE_DVI4
};
static ADEC_ATTR_G711_S stAencG711 = {0};
static AENC_CHN_ATTR_S lAencAttr = {
	//PT_ADPCMA, AUDIO_PTNUMPERFRM, 20, &lAencAdpcm
	PT_G711A, AUDIO_PTNUMPERFRM, 20, &stAencG711
};


static ADEC_ATTR_ADPCM_S lAdecAdpcm = {
	ADPCM_TYPE_DVI4
};
static ADEC_ATTR_G711_S stAdecG711 = {0};
static ADEC_CHN_ATTR_S lAdecAttr = {
//	PT_ADPCMA, 20, ADEC_MODE_STREAM, &lAdecAdpcm
	PT_G711A,20,ADEC_MODE_PACK,&stAdecG711
};

static shine_config_t lShineConfig = {
    {PCM_MONO, 44100},
    {MONO, 32, NONE, 0, 1}
};

#if 0

void set_ai_samplerate(int samplerate, int perfrm)
{
    g_aio_attr[0].enSamplerate = samplerate;
    g_aio_attr[0].u32PtNumPerFrm = perfrm;

    /*

    int fd = open("/dev/acodec", O_RDWR);
    unsigned int adc_hpf; 
    adc_hpf = 0x1; 
    if (ioctl(fd, ACODEC_SET_ADC_HP_FILTER, &adc_hpf)) 
    { 
                    printf("ioctl err!\n"); 
    } 


    close(fd);

    int fd = open("/dev/acodec", O_RDWR);
    unsigned int gain_mic = 15;
    if (ioctl(fd, ACODEC_SET_GAIN_MICL, &gain_mic))
    {
                    printf("%s: set l acodec micin volume failed\n", __FUNCTION__);
                    close(fd);
                    return HI_FAILURE;
    }
	
    if (ioctl(fd, ACODEC_GET_GAIN_MICL, &gain_mic))
    {
                    printf("%s: set l acodec micin volume failed\n", __FUNCTION__);
                    close(fd);
                    return HI_FAILURE;
    }
    printf("gain_mic...................%d\n",gain_mic);
	
    gain_mic= 0;
    if (ioctl(fd, ACODEC_SET_GAIN_MICR, &gain_mic))
    {
                    printf("%s: set r acodec micin volume failed\n", __FUNCTION__);
                    close(fd);
                    return HI_FAILURE;
    }
	

    if (ioctl(fd, ACODEC_GET_GAIN_MICR, &gain_mic))
    {
                    printf("%s: set r acodec micin volume failed\n", __FUNCTION__);
                    close(fd);
                    return HI_FAILURE;
    }
    printf("gain_mic...................%d\n",gain_mic);

    close(fd);
     */
}
#endif

static int audio_config_acodec(AUDIO_SAMPLE_RATE_E sampleRate)
{
#define ACODEC_FILE  "/dev/acodec"
    int fd = -1;

    fd = open(ACODEC_FILE, O_RDWR);
    if (fd < 0) {
        ERROR_LOG("can't open acodec: %s\n", ACODEC_FILE);
        return HLE_RET_EIO;
    }

    if (ioctl(fd, ACODEC_SOFT_RESET_CTRL)) {
        ERROR_LOG("Reset audio codec error\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    ACODEC_FS_E fs;
    if (AUDIO_SAMPLE_RATE_8000 == sampleRate) {
        fs = ACODEC_FS_8000;
    }
    else if (AUDIO_SAMPLE_RATE_16000) {
        fs = ACODEC_FS_16000;
    }
    else if (AUDIO_SAMPLE_RATE_44100 == sampleRate) {
        fs = ACODEC_FS_44100;
    }
    else {
        ERROR_LOG("not support enSample:%d\n", sampleRate);
        close(fd);
        return HLE_RET_ERROR;
    }

    if (ioctl(fd, ACODEC_SET_I2S1_FS, &fs)) {
        ERROR_LOG("set acodec sample rate failed\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    ACODEC_MIXER_E im = ACODEC_MIXER_IN_D;//ACODEC_MIXER_IN1;
    if (ioctl(fd, ACODEC_SET_MIXER_MIC, &im)) {
        ERROR_LOG("set acodec input mode failed\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    int iv = 30;
    if (ioctl(fd, ACODEC_SET_INPUT_VOL, &iv)) {
        ERROR_LOG("set acodec micin volume failed\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    int ov;
    ov = -10;//0;//6; // default -21,range -121~6  输出总音量控制，数值越大声音越大，但会引入噪音
    if (ioctl(fd, ACODEC_SET_OUTPUT_VOL, &ov)) {
        ERROR_LOG("set acodec output volume failed\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    ACODEC_VOL_CTRL vc;
    vc.vol_ctrl_mute = 0;
    vc.vol_ctrl = 3;//0; // default 6 range 127~0 左声道输出音量控制，赋值越大，音量越小
    if (ioctl(fd, ACODEC_SET_DACL_VOL, &vc)) {
        ERROR_LOG("set acodec dacl volume failed\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    int eb = 1;
    if (ioctl(fd, ACODEC_ENABLE_BOOSTL, &eb)) {
        ERROR_LOG("set acodec boost failed\n");
        close(fd);
        return HLE_RET_ERROR;
    }

    close(fd);
    return HLE_RET_OK;
}


static int audio_config_TalkVqeAttr(HI_S32 AiDevId, HI_S32 AiChn, HI_S32 AoDevId, HI_S32 AoChn)
{

	AI_TALKVQE_CONFIG_S ai_talkvqe;
	memset(&ai_talkvqe,0,sizeof(AI_TALKVQE_CONFIG_S));	
	 
	ai_talkvqe.u32OpenMask =  AI_TALKVQE_MASK_HPF|AI_TALKVQE_MASK_AEC|AI_TALKVQE_MASK_ANR;
	ai_talkvqe.s32WorkSampleRate = AUDIO_SAMPLE_RATE_16000;//AUDIO_SAMPLE_RATE_8000;
	ai_talkvqe.s32FrameSample = AUDIO_PTNUMPERFRM;
	ai_talkvqe.enWorkstate = 0;
	
	ai_talkvqe.stHpfCfg.bUsrMode = 1;
	ai_talkvqe.stHpfCfg.enHpfFreq = AUDIO_HPF_FREQ_80;

	//AEC config
	ai_talkvqe.stAecCfg.bUsrMode = 1;
	ai_talkvqe.stAecCfg.s8CngMode = 1;//舒适噪音模式
	ai_talkvqe.stAecCfg.s8NearAllPassEnergy = 1;
	ai_talkvqe.stAecCfg.s8NearCleanSupEnergy = 2;
	ai_talkvqe.stAecCfg.s16DTHnlSortQTh = 16384;
	ai_talkvqe.stAecCfg.s16EchoBandLow = 10;
	ai_talkvqe.stAecCfg.s16EchoBandHigh = 41;
	ai_talkvqe.stAecCfg.s16EchoBandLow2 = 47;
	ai_talkvqe.stAecCfg.s16EchoBandHigh2 = 63;
	HI_S16 s16ERLBand[6] = {4, 6, 36, 49, 50, 51};
	HI_S16 s16ERL[7] = {7, 10, 16, 10, 18, 18, 18};
	memcpy(ai_talkvqe.stAecCfg.s16ERLBand,s16ERLBand,sizeof(s16ERLBand));
	memcpy(ai_talkvqe.stAecCfg.s16ERL,s16ERL,sizeof(s16ERL));
	ai_talkvqe.stAecCfg.s16VioceProtectFreqL = 3;
	ai_talkvqe.stAecCfg.s16VioceProtectFreqL1 = 6;

	//ANR config
	ai_talkvqe.stAnrCfg.bUsrMode = 1;
	ai_talkvqe.stAnrCfg.s16NrIntensity = 5;
	ai_talkvqe.stAnrCfg.s16NoiseDbThr = 45;
	ai_talkvqe.stAnrCfg.s8SpProSwitch = 0;

	
	ai_talkvqe.stAgcCfg.bUsrMode = 0;
	//ai_talkvqe.stEqCfg.stEqCfg =  
	ai_talkvqe.stHdrCfg.bUsrMode = 0;

	HI_S32 ret = HI_MPI_AI_SetTalkVqeAttr(AiDevId,AiChn,AoDevId,AoChn,&ai_talkvqe);
	if(ret < 0)
	{
		ERROR_LOG("HI_MPI_AI_SetTalkVqeAttr failed! ret(%0x)\n",ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_AI_EnableVqe(AiDevId, AiChn);
	if(ret < 0)
	{
		ERROR_LOG("HI_MPI_AI_EnableVqe failed! ret(%0x)\n",ret);
		return HLE_RET_ERROR;
	}
	
	return HLE_RET_OK;

}




static int audio_start_aio(void)
{
    HLE_S32 ret;

    //配置AI设备并使能
    ret = audio_config_acodec(g_aio_attr[0].enSamplerate);
    if (HLE_RET_OK != ret) {
        return HLE_RET_ERROR;
    }
		
    ret = HI_MPI_AI_SetPubAttr(AI_DEVICE_ID, &g_aio_attr[0]);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AI_SetPubAttr fail: %#x\n", ret);
        return HLE_RET_ERROR;
    }
    ret = HI_MPI_AI_Enable(AI_DEVICE_ID);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AI_Enable(%d) fail: %#x\n", AI_DEVICE_ID, ret);
        return HLE_RET_ERROR;
    }

    //配置AO设备并使能
    ret = HI_MPI_AO_SetPubAttr(AO_DEVICE_ID, &g_aio_attr[1]);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AO_SetPubAttr(%d) fail: %#x!\n", AO_DEVICE_ID, ret);
        return HLE_RET_ERROR;
    }
    ret = HI_MPI_AO_Enable(AO_DEVICE_ID);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AO_Enable(%d) fail: %#x!\n", AO_DEVICE_ID, ret);
        return HLE_RET_ERROR;
    }

	
    //配置所有AI通道并使能
    ret = HI_MPI_AI_EnableChn(AI_DEVICE_ID, TALK_AI_CHN);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AI_EnableChn(%d,%d) fail: %#x\n", AI_DEVICE_ID, TALK_AI_CHN, ret);
        return HLE_RET_ERROR;
    }


	//配置AEC + ANR参数并使能
	#if 0
	ret = audio_config_TalkVqeAttr(AI_DEVICE_ID,TALK_AI_CHN,AO_DEVICE_ID,TALK_AO_CHN);
    if (ret != HLE_RET_OK) {
    ERROR_LOG("audio_config_TalkVqeAttr(%d,%d,%d,%d) failed!\n", AI_DEVICE_ID,TALK_AI_CHN,AO_DEVICE_ID,TALK_AO_CHN);
    return HLE_RET_ERROR;
	}
	#endif
	
    return HLE_RET_OK;
}

static int audio_stop_ai(void)
{
    HLE_S32 ret;
    ret = HI_MPI_AI_DisableChn(AI_DEVICE_ID, TALK_AI_CHN);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AI_DisableChn(%d, %d) fail: %#x!\n", AI_DEVICE_ID, TALK_AI_CHN, ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
}

#if 0
#define MP3_ENC_BUF_LEN     (576 * 2 * 2)

static struct
{
    volatile int run;
    pthread_t tid;
    shine_t hdlShine;
    HLE_U8 inBuf[MP3_ENC_BUF_LEN];
    int inIdx;
} lMp3EncCtx = {0};

static void* mp3_enc_proc(void* para)
{
    while (lMp3EncCtx.run) {
		AUDIO_FRAME_S audFrm;
		AEC_FRAME_S aecFrm;
		int ret = HI_MPI_AI_GetFrame(AI_DEVICE_ID, TALK_AI_CHN, &audFrm, &aecFrm, 1000);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_AI_GetFrame fail: %#x\n", ret);
			continue;
		}
		if ((lMp3EncCtx.inIdx + audFrm.u32Len) <= MP3_ENC_BUF_LEN) {
			memcpy(&lMp3EncCtx.inBuf[lMp3EncCtx.inIdx], audFrm.pVirAddr[0], audFrm.u32Len);
			lMp3EncCtx.inIdx += audFrm.u32Len;
		} else {
			int len = MP3_ENC_BUF_LEN - lMp3EncCtx.inIdx;
			memcpy(&lMp3EncCtx.inBuf[lMp3EncCtx.inIdx], audFrm.pVirAddr[0], len);
			
			int mp3Len;
			HLE_U8* mp3Frm = shine_encode_buffer_interleaved(lMp3EncCtx.hdlShine, (HLE_S16*)lMp3EncCtx.inBuf, &mp3Len);
			if (mp3Frm) {
				// todo: put mp3Frm to fifo
			}

			lMp3EncCtx.inIdx = audFrm.u32Len - len;
			memcpy(lMp3EncCtx.inBuf, (HLE_U8*)(audFrm.pVirAddr[0]) + len, lMp3EncCtx.inIdx);
		}
		
		// todo: release
    }

}
#endif

#ifndef USE_AI_AENC_BIND
static volatile int lRunAiAenc = 0;
static FIFO_HANDLE* hdlAiFifo = NULL;

void request_ai(FIFO_HANDLE* fifo)
{
    hdlAiFifo = fifo;
}

void release_ai(void)
{
    hdlAiFifo = NULL;
}


#define WRITE_AFRAME_LOCAL	0  //调试，将AI获取的audio数据写到本地
#define RAW_BUF_SIZE  (1024*500) //记录PCM原始帧数据
int AI_to_AO_start = 0;
/*使用海思自带AENC编码模式（g711编码）*/
static void* ai_aenc_proc(void* para)
{
    para = para;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 AiFd;
    HI_S32 s32Ret;
    AUDIO_FRAME_S audFrm;
    AEC_FRAME_S aecFrm;

	//初始化 AAC编码的发送端

	
	#if 1
		//设置音频帧缓存深度
		AI_CHN_PARAM_S ai_ch_param;
		HI_MPI_AI_GetChnParam(AI_DEVICE_ID,TALK_AI_CHN,&ai_ch_param);
	
		ai_ch_param.u32UsrFrmDepth = 5;
		int ret1 = HI_MPI_AI_SetChnParam(AI_DEVICE_ID,TALK_AI_CHN,&ai_ch_param);
		if (HI_SUCCESS != ret1)
		{   
			ERROR_LOG("HI_MPI_AI_SetChnParam err :0x%x\n",ret1);      
			//return HI_FAILURE;
			return NULL;
		}
	#endif	

	FD_ZERO(&read_fds);
	AiFd = HI_MPI_AI_GetFd(AI_DEVICE_ID, TALK_AI_CHN);
    FD_SET(AiFd, &read_fds);
	
	#if WRITE_AFRAME_LOCAL
		int aud = open("/jffs0/rawaudio", O_CREAT | O_WRONLY | O_TRUNC, 0664);
	#endif
	
    while (lRunAiAenc ) 
	{
		if(AI_to_AO_start)
		{
	        TimeoutVal.tv_sec = 1;
	        TimeoutVal.tv_usec = 0;

	        FD_ZERO(&read_fds);
			FD_SET(AiFd, &read_fds);

	        s32Ret = select(AiFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
	        if (s32Ret < 0)
	        {
	        	ERROR_LOG("select error!\n");
	            break;
	        }
	        else if (0 == s32Ret)
	        {
	            printf("%s: get ai frame select time out\n", __FUNCTION__);
	            break;
	        }

	        if (FD_ISSET(AiFd, &read_fds))
	        {
				//从AI获取帧数据
	            memset(&aecFrm, 0, sizeof(AEC_FRAME_S));
	            int ret = HI_MPI_AI_GetFrame(AI_DEVICE_ID, TALK_AI_CHN, &audFrm, &aecFrm, 0);
	            if (HI_SUCCESS != ret) 
				{
	                ERROR_LOG("HI_MPI_AI_GetFrame fail: %#x\n", ret);
	                continue;
	            }

				#if WRITE_AFRAME_LOCAL
					int size = write(aud, audFrm.pVirAddr[0] , audFrm.u32Len);
					if(size >0)
						printf("write one Audio frame (%d)bytes!\n",size);
					else
						printf("write one Audio frame error!\n");
				#endif
				
	       		/* send frame to encoder */
		            ret = HI_MPI_AENC_SendFrame(TALK_AENC_CHN, &audFrm, &aecFrm);
		            if (HI_SUCCESS != ret) 
					{
		                ERROR_LOG("HI_MPI_AENC_SendFrame fail: %#x\n", ret);
		            }
					
				#if 1
					/* send frame to AO */
					ret = HI_MPI_AO_SendFrame(AO_DEVICE_ID, TALK_AO_CHN, &audFrm, 1000);
					if (HI_SUCCESS != ret) 
					{
		                ERROR_LOG("HI_MPI_AO_SendFrame fail: %#x\n", ret);
		            }
				#endif

			
				#if 1
		            if (hdlAiFifo) 
					{
		                fifo_in(hdlAiFifo, audFrm.pVirAddr[0], audFrm.u32Len, 1);
		            }
	      	  	#endif
			
	            ret = HI_MPI_AI_ReleaseFrame(AI_DEVICE_ID, TALK_AI_CHN, &audFrm, &aecFrm);
	            if (HI_SUCCESS != ret) 
				{
	                ERROR_LOG("HI_MPI_AI_ReleaseFrame fail: %#x\n", ret);
	            }
	        }
		}
		else
		{
			sleep(1);
		}
		
		
    }

	#if WRITE_AFRAME_LOCAL
		close(aud);
	#endif
	
	return NULL;
	
}

/*使用AAC编码模式*/
static void* ai_AAC_proc(void* para)
{
    para = para;
    struct timeval TimeoutVal;
    fd_set read_fds;
    HI_S32 AiFd;
    HI_S32 s32Ret;
    AUDIO_FRAME_S audFrm;
    AEC_FRAME_S aecFrm;

	//初始化 AAC编码的发送端

	
	#if 1
		//设置音频帧缓存深度
		AI_CHN_PARAM_S ai_ch_param;
		HI_MPI_AI_GetChnParam(AI_DEVICE_ID,TALK_AI_CHN,&ai_ch_param);
	
		ai_ch_param.u32UsrFrmDepth = 5;
		int ret1 = HI_MPI_AI_SetChnParam(AI_DEVICE_ID,TALK_AI_CHN,&ai_ch_param);
		if (HI_SUCCESS != ret1)
		{   
			ERROR_LOG("HI_MPI_AI_SetChnParam err :0x%x\n",ret1);      
			return NULL;
		}
	#endif	

	FD_ZERO(&read_fds);
	AiFd = HI_MPI_AI_GetFd(AI_DEVICE_ID, TALK_AI_CHN);
    FD_SET(AiFd, &read_fds);
	
	#if WRITE_AFRAME_LOCAL
		int aud = open("/jffs0/rawaudio", O_CREAT | O_WRONLY | O_TRUNC, 0664);
		unsigned char*raw_buf = (unsigned char*)malloc(RAW_BUF_SIZE);
		if(NULL == raw_buf)
		{
			ERROR_LOG("malloc failed !\n");
			return NULL;
		}
		memset(raw_buf,0,RAW_BUF_SIZE);
		unsigned int raw_buf_w_pos = 0;
	#endif
	
	int debug_tmp_i = 0;
    while (lRunAiAenc ) 
	{
		if(AI_to_AO_start)
		{
	        TimeoutVal.tv_sec = 1;
	        TimeoutVal.tv_usec = 0;

	        FD_ZERO(&read_fds);
			FD_SET(AiFd, &read_fds);

	        s32Ret = select(AiFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
	        if (s32Ret < 0)
	        {
	        	ERROR_LOG("select error!\n");
	            break;
	        }
	        else if (0 == s32Ret)
	        {
	            printf("%s: get ai frame select time out\n", __FUNCTION__);
	            break;
	        }

	        if (FD_ISSET(AiFd, &read_fds))
	        {
				//从AI获取帧数据
	            memset(&aecFrm, 0, sizeof(AEC_FRAME_S));
	            int ret = HI_MPI_AI_GetFrame(AI_DEVICE_ID, TALK_AI_CHN, &audFrm, &aecFrm, 0);
	            if (HI_SUCCESS != ret) 
				{
	                ERROR_LOG("HI_MPI_AI_GetFrame fail: %#x\n", ret);
	                continue;
	            }
				//DEBUG_LOG("audFrm.u32Len(%d)\n",audFrm.u32Len);

				#if WRITE_AFRAME_LOCAL
				if(raw_buf_w_pos < RAW_BUF_SIZE)
				{
					if(RAW_BUF_SIZE - raw_buf_w_pos >= audFrm.u32Len)//剩余空间足以放入一帧PCM数据
					{
						memcpy(raw_buf + raw_buf_w_pos , audFrm.pVirAddr[0],audFrm.u32Len);
						raw_buf_w_pos += audFrm.u32Len;

					}
					
					if(RAW_BUF_SIZE - raw_buf_w_pos < audFrm.u32Len)  //剩余空间不足以放入下一帧PCM数据
					{
						int size = write(aud, raw_buf , RAW_BUF_SIZE);
						if(size >0)
							ERROR_LOG("write rawaudio file success !\n");
						else
							ERROR_LOG("write rawaudio file failed  !\n");
						
						raw_buf_w_pos = RAW_BUF_SIZE;//不再进入到写文件这个调试逻辑中
					}
			
				}
				#endif

				
	       		/* send frame to encoder */
		 		//发送给AAC编码 
		 		#if 1
				debug_tmp_i ++;
				if(debug_tmp_i >= 1)
				{
					//DEBUG_LOG("audFrm.u32Len(%d)\n",audFrm.u32Len);
					debug_tmp_i = 0;
				}
					
				ret = AAC_AENC_SendFrame(&audFrm);	
				if(ret < 0)
				{
					ERROR_LOG("AAC_AENC_SendFrame failed ! ret(%d)\n",ret);
				}
				#endif
	  
					
				#if 1
					/* send frame to AO */
					ret = HI_MPI_AO_SendFrame(AO_DEVICE_ID, TALK_AO_CHN, &audFrm, 1000);
					if (HI_SUCCESS != ret) 
					{
		                ERROR_LOG("HI_MPI_AO_SendFrame fail: %#x\n", ret);
		            }
				#endif

			
				#if 1
		            if (hdlAiFifo) 
					{
		                fifo_in(hdlAiFifo, audFrm.pVirAddr[0], audFrm.u32Len, 1);
		            }
	      	  	#endif
	
	            ret = HI_MPI_AI_ReleaseFrame(AI_DEVICE_ID, TALK_AI_CHN, &audFrm, &aecFrm);
	            if (HI_SUCCESS != ret) 
				{
	                ERROR_LOG("HI_MPI_AI_ReleaseFrame fail: %#x\n", ret);
	            }
	        }
		}
		else
		{
			sleep(1);
		}
		
		
    }

	#if WRITE_AFRAME_LOCAL
		close(aud);
	#endif
	
	return NULL;
	
}

#endif

static int audio_stop_aenc(void)
{
#if 1
    int ret;

#ifdef USE_AI_AENC_BIND
    MPP_CHN_S src, dst;
    src.enModId = HI_ID_AI;
    src.s32DevId = AI_DEVICE_ID;
    src.s32ChnId = TALK_AI_CHN;
    dst.enModId = HI_ID_AENC;
    dst.s32DevId = 0;
    dst.s32ChnId = TALK_AENC_CHN;
    ret = HI_MPI_SYS_UnBind(&src, &dst);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_SYS_UnBind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
                  src.enModId, src.s32DevId, src.s32ChnId,
                  dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
        return HLE_RET_ERROR;
    }
#else
	lRunAiAenc = 0;
#endif

    ret = HI_MPI_AENC_DestroyChn(TALK_AENC_CHN);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AENC_DestroyChn(%d) fail: %#x!\n", TALK_AENC_CHN, ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
#else
    if (lMp3EncCtx.run) {
        lMp3EncCtx.run = 0;
        pthread_join(lMp3EncCtx.tid, NULL);
        shine_close(lMp3EncCtx.hdlShine);
    }
#endif
}

static int audio_start_aenc(void)
{
#if 1
    int ret = HI_MPI_AENC_CreateChn(TALK_AENC_CHN, &lAencAttr);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AENC_CreateChn(%d) fail: %#x!\n", TALK_AENC_CHN, ret);
        return HLE_RET_ERROR;
    }

	#ifdef USE_AI_AENC_BIND
	    MPP_CHN_S src, dst;
	    src.enModId = HI_ID_AI;
	    src.s32DevId = AI_DEVICE_ID;
	    src.s32ChnId = TALK_AI_CHN;
	    dst.enModId = HI_ID_AENC;
	    dst.s32DevId = 0;
	    dst.s32ChnId = TALK_AENC_CHN;
	    ret = HI_MPI_SYS_Bind(&src, &dst);
	    DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
	    if (ret != HI_SUCCESS) 
		{
	        ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
	                  src.enModId, src.s32DevId, src.s32ChnId,
	                  dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
	        return HLE_RET_ERROR;
	    }
	#else
		pthread_t tid;
		lRunAiAenc = 1;
		ret = pthread_create(&tid, NULL, ai_aenc_proc, NULL);
		if (ret) {
			ERROR_LOG("pthread_create fail: %d\n", ret);
			return HLE_RET_ERROR;
		}
		pthread_detach(tid);
	#endif

    return HLE_RET_OK;
#else
    if (0 == lMp3EncCtx.run) 
	{
        lMp3EncCtx.hdlShine = shine_initialise(&lShineConfig);
        lMp3EncCtx.run = 1;
        int ret = pthread_create(&lMp3EncCtx.tid, NULL, mp3_enc_proc, NULL);
        if (ret) 
		{
            ERROR_LOG("pthread_create fail: %#x\n", ret);
        }
    }
#endif
}

static int start_talkback_ao(void)
{

    int ret = HI_MPI_AO_EnableChn(AO_DEVICE_ID, TALK_AO_CHN);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AO_EnableChn(%d) fail: %#x!\n", TALK_AO_CHN, ret);
        return HLE_RET_ERROR;
    }

#if 1
    MPP_CHN_S src, dst;
    src.enModId = HI_ID_ADEC;
    src.s32DevId = 0;
    src.s32ChnId = TALK_ADEC_CHN;
    dst.enModId = HI_ID_AO;
    dst.s32DevId = AO_DEVICE_ID;
    dst.s32ChnId = TALK_AO_CHN;
    ret = HI_MPI_SYS_Bind(&src, &dst);
    DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
    if (ret != HI_SUCCESS) {
        HI_MPI_AO_DisableChn(AO_DEVICE_ID, TALK_AO_CHN);
        ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
                  src.enModId, src.s32DevId, src.s32ChnId,
                  dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
        return HLE_RET_ERROR;
    }
#else
#endif

    return HLE_RET_OK;
}

static int start_talkback_adec(void)
{
#if 1
    HLE_S32 ret = HI_MPI_ADEC_CreateChn(TALK_ADEC_CHN, &lAdecAttr);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_ADEC_CreateChn(%d) fail: %#x!\n", TALK_ADEC_CHN, ret);
        return HLE_RET_ERROR;
    }

    return HLE_RET_OK;
#else
#endif
}

static void stop_talkback_adec(void)
{
#if 1
    MPP_CHN_S src, dst;
    src.enModId = HI_ID_ADEC;
    src.s32DevId = 0;
    src.s32ChnId = TALK_ADEC_CHN;
    dst.enModId = HI_ID_AO;
    dst.s32DevId = AO_DEVICE_ID;
    dst.s32ChnId = TALK_AO_CHN;
    int ret = HI_MPI_SYS_UnBind(&src, &dst);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_SYS_UnBind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
                  src.enModId, src.s32DevId, src.s32ChnId,
                  dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
    }

    ret = HI_MPI_ADEC_DestroyChn(TALK_ADEC_CHN);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_ADEC_DestroyChn(%d) fail: %#x!\n", TALK_ADEC_CHN, ret);
    }
#else
#endif
}

static void stop_talkback_ao(void)
{
    int ret = HI_MPI_AO_DisableChn(AO_DEVICE_ID, TALK_AO_CHN);
    if (ret != HI_SUCCESS) {
        ERROR_LOG("HI_MPI_AO_DisableChn(%d, %d) fail: %#x!\n",
                  AO_DEVICE_ID, TALK_AO_CHN, ret);
    }
}

int add_to_talkback_fifo(void *data, int size)
{
    if (audio_init == 0)
        return HLE_RET_ENOTINIT;

    //DEBUG_LOG("add_to_talkback_fifo size %d\n", size);

    pthread_mutex_lock(&tb_ctx.lock);
    if (tb_ctx.getdata_enable == 0) {
        pthread_mutex_unlock(&tb_ctx.lock);
        return HLE_RET_ENOTINIT;
    }

    int avail = fifo_avail(tb_ctx.fifo, 0);
    if (avail < size) {
        DEBUG_LOG("not enough space to hold\n");
        pthread_mutex_unlock(&tb_ctx.lock);
        return -1;
    }

    int ret = fifo_in(tb_ctx.fifo, data, size, 0);
    if (avail == TALK_BUF_SIZE) {
        pthread_cond_signal(&tb_ctx.cond);
    }

    pthread_mutex_unlock(&tb_ctx.lock);
    return ret;
}

int audioin_get_chns(void)
{
    return 1;
}

int talkback_get_audio_fmt(TLK_AUDIO_FMT *aenc_fmt, TLK_AUDIO_FMT *adec_fmt)
{
    aenc_fmt->enc_type = AENC_STD_ADPCM;
    aenc_fmt->sample_rate = AUDIO_SR_8000;
    aenc_fmt->bit_width = AUDIO_BW_16;
    aenc_fmt->sound_mode = AUDIO_SM_MONO;

    adec_fmt->enc_type = AENC_STD_ADPCM;
    adec_fmt->sample_rate = AUDIO_SR_8000;
    adec_fmt->bit_width = AUDIO_BW_16;
    adec_fmt->sound_mode = AUDIO_SM_MONO;

    return 0;
}

int talkback_start(int getmode, int putmode)
{
    if ((audio_init == 0) || ((getmode == 0) && (putmode == 0)))
        return HLE_RET_ENOTINIT;

    pthread_mutex_lock(&tb_ctx.lock);
    if (putmode) {
        if (tb_ctx.putdata_enable) {
            pthread_mutex_unlock(&tb_ctx.lock);
            return HLE_RET_EBUSY;
        }

        if (HLE_RET_OK != start_talkback_adec()) {
            pthread_mutex_unlock(&tb_ctx.lock);
            return HLE_RET_ERROR;
        }

        if (HLE_RET_OK != start_talkback_ao()) {
            stop_talkback_adec();
            pthread_mutex_unlock(&tb_ctx.lock);
            return HLE_RET_ERROR;
        }

        tb_ctx.putdata_enable = 1;
    }

    if (getmode) {
        if (tb_ctx.getdata_enable) {
            pthread_mutex_unlock(&tb_ctx.lock);
            return HLE_RET_EBUSY;
        }

        fifo_reset(tb_ctx.fifo, 0);
        tb_ctx.getdata_enable = 1;
    }

    pthread_mutex_unlock(&tb_ctx.lock);
    DEBUG_LOG("talkback_start success\n");
    return HLE_RET_OK;
}

int talkback_stop(void)
{
    if (audio_init == 0)
        return HLE_RET_ENOTINIT;

    pthread_mutex_lock(&tb_ctx.lock);
    if (tb_ctx.putdata_enable != 0) {
        stop_talkback_ao();
        stop_talkback_adec();
        tb_ctx.putdata_enable = 0;
    }

    tb_ctx.getdata_enable = 0;
    pthread_mutex_unlock(&tb_ctx.lock);

    return HLE_RET_OK;
}

int talkback_get_data(void *data, int *size)
{
    if (audio_init == 0)
        return HLE_RET_ENOTINIT;

    pthread_mutex_lock(&tb_ctx.lock);
    if (tb_ctx.getdata_enable == 0) {
        pthread_mutex_unlock(&tb_ctx.lock);
        return HLE_RET_ENOTINIT;
    }

    while (fifo_len(tb_ctx.fifo, 0) == 0) {
        pthread_cond_wait(&tb_ctx.cond, &tb_ctx.lock);
    }

    *size = fifo_out(tb_ctx.fifo, data, *size, 0);

    pthread_mutex_unlock(&tb_ctx.lock);
    return HLE_RET_OK;
}

int talkback_put_data(void *data, int size)
{
    if (audio_init == 0)
        return HLE_RET_ENOTINIT;

    pthread_mutex_lock(&tb_ctx.lock);
    if (tb_ctx.putdata_enable == 0) {
        pthread_mutex_unlock(&tb_ctx.lock);
        return HLE_RET_ENOTINIT;
    }
    pthread_mutex_unlock(&tb_ctx.lock);

#if 1
    AUDIO_STREAM_S stAudioStream;
    memset(&stAudioStream, 0, sizeof(stAudioStream));
    stAudioStream.u32Len = size;
    stAudioStream.pStream = (HLE_U8 *) data;
    HLE_S32 ret = HI_MPI_ADEC_SendStream(TALK_ADEC_CHN, &stAudioStream, HI_TRUE);
    if (ret) {
        ERROR_LOG("HI_MPI_ADEC_SendStream fail: %#x!\n", ret);
    }
#else
    // todo
#endif

    return HLE_RET_OK;
}


/*****************************************************************************
函数名称:SetInPutVolume
函数功能:设置(左/右)声道输入音量(增益)
输入参数:vol (Hi3516CV300 : 0~16,16是最小，15是最大)
返  回   值:0:成功  -1:失败
******************************************************************************/
int SetInPutVolume(unsigned int vol)
{

	HI_S32 fdAcodec = -1;
	unsigned int volume = 0;
	ACODEC_VOL_CTRL volctrl;
	
	if(vol > 16)
	{
		vol = 16;
	}
	volume = vol;
	volctrl.vol_ctrl = vol;
	volctrl.vol_ctrl_mute = 0;
	
	fdAcodec = open(ACODEC_FILE,O_RDWR);
	if(fdAcodec <0 )
	{
		close(fdAcodec);
		ERROR_LOG("open acodec fail ! \n");
		return -1;
	}
	
	if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICL, &volume))
	{
		ERROR_LOG("set acodec micin volume failed ! \n");
		close(fdAcodec);
		return -1;
	}
	if (ioctl(fdAcodec, ACODEC_SET_GAIN_MICR, &volume))
	{
		ERROR_LOG("set acodec micin volume failed ! \n");
		close(fdAcodec);
		return -1;
	}
	DEBUG_LOG("SetInPutVolume :(%d)\n",vol);

	close(fdAcodec);
	return 0;

}

int SetOutPutVolume(unsigned int vol)
{

	HI_S32 fdAcodec = -1;
	ACODEC_VOL_CTRL VolCtl;
	memset(&VolCtl,0x0,sizeof(ACODEC_VOL_CTRL));
	
	if(vol > 127)
	{
		vol = 127;
	}	

	VolCtl.vol_ctrl = vol; //[0-126]越大音量越小
	VolCtl.vol_ctrl_mute =0; //设置为 “非静音”
	
	fdAcodec = open(ACODEC_FILE,O_RDWR);
	if(fdAcodec <0 )
	{
		ERROR_LOG("SetOutPutVolume can't open acodec,%s\n", ACODEC_FILE);
		close(fdAcodec);
		return -1;
	}	
	/* //输出总音量控制
	if (ioctl(fdAcodec, ACODEC_SET_OUTPUT_VOL, &iVol))
	{
		printf("%s: set acodec micin volume failed4\n", __FUNCTION__);
		close(fdAcodec);
		return HI_FAILURE;
	}
	*/
	
	if (ioctl(fdAcodec, ACODEC_SET_DACL_VOL, &VolCtl)) //左声道输出音量控制。
	{
		ERROR_LOG("set acodec micin volume failed !\n");
		close(fdAcodec);
		return HI_FAILURE;
	}
	
	if (ioctl(fdAcodec, ACODEC_SET_DACR_VOL, &VolCtl))//右声道输出音量控制。
	{
		ERROR_LOG("set acodec micin volume failed !\n");
		close(fdAcodec);
		return HI_FAILURE;
	}
	DEBUG_LOG("SetOutPutVolume :%d\n",vol);

	close(fdAcodec);
	return 0;

}


int Ai2AoHandler(int Aidev, int Aichn, int Aodev, int Aochn)
{
	HI_S32 s32ret;
	AUDIO_FRAME_S stFrame;

	/* get audio frame form ai chn */
	s32ret = HI_MPI_AI_GetFrame(Aidev, Aichn,&stFrame,NULL,100);
	if(HI_SUCCESS != s32ret)
	{   
		ERROR_LOG("get ai frame err:0x%x ai(%d,%d)\n",s32ret,Aidev, Aichn);       
		return HI_FAILURE;
	}

	//printf("AiAo_proc:get ai frame ok %d wth : %d  md : %d  len :%d  \n" , stFrame.u32Seq, stFrame.enBitwidth, stFrame.enSoundmode, stFrame.u32Len);
	
	/* send audio frme to ao */
	s32ret = HI_MPI_AO_SendFrame(Aodev, Aochn, &stFrame, 100);
	if (HI_SUCCESS != s32ret)
	{   
		ERROR_LOG("ao send frame err:0x%x\n",s32ret);      
		return HI_FAILURE;
	}

	s32ret = HI_MPI_AI_ReleaseFrame(AI_DEVICE_ID,TALK_AI_CHN,&stFrame,NULL);
	if(HI_SUCCESS != s32ret)
	{   
		printf("HI_MPI_AI_ReleaseFrame  err:0x%x \n",s32ret);
		
	}
		
    return HI_SUCCESS;
}




//aio init
#define GPIO_BASE(group)                (0x12140000 + (group) * 0x1000)
#define GPIO_PIN_DATA(group, pin)       (GPIO_BASE(group) + (1 << (2 + pin)))
#define GPIO_DIR(group)                 (GPIO_BASE(group) + 0x400)

#define STATUS_AUDIO_MUTE   GPIO_PIN_DATA(3, 2)
#define DATA_GPIO3_2 	0x1204008C


int hal_audio_init(void)
{
	
	//------拉低GPIO3_2（否则为静音）-------------------------------------------------
	int ret = HI_MPI_SYS_SetReg(DATA_GPIO3_2, 0);//复用端口选择为 GPIO3_2
	//int ret = SYS_SetReg(DATA_GPIO3_2, 0);
	if (HI_SUCCESS != ret)
	{
		ERROR_LOG("HI_MPI_SYS_SetReg fail: %#x.\n", ret);
		return HLE_RET_ERROR;
	}

	//拉低GPIO3_2端口
	HLE_U32 val;
	ret = HI_MPI_SYS_GetReg(GPIO_DIR(3), &val);
	if (HI_SUCCESS != ret) 
	{
		ERROR_LOG("HI_MPI_SYS_GetReg(%#x) fail: %#x\n", GPIO_DIR(3), ret);
	}
	val |= 0x04;//配置成输出
	ret = HI_MPI_SYS_SetReg(GPIO_DIR(3), val);
	if (HI_SUCCESS != ret) 
	{
		ERROR_LOG("HI_MPI_SYS_SetReg(%#x, %#x) fail: %#x\n", GPIO_DIR(3), val, ret);
	}

	ret = HI_MPI_SYS_SetReg(STATUS_AUDIO_MUTE, 0);//拉低
	if (HI_SUCCESS != ret) 
	{
		ERROR_LOG("HI_MPI_SYS_SetReg(%#x, %#x) fail: %#x\n", STATUS_AUDIO_MUTE, val, ret);
	}
	//------------------------------------------------------------------------------

	
    ret = audio_start_aio();
    if (HLE_RET_OK != ret) 
	{
        ERROR_LOG("audio_start_aio fail: %d\n", ret);
        return HLE_RET_ERROR;
    }

	#if USE_HISI_AENC  //使用海思自带的AENC编码
	    ret = audio_start_aenc();
	    if (HLE_RET_OK != ret) {
	        ERROR_LOG("audio_start_aenc fail: %d\n", ret);
	        return HLE_RET_ERROR;
	    }
	#else  //使用AAC编码
		aac_config_t*aac_ret =  AAC_encode_init();
		if(NULL == aac_ret){
			ERROR_LOG("AAC_encode_init failed !\n");
	        return HLE_RET_ERROR;
		}
	DEBUG_LOG("AAC_encode_init success!\n");	
	#endif
	
    tb_ctx.putdata_enable = 0;
    tb_ctx.getdata_enable = 0;
    tb_ctx.fifo = fifo_malloc(TALK_BUF_SIZE);
    if (tb_ctx.fifo == NULL)
        return HLE_RET_ERROR;

    pthread_mutex_init(&tb_ctx.lock, NULL);
    pthread_cond_init(&tb_ctx.cond, NULL);
    audio_init = 1;
	
#if 1	
    talkback_start(0, 1);
#endif
	SetInPutVolume(6);//8
	SetOutPutVolume(12);//10


    DEBUG_LOG("hal_audio_init success !\n");
    return HLE_RET_OK;
}

//aio exit

int hal_audio_exit(void)
{
    int ret;
#if USE_HISI_AENC
    ret = audio_stop_aenc();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("audio_stop_aenc fail: %d\n", ret);
        return HLE_RET_ERROR;
    }
#else
	ret =  AAC_encode_exit();
	if (ret < 0) {
		ERROR_LOG("AAC_encode_exit fail: %d\n", ret);
		return HLE_RET_ERROR;
	}
#endif


    ret = audio_stop_ai();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("audio_stop_ai fail: %d\n", ret);
        return HLE_RET_ERROR;
    }

    tb_ctx.putdata_enable = 0;
    tb_ctx.getdata_enable = 0;
    fifo_free(tb_ctx.fifo, 0);
    tb_ctx.fifo = NULL;

    pthread_mutex_destroy(&tb_ctx.lock);
    pthread_cond_destroy(&tb_ctx.cond);

    stop_talkback_adec();
    stop_talkback_ao();

    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

#if 0

int hal_sonic_wave_init(void)
{
    int ret = audio_start_aio();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("audio_start_aio fail: %d\n", ret);
        return HLE_RET_ERROR;
    }

    tb_ctx.putdata_enable = 0;
    tb_ctx.getdata_enable = 0;
    tb_ctx.fifo = fifo_malloc(TALK_BUF_SIZE);
    if (tb_ctx.fifo == NULL)
        return HLE_RET_ERROR;

    pthread_mutex_init(&tb_ctx.lock, NULL);
    pthread_cond_init(&tb_ctx.cond, NULL);
    audio_init = 1;

    talkback_start(0, 1);
    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

int hal_sonic_wave_exit(void)
{
    int ret;

    ret = audio_stop_ai();
    if (HLE_RET_OK != ret) {
        ERROR_LOG("audio_stop_ai fail: %d\n", ret);
        return HLE_RET_ERROR;
    }

    stop_talkback_adec();
    stop_talkback_ao();

    tb_ctx.putdata_enable = 0;
    tb_ctx.getdata_enable = 0;
    fifo_free(tb_ctx.fifo);
    tb_ctx.fifo = NULL;

    pthread_mutex_destroy(&tb_ctx.lock);
    pthread_cond_destroy(&tb_ctx.cond);

    DEBUG_LOG("success\n");
    return HLE_RET_OK;
}

int sonic_aio_disable(void)
{
    HI_MPI_AI_Disable(AI_DEVICE_ID);
    HI_MPI_AO_Disable(AO_DEVICE_ID);
}
#endif

typedef struct
{
    int fd;
    PLAY_END_CALLBACK cbFunc;
    void* cbfPara;
} PLAY_TIP_CONTEX;

PLAY_TIP_CONTEX lTipCtx = {-1, NULL, NULL};


static void* play_tip_proc(void* para)
{
    PLAY_TIP_CONTEX* ctx = (PLAY_TIP_CONTEX*) para;

    char buf[160 + 4];
    buf[0] = 0;
    buf[1] = 1;
    buf[2] = 80;
    buf[3] = 0;

    int len;
    while ((len = read(ctx->fd, &buf[4], 160)) == 160) {
       if(HLE_RET_OK != talkback_put_data(buf, sizeof (buf))) 
			printf("talkback_put_data error!\n");
        //buf[3]++;
    }
    if (len < 0) {
        ERROR_LOG("read fail: %d\n", errno);
    }

	AI_to_AO_start = 1;
    close(ctx->fd);
    ctx->fd = -1;
    if (ctx->cbFunc) ctx->cbFunc(ctx->cbfPara);

    return NULL;
}

int play_tip(char* fn, PLAY_END_CALLBACK cbFunc, void* cbfPara)
{
    if (lTipCtx.fd >= 0) return HLE_RET_EBUSY;

    lTipCtx.cbFunc = cbFunc;
    lTipCtx.cbfPara = cbfPara;
    int fd = open(fn, O_RDONLY);
    if (fd < 0) {
		AI_to_AO_start = 1;
        //ERROR_LOG("open %s fail: %d\n", fn, errno);
        return HLE_RET_EIO;
    }
    lTipCtx.fd = fd;

    pthread_t tid;
    int ret = pthread_create(&tid, NULL, play_tip_proc, &lTipCtx);
    if (ret) {
        ERROR_LOG("pthread_create fail: %d\n", ret);
        close(lTipCtx.fd);
        lTipCtx.fd = -1;
        return HLE_RET_ERROR;
    }
    pthread_detach(tid);

    return HLE_RET_OK;
}


//=========AAC编码部分==================================================================

/*
	采样率：16KHZ
	每帧样本数：320 
	帧率：16000/320 = 50帧/s
*/
aac_config_t aac_config = {0};
typedef struct _audFrm_t
{
	AUDIO_FRAME_S 	audFrm; 	//发送给AAC编码线程的原始音频数据帧信息
	pthread_mutex_t audFrm_mut;
	pthread_cond_t  audFrm_cnd;

}audFrm_t;
audFrm_t audFrm_flg = {0};

InitParam easy_aac_handle = {0};
EasyAACEncoder_Handle easyAAChandle = NULL;


/*--------记录AAC编码数据到本地文件------------------*/
#define AAC_FILE_BUF_SIZE (1024*40)
typedef struct _AAC_record_file_t
{
	int 				AAC_file_fd;
	unsigned char* 		AAC_file_buf;
	unsigned char* 		AAC_file_buf_Wpos;//写指针位置
	int 				recod_exit;//写完文件后标记退出
}AAC_record_file_t;
AAC_record_file_t AAC_file = {0};
#define switch_record_AAC  0 //1：打开总控制开关 0：关闭
/*---------------------------------------------------*/
aac_config_t* AAC_encode_init(void)
{

	/*-DEBUG 用于记录AAC文件------------------------------------------*/
#if switch_record_AAC
	AAC_file.AAC_file_fd = open("/jffs0/aac_get.AAC", O_CREAT | O_WRONLY | O_TRUNC, 0664);
	if(AAC_file.AAC_file_fd < 0)
	{
		ERROR_LOG("open file failed !\n");
		return NULL;
	}

	AAC_file.AAC_file_buf = (unsigned char*)malloc(AAC_FILE_BUF_SIZE);
	if(NULL == AAC_file.AAC_file_buf)
	{
			ERROR_LOG("malloc buf failed !\n");
			return NULL;
	}
	memset(AAC_file.AAC_file_buf,0,AAC_FILE_BUF_SIZE);
	AAC_file.AAC_file_buf_Wpos = AAC_file.AAC_file_buf;
#endif
	/*----------------------------------------------------------------*/
	
	int ret;
	
   aac_config.SampleRate = PCM_SAMPLE_RATE;	//16KHZ
   aac_config.Channels = 1;
   aac_config.PCMBitSize = 16;
   //aac_config.PCMBuffer_size = PCM_BUF_SIZE; //BUG
   aac_config.InputSamples = 0; 		//注意该参数是后边faac编码接口的传入样本数量，并不是要我们输入一个样本数值
   aac_config.MaxOutputBytes = 0;//8192;	//同上; 


#if USE_EASY_AAC //easy AAC
	easy_aac_handle.ucAudioCodec = Law_PCM16;  ///????????????????????????????????????????
	easy_aac_handle.ucAudioChannel = 1;
	easy_aac_handle.u32AudioSamplerate = PCM_SAMPLE_RATE;
	easy_aac_handle.u32PCMBitSize = 16;

	#if 1
	unsigned char g711param = Rate16kBits;   
	memcpy(&easy_aac_handle.g711param,&g711param,sizeof(g711param));
	#endif
	
	easyAAChandle =  Easy_AACEncoder_Init(easy_aac_handle);
	if(NULL == easyAAChandle)
	{
		ERROR_LOG("Easy_AACEncoder_Init fialed !\n");
		return NULL;
	}

#else  //使用 faac 库
	/*用于faac 编码 凑PCM样本数的缓存空间初始化*/
	aac_config.EncHandle =  faacEncOpen(aac_config.SampleRate, \
										aac_config.Channels, \
										&aac_config.InputSamples, \
										&aac_config.MaxOutputBytes);
	if(aac_config.EncHandle == NULL)
    {
        DEBUG_LOG("failed to call faacEncOpen()\n");
        return NULL;
    }
	
	aac_config.PCMBuffer_size =  aac_config.InputSamples * (aac_config.PCMBitSize / 8);//PCM_BUF_SIZE;
	DEBUG_LOG("InputSamples(%d) MaxOutputBytes(%d) PCMBuffer_size(%d)\n",\
			   aac_config.InputSamples,aac_config.MaxOutputBytes,aac_config.PCMBuffer_size);
	aac_config.PCMBuffer = (unsigned char*)malloc(aac_config.PCMBuffer_size * sizeof(unsigned char));
	aac_config.AACBuffer = (unsigned char*)malloc(aac_config.MaxOutputBytes * sizeof(unsigned char));
    memset(aac_config.PCMBuffer, 0, aac_config.PCMBuffer_size);
	memset(aac_config.AACBuffer, 0, aac_config.MaxOutputBytes);
   
	
	//获取当前配置参数
	 faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(aac_config.EncHandle);

	//调整配置参数
	#if 1 
		pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	    pConfiguration->outputFormat = 0; /*0 - raw; 1 - ADTS 本地播放请选择 1 */  
	    pConfiguration->bitRate = PCM_SAMPLE_RATE;  //库内部默认为64000
	    pConfiguration->useTns = 0;
	    pConfiguration->allowMidside = 1;
	    pConfiguration->shortctl = SHORTCTL_NORMAL;
	    pConfiguration->aacObjectType = LOW;
	    pConfiguration->mpegVersion = MPEG4;//MPEG2
		//pConfiguration->useLfe = 1;
	#endif  

	//重新设置回去
	ret = faacEncSetConfiguration(aac_config.EncHandle,pConfiguration);
	if(ret != 1)
	{
        ERROR_LOG("failed to call faacEncSetConfiguration()\n");
        return NULL;
	}

#endif

	pthread_mutex_init(&audFrm_flg.audFrm_mut, NULL);
    pthread_cond_init(&audFrm_flg.audFrm_cnd, NULL);
	
	//创建AAC编码客户端线程（获取audio源数据并发送给AAC编码server线程）
	pthread_t tid;
	lRunAiAenc = 1;
	ret = pthread_create(&tid, NULL,ai_AAC_proc, NULL);
	if (ret) {
		ERROR_LOG("pthread_create fail: %d\n", ret);
		return NULL;
	}
	pthread_detach(tid);
		

	
	DEBUG_LOG("AAC_encode_init success!\n");

	/*---------获取解码参数---------------------------------------------------------*/
#if 1
	unsigned char* pDSI = NULL;
	unsigned long int ulDsiSize = 0;
	unsigned long int nSampleRate = 0;
	if (0 == faacEncGetDecoderSpecificInfo(aac_config.EncHandle, &pDSI, &ulDsiSize))
	{
		unsigned int nBit = 16;
		unsigned int nChannels = (pDSI[1]&0x7F)>>3;
	    int ints32SampleRateIndex = ((pDSI[0]&0x7)<<1)|(pDSI[1]>>7);
		switch (ints32SampleRateIndex)
		{
		case 0x0:
			nSampleRate = 96000;
			break;
		case 0x1:
			nSampleRate = 88200;
			break;
		case 0x2:
			nSampleRate = 64000;
			break;
		case 0x3:
			nSampleRate = 48000;
			break;
		case 0x4:
			nSampleRate = 44100;
			break;
		case 0x5:
			nSampleRate = 32000;
			break;
		case 0x6:
			nSampleRate = 24000;
			break;
		case 0x7:
			nSampleRate = 22050;
			break;
		case 0x8:
			nSampleRate = 16000;
			break;
		case 0x9:
			nSampleRate = 2000;
			break;
		case 0xa:
			nSampleRate = 11025;
			break;
		case 0xb:
			nSampleRate = 8000;
			break;
		default:
			nSampleRate = 44100;
			break;
		}

		printf("\n-----faacEncGetDecoderSpecificInfo------\n");
		printf("nChannels  :%d\n",nChannels);
		printf("nSampleRate:%d\n",nSampleRate);
		printf("----------------------------------------\n\n");
	}
	else
	{
		ERROR_LOG("faacEncGetDecoderSpecificInfo failed!\n");
	}
#endif
	/*------------------------------------------------------------------------*/
	
	return &aac_config;
}


/*
返回值：
	成功：返回编码成AAC帧的字节数（大于0）
	失败： 小于等于0的数
*/
int AAC_encode(int * inputBuffer,
					unsigned int samplesInput,
					unsigned char *outputBuffer,
					unsigned int bufferSize)
{
	if(NULL == aac_config.EncHandle)
	{
		ERROR_LOG("AAC encoder not init ....\n");
		return -1;
	}
	
	return faacEncEncode(aac_config.EncHandle,inputBuffer,samplesInput,outputBuffer,bufferSize);
}

 

/*
功能：发送一帧audio数据给AAC编码线程编码
参数： 
	pstFrm ：音频帧结构体指针
返回值： 
		成功 ： 0
		失败： -1
*/
static unsigned int send_PCM_count = 0; //统计接收（发送）的PCM帧数（在一段时间内）
int AAC_AENC_SendFrame( const AUDIO_FRAME_S *pstFrm)
{
	if(NULL == pstFrm)
	{
		ERROR_LOG("pstFrm is NULL!\n");
		return -1;
	}
	
	pthread_mutex_lock(&audFrm_flg.audFrm_mut);	
		memcpy(&audFrm_flg.audFrm,pstFrm,sizeof(AUDIO_FRAME_S));
		send_PCM_count++;	
		pthread_cond_signal(&audFrm_flg.audFrm_cnd);
	pthread_mutex_unlock(&audFrm_flg.audFrm_mut);
	
	return 0;
}

/***************************************************************
获取一帧编码后的AAC数据（编码业务在该函数内进行）
参数：
	pstStream：(输出)编码后AAC数据存放buf的首地址
返回：
	成功： AAC编码帧的实际长度
	失败：-1
***************************************************************/
int debug_count = 0;
static unsigned int PCM_residue = 0; //PCM帧在 aac_config.PCMBuffer没能放下的部分数据大小
static unsigned char PCM_residue_buf[PCM_FRAME_SIZE] = {0};//PCM帧在 aac_config.PCMBuffer没能放下的部分数据(备份)
static unsigned int get_PCM_count = 0; //统计接收端编码一帧AAC数据收到的PCM帧数。
int AAC_AENC_getFrame(AUDIO_STREAM_S *pstStream)
{
	if(NULL == pstStream)
	{
		ERROR_LOG("AAC_server_fd not init !\n");
		return -1;
	}
	int ret = -1;
	unsigned int out_len = 0;
	AUDIO_FRAME_S audFrm_bak; //PCM帧描述信息(备份)
	unsigned char PCM_buf[PCM_FRAME_SIZE] = {0};//PCM帧实际数据(备份)
	unsigned int PCMBuffer_offset = 0;
		
AAC_OUT_LEN_IS_0:
	
	pthread_mutex_lock(&audFrm_flg.audFrm_mut);
		pthread_cond_wait(&audFrm_flg.audFrm_cnd, &audFrm_flg.audFrm_mut);
		
		memcpy(&audFrm_bak,&audFrm_flg.audFrm,sizeof(AUDIO_FRAME_S)); //备份PCM帧描述信息
		//memcpy(&PCM_buf,audFrm_flg.audFrm.pVirAddr[0],audFrm_flg.audFrm.u32Len); //备份PCM帧实际数据
		get_PCM_count ++;
		//进行AAC编码
		debug_count ++;

		#if USE_EASY_AAC
			memcpy(&PCM_buf,audFrm_flg.audFrm.pVirAddr[0],audFrm_flg.audFrm.u32Len); //备份PCM帧实际数据
			//DEBUG_LOG("in Easy_AACEncoder_Encode ! audFrm_bak.u32Len(%d)\n",audFrm_bak.u32Len); //640
			//注意，传入的samples数据量小的情况下，该接口不一定每次调用都有AAC编码帧返回，有可能返回0
			ret = Easy_AACEncoder_Encode(easyAAChandle,(unsigned char*)&PCM_buf,audFrm_bak.u32Len,
									aac_config.AACBuffer,(unsigned int *)&out_len);
			if(ret < 0)
			{
				ERROR_LOG("Easy_AACEncoder_Encode failed ! ret(%d)\n",ret);
				return -1;
			}
			else if (0 == ret )//当前数据量不够编码一帧AAC帧，继续等下一次输入的原数据
			{
				pthread_mutex_unlock(&audFrm_flg.audFrm_mut);
				//DEBUG_LOG("out AAC frame length is 0 ,waite the next input frame... \n");
				goto AAC_OUT_LEN_IS_0;
			}
		
		#else //直接调用faac库
			//需要进行凑数据处理
			if(PCM_residue > 0)//上次循环还有数据没放完，先放上次剩余的部分
			{
				memcpy(aac_config.PCMBuffer + PCMBuffer_offset,PCM_residue_buf,PCM_residue); 
				PCMBuffer_offset += PCM_residue;
				PCM_residue = 0;
			}
			
			if(aac_config.PCMBuffer_size - PCMBuffer_offset > 0)//缓冲区原始PCM数据没满，凑数据
			{
			
				if(aac_config.PCMBuffer_size - PCMBuffer_offset >= audFrm_flg.audFrm.u32Len)//能直接放入当前PCM帧
				{
					memcpy(aac_config.PCMBuffer + PCMBuffer_offset,audFrm_flg.audFrm.pVirAddr[0],audFrm_flg.audFrm.u32Len); 
					PCMBuffer_offset += audFrm_flg.audFrm.u32Len;
				}
				else //原始PCM数据没凑满,但不能直接放入当前PCM帧
				{
					unsigned int PCMBuffer_residue = aac_config.PCMBuffer_size - PCMBuffer_offset;//剩余空间大小
					memcpy(aac_config.PCMBuffer + PCMBuffer_offset,audFrm_flg.audFrm.pVirAddr[0],PCMBuffer_residue); //只放入一部分
					PCMBuffer_offset += PCMBuffer_residue;
					PCM_residue = audFrm_flg.audFrm.u32Len - PCMBuffer_residue;//PCM帧剩余的部分大小记录
					memcpy(PCM_residue_buf,(unsigned char*)audFrm_flg.audFrm.pVirAddr[0] + PCMBuffer_residue,PCM_residue);//剩余部分进行数据备份
				}
				
				
			
			}
			//DEBUG_LOG("aac_config.PCMBuffer_size(%d) PCMBuffer_offset(%d)\n",aac_config.PCMBuffer_size,PCMBuffer_offset);
			if(aac_config.PCMBuffer_size - PCMBuffer_offset == 0)//缓冲区原始PCM数据满,进行AAC编码
			{
				
				unsigned int nInputSamples = (aac_config.PCMBuffer_size / (aac_config.PCMBitSize / 8));
				//DEBUG_LOG("AAC_encode need PCM samples (%d) actual input PCM samples (%d)\n",aac_config.InputSamples,nInputSamples);
				ret = AAC_encode((int *)aac_config.PCMBuffer,nInputSamples,aac_config.AACBuffer,aac_config.MaxOutputBytes);
				if(ret < 0)
				{
					ERROR_LOG("AAC_encode failed !\n");
					return -1;
				}
				out_len = ret;
				//printf("send_PCM_count = %d  get_PCM_count = %d\n",send_PCM_count,get_PCM_count);
				get_PCM_count = 0;
				send_PCM_count = 0;//统计编码一帧AAC实际凑了多少帧pcm数据
				
			}
			else if(aac_config.PCMBuffer_size - PCMBuffer_offset < 0)//凑数据越界了，异常！！！
			{
				ERROR_LOG("aac_config.PCMBuffer overflow!!\n");
				return -1;
			}
			else //缓存没满，继续接受下一帧PCM数据
			{
				pthread_mutex_unlock(&audFrm_flg.audFrm_mut);
				//DEBUG_LOG("Lack PCM data---waite the next input frame... \n");
				goto AAC_OUT_LEN_IS_0;
			}

			
		#endif
		
		if(debug_count >= 10)
		{
			//DEBUG_LOG("encode AAC frame size(%d) out_len(%d)\n",ret,out_len);
			debug_count = 0;
		}


		//填充返回参数
		pstStream->pStream = (unsigned char*)aac_config.AACBuffer;
		pstStream->u32PhyAddr = 0;
		pstStream->u32Len = out_len;
//		printf("AAC out_len(%d)\n",out_len);
		pstStream->u64TimeStamp = audFrm_bak.u64TimeStamp;
		pstStream->u32Seq = 0;  //该变量如有问题后续修改
	
	pthread_mutex_unlock(&audFrm_flg.audFrm_mut);

		/*--DEBUG 将文件写到缓存buf----------------------------------------*/
		#if switch_record_AAC
			if(!AAC_file.recod_exit)//记录AAC编码文件还没有退出
			{
				if(AAC_file.AAC_file_buf_Wpos + out_len > AAC_file.AAC_file_buf + AAC_FILE_BUF_SIZE)//写满缓存了再统一写入文件
				{
					ret = write(AAC_file.AAC_file_fd,AAC_file.AAC_file_buf, AAC_file.AAC_file_buf_Wpos - AAC_file.AAC_file_buf);
					if(ret < 0)
					{
						ERROR_LOG("write error!\n");
						return -1;
					}
					close(AAC_file.AAC_file_fd);
					AAC_file.recod_exit = 1; //写满缓存空间就退出
					ERROR_LOG("AAC write file success!\n\n\n");
					
					//return -1;
				}
				else
				{
					memcpy(AAC_file.AAC_file_buf_Wpos,aac_config.AACBuffer,out_len);
					AAC_file.AAC_file_buf_Wpos += out_len;
				}
			}
			
		#endif
		/*----------------------------------------------------------------*/

	return ret;
}


int  AAC_encode_exit(void)
{
	#if USE_EASY_AAC
		Easy_AACEncoder_Release(easyAAChandle);
	#else
		 if(aac_config.EncHandle)
	    {  
	        faacEncClose(aac_config.EncHandle);  
	        aac_config.EncHandle = NULL;  
	    }
		else
		{
			ERROR_LOG("AAC_encode_exit failed !\n");
			return -1;
		}
	#endif

	#if switch_record_AAC
		close(AAC_file.AAC_file_fd);
	#endif
	
	free(aac_config.AACBuffer);
	
	DEBUG_LOG("AAC_encode_exit success !\n");
	 return 0;
}













