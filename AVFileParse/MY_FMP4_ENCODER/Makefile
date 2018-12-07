
TARGET=fmp4Encode.a
CC=arm-himix100-linux-gcc


all:*.o
	ar crs $(TARGET) $^  

%.o:%.c
	$(CC) -c $<  $^

.PHONY:clean

clean:
	rm -f ./*.o
	rm -f $(TARGET)
 
