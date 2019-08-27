/*
 * Ingenic IMP RTSPServer MAIN.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <yakun.li@ingenic.com>
 */

#include <BasicUsageEnvironment.hh>
#include "H264VideoServerMediaSubsession.hh"
#include "VideoInput.hh"
#include <RTSPServer.hh>
#include <stdio.h>

#ifdef  __cplusplus
extern "C" {
#endif

portNumBits rtspServerPortNum = 8554;
char* streamDescription = strDup("RTSP/RTP stream from Ingenic Media");

static void *__rtsp_server_start(void *data)
{
#if 1
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	VideoInput* videoInput = VideoInput::createNew(*env, 0);
	if (videoInput == NULL) {
		printf("Video Input init failed %s: %d\n", __func__, __LINE__);
		exit(1);
	}

	// Create the RTSP server:
	RTSPServer* rtspServer = NULL;
	// Normal case: Streaming from a built-in RTSP server:
	rtspServer = RTSPServer::createNew(*env, rtspServerPortNum, NULL);
	if (rtspServer == NULL) {
		printf("Failed to create RTSP server %s: %d\n", __func__, __LINE__);
		exit(1);
	}

	ServerMediaSession* sms_main =
		ServerMediaSession::createNew(*env, "main", NULL, streamDescription, False);

	sms_main->addSubsession(H264VideoServerMediaSubsession::createNew(sms_main->envir(), *videoInput, 1920 * 1080 * 3 / 2 + 128));

	rtspServer->addServerMediaSession(sms_main);
	char *url = rtspServer->rtspURL(sms_main);
	printf("Play this video stream using the URL: %s\n", url);
	delete[] url;

	// Begin the LIVE555 event loop:
	env->taskScheduler().doEventLoop(); // does not return

#endif
	//return; //only to prevent compiler warning

	return NULL;
}


pthread_t tid_rtsp_server;
int rtsp_server_start(void)
{
	int ret = 0;

	ret = pthread_create(&tid_rtsp_server, NULL, __rtsp_server_start, NULL);
	if (ret < 0) {
		printf("ERROR: pthread_create rtsp server error!\n");
		return -1;
	}

	return 0;
}

int rtsp_server_stop(void)
{
	return 0;
}

#ifdef  __cplusplus
}
#endif
