/******************************************************************************************************************
					linux_X64  version
******************************************************************************************************************/
1.该文件下:live/mediaServer/  库源码文件已经做过修改，针对嵌入式（linux_X64 / 君正T20 / 海思Hi3516EV100_liteos）移植
	A:live555MediaServer.cpp  	modified
	B:live555MediaServer.cpp/.h  	added
	C:WW_H264VideoSource.cpp/.h 	added
2.app目录下为应用层往有名管道(/tmp/H264_fifo)传输H264数据的代码
3.live555最终生成的服务端程序是：live/mediaServer/live555MediaServer  ，只需要运行该文件就是启动服务端程序
