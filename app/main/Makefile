include ../Make.param

SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
TARGET=ipc18

.PHONY: all clean

all:$(TARGET)
	cp -f $(TARGET).bin $(MY_OUT)/
	cp -f $(TARGET).bin /work/tftp/

%.d:%.c
	@set -e; rm -f $@; \
	$(CC) -MM $< $(INC_FLAGS) $(CFLAGS) > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

sinclude $(SRCS:.c=.d)

%.o:%.c
	@$(CC) -o $@ -c $< $(CFLAGS) $(INC_FLAGS)

$(TARGET):$(OBJS)
	$(LD) $(LDFLAGS) -Map=$@.map -o $@ $^ --start-group $(SDK_LIB) $(LIBDEP) -lencoder -lamazon -lstream -lhi1131wifi -lwpa -lshine -lPPCS_API -liperf -lhisilink -lhilink_adapt -lhilinksmartlink -lhisidata_backup --end-group
	$(OBJCOPY) -O binary $@ $@.bin
	$(OBJDUMP) -d $@ > $@.asm

clean:
	-rm -f $(TARGET) *.o *.d *.d.*
