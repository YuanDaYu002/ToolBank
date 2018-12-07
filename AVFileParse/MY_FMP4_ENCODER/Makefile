
TARGET=libfmp4Encode.a
CC=arm-himix100-linux-gcc
CFLAG= -DUSE_LITE_OS_SYSTEM

all:*.o
	ar crs $(TARGET) $^  

%.o:%.c
	$(CC) -c $<  $^ $(CFLAG)

.PHONY:clean

clean:
	rm -f ./*.o
	rm -f $(TARGET)
 
