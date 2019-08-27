/*
 * Ingenic IMP RTSPServer VideoInput.hh
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <yakun.li@ingenic.com>
 */

#ifndef __VIDEOINPUT_HH__
#define __VIDEOINPUT_HH__

using namespace std;

#include <list>

#include <pthread.h>
#include <MediaSink.hh>
#include <semaphore.h>
#include <imp/imp_osd.h>
#include <imp/imp_encoder.h>

#define MAX_STREAM_CNT		2
#define MAX_LINE_NUN			5
#define MAX_POINT_NUN			5
#define MAX_PERM_NUM			8

class VideoInput: public Medium {
public:
  static VideoInput* createNew(UsageEnvironment& env, int streamNum = 0);
  FramedSource* videoSource();
  static void clear_line(void);
  static int insert_line(int x0, int y0, int x1, int y1);
  static int remove_line(int x0, int y0, int x1, int y1);

  int getStream(void* to, unsigned int* len, struct timeval* timestamp, unsigned fMaxSize);
  int pollingStream(void);
  int streamOn(void);
  int streamOff(void);
  int scheduleThread1(void);
  int skipPolling(void) { return !!curPackIndex; }
  static void *osdUpdateThread(void *p);
  static int SetResolution(int w, int h);
  static int setFrameSourceFormat(char *path, uint32_t format);

public:
  static pthread_t cmdTid;
  static pthread_t ispTuneTid;
  static Boolean fFontAvailable;
  static Boolean fCoverAvailable;
  static Boolean fPicAvailable;
  static Boolean fOsdAvailable;
  static Boolean fISVAvailable;
  static Boolean fpsIsOn[MAX_STREAM_CNT];
  static Boolean fontIsOn[MAX_STREAM_CNT];
  static Boolean coverIsOn[MAX_STREAM_CNT];
  static Boolean picIsOn[MAX_STREAM_CNT];
  static int bUseMixFigure;
  static Boolean IsOn3d[MAX_STREAM_CNT];
  static int permPoint[MAX_PERM_NUM];
  static int permLastTime;
  static uint32_t moveIvsOnBitmap;
  static Boolean roiIsOn;
  static Boolean sdcIsOn;
  static Boolean isvIsOn;

  static double gFps[MAX_STREAM_CNT];
  static double gBitRate[MAX_STREAM_CNT];

  static IMPRgnHandle fontRgnHandler;
  static IMPRgnHandle fontRgnHandler_sec;
  static IMPRgnHandle coverRgnHandler;
  static IMPRgnHandle picRgnHandler;
  static IMPRgnHandle picRgnHandler_sec;
  static int fontRgnThreadRefCnt;
  static int coverRgnThreadRefCnt;
  static int picRgnThreadRefCnt;
  FramedSource* fVideoSource;
  static Boolean isvIsStart;
  static int	  isvfd;
  static Boolean b_dynamic_mode;

  static IMPCell secondLastPrev;
  static void *fisheye_handle;

  // For Spirit
  static double filter_score;
  static int filter_life;

private:
  VideoInput(UsageEnvironment& env, int streamNum = 0);
  virtual ~VideoInput();
  static bool initialize(UsageEnvironment& env);
  static void *cmdListenThread(void *p);
  static void *ispAutoTuningThread(void *p);

private:
  static Boolean fHaveInitialized;
  Boolean fpsIsStart;
  Boolean fontIsStart;
  Boolean coverIsStart;
  Boolean picIsStart;
  Boolean osdIsStart;
  Boolean roiIsStart;
  Boolean sdcIsStart;
  int     osdStartCnt;
  unsigned int nrFrmFps;
  unsigned int totalLenFps;
  uint64_t startTimeFps;
  int streamNum;
  pthread_t scheduleTid;
  int orgfrmRate;
  Boolean hasSkipFrame;
  uint32_t curPackIndex;
  IMPEncoderStream bitStream;
  bool requestIDR;
};

#endif
