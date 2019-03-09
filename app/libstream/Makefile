include ../Make.param

SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))

##########################################################
INC_FLAGS+= -I$(LINUX_ROOT)/net/lwip_sack/include/lwip
INC_FLAGS+= -I$(LINUX_ROOT)/lib/libc/kernel/uapi/liteos
INC_FLAGS+= -I$(LINUX_ROOT)/lib/libc/include
INC_FLAGS+= -I$(MY_PATH)/3rdinc/PPCS 
INC_FLAGS+= -I$(MY_PATH)/3rdinc/curl 
####Flag##################################################
MEDIA_SERVER_LDFLAG= -lPPCS_API -lcurl -lc -lpthread
MEDIA_SERVER_LDFLAG+=  -DDEBUG

#LiteOS 系统必须使用支持唤醒的参数（Advanced P2P(RTOS/Wakeup)）
MEDIA_SERVER_LDFLAG+= -DP2P_SUPORT_WAKEUP  
MEDIA_SERVER_LDFLAG+= -DENABLE_NTP_CLIENT_DEBUG

###Lib####################################################
MEDIA_SERVER_LIB =-L../3rdlibs
MEDIA_SERVER_LIB+=-L$(LINUX_ROOT)/lib/libc


TARGET=libstream.a
TEST_TARGET=test_stream

.PHONY: all clean test

all:$(TARGET)
	cp -f $(TARGET) $(MY_LIBOUT)/

%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -MM $< $(INC_FLAGS) $(CFLAGS) $(MEDIA_SERVER_LDFLAG) $(MEDIA_SERVER_LIB)  > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

sinclude $(SRCS:.c=.d)

%.o:%.c
	@$(CC) -o $@ -c $< $(CFLAGS) $(INC_FLAGS) $(MEDIA_SERVER_LDFLAG) $(MEDIA_SERVER_LIB)

$(TARGET):$(OBJS)
	$(AR) -rc $@ $^

test:$(OBJS) ../main/main.o
	$(LD) $(LDFLAGS) -Map=$@.map -o $@ $^ --start-group $(SDK_LIB) $(LIBDEP) -lencoder -lamazon -lstream --end-group
	$(OBJCOPY) -O binary $(TEST_TARGET) $(TEST_TARGET).bin
	$(OBJDUMP) -d $(TEST_TARGET) > $(TEST_TARGET).asm

clean:
	-rm -f $(TARGET) *.o *.d *.d.*
