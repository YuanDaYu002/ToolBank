#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "hi_sns_ctrl.h"
#include "hi_comm_isp.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpss.h"
#include "mpi_vi.h"
#include "mpi_isp.h"
#include "mpi_vo.h"
#include "mpi_sys.h"
#include "mpi_region.h"
#include "mpi_vpss.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"
#include "hi_mipi.h"
//#include "hi_pq_bin.h"

#include "hal_def.h"
//#include "hal.h"
#include "video.h"
#include "encoder.h"
#include "motion_detect.h"

#define GET_COVER_HANDLE(vi_chn, index)    ((index) + (vi_chn)*VI_COVER_NUM)


#define SENSOR_IMX122       0
#define SENSOR_OV2718       1
#define SENSOR_SC2235       2

static ISP_PUB_ATTR_S ispPubAttr[] = {
	{
		{0, 0, 1920, 1080}, 30, BAYER_RGGB
	},
	{
		{0, 0, 1920, 1080}, 30, BAYER_BGGR
	},
    {
		{0, 0, 1920, 1080}, 30, BAYER_BGGR
    },
};

static combo_dev_attr_t mipiDevAttr[] = {
	{ 0, INPUT_MODE_CMOS,{}},
	{ 0, INPUT_MODE_MIPI,{
			{RAW_DATA_12BIT, HI_MIPI_WDR_MODE_NONE,{0, 1, 2, 3}}}},
	{ 0, INPUT_MODE_CMOS,{}},
};

static VI_DEV_ATTR_S viDevAttr[] = {
	{
		VI_MODE_DIGITAL_CAMERA,
		VI_WORK_MODE_1Multiplex,
		{0xfff00000, 0x0},
		VI_SCAN_PROGRESSIVE,
		{-1, -1, -1, -1},
		VI_INPUT_DATA_YUYV,
		{
			VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH,
			VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH,
			VI_VSYNC_NORM_PULSE, VI_VSYNC_VALID_NEG_HIGH,
			{
				0, 1920, 0,
				0, 1080, 0,
				0, 0, 0
            }
        },
		VI_PATH_ISP,
		VI_DATA_TYPE_RGB,
		HI_FALSE,
		{0, 0, 1920, 1080}
	},
	{
		VI_MODE_MIPI,
		VI_WORK_MODE_1Multiplex,
		{0xfff00000, 0x0},
		VI_SCAN_PROGRESSIVE,
		{-1, -1, -1, -1},
		VI_INPUT_DATA_YUYV,
		{
			VI_VSYNC_PULSE, VI_VSYNC_NEG_HIGH,
			VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH,
			VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,
			{
				0, 1920, 0,
				0, 1080, 0,
				0, 0, 0
            }
        },
		VI_PATH_ISP,
		VI_DATA_TYPE_RGB,
		HI_FALSE,
		{0, 0, 1920, 1080}
	},
    {
        VI_MODE_DIGITAL_CAMERA,
        VI_WORK_MODE_1Multiplex,
        {0xFFF0000,    0x0},
        VI_SCAN_PROGRESSIVE,
        { -1, -1, -1, -1},
        VI_INPUT_DATA_YUYV,
        {
            VI_VSYNC_FIELD, VI_VSYNC_NEG_HIGH, 
            VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, 
            VI_VSYNC_NORM_PULSE, VI_VSYNC_VALID_NEG_HIGH,
            {
                0, 1920, 0,
                0, 1080, 0,
                0, 0, 0
            }
        },
        VI_PATH_ISP,
        VI_DATA_TYPE_RGB,
        HI_FALSE,
        {0, 0, 1920, 1080}
    },
};

static VI_CHN_ATTR_S viChnAttr = {
	{0, 0, 1920, 1080},
	{1920, 1080},
	VI_CAPSEL_BOTH,
	PIXEL_FORMAT_YUV_SEMIPLANAR_420,
	COMPRESS_MODE_NONE,
	HI_FALSE,
	HI_FALSE,
	-1,
	-1
};

#if 0
static VI_EXT_CHN_ATTR_S viVdaChnAttr = {
	0,
	{480, 272}, 30, 15, PIXEL_FORMAT_YUV_SEMIPLANAR_420, VIU_EXT_CHN_START
};
#endif

static VPSS_GRP_ATTR_S vpssGrpAttr = {
	1920, 1080, PIXEL_FORMAT_YUV_SEMIPLANAR_420,
	HI_FALSE, HI_FALSE, HI_TRUE, HI_FALSE,
	VPSS_DIE_MODE_NODIE, HI_TRUE
};

#if 0
static VPSS_GRP_ATTR_S vpssVdaGrpAttr = {
	480, 272, PIXEL_FORMAT_YUV_SEMIPLANAR_420,
	HI_FALSE, HI_FALSE, HI_FALSE, HI_TRUE, HI_TRUE,
	VPSS_DIE_MODE_AUTO
};
#endif

static int hal_mipi_init(void)
{
	int sns = SENSOR_SC2235;
	int fd = open("/dev/hi_mipi", O_RDWR);
	if (fd < 0) {
		ERROR_LOG("open hi_mipi dev fail\n");
		return HLE_RET_EIO;
	}

	ioctl(fd, HI_MIPI_RESET_MIPI, &mipiDevAttr[sns].devno);
	ioctl(fd, HI_MIPI_RESET_SENSOR, &mipiDevAttr[sns].devno);
	if (ioctl(fd, HI_MIPI_SET_DEV_ATTR, &mipiDevAttr[sns])) {
		ERROR_LOG("set mipi attr fail\n");
		close(fd);
		return HLE_RET_EIO;
	}
    usleep(10000);
	ioctl(fd, HI_MIPI_UNRESET_MIPI, &mipiDevAttr[sns].devno);
	ioctl(fd, HI_MIPI_UNRESET_SENSOR, &mipiDevAttr[sns].devno);
	close(fd);

	return HLE_RET_OK;
}

void get_vda_size(SIZE_S *size)
{
	size->u32Width = 480;
	size->u32Height = 272;
}

static int hal_sensor_init(void)
{
	HLE_S32 ret;
	ret = sensor_register_callback();
	if (ret != HI_SUCCESS) {
		ERROR_LOG("sensor_register_callback fail: %#x!\n", ret);
		return ret;
	}
	return HLE_RET_OK;
}

static int vi_start_dev(VI_DEV vi_dev_id, WDR_MODE_E wm)
{
	HLE_S32 ret;
	int sns = SENSOR_SC2235;

	ret = HI_MPI_VI_SetDevAttr(vi_dev_id, &viDevAttr[sns]);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_VI_SetDevAttr fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	VI_WDR_ATTR_S wdr;
	//wdr.enWDRMode = WDR_MODE_2To1_LINE;
	wdr.enWDRMode = wm;
	wdr.bCompress = HI_FALSE;
	ret = HI_MPI_VI_SetWDRAttr(vi_dev_id, &wdr);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_VI_SetWDRAttr fail: %#x!\n", ret);
	}

	ret = HI_MPI_VI_EnableDev(vi_dev_id);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_EnableDev fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}


//int load_defect_pixel_attr(ISP_DP_ATTR_S *dp_attr);

static int hal_isp_init(void)
{
	HLE_S32 ret;
	ALG_LIB_S stLib;
	int sns = SENSOR_SC2235;
	ISP_DEV ispDev = 0;

	/* 1. register ae lib */
	stLib.s32Id = 0;
	strncpy(stLib.acLibName, HI_AE_LIB_NAME, sizeof (stLib.acLibName));
	ret = HI_MPI_AE_Register(ispDev, &stLib);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_AE_Register fail: %#x!\n", ret);
		return ret;
	}

	/* 2. register awb lib */
	stLib.s32Id = 0;
	strncpy(stLib.acLibName, HI_AWB_LIB_NAME, sizeof (stLib.acLibName));
	ret = HI_MPI_AWB_Register(ispDev, &stLib);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_AWB_Register fail: %#x!\n", ret);
		return ret;
	}

	/* 3. register af lib */
	stLib.s32Id = 0;
	strncpy(stLib.acLibName, HI_AF_LIB_NAME, sizeof (stLib.acLibName));
	ret = HI_MPI_AF_Register(ispDev, &stLib);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_AF_Register fail: %#x!\n", ret);
		return ret;
	}

	// isp mem init
	ret = HI_MPI_ISP_MemInit(ispDev);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_MemInit fail: %#x!\n", ret);
		return ret;
	}

	// isp set WDR mode
	ISP_WDR_MODE_S wdrMode;
	//wdrMode.enWDRMode = WDR_MODE_2To1_LINE;
	wdrMode.enWDRMode = WDR_MODE_NONE;
	ret = HI_MPI_ISP_SetWDRMode(ispDev, &wdrMode);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetWDRMode fail: %#x!\n", ret);
		return ret;
	}

	// isp set pub attributes
	ret = HI_MPI_ISP_SetPubAttr(ispDev, &ispPubAttr[sns]);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_SetPubAttr fail: %#x!\n", ret);
		return ret;
	}

	/* 4. isp init */
	ret = HI_MPI_ISP_Init(ispDev);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_Init fail: %#x!\n", ret);
		return ret;
	}

	return HLE_RET_OK;
}

void *hal_isp_routine(void *arg)
{
	DEBUG_LOG("pid = %d\n", getpid());
	prctl(PR_SET_NAME, "hal_isp", 0, 0, 0);

	return (void *) (HI_MPI_ISP_Run(0));
}

static pthread_t isp_pid;

static int hal_isp_run(void)
{
	HLE_S32 ret;

	ret = hal_isp_init();
	if (HLE_RET_OK != ret) {
		ERROR_LOG("hal_isp_init fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	if (0 != pthread_create(&isp_pid, 0, hal_isp_routine, NULL)) {
		ERROR_LOG("create isp running thread failed!\n");
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

static void hal_isp_stop(void)
{
	HI_MPI_ISP_Exit(0);
	pthread_join(isp_pid, 0);
}

static int vi_start_chn(void)
{
	int ret = HI_MPI_VI_SetChnAttr(VI_PHY_CHN, &viChnAttr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_SetChnAttr(%d) fail: %#x!\n", VI_PHY_CHN, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VI_EnableChn(VI_PHY_CHN);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_EnableChn(%d) fail: %#x!\n", VI_PHY_CHN, ret);
		return HLE_RET_ERROR;
	}

#if 0
	ret = HI_MPI_VI_SetFrameDepth(VI_PHY_CHN, 2);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_SetFrameDepth(%d, 2) fail: %#x!\n", VI_PHY_CHN, ret);
		return HLE_RET_ERROR;
	}
#endif
        
#if 0
	ret = HI_MPI_VI_SetExtChnAttr(VI_CHN_VDA, &viVdaChnAttr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_SetExtChnAttr(%d) fail: %#x!\n", VI_CHN_VDA, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VI_EnableChn(VI_CHN_VDA);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_EnableChn(%d) fail: %#x!\n", VI_PHY_CHN, ret);
		return HLE_RET_ERROR;
	}
#endif

	return HLE_RET_OK;
}

int vi_set_mirror_flip(int mirror, int flip)
{
	int ret = HI_MPI_VI_GetChnAttr(VI_PHY_CHN, &viChnAttr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_GetChnAttr(%d) fail: %#x!\n", VI_PHY_CHN, ret);
		return HLE_RET_ERROR;
	}

	flip = !flip;
	mirror = !mirror;
	if ((mirror == viChnAttr.bMirror) && (flip == viChnAttr.bFlip)) {
		return HLE_RET_OK;
	}

	viChnAttr.bMirror = mirror;
	viChnAttr.bFlip = flip;
	ret = HI_MPI_VI_SetChnAttr(VI_PHY_CHN, &viChnAttr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_SetChnAttr(%d) fail: %#x!\n", VI_PHY_CHN, ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

static int vi_cover_init(void)
{
	int i, j;
	for (i = 0; i < VI_PORT_NUM; i++) {
		for (j = 0; j < VI_COVER_NUM; j++) {
			RGN_HANDLE cover_handle = GET_COVER_HANDLE(i, j);
			RGN_ATTR_S cover_rgn_attr;
			cover_rgn_attr.enType = COVEREX_RGN;
			int ret = HI_MPI_RGN_Create(cover_handle, &cover_rgn_attr);
			if (HI_SUCCESS != ret) {
				ERROR_LOG("HI_MPI_RGN_Create (%d) fail: %#x!\n", cover_handle, ret);
				continue;
			}

			MPP_CHN_S cover_chn;
			RGN_CHN_ATTR_S cover_chn_attr;
			cover_chn.enModId = HI_ID_VIU;
			cover_chn.s32DevId = VI_DEV_ID;
			cover_chn.s32ChnId = VI_PHY_CHN;
			cover_chn_attr.bShow = HI_FALSE;
			cover_chn_attr.enType = COVEREX_RGN;
			cover_chn_attr.unChnAttr.stCoverExChn.enCoverType = AREA_RECT;
			cover_chn_attr.unChnAttr.stCoverExChn.stRect.s32X = 0;
			cover_chn_attr.unChnAttr.stCoverExChn.stRect.s32Y = 0;
			cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Width = 32;
			cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Height = 32;
			cover_chn_attr.unChnAttr.stCoverExChn.u32Color = 0;
			cover_chn_attr.unChnAttr.stCoverExChn.u32Layer = j;
			ret = HI_MPI_RGN_AttachToChn(cover_handle, &cover_chn, &cover_chn_attr);
			if (HI_SUCCESS != ret) {
				ERROR_LOG("HI_MPI_RGN_AttachToChn fail: %#x!\n", ret);
			}
		}
	}

	DEBUG_LOG("success\n");
	return HLE_RET_OK;
}

static int vi_get_phy_chn_size(int *width, int *height)
{
	*width = viChnAttr.stDestSize.u32Width;
	*height = viChnAttr.stDestSize.u32Height;
#if 0
	VI_CHN_ATTR_S vi_chn_attr;
	int ret = HI_MPI_VI_GetChnAttr(VI_PHY_CHN, &vi_chn_attr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VI_GetChnAttr(%d) fail: %#x!\n", phy_chn, ret);
		return HLE_RET_ERROR;
	}

	*width = vi_chn_attr.stDestSize.u32Width;
	*height = vi_chn_attr.stDestSize.u32Height;
	DEBUG_LOG("vi_get_phy_chn_size phy_chn %d, width %d, height %d\n", phy_chn, *width, *height);
#endif

	return HLE_RET_OK;
}

int vi_set_cover(int channel, int index, VIDEO_COVER_ATTR *cover_attr)
{
	RGN_HANDLE cover_handle = GET_COVER_HANDLE(channel, index);
	MPP_CHN_S mpp_chn;
	RGN_CHN_ATTR_S cover_chn_attr;

	mpp_chn.enModId = HI_ID_VIU;
	mpp_chn.s32DevId = VI_DEV_ID;
	mpp_chn.s32ChnId = VI_PHY_CHN;
	int ret = HI_MPI_RGN_GetDisplayAttr(cover_handle, &mpp_chn, &cover_chn_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_RGN_GetDisplayAttr index(%d) fail: %#x!\n", index, ret);
		return HLE_RET_ERROR;
	}

	int width, height;
	ret = vi_get_phy_chn_size(&width, &height);
	if (ret != 0) {
		ERROR_LOG("impossible!!!");
		return HLE_RET_ERROR;
	}
	cover_chn_attr.bShow = cover_attr->enable;
	cover_chn_attr.unChnAttr.stCoverExChn.enCoverType = AREA_RECT;
	cover_chn_attr.unChnAttr.stCoverExChn.u32Color = cover_attr->color;
	// X 和Y 的范围为[0, 4096]，要求以2 对齐
	cover_chn_attr.unChnAttr.stCoverExChn.stRect.s32X = (cover_attr->left * width / REL_COORD_WIDTH) & (~1);
	cover_chn_attr.unChnAttr.stCoverExChn.stRect.s32Y = (cover_attr->top * height / REL_COORD_HEIGHT) & (~1);
	// width 和 height 的范围为[16, 4096],并且要以2 对齐
	cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Width =
		((cover_attr->right - cover_attr->left) * width / REL_COORD_WIDTH) & (~1);
	if (cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Width < 16) {
		cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Width = 16;
	}
	cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Height =
		((cover_attr->bottom - cover_attr->top) * height / REL_COORD_HEIGHT) & (~1);
	if (cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Height < 16)
		cover_chn_attr.unChnAttr.stCoverExChn.stRect.u32Height = 16;
	ret = HI_MPI_RGN_SetDisplayAttr(cover_handle, &mpp_chn, &cover_chn_attr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_RGN_SetDisplayAttr index(%d) fail: %#x!\n", index, ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

#if 0

static int sensor_tune()
{
	sleep(1);
	HI_PQ_BIN_ATTR_S stBinAttr;
	ISP_REG_ATTR_S stIspRegAttr;
	int ret = HI_MPI_ISP_GetISPRegAttr(0, &stIspRegAttr);
	if (0 != ret) {
		ERROR_LOG("HI_MPI_ISP_GetISPRegAttr error 0x%x\n", ret);
		return -1;
	}
	stBinAttr.stIspRegAttr.u32AeExtRegAddr = stIspRegAttr.u32AeExtRegAddr;
	stBinAttr.stIspRegAttr.u32AeExtRegSize = stIspRegAttr.u32AeExtRegSize;
	stBinAttr.stIspRegAttr.u32AwbExtRegAddr = stIspRegAttr.u32AwbExtRegAddr;
	stBinAttr.stIspRegAttr.u32AwbExtRegSize = stIspRegAttr.u32AwbExtRegSize;
	stBinAttr.stIspRegAttr.u32IspExtRegAddr = stIspRegAttr.u32IspExtRegAddr;
	stBinAttr.stIspRegAttr.u32IspExtRegSize = stIspRegAttr.u32IspExtRegSize;
	stBinAttr.stIspRegAttr.u32IspRegAddr = stIspRegAttr.u32IspRegAddr;
	stBinAttr.stIspRegAttr.u32IspRegSize = stIspRegAttr.u32IspRegSize;
	stBinAttr.u32ChipId = 0x3516c;
	HI_PQ_BIN_Init(stBinAttr);

	FILE* fl = fopen("sensor.cfg", "r");
	if (NULL == fl) {
		ERROR_LOG("open sensor.cfg fail\n");
		return -2;
	}

	fseek(fl, 0, SEEK_END);
	int size = ftell(fl);
	fseek(fl, 0, SEEK_SET);

	HLE_U8* buf = (HLE_U8*) malloc(size);
	if (NULL == buf) {
		ERROR_LOG("malloc(%d) fail\n", size);
		fclose(fl);
		return -3;
	}

	memset(buf, 0, size);
	int len = fread(buf, 1, size, fl);
	fclose(fl);
	if (len <= 0) {
		ERROR_LOG("read file error\n");
		free(buf);
		return -4;
	}

	ret = HI_PQ_BIN_ParseBinData(buf, size);
	if (0 != ret) {
		ERROR_LOG("HI_PQ_BIN_ParseBinData error(0x%x)\n", ret);
	}

	free(buf);
	return 0;
}
#endif

int hal_vi_init(void)
{
	if (HLE_RET_OK != hal_mipi_init())
		return HLE_RET_ERROR;
        
	//step 1: configure sensor.
	if (HLE_RET_OK != hal_sensor_init())
		return HLE_RET_ERROR;

	//step 2: configure & run isp thread
	if (HLE_RET_OK != hal_isp_run())
		return HLE_RET_ERROR;

	//step 3 : config & start vicap dev
	if (HLE_RET_OK != vi_start_dev(VI_DEV_ID, WDR_MODE_NONE))
		return HLE_RET_ERROR;

	//step 4: config & start vicap chn
	if (HLE_RET_OK != vi_start_chn()) {
		hal_isp_stop();
		return HLE_RET_ERROR;
	}

	//sensor_tune();

	vi_cover_init();

	DEBUG_LOG("success\n");
	return HLE_RET_OK;
}

void hal_vi_exit(void)
{
	/*调用HI_MPI_SYS_Exit后，除了音频的编解码通道外，所有的音频输入输出、
	视频输入输出、视频编码、视频叠加区域、视频侦测分析通道都会被销毁或者
	禁用，所以这里要做的只有停掉ISP模块*/
	hal_isp_stop();
}

int vpss_start_grp(void)
{
	VPSS_GRP vpssGrp = VPSS_GRP_ID;
	int ret = HI_MPI_VPSS_CreateGrp(vpssGrp, &vpssGrpAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_CreateGrp(%d) fail: %#x!\n", vpssGrp, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VPSS_StartGrp(vpssGrp);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_StartGrp(%d) fail: %#x!\n", vpssGrp, ret);
		return HLE_RET_ERROR;
	}

	MPP_CHN_S src, dst;
	src.enModId = HI_ID_VIU;
	src.s32DevId = VI_DEV_ID;
	src.s32ChnId = VI_PHY_CHN;
	dst.enModId = HI_ID_VPSS;
	dst.s32DevId = vpssGrp;
	dst.s32ChnId = 0;
	ret = HI_MPI_SYS_Bind(&src, &dst);
	DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
			src.enModId, src.s32DevId, src.s32ChnId,
			dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
		return HLE_RET_ERROR;
	}

#if 0
	vpssGrp = VPSS_GRP_VDA;
	ret = HI_MPI_VPSS_CreateGrp(vpssGrp, &vpssVdaGrpAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_CreateGrp(%d) fail: %#x!\n", vpssGrp, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VPSS_StartGrp(vpssGrp);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_StartGrp(%d) fail: %#x!\n", vpssGrp, ret);
		return HLE_RET_ERROR;
	}

	src.enModId = HI_ID_VIU;
	src.s32DevId = VI_DEV_ID;
	src.s32ChnId = VI_CHN_VDA;
	dst.enModId = HI_ID_VPSS;
	dst.s32DevId = vpssGrp;
	dst.s32ChnId = 0;
	ret = HI_MPI_SYS_Bind(&src, &dst);
	DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
			src.enModId, src.s32DevId, src.s32ChnId,
			dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
		return HLE_RET_ERROR;
	}
#endif

	return HLE_RET_OK;
}

static int vpss_config_ext_chn(VPSS_GRP vpssGrp, VPSS_CHN vpssChn, VPSS_CHN bindChn, int width, int height, int frmRate)
{
	VPSS_EXT_CHN_ATTR_S extAttr;

	extAttr.s32BindChn = bindChn;
	extAttr.u32Width = width;
	extAttr.u32Height = height;
	extAttr.s32SrcFrameRate = 30;//15;
	extAttr.s32DstFrameRate = frmRate;
	extAttr.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	int ret = HI_MPI_VPSS_SetExtChnAttr(vpssGrp, vpssChn, &extAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetExtChnAttr(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VPSS_EnableChn(vpssGrp, vpssChn);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_EnableChn(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

static int vpss_config_chn(VPSS_GRP vpssGrp, VPSS_CHN vpssChn, int width, int height)
{
	VPSS_CHN_ATTR_S chnAttr;
	VPSS_CHN_MODE_S chnMode;

	memset(&chnAttr, 0, sizeof (chnAttr));
	chnAttr.bSpEn = HI_FALSE;
	chnAttr.bBorderEn = HI_FALSE;
	chnAttr.s32SrcFrameRate = 30;
	chnAttr.s32DstFrameRate = 15;
	int ret = HI_MPI_VPSS_SetChnAttr(vpssGrp, vpssChn, &chnAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetChnAttr(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	chnMode.enChnMode = VPSS_CHN_MODE_USER;
	chnMode.u32Width = width;
	chnMode.u32Height = height;
	chnMode.bDouble = HI_FALSE;
	chnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	chnMode.enCompressMode = COMPRESS_MODE_NONE;
	ret = HI_MPI_VPSS_SetChnMode(vpssGrp, vpssChn, &chnMode);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetChnMode(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VPSS_EnableChn(vpssGrp, vpssChn);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_EnableChn(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

int vpss_enable_chn(void)
{
	int ret;

	ret = vpss_config_chn(VPSS_GRP_ID, VPSS_CHN_ENC0, 1920, 1080);
	if (HLE_RET_OK != ret) {
		return ret;
	}

	ret = vpss_config_chn(VPSS_GRP_ID, VPSS_CHN_ENC1, 960, 544);
	if (HLE_RET_OK != ret) {
		return ret;
	}

	ret = vpss_config_chn(VPSS_GRP_ID, VPSS_CHN_ENC2, 480, 272);
	if (HLE_RET_OK != ret) {
		return ret;
	}

	ret = vpss_config_ext_chn(VPSS_GRP_ID, VPSS_CHN_JPEG, VPSS_CHN_ENC0, 1920, 1080, 15);
	if (HLE_RET_OK != ret) {
		return ret;
	}

	ret = vpss_config_ext_chn(VPSS_GRP_ID, VPSS_CHN_MD, VPSS_CHN_ENC2, 480, 272, 5);
	if (HLE_RET_OK != ret) {
		return ret;
	}

	ret = HI_MPI_VPSS_SetDepth(VPSS_GRP_ID, VPSS_CHN_MD, 2);
	if (HLE_RET_OK != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetDepth(%d, %d, %d) fail: %#x!\n", VPSS_GRP_ID, VPSS_CHN_MD, 2, ret);
		return ret;
	}

#if 0
	ret = vpss_config_ext_chn(VPSS_GRP_ID, VPSS_CHN_OD, VPSS_CHN_ENC2, 480, 272, 5);
	if (HLE_RET_OK != ret) {
		return ret;
	}
#endif

#if 0
	vpssGrp = VPSS_GRP_VDA;
	vpssChn = VPSS_CHN_MD;
	memset(&chnAtrr, 0, sizeof (chnAtrr));
	chnAtrr.bSpEn = HI_TRUE;
	chnAtrr.bFrameEn = HI_FALSE;
	ret = HI_MPI_VPSS_SetChnAttr(vpssGrp, vpssChn, &chnAtrr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetChnAttr(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	chnMode.enChnMode = VPSS_CHN_MODE_USER;
	chnMode.u32Width = 480;
	chnMode.u32Height = 272;
	chnMode.bDouble = HI_FALSE;
	chnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	ret = HI_MPI_VPSS_SetChnMode(vpssGrp, vpssChn, &chnMode);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetChnMode(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VPSS_EnableChn(vpssGrp, vpssChn);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_EnableChn(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	vpssGrp = VPSS_GRP_VDA;
	vpssChn = VPSS_CHN_OD;
	memset(&chnAtrr, 0, sizeof (chnAtrr));
	chnAtrr.bSpEn = HI_TRUE;
	chnAtrr.bFrameEn = HI_FALSE;
	ret = HI_MPI_VPSS_SetChnAttr(vpssGrp, vpssChn, &chnAtrr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetChnAttr(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	chnMode.enChnMode = VPSS_CHN_MODE_USER;
	chnMode.u32Width = 480;
	chnMode.u32Height = 272;
	chnMode.bDouble = HI_FALSE;
	chnMode.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
	ret = HI_MPI_VPSS_SetChnMode(vpssGrp, vpssChn, &chnMode);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_SetChnMode(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}

	ret = HI_MPI_VPSS_EnableChn(vpssGrp, vpssChn);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_VPSS_EnableChn(%d, %d) fail: %#x!\n", vpssGrp, vpssChn, ret);
		return HLE_RET_ERROR;
	}
#endif

	return HLE_RET_OK;
}

#if 0

int vpss_set_chn_size(int chn, int width, int height)
{
	int old_width;
	int old_height;
	VPSS_EXT_CHN_ATTR_S extAttr;
	VPSS_GRP vpssGrp = VPSS_GRP_ID;

	int ret = HI_MPI_VPSS_GetExtChnAttr(vpssGrp, chn, &extAttr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VPSS_GetExtChnAttr(%d) fail: %#x!\n", chn, ret);
		return HLE_RET_ERROR;
	}

	old_width = extAttr.u32Width;
	old_height = extAttr.u32Height;
	extAttr.u32Width = width;
	extAttr.u32Height = height;
	ret = HI_MPI_VPSS_SetExtChnAttr(vpssGrp, chn, &extAttr);
	if (ret != HLE_RET_OK) {
		ERROR_LOG("HI_MPI_VPSS_SetExtChnAttr(%d) fail: %#x! width %d, height %d\n", chn, ret, width, height);
		return HLE_RET_ERROR;
	} else {
		DEBUG_LOG("chn %d size (%d, %d) -> (%d, %d)\n", chn, old_width, old_height, width, height);
	}

	return HLE_RET_OK;
}
#endif

int hal_vpss_init(void)
{
	if (HLE_RET_OK != vpss_start_grp()) {
		return HLE_RET_ERROR;
	}

	if (HLE_RET_OK != vpss_enable_chn()) {
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

void hal_vpss_exit(void)
{
}

int hal_vo_init(void)
{
	return HLE_RET_ENOTSUPPORTED;
#if 0
	HLE_S32 ret;

	/*config and enable VO device*/
	VO_PUB_ATTR_S pub_attr = {0};
	pub_attr.enIntfType = VO_INTF_CVBS;
	pub_attr.enIntfSync = ((hal_get_max_frame_rate() == NTSC_MAX_FRAME_RATE)
		? VO_OUTPUT_NTSC : VO_OUTPUT_PAL);
	pub_attr.u32BgColor = 0x00FF00;
	ret = HI_MPI_VO_SetPubAttr(VO_DEV_ID, &pub_attr);
	if (HLE_RET_OK != ret) {
		ERROR_LOG("HI_MPI_VO_SetPubAttr fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}
	ret = HI_MPI_VO_Enable(VO_DEV_ID);
	if (HLE_RET_OK != ret) {
		ERROR_LOG("HI_MPI_VO_Enable fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	/*config and enable video layer of VO device*/
	VO_VIDEO_LAYER_ATTR_S layer_attr;
	layer_attr.stDispRect.s32X = 0;
	layer_attr.stDispRect.s32Y = 0;
	layer_attr.stDispRect.u32Width = dest_size[4].u32Width;
	layer_attr.stDispRect.u32Height = dest_size[4].u32Height;
	layer_attr.stImageSize.u32Width = dest_size[4].u32Width;
	layer_attr.stImageSize.u32Height = dest_size[4].u32Height;
	layer_attr.u32DispFrmRt = hal_get_max_frame_rate();
	layer_attr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_422;
	layer_attr.s32PiPChn = VO_DEFAULT_CHN;
	ret = HI_MPI_VO_SetVideoLayerAttr(VO_DEV_ID, &layer_attr);
	if (HLE_RET_OK != ret) {
		ERROR_LOG("HI_MPI_VO_SetVideoLayerAttr fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}
	ret = HI_MPI_VO_EnableVideoLayer(VO_DEV_ID);
	if (HLE_RET_OK != ret) {
		ERROR_LOG("HI_MPI_VO_EnableVideoLayer fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	/*config and enable vo channel*/
	VO_CHN_ATTR_S chn_attr;
	chn_attr.stRect.s32X = 0;
	chn_attr.stRect.s32Y = 0;
	chn_attr.stRect.u32Width = dest_size[4].u32Width;
	chn_attr.stRect.u32Height = dest_size[4].u32Height;
	chn_attr.bZoomEnable = 1;
	chn_attr.bDeflicker = 1;
	chn_attr.u32Priority = 1;
	ret = HI_MPI_VO_SetChnAttr(VO_DEV_ID, VO_CHN_ID, &chn_attr);
	if (HLE_RET_OK != ret) {
		DEBUG_LOG("HI_MPI_VO_SetChnAttr fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}
	ret = HI_MPI_VO_EnableChn(VO_DEV_ID, VO_CHN_ID);
	if (HLE_RET_OK != ret) {
		DEBUG_LOG("HI_MPI_VO_EnableChn fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	/*bind vo channel to sd vi channel*/
	MPP_CHN_S band_src, band_dst;
	band_dst.enModId = HI_ID_VOU;
	band_dst.s32ChnId = VO_CHN_ID;
	band_dst.s32DevId = VO_DEV_ID;
	band_src.enModId = HI_ID_VIU;
	band_src.s32ChnId = VI_PHYCHN_SD;
	band_src.s32DevId = VI_DEV_ID;
	ret = HI_MPI_SYS_Bind(&band_src, &band_dst);
	if (HLE_RET_OK != ret) {
		DEBUG_LOG("HI_MPI_SYS_Bind fail: %#x!\n", ret);
		return HLE_RET_ERROR;
	}

	DEBUG_LOG("success\n");
	return HLE_RET_OK;
#endif
}

void hal_vo_exit(void)
{
	/*调用HI_MPI_SYS_Exit后，除了音频的编解码通道外，所有的音频输入输出、
	视频输入输出、视频编码、视频叠加区域、视频侦测分析通道都会被销毁或者
	禁用，所以这里啥都不用做*/
}

static WDR_MODE_E lastMode = WDR_MODE_NONE;

int hal_set_wdr(WDR_MODE_E wm)
{
	if (lastMode != wm) {
		lastMode = wm;
		int ret;
		ISP_WDR_MODE_S wdrMode;
		ret = HI_MPI_ISP_GetWDRMode(0, &wdrMode);
		if ((ret != HI_SUCCESS) || (wm != wdrMode.enWDRMode)) {
			wdrMode.enWDRMode = wm;
			ret = HI_MPI_ISP_SetWDRMode(0, &wdrMode);
			if (HI_SUCCESS != ret) {
				ERROR_LOG("HI_MPI_ISP_SetWDRMode fail: %#x!\n", ret);
				return ret;
			}
		}

		VI_WDR_ATTR_S wdr;
		ret = HI_MPI_VI_GetWDRAttr(VI_DEV_ID, &wdr);
		if ((ret != HI_SUCCESS) || (wm != wdr.enWDRMode)) {
			encoder_pause();
			motion_detect_exit();

			ret = HI_MPI_VI_DisableChn(VI_PHY_CHN);
			if (ret != HLE_RET_OK) {
				ERROR_LOG("HI_MPI_VI_DisableChn(%d) fail: %#x!\n", VI_PHY_CHN, ret);
				return HLE_RET_ERROR;
			}

			ret = HI_MPI_VI_DisableDev(VI_DEV_ID);
			if (ret != HLE_RET_OK) {
				ERROR_LOG("HI_MPI_VI_DisableDev fail: %#x!\n", ret);
				return HLE_RET_ERROR;
			}

			vi_start_dev(VI_DEV_ID, wm);
			vi_start_chn();
			encoder_resume();
			motion_detect_init();
		}
	}

	return HI_SUCCESS;
}

WDR_MODE_E hal_get_wdr(void)
{
	return lastMode;
}

int hal_set_antiflick(int hz)
{
	ISP_EXPOSURE_ATTR_S expAttr;
	int ret = HI_MPI_ISP_GetExposureAttr(0, &expAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetExposureAttr fail: %#x\n", ret);
		return ret;
	}

	if (OP_TYPE_AUTO == expAttr.enOpType) {
		expAttr.stAuto.stAntiflicker.bEnable = hz ? HI_TRUE : HI_FALSE;
		expAttr.stAuto.stAntiflicker.u8Frequency = hz;
		ret = HI_MPI_ISP_SetExposureAttr(0, &expAttr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_ISP_SetExposureAttr fail: %#x\n", ret);
			return ret;
		}
	}

	return HI_SUCCESS;
}

int hal_get_antiflick(void)
{
	ISP_EXPOSURE_ATTR_S expAttr;
	int ret = HI_MPI_ISP_GetExposureAttr(0, &expAttr);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_ISP_GetExposureAttr fail: %#x\n", ret);
	}
	if (OP_TYPE_AUTO == expAttr.enOpType) {
		return expAttr.stAuto.stAntiflicker.bEnable ? expAttr.stAuto.stAntiflicker.u8Frequency : 0;
	}
	return 0;
}
