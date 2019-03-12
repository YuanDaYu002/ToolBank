该工程用来生成BMP字库文件,目前只支持部分ASCII码字库。
前提:
	运行程序前需要先设置环境变量：
	export LD_LIBRARY_PATH=$(CURRENT_PATH)/libs
		其中，$(CURRENT_PATH)依据具体工程路径而定
执行：
1. 	make clean
	make    /*生成BMP位图生成程序 ：create_bmp 
			位图解析+点阵字库打包程序：parse_bmp*/
			
2. ./create_bmp
3. ./parse_bmp

输出：
   ./ASCII_vector 即为最终字库文件
