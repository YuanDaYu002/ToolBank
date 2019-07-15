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
	
}

WW_H264VideoSource::~WW_H264VideoSource(void)
{
	#if 0
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

	printf("[MEDIA SERVER] rtsp connection closed  ####### ~WW_H264VideoSource #####!!!!!\n");
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

void WW_H264VideoSource::GetFrameData()
{
	gettimeofday(&fPresentationTime, 0);

    fFrameSize = 0;

	//printf("[MEDIA SERVER]*********sem_post(semw)**************\n");
	sem_post(semw); //打开“写”开关 
	
	sem_wait(semr); //等待“可读”
	printf("[MEDIA SERVER]******** GetFrameData can read !********\n");
	memcpy(&fFrameSize,m_pFrameBuffer,4);  //读取数据长度信息
	//printf("[recv]shm_buf[]=%d %d %d %d %d\n",m_pFrameBuffer[0],m_pFrameBuffer[1],m_pFrameBuffer[2],m_pFrameBuffer[3],m_pFrameBuffer[4]);
	if(fFrameSize > 0)
	{
		memcpy(fTo,m_pFrameBuffer + 4,fFrameSize);
	}
	else
	{
		printf("[MEDIA SERVER] GetFrameData fFrameSize[%d] error !\n",fFrameSize);
	}
	//printf("[media server] fTo[] = %#x %#x %#x %#x %#x\n",fTo[0],fTo[1],fTo[2],fTo[3],fTo[4]);
	//sem_post(semw); 

	/*---#安全判断------------------------------------------------------------*/
	if (fFrameSize > fMaxSize)//buf放不下
	{
		
		fNumTruncatedBytes = fFrameSize - fMaxSize; //计算截断的字节数
		fFrameSize = fMaxSize;
		printf("[MEDIA SERVER]warning: fNumTruncatedBytes = [%d]\n",fNumTruncatedBytes);
	}
	else//buf能放下
	{
		fNumTruncatedBytes = 0;
	}
	
	fDurationInMicroseconds = 1000/FRAME_PER_SEC*1000;
				 
	afterGetting(this);
}




