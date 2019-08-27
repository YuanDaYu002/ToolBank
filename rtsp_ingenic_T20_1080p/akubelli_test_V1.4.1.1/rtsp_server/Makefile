
CROSS_COMPILE=mips-linux-gnu-
CC=$(CROSS_COMPILE)gcc
CXX=$(CROSS_COMPILE)g++
STRIP=$(CROSS_COMPILE)strip
AR=$(CROSS_COMPILE)ar

CFLAGS= -Os -Wall \
	-L../libs/t30_sdk/lib/glibc -limp -lsysutils -lalog \
	-L./live555/lib/glibc -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock \
	-lpthread -lrt

INCLUDE = -I../libs/t30_sdk/include -I./ \
	-I./live555/include/liveMedia \
	-I./live555/include/groupsock \
	-I./live555/include/UsageEnvironment \
	-I./live555/include/BasicUsageEnvironment

OBJS = H264VideoServerMediaSubsession.o H264VideoStreamSource.o RTSPServer.o VideoInput.o

TARGET = librtsp_server.a

all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o:%.cpp
	$(CXX) $(CFLAGS) $(INCLUDE) -c -o $(notdir $@) $<

clean:
	rm $(TARGET) $(OBJS) -rf
