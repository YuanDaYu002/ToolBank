




#include <stdio.h>    
#include "RTMPStream.h"
#include "rtmp_typeport.h"
    
int main(int argc,char**argv)    
{       
    if(CRTMPStream_init()<0)
    {
		RTMP_ERROR_LOG("CRTMPStream_init failed!\n");
		return -1;
	}
	
    if(!CRTMPStream_Connect("rtmp://192.168.3.1"))
    {
		RTMP_ERROR_LOG("CRTMPStream_Connect failed!\n");
		goto ERR;
	}
    
	if(!CRTMPStream_SendH264File("/system/bin/test.264"))
	{
		RTMP_ERROR_LOG("CRTMPStream_SendH264File failed!\n");
		goto ERR;
	}

	CRTMPStream_Close();
   	CRTMPStream_exit();
	return 0;

ERR:    
    CRTMPStream_Close();
	CRTMPStream_exit();
	return -1; 
}  








