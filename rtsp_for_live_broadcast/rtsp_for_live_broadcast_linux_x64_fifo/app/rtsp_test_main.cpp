

#include <stdio.h>
#include "RTSPStream.h"
#include <stdlib.h>

int main(int argc,char* argv[])
{
	CRTSPStream rtspSender;
	bool bRet = rtspSender.Init();
	rtspSender.SendH264File("./test.264");
	printf("send success!\n");
	//system("pause");  
	return 0;
}
