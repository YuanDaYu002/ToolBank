#ifndef _WW_H264VideoSource_H
#define _WW_H264VideoSource_H


#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>


#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "FramedSource.hh"

#define FRAME_PER_SEC 15

class WW_H264VideoSource : public FramedSource
{
public:
	WW_H264VideoSource(UsageEnvironment & env);
	~WW_H264VideoSource(void);

public:
	virtual void doGetNextFrame();
	virtual unsigned int maxFrameSize() const;

	static void getNextFrame(void * ptr);
	void GetFrameData();

private:
	void *m_pToken;
	char *m_pFrameBuffer; //指向下方 shm_buf 地址
	//int  m_hFifo;

	//有名信号灯部分相关参数
	sem_t* semr;
	sem_t* semw;
	
	//共享内存部分相关参数
	int 	shmid;		//共享内存的 id
	void * 	shm_buf;  	//共享内存的起始地址
};

#endif


