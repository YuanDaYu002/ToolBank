#include <stdio.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "hi_comm_ive.h"
#include "mpi_ive.h"
#include "mpi_sys.h"
#include "mpi_vpss.h"
#include "ivs_md.h"

#include "hal_def.h"
#include "motion_detect.h"

#include "hi_sns_ctrl.h"


#define GET_MOTION_CHN_ID(chn)  (chn)

/* (0-4),0 is most sensitive */
static HLE_S32 sensitive_level[5] = {5, 10, 20, 35, 50};
static HLE_U16 md_thres[5] = {100, 150, 200, 200, 200};
HLE_S32 motion_detect_val = 0; //MD告警触发 标志变量

typedef struct {
	//pthread_mutex_t lock;
	HLE_S32 motion_chn_ison;
	HLE_S32 usr_config_level; //灵敏度
	//HLE_RECT usr_motion_rect[MAX_MD_AREA_NUM];
	IVE_SRC_IMAGE_S img[2];
	IVE_DST_MEM_INFO_S blob;
	MD_CHN chn;
	int dirty; //描述该下标所对应的的元素已被使用
	int count;
} MOTION_CONTEX;
MOTION_CONTEX mdCtx[VI_PORT_NUM];

static SIZE_S mdSize;

int motion_detect_data_proc(MOTION_CONTEX* ctx)
{
	if (0 == ctx->motion_chn_ison) return HLE_RET_OK;

	HLE_U32 i;
	IVE_CCBLOB_S* blob = (IVE_CCBLOB_S*) ctx->blob.pu8VirAddr;
	HLE_U32 area = 0;

	for (i = 1; i < blob->u8RegionNum; ++i) 
	{
		area += blob->astRegion[i].u32Area; 
		//printf("{%u;;[%d].%u }\n", area,i, blob->astRegion[i].u32Area);
	}

	HLE_S32 lvl = area * 100 / (mdSize.u32Width * mdSize.u32Height);
	if (lvl >= sensitive_level[ctx->usr_config_level]) //检测结果连续大于阈值的次数统计
	{
		if (ctx->count < 25) ctx->count++;
		//motion_detect_val |= (1 << ctx->chn);
		//printf("100000 ,,level=%d; %d\n",sensitive_level[ctx->usr_config_level],motion_detect_val);
		//printf("1111lvl=%d, count=%d\n ", lvl, ctx->count);

	} 
	else 
	{
		if (ctx->count) ctx->count--;
		//motion_detect_val &= ~(1  << ctx->chn);
		//printf("000lvl=%d, count=%d\n ", lvl, ctx->count);
	}

	/***MD告警触发     结果判断******************/
	if (ctx->count >= 3) 
	{
		motion_detect_val |= (1 << ctx->chn);//触发成功：标志位置位
	} 
	else if (ctx->count == 0) 
	{
		motion_detect_val &= ~(1 << ctx->chn);//结束触发：标志位清零
	}

	return HLE_RET_OK;
}

static pthread_t vda_md_proc_pid[VI_PORT_NUM];
static int vda_md_proc_flag;  //MD线程运行的标志

static void motion_proc_exit(void)
{
	if (1 == vda_md_proc_flag) {
		vda_md_proc_flag = 0;
		int i;
		for (i = 0; i < VI_PORT_NUM; ++i) {
			pthread_join(vda_md_proc_pid[i], NULL);
		}
	}
}

#if 0
int global_reset(void);
void __suicide(void);
#define RESET_SYS_THRES     5

static void deal_with_timeout(int count)
{
	if (count == RESET_SYS_THRES) {
		FATAR_LOG("------VI NO INTERUPT, RESET HOLE SYSTEM------ \n");
		//global_reset();
		__suicide();
	}
}
#endif

static int ive_dma_image(VIDEO_FRAME_INFO_S *frm, IVE_DST_IMAGE_S *img, int instant)
{
	IVE_SRC_DATA_S src;
	IVE_DST_DATA_S dst;

	src.pu8VirAddr = (HI_U8*) frm->stVFrame.pVirAddr[0];
	src.u32PhyAddr = frm->stVFrame.u32PhyAddr[0];
	src.u16Width = (HI_U16) frm->stVFrame.u32Width;
	src.u16Height = (HI_U16) frm->stVFrame.u32Height;
	src.u16Stride = (HI_U16) frm->stVFrame.u32Stride[0];

	dst.pu8VirAddr = img->pu8VirAddr[0];
	dst.u32PhyAddr = img->u32PhyAddr[0];
	dst.u16Width = img->u16Width;
	dst.u16Height = img->u16Height;
	dst.u16Stride = img->u16Stride[0];

	IVE_HANDLE iveHdl;
	HLE_S32 ret;
	IVE_DMA_CTRL_S ctrl = {IVE_DMA_MODE_DIRECT_COPY, 0};
	ret = HI_MPI_IVE_DMA(&iveHdl, &src, &dst, &ctrl, instant);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_MPI_IVE_DMA fail: %#x\n", ret);
		return ret;
	}

	if (instant) {
		HI_BOOL finish = HI_FALSE;
		ret = HI_MPI_IVE_Query(iveHdl, &finish, HI_TRUE);
		while (HI_ERR_IVE_QUERY_TIMEOUT == ret) {
			usleep(100);
			ret = HI_MPI_IVE_Query(iveHdl, &finish, HI_TRUE);
		}
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_IVE_Query fail: %#x\n", ret);
			return ret;
		}
	}

	return HLE_RET_OK;
}

void *motion_detect_proc(void *arg)
{
	DEBUG_LOG("pid = %d\n", getpid());
	prctl(PR_SET_NAME, "hal_md", 0, 0, 0);

	MOTION_CONTEX* ctx = (MOTION_CONTEX*) arg;
	int first = 1;
	int idx = 0;
	IVE_DST_IMAGE_S sad;

	memset(&sad, 0, sizeof (sad));

	while (vda_md_proc_flag) 
	{
		HLE_S32 ret;
		//pthread_mutex_lock(&ctx->lock);

		if (ctx->dirty) 
		{
			ctx->dirty = 0;
			ctx->count = 0;
			MD_ATTR_S attr;
			ret = HI_IVS_MD_GetChnAttr(ctx->chn, &attr);
			if (HI_SUCCESS == ret) 
			{
				attr.u16SadThr = md_thres[ctx->usr_config_level];
				ret = HI_IVS_MD_SetChnAttr(ctx->chn, &attr);
				if (HI_SUCCESS != ret) 
				{
					ERROR_LOG("HI_IVS_MD_SetChnAttr fail: %#x\n", ret);
				}
			} 
			else 
			{
				ERROR_LOG("HI_IVS_MD_GetChnAttr fail: %#x\n", ret);
			}
		}

		VIDEO_FRAME_INFO_S frm;
		ret = HI_MPI_VPSS_GetChnFrame(VPSS_GRP_ID, VPSS_CHN_MD, &frm, 0);
		if (HI_ERR_VPSS_BUF_EMPTY == ret) 
		{
			//pthread_mutex_unlock(&ctx->lock);
			usleep(10 * 1000);
			continue;
		} 
		else if (HI_SUCCESS != ret) 
		{
			//pthread_mutex_unlock(&ctx->lock);
			ERROR_LOG("HI_MPI_VPSS_GetChnFrame fail: %#x\n", ret);
			continue;
		}

		if (!first) 
		{
			ret = ive_dma_image(&frm, &ctx->img[idx], 1);
			if (HLE_RET_OK == ret) 
			{
				ret = HI_IVS_MD_Process(ctx->chn, &ctx->img[idx], &ctx->img[1 - idx], &sad, &ctx->blob); //算法把当前帧和上一帧进行比较
				if (HI_SUCCESS == ret) 
				{
					//结果分析
					motion_detect_data_proc(ctx);
					idx = 1 - idx;
				}
			}
		} 
		else 
		{
			ret = ive_dma_image(&frm, &ctx->img[1 - idx], 1);
			if (HLE_RET_OK == ret) 
			{
				first = 0;
			}
		}

		ret = HI_MPI_VPSS_ReleaseChnFrame(VPSS_GRP_ID, VPSS_CHN_MD, &frm);
		if (HI_SUCCESS != ret) 
		{
			ERROR_LOG("HI_MPI_VPSS_ReleaseChnFrame fail: %#x\n", ret);
		}

		//pthread_mutex_unlock(&ctx->lock);
	}

	return NULL;
}

static int motion_detect_stop(MD_CHN mdChn)
{
	HLE_S32 ret;

	ret = HI_IVS_MD_DestroyChn(mdChn);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("HI_IVS_MD_DestroyChn fail: %#x \n", ret);
		return ret;
	}

	return HLE_RET_OK;
}

#if 0

static int motion_detect_bind_vi(MD_CHN mdChn, VI_CHN vi_chn_id)
{
	MPP_CHN_S src, dst;
	src.enModId = HI_ID_VIU;
	src.s32DevId = VI_DEV_ID;
	src.s32ChnId = vi_chn_id;
	dst.enModId = HI_ID_VDA;
	dst.s32DevId = 0;
	dst.s32ChnId = mdChn;

	HLE_S32 ret;
	ret = HI_MPI_SYS_Bind(&src, &dst);
	DEBUG_LOG("(%d, %d, %d)----->(%d, %d, %d)\n", src.enModId, src.s32DevId, src.s32ChnId, dst.enModId, dst.s32DevId, dst.s32ChnId);
	if (ret != HI_SUCCESS) {
		ERROR_LOG("HI_MPI_SYS_Bind((%d, %d, %d), (%d, %d, %d)) fail: %#x!\n",
			src.enModId, src.s32DevId, src.s32ChnId,
			dst.enModId, dst.s32DevId, dst.s32ChnId, ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}
#endif

void motion_detect_exit(void)
{
	motion_proc_exit();

	HLE_S32 chn;
	for (chn = 0; chn < VI_PORT_NUM; chn++) {
		motion_detect_stop(GET_MOTION_CHN_ID(chn));
		//pthread_mutex_destroy(&mdCtx[chn].lock);
	}

	HI_IVS_MD_Exit();
}

static int ive_create_image(IVE_IMAGE_S* img, HLE_U16 width, HLE_U16 height)
{
	if (NULL == img) {
		return HLE_RET_EINVAL;
	}

	img->enType = IVE_IMAGE_TYPE_U8C1;
	img->u16Width = width;
	img->u16Height = height;
	img->u16Stride[0] = (width + 15) & ~15;
	HLE_U32 size = img->u16Stride[0] * height;
	int ret = HI_MPI_SYS_MmzAlloc(&img->u32PhyAddr[0], (void**) &img->pu8VirAddr[0], NULL, HI_NULL, size);
	if (HI_SUCCESS != ret) {
		ERROR_LOG("mmz alloc fail: %#x\n", ret);
		return HLE_RET_ERROR;
	}

	return HLE_RET_OK;
}

void get_vda_size(SIZE_S *size);
int motion_detect_arg_init(void);

int motion_detect_init(void)
{
	if (HLE_RET_OK != motion_detect_arg_init())
		return HLE_RET_ERROR;

	HLE_S32 ret;
	ret = HI_IVS_MD_Init();
	if (HI_SUCCESS != ret) 
	{
		ERROR_LOG("HI_IVS_MD_Init fail: %#x\n", ret);
	}

	get_vda_size(&mdSize);

	MD_ATTR_S mdAttr;
	mdAttr.enAlgMode = MD_ALG_MODE_BG; //MD 算法模式:背景法
	mdAttr.enSadMode = IVE_SAD_MODE_MB_4X4; //按 4x4 像素块计算 SAD
	mdAttr.enSadOutCtrl = IVE_SAD_OUT_CTRL_THRESH; //阈值化图输出模式
	mdAttr.u16SadThr = 200;			//Sad 阈值
	mdAttr.u16Width = mdSize.u32Width;
	mdAttr.u16Height = mdSize.u32Height;
	mdAttr.stAddCtrl.u0q16X = 32768; //加权加“xA+yB”中的权重“x”。取值范围：[1, 65535]。
	mdAttr.stAddCtrl.u0q16Y = 32768; //加权加“xA+yB”中的权重“y”。取值范围：{65536 - u0q16X}。
	mdAttr.stCclCtrl.enMode = IVE_CCL_MODE_4C;
	mdAttr.stCclCtrl.u16Step = 1 << (2 + mdAttr.enSadMode);
	mdAttr.stCclCtrl.u16InitAreaThr = mdAttr.stCclCtrl.u16Step * mdAttr.stCclCtrl.u16Step;

	HLE_S32 chn;
	for (chn = 0; chn < VI_PORT_NUM; chn++) 
	{
		MD_CHN mdChn = GET_MOTION_CHN_ID(chn);
		mdCtx[chn].chn = mdChn;
		HLE_S32 ret;
		ret = HI_IVS_MD_CreateChn(mdChn, &mdAttr);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("HI_MPI_VDA_CreateChn fail with %#x \n", ret);
			return HLE_RET_ERROR;
		}

		//源图像内存申请
		ret = ive_create_image(&mdCtx[chn].img[0], mdSize.u32Width, mdSize.u32Height);
		if (HLE_RET_OK != ret) {
			return HLE_RET_ERROR;
		}

		ret = ive_create_image(&mdCtx[chn].img[1], mdSize.u32Width, mdSize.u32Height);
		if (HLE_RET_OK != ret) {
			return HLE_RET_ERROR;
		}

		//目标图像内存申请
		mdCtx[chn].blob.u32Size = sizeof (IVE_CCBLOB_S);
		ret = HI_MPI_SYS_MmzAlloc(&mdCtx[chn].blob.u32PhyAddr, (void**) &mdCtx[chn].blob.pu8VirAddr, NULL, HI_NULL, mdCtx[chn].blob.u32Size);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("mmz alloc fail: %#x\n", ret);
			return HLE_RET_ERROR;
		}

#if 0
		ret = motion_detect_bind_vi(mdChn, VI_CHN_VDA);
		if (HI_SUCCESS != ret) {
			ERROR_LOG("motion_detect_bind_vi fail with %#x \n", ret);
			return HLE_RET_ERROR;
		}

		ret = HI_MPI_VDA_StartRecvPic(mdChn);
		if (HLE_RET_OK != ret) {
			ERROR_LOG("HI_MPI_VDA_StartRecvPic fail with %#x \n", ret);
			return HLE_RET_ERROR;
		}
#endif

		//pthread_mutex_init(&mdCtx[chn].lock, NULL);
	}

	if (0 == vda_md_proc_flag) 
	{
		vda_md_proc_flag = 1;
		for (chn = 0; chn < VI_PORT_NUM; ++chn) 
		{
			pthread_create(&vda_md_proc_pid[chn], 0, motion_detect_proc, &mdCtx[chn]);
		}
	}

	return HLE_RET_OK;
}

int motion_detect_config(int chn, MOTION_DETECT_ATTR *attr)
{
	if ((0 != chn) || (NULL == attr)) 
	{
		ERROR_LOG("para error.");
		return HLE_RET_EINVAL;
	}

	//pthread_mutex_lock(&mdCtx[chn].lock);
	if (attr->enable == 0)
		motion_detect_val &= ~(1 << chn);
	mdCtx[chn].motion_chn_ison = attr->enable;
	//memcpy((mdCtx[chn].usr_motion_rect), attr->rect, sizeof(attr->rect));
	mdCtx[chn].usr_config_level = attr->level;
	DEBUG_LOG("usr_config_level %d\n", attr->level);
	mdCtx[chn].dirty = 1;
	//pthread_mutex_unlock(&mdCtx[chn].lock);

	return HLE_RET_OK;
}

int motion_detect_get_state(int *state)
{
	if (NULL == state) {
		ERROR_LOG("para error.");
		return HLE_RET_EINVAL;
	}

	*state = motion_detect_val;
	//printf("10000000000  ,motion = %d  \n",motion_detect_val );
 
	return HLE_RET_OK;
}


#define MOTION_TETECT_CONFIG "/jffs0/motion_detect"
MOTION_DETECT_ATTR g_motion_detect_artr;

int motion_detect_arg_init(void)
{
	FILE *index_fp = NULL;
	int ret;
	if (access(MOTION_TETECT_CONFIG, F_OK))//文件不存在,配置默认参数并创建该文件 
	{
		g_motion_detect_artr.enable = 1;
		g_motion_detect_artr.level = 3;
		//memset(&g_motion_detect_artr.rect[0],0,(MAX_MD_AREA_NUM)*sizeof(HLE_RECT));
		//g_motion_detect_artr.rect[0].left = 0;//?
		//g_motion_detect_artr.rect[0].right = 480;
		//g_motion_detect_artr.rect[0].top = 272;
		//g_motion_detect_artr.rect[0].bottom = 0;


		index_fp = fopen(MOTION_TETECT_CONFIG, "w");
		if (NULL == index_fp) 
		{
			ERROR_LOG("open /configs/motion_detect fail!\n");
			return -1;
		}
		ret = fprintf(index_fp, "Enable=%d\nLevel=%d\n"/*"Rec_left=%u\nRec_right=%u\nRec_top=%u\nRec_bottom=%u\n"*/,
			g_motion_detect_artr.enable,
			g_motion_detect_artr.level/*,
			g_motion_detect_artr.rect[0].left,
			g_motion_detect_artr.rect[0].right,
			g_motion_detect_artr.rect[0].top,
			g_motion_detect_artr.rect[0].bottom */);
		if (ret < 0) 
		{
			ERROR_LOG("motion_detect fprintf fail: %d\n", ret);
			fclose(index_fp);
			return -1;
		}

		fclose(index_fp);
	} 
	else  //文件存在
	{
		index_fp = fopen(MOTION_TETECT_CONFIG, "r");
		if (NULL == index_fp) 
		{
			ERROR_LOG("open %s fail!\n", MOTION_TETECT_CONFIG);
			return -1;
		}
		int t;
		fscanf(index_fp, "Enable=%d\n", &t);
		g_motion_detect_artr.enable = t;
		fscanf(index_fp, "Level=%d\n", &t);
		g_motion_detect_artr.level = t;
		//fscanf(index_fp, "Rec_left=%u\n", &g_motion_detect_artr.rect[0].left);
		//fscanf(index_fp, "Rec_right=%u\n", &g_motion_detect_artr.rect[0].right);
		//fscanf(index_fp, "Rec_top=%u\n", &g_motion_detect_artr.rect[0].top);
		//fscanf(index_fp, "Rec_bottom=%u\n", &g_motion_detect_artr.rect[0].bottom);


		fclose(index_fp);
	}

	motion_detect_config(0, &g_motion_detect_artr);
	return 0;
}

int motion_detect_write_cfg(void)
{
	FILE *index_fp;
	int ret;

	if ((index_fp = fopen(MOTION_TETECT_CONFIG, "w")) == NULL) {
		ERROR_LOG("open /configs/motion_detect: %d\n", errno);
		return -1;
	}
	ret = fprintf(index_fp, "Enable=%d\nLevel=%d\n"/*"Rec_left=%u\nRec_right=%u\nRec_top=%u\nRec_bottom=%u\n"*/,
		g_motion_detect_artr.enable,
		g_motion_detect_artr.level/*,
		g_motion_detect_artr.rect[0].left,
		g_motion_detect_artr.rect[0].right,
		g_motion_detect_artr.rect[0].top,
		g_motion_detect_artr.rect[0].bottom*/);
	if (ret < 0) {
		ERROR_LOG("motion_detect fprintf fail: %d\n", ret);
		fclose(index_fp);
		return -1;
	}
	fclose(index_fp);

	return motion_detect_config(0, &g_motion_detect_artr);
}






