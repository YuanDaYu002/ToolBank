
MY_PATH=$(shell pwd)

INC_FLAGS=-I$(MY_PATH)/freetype-2.4.10_out/include
INC_FLAGS+=-I$(MY_PATH)/freetype-2.4.10_out/include/freetype2/freetype
INC_FLAGS+=-I$(MY_PATH)/SDL2-2.0.8_out/include/SDL2
INC_FLAGS+=-I$(MY_PATH)/SDL2_ttf-2.0.15_out/include/SDL2

LDFLAGS=-L$(MY_PATH)/freetype-2.4.10_out/lib
LDFLAGS+=-L$(MY_PATH)/SDL2-2.0.8_out/lib
LDFLAGS+=-L$(MY_PATH)/SDL2_ttf-2.0.15_out/lib

TARGET=creat_bmp
TARGET1=parse_bmp
SRC= creat_bmp_demo.c
LIB:=  -lfreetype  -lSDL2_test  -lSDL2main  -lSDL2  -lSDL2_ttf -ldl -lm -lpthread
#OTHER_LIB =./SDL2-2.0.8_out/lib/libSDL2-2.0.so.0 
#OTHER_LIB2 =./SDL2_ttf-2.0.15_out/lib/libSDL2_ttf-2.0.so.0 


# /home/yuanda.yu/git/font/for_linux_x64/back_001/creat_bmp_demo/SDL2-2.0.8_out/lib
all:  
	gcc   $(SRC)  -o $(TARGET)  $(LDFLAGS) $(INC_FLAGS) -Wl,-Bdynamic $(LIB) 
	rm -rf ./libs/*
	cp -rf ./SDL2-2.0.8_out/lib/* ./libs/
	cp -rf ./SDL2_ttf-2.0.15_out/lib/* ./libs/
	gcc parse_bmp.c -o $(TARGET1)	

.PHONY : clean

clean:
	rm -f $(TARGET) $(TARGET1)
	rm -rf ./bmp/*

