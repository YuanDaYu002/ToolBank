/*
 * Ingenic IMP RTSPServer VideoInput
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <yakun.li@ingenic.com>
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/ioctl.h>

#include <imp/imp_log.h>
#include <imp/imp_common.h>
#include <imp/imp_system.h>
#include <imp/imp_utils.h>
#include <imp/imp_framesource.h>
#include <imp/imp_encoder.h>
#include <imp/imp_isp.h>

#include "VideoInput.hh"
#include "H264VideoStreamSource.hh"

#define TAG 						"sample-RTSPServer"

#define SENSOR_IMX307           1

#ifdef SENSOR_IMX307
#define CONFIG_SENSOR_NAME		"imx307"
#define CONFIG_SENSOR_ADDR		0x1a
#define CONFIG_FPS_NUM			25
#define CONFIG_FPS_DEN			1
#define CONFIG_VIDEO_WIDTH		1920
#define CONFIG_VIDEO_HEIGHT		1080
#define CONFIG_VIDEO_BITRATE 	2000
#elif SENSOR_IMX385
#define CONFIG_SENSOR_NAME		"imx385"
#define CONFIG_SENSOR_ADDR		0x1a
#define CONFIG_FPS_NUM			25
#define CONFIG_FPS_DEN			1
#define CONFIG_VIDEO_WIDTH		1920
#define CONFIG_VIDEO_HEIGHT		1080
#define CONFIG_VIDEO_BITRATE 	2000
#endif

#define MAX_IVS_OSD_REGION		400
#define MAX_IVS_ID_OSD_REGION	100

Boolean VideoInput::fpsIsOn[MAX_STREAM_CNT] = {False, False};
Boolean VideoInput::fHaveInitialized = False;
double VideoInput::gFps[MAX_STREAM_CNT] = {0.0, 0.0};
double VideoInput::gBitRate[MAX_STREAM_CNT] = {0.0, 0.0};

extern IMPRgnHandle ivsRgnHandler[MAX_IVS_OSD_REGION + MAX_IVS_ID_OSD_REGION];

static int framesource_init(IMPFSChnAttr *imp_chn_attr)
{
	int ret;

	/* create channel i*/
	ret = IMP_FrameSource_CreateChn(0, imp_chn_attr);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "IMP_FrameSource_CreateChn(chn%d) error!\n", 0);
		return -1;
	}

	ret = IMP_FrameSource_SetMaxDelay(0, 15);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_SetMaxDelay(chn0) error!\n");
		return -1;
	}

	ret = IMP_FrameSource_SetDelay(0, 15);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_SetDelay(chn0) error!\n");
		return -1;
	}

	ret = IMP_FrameSource_SetChnAttr(0, imp_chn_attr);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_SetChnAttr(%d) error: %d\n", ret, 0);
		return -1;
	}

	/* Check channel i attr */
	IMPFSChnAttr imp_chn_attr_check;
	ret = IMP_FrameSource_GetChnAttr(0, &imp_chn_attr_check);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_GetChnAttr(%d) error: %d\n", ret, 0);
		return -1;
	}

	ret = IMP_ISP_EnableTuning();
	if(ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_ISP_EnableTuning error\n");
		return -1;
	}

	return 0;
}

static int encoder_init(void)
{
	int ret;
	IMPEncoderCHNAttr chnAttr;
	IMPEncoderAttr *encAttr = &chnAttr.encAttr;
	IMPEncoderAttrRcMode *rcAttr = &chnAttr.rcAttr.attrRcMode;

	memset(&chnAttr, 0, sizeof(IMPEncoderCHNAttr));

	encAttr->enType = PT_H264;
	encAttr->bufSize = 0;
	encAttr->profile = 1;
	encAttr->picWidth = CONFIG_VIDEO_WIDTH;
	encAttr->picHeight = CONFIG_VIDEO_HEIGHT;

	chnAttr.rcAttr.maxGop = CONFIG_FPS_NUM * 2 / CONFIG_FPS_DEN;
	chnAttr.rcAttr.outFrmRate.frmRateNum = CONFIG_FPS_NUM;
	chnAttr.rcAttr.outFrmRate.frmRateDen = CONFIG_FPS_DEN;

	rcAttr->rcMode = ENC_RC_MODE_CBR;
	//rcAttr->attrH264Cbr.maxGop = CONFIG_FPS_NUM * 2 / CONFIG_FPS_DEN;
	//rcAttr->attrH264Cbr.outFrmRate.frmRateNum = CONFIG_FPS_NUM;
	//rcAttr->attrH264Cbr.outFrmRate.frmRateDen = CONFIG_FPS_DEN;
	rcAttr->attrH264Cbr.outBitRate = CONFIG_VIDEO_BITRATE;
	rcAttr->attrH264Cbr.maxQp = 38;
	rcAttr->attrH264Cbr.minQp = 15;
	rcAttr->attrH264Cbr.iBiasLvl = 2;
	rcAttr->attrH264Cbr.frmQPStep = 3;
	rcAttr->attrH264Cbr.gopQPStep = 15;
	rcAttr->attrH264Cbr.adaptiveMode = false;
	rcAttr->attrH264Cbr.gopRelation = false;

	ret = IMP_Encoder_CreateGroup(0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_CreateGroup(%d) error: %d\n", 0, ret);
		return -1;
	}

	/* Create Channel */
	ret = IMP_Encoder_CreateChn(0, &chnAttr);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_CreateChn(%d) error: %d\n", 0, ret);
		return -1;
	}

	/* Resigter Channel */
	ret = IMP_Encoder_RegisterChn(0, 0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_RegisterChn(%d, %d) error: %d\n", 0, 0, ret);
		return -1;
	}

	return 0;
}

static int ImpSystemInit()
{
	int ret = 0;
	IMPSensorInfo sensor_info;

	memset(&sensor_info, 0, sizeof(IMPSensorInfo));

	strcpy(sensor_info.name, CONFIG_SENSOR_NAME);
	sensor_info.cbus_type = TX_SENSOR_CONTROL_INTERFACE_I2C;
	strcpy(sensor_info.i2c.type, CONFIG_SENSOR_NAME);
	sensor_info.i2c.addr = CONFIG_SENSOR_ADDR;

	IMP_LOG_DBG(TAG, "ImpSystemInit start\n");

	if(IMP_ISP_Open()){
		IMP_LOG_ERR(TAG, "failed to EmuISPOpen\n");
		return -1;
	}

	ret = IMP_ISP_AddSensor(&sensor_info);
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to AddSensor\n");
		return -1;
	}

	ret = IMP_ISP_EnableSensor();
	if(ret < 0){
		IMP_LOG_ERR(TAG, "failed to EnableSensor\n");
		return -1;
	}

	IMP_System_Init();

	IMP_LOG_DBG(TAG, "ImpSystemInit success\n");

	return 0;
}

static int imp_init(void)
{
	int ret;

	/* System init */
	ret = ImpSystemInit();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_System_Init() failed\n");
		return -1;
	}

	/* sync time */
	struct timeval Timestamp;
	gettimeofday(&Timestamp, NULL);
	IMP_System_RebaseTimeStamp(Timestamp.tv_sec*(uint64_t)1000000 + (uint64_t)Timestamp.tv_usec);

	/* FrameSource init */
	IMPFSChnAttr imp_chn_attr[2];
	imp_chn_attr[0].pixFmt = PIX_FMT_NV12;
	imp_chn_attr[0].outFrmRateNum = CONFIG_FPS_NUM;
	imp_chn_attr[0].outFrmRateDen = CONFIG_FPS_DEN;
	imp_chn_attr[0].nrVBs = 2;
	imp_chn_attr[0].type = FS_PHY_CHANNEL;

	imp_chn_attr[0].crop.enable = 1;
	imp_chn_attr[0].crop.top = 0;
	imp_chn_attr[0].crop.left = 0;

	imp_chn_attr[0].crop.width = CONFIG_VIDEO_WIDTH;
	imp_chn_attr[0].crop.height = CONFIG_VIDEO_HEIGHT;

	imp_chn_attr[0].scaler.enable = 0;

	imp_chn_attr[0].picWidth = CONFIG_VIDEO_WIDTH;
	imp_chn_attr[0].picHeight = CONFIG_VIDEO_HEIGHT;

	ret = framesource_init(imp_chn_attr);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "FrameSource init failed\n");
		return -1;
	}

	/* Encoder init */
	ret = encoder_init();
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "Encoder init failed\n");
		return -1;
	}

	/* Bind */
	IMPCell framesource_chn0_output0 = {DEV_ID_FS, 0, 0};
	IMPCell encoder0 = {DEV_ID_ENC, 0, 0};

	ret = IMP_System_Bind(&framesource_chn0_output0, &encoder0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_System_Bind(&framesource_chn0_output0, &encoder0) failed...\n");
		return ret;
	}

	IMP_FrameSource_SetFrameDepth(0, 0);

	/* OSD */
	if (IMP_OSD_CreateGroup(0) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_CreateGroup(0) error !\n");
		return -1;
	}

	IMPCell osdcell = {DEV_ID_OSD, 0, 0};
	IMPCell *dstcell = &encoder0;
	if (IMP_OSD_AttachToGroup(&osdcell, dstcell) < 0) {
		IMP_LOG_ERR(TAG, "IMP_OSD_AttachToGroup() error !\n");
		return -1;
	}

	/* rect osd region register */
	IMPOSDRgnAttr rAttr = {
		.type = OSD_REG_RECT,
		.rect = {{0, 0}, {0, 0}},
		.fmt = PIX_FMT_MONOWHITE,
		.data = {
			.lineRectData = {
				.color = OSD_GREEN,
				.linewidth = 2,
			},
		},
	};

	IMPOSDGrpRgnAttr grAttr;
	memset(&grAttr, 0, sizeof(IMPOSDGrpRgnAttr));
	grAttr.scalex = 1;
	grAttr.scaley = 1;

	int i;
	for (i = 0; i < MAX_IVS_OSD_REGION; i++) {
		ivsRgnHandler[i] = IMP_OSD_CreateRgn(&rAttr);
		if (ivsRgnHandler[i] == INVHANDLE) {
			IMP_LOG_ERR(TAG, "IVS IMP_OSD_CreateRgn %d failed\n", i);
		}
		if (IMP_OSD_RegisterRgn(ivsRgnHandler[i], 0, &grAttr) < 0) {
			IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn %d failed\n", ivsRgnHandler[i]);
		}
	}

	/* id osd region register */
	IMPOSDRgnAttr rAttr_id = {
		.type = OSD_REG_PIC,
		.rect = {{0, 0}, {0, 0}},
		.fmt = PIX_FMT_BGRA,
		.data = {
			.picData = {
				.pData = NULL,
			},
		},
	};

	for (i = 0; i < MAX_IVS_ID_OSD_REGION; i++) {
		ivsRgnHandler[i + MAX_IVS_OSD_REGION] = IMP_OSD_CreateRgn(&rAttr);
		if (ivsRgnHandler[i + MAX_IVS_OSD_REGION] == INVHANDLE) {
			IMP_LOG_ERR(TAG, "IVS IMP_OSD_CreateRgn %d failed\n", i + MAX_IVS_OSD_REGION);
		}
		if (IMP_OSD_RegisterRgn(ivsRgnHandler[i + MAX_IVS_OSD_REGION], 0, &grAttr) < 0) {
			IMP_LOG_ERR(TAG, "IVS IMP_OSD_RegisterRgn %d failed\n", ivsRgnHandler[i + MAX_IVS_OSD_REGION]);
		}
	}

	IMP_OSD_Start(0);

	ret = IMP_FrameSource_EnableChn(0);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_FrameSource_EnableChn(0) error: %d\n", ret);
		return -1;
	}

	return 0;
}

static inline int close_stream_file(int fd)
{
	return close(fd);
}

VideoInput* VideoInput::createNew(UsageEnvironment& env, int streamNum) {
    if (!fHaveInitialized) {
		if (!initialize(env)) {
			printf("%s: %d\n", __func__, __LINE__);
			return NULL;
		}
		fHaveInitialized = True;
    }

	VideoInput *videoInput = new VideoInput(env, streamNum);

    return videoInput;
}

VideoInput::VideoInput(UsageEnvironment& env, int streamNum)
	: Medium(env), fVideoSource(NULL), fpsIsStart(False), fontIsStart(False),
	  osdIsStart(False), osdStartCnt(0), nrFrmFps(0),
	  totalLenFps(0), startTimeFps(0), streamNum(streamNum), scheduleTid(-1),
	  orgfrmRate(CONFIG_FPS_NUM), hasSkipFrame(false),
	  curPackIndex(0), requestIDR(false) {
}

VideoInput::~VideoInput() {
}

extern "C" {
	int IMP_FrameSource_EnableChnUndistort(int chnNum, void *handle);
	int IMP_FrameSource_DisableChnUndistort(int chnNum);
}

bool VideoInput::initialize(UsageEnvironment& env) {
	int ret;

	ret = imp_init();
	if (ret < 0) {
		printf("%s: %d\n", __func__, __LINE__);
		return false;
	}

	return true;
}

FramedSource* VideoInput::videoSource() {
	IMP_Encoder_FlushStream(streamNum);
	fVideoSource = new H264VideoStreamSource(envir(), *this);
	return fVideoSource;
}

int VideoInput::getStream(void* to, unsigned int* len, struct timeval* timestamp, unsigned fMaxSize) {
	int ret;
	unsigned int stream_len = 0;

	if (curPackIndex == 0) {
		ret = IMP_Encoder_GetStream(streamNum, &bitStream, 1);
		if (ret < 0) {
			IMP_LOG_ERR(TAG, "IMP_Encoder_GetStream() failed\n");
			return -1;
		}
	}

	if (requestIDR) {
		if (bitStream.packCount == 1) {
			IMP_Encoder_ReleaseStream(streamNum, &bitStream);
			goto out;
		} else {
			requestIDR = false;
		}
	}

	stream_len = bitStream.pack[curPackIndex].length - 4;

	if (stream_len > fMaxSize) {
		IMP_LOG_WARN(TAG, "drop stream: length=%u, fMaxSize=%d\n", stream_len, fMaxSize);
		stream_len = 0;
		curPackIndex = 0;
		IMP_Encoder_ReleaseStream(streamNum, &bitStream);
		requestIDR = true;
		goto out;
	}

	memcpy((void*)((unsigned int)to),
		   (void *)(bitStream.pack[curPackIndex].virAddr + 4),
		   stream_len);

	if (curPackIndex < bitStream.packCount - 1)
		curPackIndex++;
	else if (curPackIndex == bitStream.packCount - 1)
		curPackIndex = 0;

	if (curPackIndex == 0) {
		IMP_Encoder_ReleaseStream(streamNum, &bitStream);
	}

	gettimeofday(timestamp, NULL);

	if (fpsIsOn[streamNum]) {
		if (fpsIsStart == False) {
			fpsIsStart = True;
			nrFrmFps = 0;
			totalLenFps = 0;
			startTimeFps = IMP_System_GetTimeStamp();
		}
		totalLenFps += stream_len;
		if (curPackIndex == 0)
			nrFrmFps++;
		if ((nrFrmFps > 0) && ((nrFrmFps % (CONFIG_FPS_NUM * 2 / CONFIG_FPS_DEN)) == 0)) {
			uint64_t now = IMP_System_GetTimeStamp();
			uint64_t elapsed = now - startTimeFps;

			double fps = nrFrmFps * 1000000. / (elapsed > 0 ? elapsed : 1);
			double kbitrate = (double)totalLenFps * 8  * 1000. / (elapsed > 0 ? elapsed : 1);

			printf("\rstreamNum[%d]:FPS: %0.2f,Bitrate: %0.2f(kbps)", streamNum, fps, kbitrate);
			fflush(stdout);

			gFps[streamNum] = fps;
			gBitRate[streamNum] = kbitrate;

			startTimeFps = now;
			totalLenFps = 0;
			nrFrmFps = 0;
		}
	} else if (fpsIsStart) {
		fpsIsStart = False;
	}

out:
	*len = stream_len;

	return 0;
}

int VideoInput::pollingStream(void)
{
	int ret;

	ret = IMP_Encoder_PollingStream(streamNum, 2000);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "chnNum:%d, Polling stream timeout\n", streamNum);
		return -1;
	}

	return 0;
}

int VideoInput::streamOn(void)
{

	IMP_Encoder_RequestIDR(streamNum);
	requestIDR = true;

	int ret = IMP_Encoder_StartRecvPic(streamNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StartRecvPic(%d) failed\n", streamNum);
		return -1;
	}

	return 0;
}

int VideoInput::streamOff(void)
{
	int ret;

	if (curPackIndex != 0)
		IMP_Encoder_ReleaseStream(streamNum, &bitStream);

	ret = IMP_Encoder_StopRecvPic(streamNum);
	if (ret < 0) {
		IMP_LOG_ERR(TAG, "IMP_Encoder_StopRecvPic(%d) failed\n", streamNum);
		return -1;
	}

	return 0;
}
