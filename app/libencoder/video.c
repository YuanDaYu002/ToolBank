#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mpi_sys.h"
#include "hi_defines.h"
#include "hi_comm_video.h"
#include "mpi_venc.h"
#include "hi_comm_vi.h"
#include "mpi_vi.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_af.h"
#include "mpi_awb.h"
#include "hi_comm_3a.h"

#include "hal_def.h"
//#include "hal.h"
#include "video.h"
//#include "ziku.h"


/*海思不支持手动光圈，所以CAMERA_CAPS_IRIS能力需要注释掉*/
#define ISP_CAP_SUPPORT (CAMERA_CAPS_EXPOSURE | CAMERA_CAPS_WB /*| CAMERA_CAPS_IRIS*/ \
                    | CAMERA_CAPS_DENOISE | CAMERA_CAPS_DRC | CAMERA_CAPS_MIRROR_FLIP \
                    | CAMERA_CAPS_COLOR | CAMERA_CAPS_IRCUTTER)

int hal_vi_init(void);
void hal_vi_exit(void);
int hal_vo_init(void);
void hal_vo_exit(void);
int hal_vpss_init(void);
void hal_vpss_exit(void);

int hal_video_init(void)
{
	if (HLE_RET_OK != hal_vi_init()) {
		return HLE_RET_ERROR;
	}

	/*海思不支持手动光圈，系统初始化时直接设置成自动光圈*/
	camera_set_iris_mode(0, 0);

	//VO 初始化失败不返回错误，编码才是主要功能
	hal_vo_init();

	if (HLE_RET_OK != hal_vpss_init()) {
		return HLE_RET_ERROR;
	}

	DEBUG_LOG("success\n");
	return HLE_RET_OK;
}

void hal_video_exit(void)
{
	hal_vpss_exit();
	hal_vo_exit();
	hal_vi_exit();
}

int videoin_get_chns(void)
{
	return VI_PORT_NUM;
}

int videoin_get_capability(VIDEOIN_CAP *vi_cap)
{
	vi_cap->width = 1920;
	vi_cap->height = 1080;
	vi_cap->osd_count = VI_OSD_NUM;
	vi_cap->cover_count = VI_COVER_NUM;
	vi_cap->caps_mask = ISP_CAP_SUPPORT;

	return HLE_RET_OK;
}

int encoder_set_osd(int channel, int osd_index, OSD_BITMAP_ATTR *osd_attr);

int videoin_set_osd(int channel, int index, OSD_BITMAP_ATTR *osd_attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM ||
		index < 0 || index >= VI_OSD_NUM || osd_attr == NULL)
		return HLE_RET_EINVAL;

	if (osd_attr->x >= REL_COORD_WIDTH || osd_attr->y >= REL_COORD_HEIGHT)
		return HLE_RET_EINVAL;

	return encoder_set_osd(channel, index, osd_attr);
}


int vi_set_cover(int channel, int index, VIDEO_COVER_ATTR *cover_attr);

int videoin_set_cover(int channel, int index, VIDEO_COVER_ATTR *cover_attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM ||
		index < 0 || index >= VI_COVER_NUM || cover_attr == NULL)
		return HLE_RET_EINVAL;

	return vi_set_cover(channel, index, cover_attr);
}

int camera_set_color(int channel, VIDEO_COLOR_ATTR *color)
{
	VI_CSC_ATTR_S tmp_csc_attr;
	HLE_S32 ret;

	ret = HI_MPI_VI_GetCSCAttr(channel, &tmp_csc_attr);

	tmp_csc_attr.enViCscType = VI_CSC_TYPE_709;
	tmp_csc_attr.u32ContrVal = color->contrast;
	tmp_csc_attr.u32HueVal = color->hue;
	tmp_csc_attr.u32LumaVal = color->brightness;
	tmp_csc_attr.u32SatuVal = color->saturation;

	ret = HI_MPI_VI_SetCSCAttr(channel, &tmp_csc_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VI_SetCscAttr fail: %#x!\n", ret);
		return HI_FAILURE;
	}

	return ret;
}

int camera_set_ldc(int channel, VI_LDC_ATTR_S* ldc)
{
	int ret;
	ret = HI_MPI_VI_SetLDCAttr(channel, ldc);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VI_SetLDCAttr(%d, (%d, %d, %d, %d, %d, %d)) fail: %#x\n", channel,
			ldc->bEnable, ldc->stAttr.enViewType, ldc->stAttr.s32CenterXOffset,
			ldc->stAttr.s32CenterYOffset, ldc->stAttr.s32Ratio, ldc->stAttr.s32MinRatio, ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int camera_set_antiflick(int channel, CAMERA_ANTIFLICK_MODE md)
{
	if (channel < 0 || channel >= VI_PORT_NUM)
		return HLE_RET_EINVAL;

	HLE_S32 ret;
	ISP_EXPOSURE_ATTR_S expAttr;
	ISP_DEV ispDev = 0;
	ret = HI_MPI_ISP_GetExposureAttr(ispDev, &expAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetExposureAttr fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	if (expAttr.enOpType == OP_TYPE_AUTO) {
		expAttr.stAuto.stAntiflicker.bEnable = (md != CAM_NONE) ? HI_TRUE : HI_FALSE;
		expAttr.stAuto.stAntiflicker.u8Frequency = (md == CAM_50HZ) ? 50 : 60;
		expAttr.stAuto.stAntiflicker.enMode = ISP_ANTIFLICKER_AUTO_MODE;
		ret = HI_MPI_ISP_SetExposureAttr(ispDev, &expAttr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_SetExposureAttr fail: %#x\n", ret);
			return HLE_RET_ERROR;
		}
	}

	return HLE_RET_OK;
}

#define GET_ISP_OP_TYPE(mode)  (mode ? OP_TYPE_MANUAL : OP_TYPE_AUTO)

int camera_set_exposure_attr(int channel, CAMERA_EXPOSURE_ATTR *attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM || attr == NULL)
		return HLE_RET_EINVAL;

	HLE_S32 ret;
	ISP_EXPOSURE_ATTR_S expAttr;
	ISP_DEV ispDev = 0;
	ret = HI_MPI_ISP_GetExposureAttr(ispDev, &expAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetExposureAttr fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	ISP_OP_TYPE_E op_type = GET_ISP_OP_TYPE(attr->mode);
	expAttr.enOpType = op_type;

	if (OP_TYPE_AUTO == op_type) {
		DEBUG_LOG("ISP_AE_ATTR_S enAEMode %d\n", expAttr.stAuto.enAEMode);
		DEBUG_LOG("stExpTimeRange.u32Max %d, stExpTimeRange.u32Min %d\n", expAttr.stAuto.stExpTimeRange.u32Max, expAttr.stAuto.stExpTimeRange.u32Min);
		DEBUG_LOG("stDGainRange.u32Max %d, stDGainRange.u32Min %d\n", expAttr.stAuto.stDGainRange.u32Max, expAttr.stAuto.stDGainRange.u32Min);
		DEBUG_LOG("stAGainRange.u32Max %d, stAGainRange.u32Min %d\n", expAttr.stAuto.stAGainRange.u32Max, expAttr.stAuto.stAGainRange.u32Min);
		DEBUG_LOG("u8Speed %d, u8Tolerance %d\n", expAttr.stAuto.u8Speed, expAttr.stAuto.u8Tolerance);
		DEBUG_LOG("u8Compensation %d\n", expAttr.stAuto.u8Compensation);

		expAttr.stAuto.stAGainRange.u32Max = ((HLE_U32) (attr->a_gain)) << 10;
		expAttr.stAuto.stDGainRange.u32Max = ((HLE_U32) (attr->d_gain)) << 10;
		expAttr.stAuto.stExpTimeRange.u32Max = attr->exposure_time;
	} else {
		DEBUG_LOG("ISP_ME_ATTR_S u32AGain %d, u32DGain %d, u32ExpTime %d\n",
			expAttr.stManual.u32AGain, expAttr.stManual.u32DGain, expAttr.stManual.u32ExpTime);
		DEBUG_LOG("enExpTimeOpType %d, enAGainOpType %d, enDGainOpType %d\n",
			expAttr.stManual.enExpTimeOpType, expAttr.stManual.enAGainOpType, expAttr.stManual.enDGainOpType);

		expAttr.stManual.enAGainOpType = OP_TYPE_MANUAL;
		expAttr.stManual.u32AGain = ((HLE_U32) (attr->a_gain)) << 10;
		expAttr.stManual.enDGainOpType = OP_TYPE_MANUAL;
		expAttr.stManual.u32DGain = ((HLE_U32) (attr->d_gain)) << 10;
		expAttr.stManual.enExpTimeOpType = OP_TYPE_MANUAL;
		expAttr.stManual.u32ExpTime = attr->exposure_time;
	}

	ret = HI_MPI_ISP_SetExposureAttr(ispDev, &expAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetExposureAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int camera_set_wb_attr(int channel, CAMERA_WB_ATTR *attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM || attr == NULL)
		return HLE_RET_EINVAL;

#if 0   /*海思反馈只要不去调用AWB相关接口，图像就不会出现偶尔偏蓝的情况*/
	ISP_OP_TYPE_E op_type = GET_ISP_OP_TYPE(attr->mode);
	HLE_S32 ret;
	ret = HI_MPI_ISP_SetWBType(op_type);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetWBType fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	if (OP_TYPE_AUTO == op_type) {
		ISP_AWB_ATTR_S awb_attr;
		ret = HI_MPI_ISP_GetAWBAttr(&awb_attr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_GetAWBAttr fail: %#x!\n", ret);
			return ret;
		}
		DEBUG_LOG("ISP_AWB_ATTR_S u8RGStrength %d, u8BGStrength %d, u8ZoneSel %d\n",
			awb_attr.u8RGStrength, awb_attr.u8BGStrength, awb_attr.u8ZoneSel);

		awb_attr.u8RGStrength = 0x80;
		awb_attr.u8BGStrength = 0x80;
		awb_attr.u8ZoneSel = 0x10;
		//awb_attr.u8ZoneSel = 0x3F;
		ret = HI_MPI_ISP_SetAWBAttr(&awb_attr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_SetAWBAttr fail: %#x!\n", ret);
			return ret;
		}
	} else {
		ISP_MWB_ATTR_S mwb_attr;
		ret = HI_MPI_ISP_GetMWBAttr(&mwb_attr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_GetMWBAttr fail: %#x\n", ret);
			return HLE_RET_ERROR;
		}
		DEBUG_LOG("ISP_MWB_ATTR_S u16Rgain %d, u16Ggain %d, u16Bgain %d\n",
			mwb_attr.u16Rgain, mwb_attr.u16Ggain, mwb_attr.u16Bgain);

		//ISP_MWB_ATTR_S u16Rgain 537, u16Ggain 257, u16Bgain 413
		//值比较怪异，先勉强对接一下
		mwb_attr.u16Rgain = attr->r_gain << 2;
		mwb_attr.u16Bgain = attr->b_gain << 2;
		ret = HI_MPI_ISP_SetMWBAttr(&mwb_attr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_SetMWBAttr fail: %#x\n", ret);
			return HLE_RET_ERROR;
		}
	}
#endif

	return HLE_RET_OK;
}

int camera_set_focus_attr(int channel, CAMERA_FOCUS_ATTR *attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM || attr == NULL)
		return HLE_RET_EINVAL;

	ISP_OP_TYPE_E op_type = GET_ISP_OP_TYPE(attr->mode);
	HLE_S32 ret;

	/* FOCUS_ATTR时，op_type 为OP_TYPE_MANUAL 的情况无效 */
	ret = HI_MPI_ISP_SetFocusType(0, op_type);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetFocusType fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int camera_set_iris_mode(int channel, int mode)
{
	if (channel < 0 || channel >= VI_PORT_NUM || ((0 != mode) && (1 != mode)))
		return HLE_RET_EINVAL;

	HLE_S32 ret;
	ISP_IRIS_ATTR_S irisAttr;
	ISP_DEV ispDev = 0;
	ret = HI_MPI_ISP_GetIrisAttr(ispDev, &irisAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetIrisAttr fail: %#x\n", ret);
		return ret;
	}

	ISP_OP_TYPE_E op_type = GET_ISP_OP_TYPE(mode);
	/* iris_mode时，op_type 为OP_TYPE_MANUAL 的情况无效 */
	if (OP_TYPE_AUTO == op_type) {
		irisAttr.bEnable = HI_FALSE;
		irisAttr.enIrisType = ISP_IRIS_DC_TYPE;
		irisAttr.enOpType = OP_TYPE_AUTO;
	}

	ret = HI_MPI_ISP_SetIrisAttr(ispDev, &irisAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetIrisAttr fail: %#x\n", ret);
		return ret;
	}

	return HLE_RET_OK;
}

int camera_set_denoise_attr(int channel, CAMERA_DENOISE_ATTR *attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM || attr == NULL)
		return HLE_RET_EINVAL;

	HLE_S32 ret;
	ISP_NR_ATTR_S nrAttr;
	ISP_DEV ispDev = 0;
	ret = HI_MPI_ISP_GetNRAttr(ispDev, &nrAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetNRAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	nrAttr.bEnable = attr->enable;
	nrAttr.enOpType = OP_TYPE_AUTO;

	ret = HI_MPI_ISP_SetNRAttr(ispDev, &nrAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetNRAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int camera_set_drc_attr(int channel, CAMERA_DRC_ATTR *attr)
{
	if (channel < 0 || channel >= VI_PORT_NUM || attr == NULL)
		return HLE_RET_EINVAL;

	HLE_S32 ret;
	ISP_DRC_ATTR_S drc_attr;
	ret = HI_MPI_ISP_GetDRCAttr(0, &drc_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetDRCAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	drc_attr.bEnable = HI_TRUE;
	drc_attr.enOpType = GET_ISP_OP_TYPE(attr->enable);
	drc_attr.stManual.u8Strength = attr->level;
	ret = HI_MPI_ISP_SetDRCAttr(0, &drc_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetDRCAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

extern int vi_set_mirror_flip(int mirror, int flip);

int camera_set_mirror_flip(int channel, int mirror, int flip)
{
	if ((!((0 == mirror) || (1 == mirror))) ||
		(!((0 == flip) || (1 == flip))) ||
		(channel < 0 || channel >= VI_PORT_NUM)) {
		ERROR_LOG("error para!\n");
		return HLE_RET_EINVAL;
	}

	return vi_set_mirror_flip(mirror, flip);
}

int isp_ai_close(void)
{
	return HLE_RET_ENOTSUPPORTED;
#if 0
	ISP_AI_ATTR_S ai_attr;
	HLE_S32 ret;

	ret = HI_MPI_ISP_GetAIAttr(&ai_attr);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_ISP_GetAIAttr fail: %#x!\n", ret);
		return ret;
	}

	ai_attr.bIrisEnable = 1;
	ai_attr.enIrisStatus = ISP_IRIS_CLOSE;
	ret = HI_MPI_ISP_SetAIAttr(&ai_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetAIAttr Failed with %#x, ErrorCode=%d\n",
			ret, ai_attr.enTriggerStatus);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
#endif
}

int isp_ai_open(void)
{
	return HLE_RET_ENOTSUPPORTED;
#if 0
	ISP_AI_ATTR_S ai_attr;
	HLE_S32 ret;

	ret = HI_MPI_ISP_GetAIAttr(&ai_attr);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_ISP_GetAIAttr fail: %#x!\n", ret);
		return ret;
	}

	ai_attr.bIrisEnable = 1;
	ai_attr.enIrisStatus = ISP_IRIS_OPEN;
	ret = HI_MPI_ISP_SetAIAttr(&ai_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetAIAttr Failed with %#x, ErrorCode=%d\n",
			ret, ai_attr.enTriggerStatus);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
#endif
}

int isp_ai_detect(int *hold_val)
{
	return HLE_RET_ENOTSUPPORTED;
#if 0
	ISP_AI_ATTR_S ai_attr = {0};

	/********** get hold value *********/
	ai_attr.bIrisEnable = HI_FALSE; /*Auto iris  on/off*/
	ai_attr.bIrisCalEnable = HI_TRUE; /*iris calibration on/off*/
	ai_attr.u16IrisStopValue = 600; /*the initial stop value for AI trigger*/
	ai_attr.u16IrisCloseDrive = 750; /*the drive value to close Iris, [700, 900]. A larger value means faster.*/
	ai_attr.u16IrisTriggerTime = 1000; /*frame numbers of AI trigger lasting time. > 600*/
	ai_attr.u8IrisInertiaValue = 5; /*frame numbers of  AI moment of inertia */

	HLE_S32 ret;
	ret = HI_MPI_ISP_SetAIAttr(&ai_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("SetAIAttr Failed with %#x, ErrorCode=%d\n",
			ret, ai_attr.enTriggerStatus);
		return HLE_RET_ERROR;
	}
	while (1) {
		memset(&ai_attr, 0, sizeof (ISP_AI_ATTR_S));
		ret = HI_MPI_ISP_GetAIAttr(&ai_attr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_GetAIAttr fail: %#x!\n", ret);
			return HLE_RET_ERROR;
		}
		if (ISP_TRIGGER_SUCCESS == ai_attr.enTriggerStatus) {
			DEBUG_LOG("detect success!\n");
			break;
		} else if (ISP_TRIGGER_TIMEOUT == ai_attr.enTriggerStatus) {
			ERROR_LOG("TIMEOUT! Failed!\n");
			return HLE_RET_ERROR;
		}
		sleep(1);
	}

	*hold_val = ai_attr.u32IrisHoldValue;
	DEBUG_LOG("hold value is %#x\n", *hold_val);

	memset(&ai_attr, 0x0, sizeof (ISP_AI_ATTR_S));
	ai_attr.bIrisEnable = 1; /*Auto iris  on/off*/
	ai_attr.bIrisCalEnable = 0; /*iris calibration on/off*/
	ai_attr.enIrisStatus = 0; /*status of Iris*/
	ai_attr.u32IrisHoldValue = *hold_val; /*iris hold value*/
	ret = HI_MPI_ISP_SetAIAttr(&ai_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("call HI_MPI_ISP_SetAIAttr Failed with %#x\n", ret);
		return HI_FAILURE;
	}

	return HLE_RET_OK;
#endif
}

void set_irc_mode(int way);
//mode: 0---auto, 1---open, 2---close

int camera_set_irc_mode(int channel, int mode)
{
	if ((mode < 0) || (mode > 2) || channel < 0 || channel >= VI_PORT_NUM)
		return HLE_RET_EINVAL;

	set_irc_mode(mode);
	return HLE_RET_OK;
}

int misc_get_irc_status(void);

int camera_get_irc_status(void)
{
	return misc_get_irc_status();
}

#if 0

static int isp_dp_set(ISP_DP_ATTR_S *defect_pixel_attr)
{
	if (NULL == defect_pixel_attr)
		return HLE_RET_EINVAL;

	//  dp_attr->bEnableDynamic= HI_TRUE;
	defect_pixel_attr->bEnableDetect = HI_FALSE;
	defect_pixel_attr->bEnableStatic = HI_TRUE;

	int ret;
	ret = HI_MPI_ISP_SetDefectPixelAttr(defect_pixel_attr);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_ISP_SetDefectPixelAttr fail: %#x \n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

//坏点检测，获取坏点个数和坐标属性

static int isp_dp_detect(ISP_DP_ATTR_S *defect_pixel_attr)
{
	if (defect_pixel_attr == NULL)
		return HLE_RET_EINVAL;

	defect_pixel_attr->bEnableDetect = HI_TRUE;
	defect_pixel_attr->u16BadPixelCountMin = 0x70;
	defect_pixel_attr->u16BadPixelCountMax = 0x300;
	defect_pixel_attr->u16BadPixelTriggerTime = 150;
	defect_pixel_attr->u8BadPixelStartThresh = 4;

	int ret;
	ret = HI_MPI_ISP_SetDefectPixelAttr(defect_pixel_attr);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_ISP_SetDefectPixelAttr fail: %#x \n", ret);
		return HLE_RET_ERROR;
	}

	while (1) {
		memset(defect_pixel_attr, 0, sizeof (ISP_DP_ATTR_S));
		ret = HI_MPI_ISP_GetDefectPixelAttr(defect_pixel_attr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_GetDefectPixelAttr fail: %#x!\n", ret);
			return HI_FAILURE;
		}

		sleep(1);
		if (ISP_TRIGGER_SUCCESS == defect_pixel_attr->enTriggerStatus) {
			break;
		} else if (ISP_TRIGGER_TIMEOUT == defect_pixel_attr->enTriggerStatus) {
			ERROR_LOG("\nBad pixel detect TIMEOUT!\n");
			return HLE_RET_ERROR;
		}
	}

	//  DEBUG_LOG("bEnableStatic = %d, bEnableDynamic = %d\n",
	//      defect_pixel_attr->bEnableStatic,defect_pixel_attr->bEnableDynamic);
	//  DEBUG_LOG("bEnableDetect = %d, enTriggerStatus = %d\n",
	//      defect_pixel_attr->bEnableDetect,defect_pixel_attr->enTriggerStatus);
	DEBUG_LOG("u8BadPixelStartThresh = %d, u8BadPixelFinishThresh = %d\n",
		defect_pixel_attr->u8BadPixelStartThresh, defect_pixel_attr->u8BadPixelFinishThresh);
	DEBUG_LOG("u16BadPixelCountMax = %d, u16BadPixelCountMin = %d\n",
		defect_pixel_attr->u16BadPixelCountMax, defect_pixel_attr->u16BadPixelCountMin);
	DEBUG_LOG("u16BadPixelCount = %d, u16BadPixelTriggerTime = %d\n",
		defect_pixel_attr->u16BadPixelCount, defect_pixel_attr->u16BadPixelTriggerTime);

	return HLE_RET_OK;
}

#define DEFECT_PIXEL_CONFIG_ATTR    "/app/defect_pixel_attr"
static pthread_mutex_t dp_attr_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	HLE_SYS_TIME correct_dp_time;
	int dp_count;
	int data[0];
} DP_ATTR_CONFIG;

static int save_defect_pixel_attr(ISP_DP_ATTR_S *dp_attr)
{
	if (dp_attr == NULL)
		return HLE_RET_EINVAL;

	pthread_mutex_lock(&dp_attr_lock);
	int fd = open(DEFECT_PIXEL_CONFIG_ATTR, O_CREAT | O_WRONLY | O_TRUNC, 0664);
	if (fd == -1) {
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}

	DP_ATTR_CONFIG *dp_attr_config;
	int size = dp_attr->u16BadPixelCount * 4 + sizeof (DP_ATTR_CONFIG);
	dp_attr_config = (DP_ATTR_CONFIG *) malloc(size);
	if (NULL == dp_attr_config) {
		close(fd);
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_ERROR;
	}

	hal_get_time(&dp_attr_config->correct_dp_time, 1); //得到保存坏点属性时间
	dp_attr_config->dp_count = dp_attr->u16BadPixelCount;
	memcpy(dp_attr_config->data, dp_attr->u32BadPixelTable, dp_attr->u16BadPixelCount * 4);

	if (hal_writen(fd, dp_attr_config, size) != size) {
		free(dp_attr_config);
		dp_attr_config = NULL;
		close(fd);
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}

	free(dp_attr_config);
	dp_attr_config = NULL;
	close(fd);
	pthread_mutex_unlock(&dp_attr_lock);
	return HLE_RET_OK;
}
#endif

int load_defect_pixel_attr(ISP_DP_STATIC_ATTR_S *dp_attr)
{
	return HLE_RET_ENOTSUPPORTED;
#if 0
	if (dp_attr == NULL)
		return HLE_RET_EINVAL;

	pthread_mutex_lock(&dp_attr_lock);
	int fd = open(DEFECT_PIXEL_CONFIG_ATTR, O_RDONLY);
	if (fd == -1) {
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}

	DP_ATTR_CONFIG dp_attr_config;
	int size = sizeof (DP_ATTR_CONFIG);
	if (hal_readn(fd, &dp_attr_config, size) != size) {
		close(fd);
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}
	dp_attr->u16BadPixelCount = dp_attr_config.dp_count;

	size = dp_attr_config.dp_count * 4; //为int 类型,所以要乘以4
	if (hal_readn(fd, dp_attr->u32BadPixelTable, size) != size) {
		close(fd);
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}

	close(fd);
	pthread_mutex_unlock(&dp_attr_lock);
	return HLE_RET_OK;
#endif
}

int hal_correct_defect_pixel(void)
{
	HLE_S32 ret;
	ISP_DP_DYNAMIC_ATTR_S dpAttr;
	ISP_DEV ispDev = 0;

	ret = HI_MPI_ISP_GetDPDynamicAttr(ispDev, &dpAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetDPDynamicAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	dpAttr.bEnable = HI_TRUE;
	dpAttr.enOpType = OP_TYPE_AUTO;

	ret = HI_MPI_ISP_SetDPDynamicAttr(ispDev, &dpAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetDPDynamicAttr fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int hal_get_dpc_time(HLE_SYS_TIME *dpc_time)
{
	return HLE_RET_ENOTSUPPORTED;
#if 0
	if (dpc_time == NULL)
		return HLE_RET_EINVAL;

	pthread_mutex_lock(&dp_attr_lock);
	int fd = open(DEFECT_PIXEL_CONFIG_ATTR, O_RDONLY);
	if (fd == -1) {
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}

	int size = sizeof (HLE_SYS_TIME);
	if (hal_readn(fd, dpc_time, size) != size) {
		close(fd);
		pthread_mutex_unlock(&dp_attr_lock);
		return HLE_RET_EIO;
	}

	close(fd);
	pthread_mutex_unlock(&dp_attr_lock);
	return HLE_RET_OK;
#endif
}


