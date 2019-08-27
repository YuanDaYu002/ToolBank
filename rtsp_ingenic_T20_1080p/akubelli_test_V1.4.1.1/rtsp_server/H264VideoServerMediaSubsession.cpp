/*
 * Ingenic IMP RTSPServer subsession equal to H264VideoFileServerMediaSubsession.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <yakun.li@ingenic.com>
 */

#include "H264VideoServerMediaSubsession.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamSource.hh"
#include "H264VideoStreamFramer.hh"
#include "H264VideoStreamDiscreteFramer.hh"
#include "ByteStreamFileSource.hh"
#include "VideoInput.hh"

H264VideoServerMediaSubsession*
H264VideoServerMediaSubsession
::createNew(UsageEnvironment& env, VideoInput& videoInput, unsigned estimatedBitrate) {
	return new H264VideoServerMediaSubsession(env, videoInput, estimatedBitrate);
}

H264VideoServerMediaSubsession
::H264VideoServerMediaSubsession(UsageEnvironment& env, VideoInput& videoInput, unsigned estimatedBitrate)
	: OnDemandServerMediaSubsession(env, True/*reuse the first source*/),
	  fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL),
	  fVideoInput(videoInput) {
	fEstimatedKbps = (estimatedBitrate + 500)/1000;
}

H264VideoServerMediaSubsession
::~H264VideoServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H264VideoServerMediaSubsession* subsess = (H264VideoServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H264VideoServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H264VideoServerMediaSubsession* subsess = (H264VideoServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H264VideoServerMediaSubsession::checkForAuxSDPLine1() {
  char const* dasl;
  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();

  } else if (!fDoneFlag) {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* H264VideoServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL) return fAuxSDPLine; // it's already been set up (for a previous client)

  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
    // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }
  envir().taskScheduler().doEventLoop(&fDoneFlag);
  return fAuxSDPLine;
}

FramedSource* H264VideoServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate) {
	estBitrate = fEstimatedKbps;

	// Create a framer for the Video Elementary Stream:
	return H264VideoStreamDiscreteFramer::createNew(envir(), fVideoInput.videoSource());
}

RTPSink* H264VideoServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
	OutPacketBuffer::maxSize = 1920 * 1080;
	return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
