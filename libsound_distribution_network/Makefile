include ../Make.param

SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))

TARGET=libacoustic_recognition.a

.PHONY: all clean 

all:$(TARGET)

%.o:%.c
	@$(CC) -o $@ -c $<  $(CFLAGS) -lpthread

$(TARGET):$(OBJS)
	$(AR) -rc $@ $^
	cp -f $(TARGET) $(MY_LIBOUT)/

clean:
	-rm -f $(TARGET) *.o 
