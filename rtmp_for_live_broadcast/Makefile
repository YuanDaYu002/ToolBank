
#CROSS_COMPILE ?= mips-linux-gnu-
CROSS_COMPILE ?= mips-linux-uclibc-gnu-
CC = $(CROSS_COMPILE)gcc
CPLUSPLUS = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)ld
AR = $(CROSS_COMPILE)ar cr
STRIP = $(CROSS_COMPILE)strip
OBJDUMP = $(CROSS_COMPILE)objdump


CFLAGS = 
LDFLAGS = -L./3rdlibs
INC_FLAGS = -I./librtmpdump/rtmpdump/librtmp
INC_FLAGS += -I./3rdinc
INC_FLAGS += -I./3rdinc/librtmp
INC_FLAGS += -I./3rdinc/openssl
INC_FLAGS += -I./3rdinc/zlib

LIBS = -lm  -lrtmp -lssl -lcrypto -lz -ldl
#LIBS = -lm -static  -lrtmp

########################################################################
#			make command 				       #
########################################################################
TARGET = app_t20.bin

C_FILES = $(wildcard ./*.c)

.PHONY: all clean


all:$(TARGET)
	
%.o:%.c 
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): 
	$(CC) $(CFLAGS) $(LDFLAGS) $(C_FILES) -o $@  $(INC_FLAGS)  $(LIBS) 
	$(STRIP) $@
	cp -f $@ /home/nfs/
	cp -f $@ /mnt/hgfs/vmware_share/
	
clean:
	rm -f ./*.o
	rm -r $(TARGET)


