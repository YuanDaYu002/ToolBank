
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/times.h>

#include "hal_def.h"
#include "spm.h"
#include "lsslab.h"
#include "encoder.h"

typedef struct __tag_STREAM_QUEUE_NODE {
	struct __tag_STREAM_QUEUE_NODE *next;
	ENC_STREAM_PACK *pack;
} STREAM_QUEUE_NODE;

typedef struct {
	int active;
	int count; /*队列中结点数目*/
	int vframe_count;
	int block_level; /*当前拥塞等级*/
	int drop_frame; /*是否丢弃到I帧*/
	int down_count; /*连续降低拥塞等级的次数*/
	pthread_mutex_t lock;
	pthread_cond_t cond;
	STREAM_QUEUE_NODE *head;
	STREAM_QUEUE_NODE *tail;
} STREAM_QUEUE;

#define MAX_QUEUED_VFRAME   50      /*队列中帧数超过该值开始丢帧*/
#define QUEUES_PER_STREAM   10      /*每个码流的最大分发队列数*/


#define MIN_DOWN_THRES      5
#define MAX_DOWN_THRES      300

typedef struct {
	int count;
	int level; /*该路码流的拥塞等级*/
	int down_thres; /*降低拥塞等级的阈值*/
	HLE_U32 down_time; /*上次降低拥塞等级的时间，单位ticks*/
	int is_down; /*上次调整拥塞等级是升还是降，0升，1降*/
	pthread_mutex_t lock;
	STREAM_QUEUE stream_queues[QUEUES_PER_STREAM];
} STREAM_DISPATCHER;

static LSSLAB_HANDLE slb_hdl;
static STREAM_DISPATCHER stream_dps[ENC_STREAM_NUM];


#define ENC_GET_VI_CHN(enc_chn)         ((enc_chn)/STREAMS_PER_CHN)
#define ENC_GET_STREAN_INDEX(enc_chn)   ((enc_chn)%STREAMS_PER_CHN)

int sdp_max_prev_count(void)
{
	/*减去录像和RTSP占用的路数*/
	return QUEUES_PER_STREAM - 2;
}

int encoder_set_enc_level(int channel, int stream_index, int level);

void __calc_enc_block_level(STREAM_DISPATCHER *psdp, int channel, int stream_index)
{
#if 1
	int i;
	int count = 0;
	int level_times[MAX_BLOCK_LEVEL] = {0};

	//计算出每个level出现的次数
	for (i = 0; i < QUEUES_PER_STREAM; i++) {
		STREAM_QUEUE *queue = psdp->stream_queues + i;

		if (queue->active && queue->block_level >= 0) {
			if (queue->block_level >= MAX_BLOCK_LEVEL) {
				ERROR_LOG("queue->block_level >= MAX_BLOCK_LEVEL %d\n", queue->block_level);
			} else {
				count++;
				level_times[queue->block_level]++;
			}
		}
	}

	int level = 0;
	int sum = 0; //比当前值小的元素个数，i为当前值
	for (i = 0; i < MAX_BLOCK_LEVEL; i++) {
		if (level_times[i] > 0) {
			if (sum < count - sum) {
				level = i;
			} else {
				break;
			}

			sum += level_times[i];
		}
	}
#else
	int i;
	int count = 0;
	int level = 0;
	for (i = 0; i < QUEUES_PER_STREAM; i++) {
		STREAM_QUEUE *queue = psdp->stream_queues + i;

		if (queue->active && queue->block_level >= 0) {
			level += queue->block_level;
			count++;
		}
	}
	if (count != 0)
		level /= count;
#endif

	if (level == psdp->level)
		return;

	if (level < psdp->level) {
		//当前为降低拥塞等级
		psdp->down_thres /= 2;
		if (psdp->down_thres < MIN_DOWN_THRES)
			psdp->down_thres = MIN_DOWN_THRES;

		psdp->down_time = times(NULL);
		psdp->is_down = 1;
	} else if (psdp->is_down) {
		//当前为提高拥塞等级，但上次拥塞调整为降低拥塞等级
		HLE_U32 curr_tm = times(NULL);
		curr_tm = (curr_tm - psdp->down_time) / 100; //每秒100个ticks
		if (curr_tm <= 10) {
			//10秒内的抖动我们认为是上次降低拥塞等级引起的，所以要提高拥塞调整间隔
			psdp->down_thres *= 4;
			if (psdp->down_thres > MAX_DOWN_THRES)
				psdp->down_thres = MAX_DOWN_THRES;
		} else {
			//不是由拥塞控制引起的，应该是网络本身带宽发生变化，把拥塞调整间隔设置到最小
			psdp->down_thres = MIN_DOWN_THRES;
		}
		psdp->is_down = 0;
	}

	encoder_set_enc_level(channel, stream_index, level);
	psdp->level = level;
}

int encoder_start(int channel, int stream_index);

/*
	function:  sdp_request_queue
	description:  从指定的编码通道中申请一路码流队列
	args:
		int enc_chn[in]，编码通道号
		int auto_rc[in]，是否影响自动码率控制，1---影响，0---不影响
	return:
		>=0  success，返回申请到的队列的ID
		<0  fail
 */
int sdp_request_queue(int enc_chn, int auto_rc)
{
	if (slb_hdl == NULL)
		return -1;
	if (enc_chn < 0 || enc_chn >= ENC_STREAM_NUM)
		return HLE_RET_EINVAL;

	int i;
	STREAM_DISPATCHER *psdp = stream_dps + enc_chn;
	pthread_mutex_lock(&psdp->lock);
	for (i = 0; i < QUEUES_PER_STREAM; i++) 
	{
		STREAM_QUEUE *queue = psdp->stream_queues + i;
		pthread_mutex_lock(&queue->lock);
		if (!queue->active) 
		{
			int channel = ENC_GET_VI_CHN(enc_chn);
			int stream_index = ENC_GET_STREAN_INDEX(enc_chn);

			queue->active = 1;
			queue->count = 0;
			queue->vframe_count = 0;
			if (auto_rc)
				queue->block_level = 0;
			else
				queue->block_level = -1;
			queue->drop_frame = 0;
			queue->down_count = 0;
			psdp->count++;
			if (psdp->count == 1) 
			{
				if (encoder_start(channel, stream_index) != HLE_RET_OK)
					ERROR_LOG("encoder_start channel[%d], stream_index[%d] failed!\n",
					channel, stream_index);
			} 
			else 
			{
				encoder_force_iframe(channel, stream_index);
				if (auto_rc)
					__calc_enc_block_level(psdp, channel, stream_index);
			}

			pthread_mutex_unlock(&queue->lock);
			pthread_mutex_unlock(&psdp->lock);
			int queue_id = (enc_chn << 16) | i;
			DEBUG_LOG("request queue success, queue_id=%#x\n", queue_id);
			return queue_id;
		}
		pthread_mutex_unlock(&queue->lock);
	}
	pthread_mutex_unlock(&psdp->lock);

	return -1;
}


int encoder_stop(int channel, int stream_index);

/*
	function:  sdp_free_queue
	description:  释放一路码流队列
	args:
		int queue_id[in]，队列ID
	return:
		0  success
		<0  fail
 */
int sdp_free_queue(int queue_id)
{
	if (slb_hdl == NULL)
		return -1;

	int enc_chn = queue_id >> 16;
	int queue_index = queue_id & 0xFFFF;
	if (enc_chn < 0 || enc_chn >= ENC_STREAM_NUM)
		return HLE_RET_EINVAL;
	if (queue_index < 0 || queue_index >= QUEUES_PER_STREAM)
		return HLE_RET_EINVAL;

	STREAM_DISPATCHER *psdp = stream_dps + enc_chn;
	STREAM_QUEUE *queue = psdp->stream_queues + queue_index;

	pthread_mutex_lock(&queue->lock);
	if (!queue->active) {
		pthread_mutex_unlock(&queue->lock);
		return -1;
	}

	//clear the queue
	while (queue->head != NULL) {
		STREAM_QUEUE_NODE *node = queue->head;
		queue->head = node->next;

		spm_dec_pack_ref(node->pack);
		lsslab_free(slb_hdl, node);
	}
	queue->tail = NULL;
	queue->active = 0;
	queue->count = 0;
	queue->vframe_count = 0;
	pthread_mutex_unlock(&queue->lock);
	pthread_cond_signal(&queue->cond); //防止调用sdp_dequeue的线程一直阻塞无法退出

	int channel = ENC_GET_VI_CHN(enc_chn);
	int stream_index = ENC_GET_STREAN_INDEX(enc_chn);
	pthread_mutex_lock(&psdp->lock);
	psdp->count--;
	if (psdp->count == 0) {
		if (encoder_stop(channel, stream_index) != HLE_RET_OK)
			ERROR_LOG("encoder_stop channel[%d], stream_index[%d] failed!\n",
			channel, stream_index);
	}
	//只有网络会频繁释放码率队列，而释放时会影响到网络情况，所以把拥塞调整间隔设置到最小
	psdp->down_thres = MIN_DOWN_THRES;
	pthread_mutex_unlock(&psdp->lock);

	DEBUG_LOG("free queue success, queue_id=%#x\n", queue_id);
	return 0;
}

/*
	function:  sdp_enqueue
	description:  码流包加入分发队列接口
	args:
		int enc_chn[in]，该包所属的编码通道
		ENC_STREAM_PACK *pack[in],  要加入的码流包
	return:
		0  success
		<0  fail
 */
int sdp_enqueue(int enc_chn, ENC_STREAM_PACK *pack)
{
	if (slb_hdl == NULL)
		return -1;
	if (enc_chn < 0 || enc_chn >= ENC_STREAM_NUM)
		return HLE_RET_EINVAL;
	if (pack == NULL)
		return HLE_RET_EINVAL;

	int i, calc_bl = 0;
	STREAM_DISPATCHER *psdp = stream_dps + enc_chn;
	spm_inc_pack_ref(pack);
	for (i = 0; i < QUEUES_PER_STREAM; i++) 
	{
		STREAM_QUEUE *queue = psdp->stream_queues + i;
		pthread_mutex_lock(&queue->lock);
		if (queue->active) 
		{
			if (queue->vframe_count > MAX_QUEUED_VFRAME) 
			{
				ERROR_LOG("enc_chn[%d] queue[%d], drop to iframe\n", enc_chn, i);
				queue->drop_frame = 1;
				pthread_mutex_unlock(&queue->lock);
				continue;

			}
			
			if (queue->drop_frame) 
			{
				FRAME_HDR* fh = (FRAME_HDR *) pack->data;
				if (fh->type == 0xf8) 
				{
					queue->drop_frame = 0;
				} 
				else 
				{
					pthread_mutex_unlock(&queue->lock);
					continue;
				}
			}

			STREAM_QUEUE_NODE *node = (STREAM_QUEUE_NODE *) lsslab_alloc(slb_hdl);
			if (node == NULL) 
			{
				ERROR_LOG("lsslab_alloc failed!\n");
				pthread_mutex_unlock(&queue->lock);
				continue;
			}

			int trigger = 0;
			/*add to stream queue tail*/
			spm_inc_pack_ref(pack);
			node->pack = pack;
			node->next = NULL;
			if (queue->tail == NULL) 
			{
				queue->tail = node;
				queue->head = node;
				trigger = 1;
			} 
			else 
			{
				queue->tail->next = node;
				queue->tail = node;
			}
			FRAME_HDR* fh = (FRAME_HDR *) pack->data;
			if (fh->type == 0xF8 || fh->type == 0xF9)
				queue->vframe_count++;
			queue->count++;

			pthread_mutex_unlock(&queue->lock);
			if (trigger) /*trigger a signal if inner list was empty before add this node*/
				pthread_cond_signal(&queue->cond);

			if (queue->block_level != -1) //queue->block_level等于-1表示该队列不参与拥塞控制
			{
				int level = queue->vframe_count / (MAX_QUEUED_VFRAME / MAX_BLOCK_LEVEL);
				if (level >= MAX_BLOCK_LEVEL)
					level = MAX_BLOCK_LEVEL - 1;

				if (level != queue->block_level) 
				{
					if (level > queue->block_level) 
					{
						queue->block_level = level;
						queue->down_count = 0;
						calc_bl = 1;
					} 
					else if (level == 0) 
					{
						if (fh->type == 0xf8) //升码率只在当前帧为I帧时进行判断
						{
							queue->down_count++;
							if (queue->down_count >= psdp->down_thres) 
							{
								queue->block_level--; //升码率必须缓慢的升，为了防止丢到I帧时出现block_level陡升的情况
								queue->down_count = 0;
								calc_bl = 1;
							}
						}
					}
				}
			}
		} else
			pthread_mutex_unlock(&queue->lock);
	}
	spm_dec_pack_ref(pack);
	//debug
	//printf("A001 after dec ref");
	//spm_pack_print_ref(pack);


	if (calc_bl) {
		int channel = ENC_GET_VI_CHN(enc_chn);
		int stream_index = ENC_GET_STREAN_INDEX(enc_chn);

		pthread_mutex_lock(&psdp->lock);
		__calc_enc_block_level(psdp, channel, stream_index);
		pthread_mutex_unlock(&psdp->lock);
	}

	return 0;
}

/*
	function:  spm_dequeue
	description:  从分发队列取出码流包接口
	args:
		int queue_id[in], 码流分发队列ID
	return:
		non-NULL  success  指向所取出的码流包的指针
		NULL    fail
 */
ENC_STREAM_PACK *sdp_dequeue(int queue_id)
{
	if (slb_hdl == NULL)
		return NULL;

	int enc_chn = queue_id >> 16;
	int queue_index = queue_id & 0xFFFF;
	if (enc_chn < 0 || enc_chn >= ENC_STREAM_NUM)
		return NULL;
	if (queue_index < 0 || queue_index >= QUEUES_PER_STREAM)
		return NULL;

	STREAM_DISPATCHER *psdp = stream_dps + enc_chn;
	STREAM_QUEUE *queue = psdp->stream_queues + queue_index;
	pthread_mutex_lock(&queue->lock);
	while (queue->head == NULL) 
	{
		if (queue->active == 0) 
		{
			pthread_mutex_unlock(&queue->lock);
			return NULL;
		}
		pthread_cond_wait(&queue->cond, &queue->lock);
	}

	STREAM_QUEUE_NODE *node = queue->head;
	queue->head = node->next;
	if (queue->head == NULL)
		queue->tail = NULL;

	ENC_STREAM_PACK *pack = node->pack;
	FRAME_HDR* fh = (FRAME_HDR *) pack->data;
	if (fh->type == 0xF8 || fh->type == 0xF9)
		queue->vframe_count--;
	queue->count--;
	pthread_mutex_unlock(&queue->lock);

	lsslab_free(slb_hdl, node);
	return pack;
}

/*
	function:  sdp_init
	description:  SPD模块初始化接口
	args:
		int pack_count[in], SPM模块最大产出的pack数目
	return:
		0  success
		<0  fail
 */
int sdp_init(int pack_count)
{
	if (slb_hdl != NULL)
		return -1;

	slb_hdl = lsslab_create(sizeof (STREAM_QUEUE_NODE), QUEUES_PER_STREAM * pack_count);
	if (slb_hdl == NULL)
		return HLE_RET_ENORESOURCE;

	int i;
	for (i = 0; i < ENC_STREAM_NUM; i++) {
		STREAM_DISPATCHER *psdp = stream_dps + i;
		psdp->count = 0;
		psdp->level = 0;
		psdp->down_thres = MIN_DOWN_THRES;
		psdp->is_down = 0;
		pthread_mutex_init(&psdp->lock, NULL);

		int j;
		for (j = 0; j < QUEUES_PER_STREAM; j++) {
			psdp->stream_queues[j].active = 0;
			psdp->stream_queues[j].count = 0;
			pthread_mutex_init(&psdp->stream_queues[j].lock, NULL);
			pthread_cond_init(&psdp->stream_queues[j].cond, NULL);
			psdp->stream_queues[j].head = NULL;
			psdp->stream_queues[j].tail = NULL;
		}
	}

	return 0;
}

void sdp_exit(void)
{
	if (slb_hdl == NULL)
		return;

	int i;
	for (i = 0; i < ENC_STREAM_NUM; i++) {
		int j;
		STREAM_DISPATCHER *psdp = stream_dps + i;
		for (j = 0; j < QUEUES_PER_STREAM; j++) {
			pthread_cond_destroy(&psdp->stream_queues[j].cond);
			pthread_mutex_destroy(&psdp->stream_queues[j].lock);
		}
		pthread_mutex_destroy(&psdp->lock);
	}
	lsslab_destroy(slb_hdl);
	slb_hdl = NULL;
}

void lsslab_debug_info(LSSLAB_HANDLE hslab);
void spm_debug_info(void);

void sdp_debug_info(void)
{
	if (slb_hdl == NULL)
		return;

	int i;
	DEBUG_LOG("--------------------active queues-------------------\n");
	for (i = 0; i < ENC_STREAM_NUM; i++) {
		STREAM_DISPATCHER *psdp = stream_dps + i;

		int j;
		for (j = 0; j < QUEUES_PER_STREAM; j++) {
			pthread_mutex_lock(&psdp->stream_queues[j].lock);
			if (psdp->stream_queues[j].active) {
				DEBUG_LOG("stream %d, queue %d, count %d, vframe count %d, block_level %d, down_count %d\n",
					i, j, psdp->stream_queues[j].count, psdp->stream_queues[j].vframe_count,
					psdp->stream_queues[j].block_level, psdp->stream_queues[j].down_count);
			}
			pthread_mutex_unlock(&psdp->stream_queues[j].lock);
		}
	}


	lsslab_debug_info(slb_hdl);
	spm_debug_info();
}


