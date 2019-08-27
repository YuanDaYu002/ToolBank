
TOPDIR = .
CROSS_COMPILE := mips-linux-gnu-

CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip
AR := $(CROSS_COMPILE)ar

CFLAGS := -g -O2 -Wall

ifeq ($(FR), ON)
	CFLAGS += -DFACE_RECOG
endif

INCLUDE := -I./libs/akubelli/include \
	-I./libs/nemer/include \
	-I./rtsp_server/include \
	-I./libs/t30_sdk/include \
	-I./libs/proto/include

LIBS := -L./libs/akubelli -lakubelli.glibc -pthread \
	-L./libs/nemer -lakub_nemer \
	-L./rtsp_server -lrtsp_server \
	-L./rtsp_server/live555/lib/glibc -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock \
	-L./libs/t30_sdk/lib/glibc -limp -lalog -lsysutils

OBJS := akubelli_test.o imp.o ioattr_option.o

TARGET := akubelli_test

all: $(TARGET)

$(TARGET): $(OBJS) librtsp_server
	$(CXX) $(OBJS) -o $(TARGET) $(INCLUDE) $(LIBS) -lpthread -lm -ldl -lrt -lstdc++
	$(STRIP) $@

librtsp_server:
	make -C $(TOPDIR)/rtsp_server

%o:%c
	$(CC) $(CFLAGS) $(INCLUDE) $(LIBS) -o $@ -c $^

clean:
	rm -rf $(TARGET) $(OBJS)
	make -C $(TOPDIR)/rtsp_server clean
