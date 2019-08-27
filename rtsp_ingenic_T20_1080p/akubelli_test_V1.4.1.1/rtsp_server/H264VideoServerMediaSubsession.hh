/*
 * Ingenic IMP RTSPServer subsession equal to H264VideoFileServerMediaSubsession.
 *
 * Copyright (C) 2014 Ingenic Semiconductor Co.,Ltd
 * Author: Zoro <yakun.li@ingenic.com>
 */

#ifndef H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define H264_VIDEO_SERVER_MEDIA_SUBSESSION_HH

#ifndef _ONDEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif
#include "VideoInput.hh"

class H264VideoServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  static H264VideoServerMediaSubsession*
  createNew(UsageEnvironment& env, VideoInput& videoInput, unsigned estimatedBitrate);

  // Used to implement "getAuxSDPLine()":
  void checkForAuxSDPLine1();
  void afterPlayingDummy1();

protected:

  H264VideoServerMediaSubsession(UsageEnvironment& env, VideoInput& videoInput, unsigned estimatedBitrate);
      // called only by createNew()
  virtual ~H264VideoServerMediaSubsession();
  void setDoneFlag() { fDoneFlag = ~0; }

protected:
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);
protected:
  unsigned fEstimatedKbps;

private:
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; // ditto
  VideoInput& fVideoInput;
};

#endif
