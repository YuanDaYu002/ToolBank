

#include <stdio.h>
#include "RTSPStream.h"

int main(int argc,char* argv[])
{
	CRTSPStream rtspSender;
	bool bRet = rtspSender.Init();
	rtspSender.SendH264File("E:\\测试视频\\test.264");
	system("pause");  
}
