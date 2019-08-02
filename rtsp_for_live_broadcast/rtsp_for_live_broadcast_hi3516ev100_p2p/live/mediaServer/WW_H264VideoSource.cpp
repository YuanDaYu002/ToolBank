#include "WW_H264VideoSource.h"
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#endif

#include "P2P_ReadWriteTester.h"
#include "media_server_signal_def.h"

#define FIFO_NAME     "/tmp/H264_fifo"
#define BUFFER_SIZE   PIPE_BUF
#define REV_BUF_SIZE  (1024*1024)

//共享内存相关参数
#define SHM_FLAG_ID		13
#define SHM_SIZE		1024*300
//进程同步相关参数
#define SEM_MUTEX_R 	"rtsp_mutex_r" 
#define SEM_MUTEX_W 	"rtsp_mutex_w" 


#ifdef WIN32
#define mSleep(ms)    Sleep(ms)
#else
#define mSleep(ms)    usleep(ms*1000)
#endif


WW_H264VideoSource::WW_H264VideoSource(UsageEnvironment & env) : 
FramedSource(env),
m_pToken(0),
m_pFrameBuffer(NULL),
shm_buf(NULL)
{
#if 0
	/*---#有名信号灯初始化------------------------------------------------------------*/
	semr = sem_open(SEM_MUTEX_R, O_CREAT | O_RDWR , 0666, 0);
	if (semr == SEM_FAILED)
	{
		printf("[MEDIA SERVER] error sem_open semr failed errno=%d\n",errno);
		return;
	}

	semw = sem_open(SEM_MUTEX_W, O_CREAT | O_RDWR, 0666, 0);
	if (semw == SEM_FAILED)
	{
		printf("[MEDIA SERVER] error sem_open semw failed errno=%d\n",errno);
		return;
	}

	/*---#共享内存初始化------------------------------------------------------------*/
	key_t key;
	key = ftok("/tmp", SHM_FLAG_ID);
	if (key < 0)
	{
		printf("[MEDIA SERVER] error ftok failed\n");
		return;
	}
	
	shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);//创建共享内存,IPC_CREAT | 0666为权限设置，//类似open（）函数的权限位一般这样写就行
	if (shmid < 0)
	{
		printf("[MEDIA SERVER] error shmget failed\n");
		return ;
	}

	shm_buf = shmat(shmid, NULL, 0);//共享内存映射。NULL表示系统自动分配；0：表共享内存可读写
	if (NULL == shm_buf)
	{
		printf("[MEDIA SERVER] error shmat failed\n");
		shmctl(shmid, IPC_RMID, NULL);//对共享内存的控制或者处理；IPC_RMID:删除对象，删除创建的共享内存
		return;
	}

	m_pFrameBuffer = (char*)shm_buf;
#endif
	
}

WW_H264VideoSource::~WW_H264VideoSource(void)
{
	#if 0  //加上就会打不开视频流
	sem_unlink(SEM_MUTEX_R);
	sem_unlink(SEM_MUTEX_W);

	if(NULL != shm_buf)
	{
		shmdt(shm_buf);//共享内存分离
		shm_buf = NULL;
		m_pFrameBuffer = NULL;
	}
	shmctl(shmid, IPC_RMID, NULL);//对共享内存的控制或者处理；IPC_RMID:删除对象,删除创建的共享内存

	#endif

	envir().taskScheduler().unscheduleDelayedTask(m_pToken);

	static int exit_count = 0;
	exit_count ++;
	if(exit_count >= 2)//拉流成功后该释放函数会被调用一次，原因未查，所以，第二次调用才允许进行释放操作
	{
		
		living_stream_exit();
		printf("[MEDIA SERVER] rtsp connection closed  ####### ~WW_H264VideoSource #####!!!!!\n");
	}
	//printf("[MEDIA SERVER] rtsp connection closed  ####### ~WW_H264VideoSource #####!!!!!\n");
}

void WW_H264VideoSource::doGetNextFrame()
{
	// 根据 fps，计算等待时间
	double delay = 1000.0 / (FRAME_PER_SEC*2);  // ms
	int to_delay = delay * 1000;  // us

	m_pToken = envir().taskScheduler().scheduleDelayedTask(to_delay, getNextFrame, this);
}

unsigned int WW_H264VideoSource::maxFrameSize() const
{
	return SHM_SIZE;
}

void WW_H264VideoSource::getNextFrame(void * ptr)
{
	((WW_H264VideoSource *)ptr)->GetFrameData();
}

static int P2P_init_flag = 0;
void WW_H264VideoSource::GetFrameData()
{
	if(0 == P2P_init_flag)
	{
		/*---#P2P实时流初始化------------------------------------------------------------*/
		printf("start living stream  transfer......\n");
		int ret= living_stream_init();
		if(ret < 0)
		{
			printf("living_stream_init failed !\n");
			return ;
		}
		
		
		ret = living_stream_satrt();
		if(ret < 0)
		{
			printf("living_stream_satrt failed !\n");
			return ;
		}
		
		P2P_init_flag = 1;
		
	}
	
	gettimeofday(&fPresentationTime, 0);
    fFrameSize = 0;//帧数据大小

	/*---#P2P获取帧数据------------------------------------------------------------*/
	char* frame_data = NULL;
	int ret = living_stream_get_frame_data((void**)&frame_data, (unsigned int *)&fFrameSize);
	if(ret < 0)
	{
		printf("get frame data failed!\n");
		return ;
	}
	//printf("[MEDIA SERVER] living_stream_get_frame_data len = [%d],fMaxSize = [%d]\n",fFrameSize,fMaxSize);

	// fill frame data
	if (fFrameSize > fMaxSize)
	{
		ERROR_LOG("fFrameSize is too big!! fFrameSize(%d) fMaxSize(%d) \n",fFrameSize,fMaxSize);
		fNumTruncatedBytes = fFrameSize - fMaxSize;
		fFrameSize = fMaxSize;
	}
	else
	{
		fNumTruncatedBytes = 0;
	}
	memcpy(fTo,frame_data,fFrameSize);
	fDurationInMicroseconds = 1000/FRAME_PER_SEC*1000;
				 
	afterGetting(this);
	
	
}




