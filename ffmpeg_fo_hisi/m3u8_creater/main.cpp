#include "ffmpeg.h"

int main(int argc ,char ** argv)
{
	av_register_all();
	avformat_network_init();

	init_demux(INPUTURL,&icodec);
	printf("--------程序运行开始----------\n");
	//////////////////////////////////////////////////////////////////////////
	slice_up();
	//////////////////////////////////////////////////////////////////////////
	uinit_demux();
	printf("--------程序运行结束----------\n");
	printf("-------请按任意键退出---------\n");
	return getchar();
}