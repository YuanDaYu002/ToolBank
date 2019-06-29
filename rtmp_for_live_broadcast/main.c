#include <stdio.h>    
#include "RTMPStream\RTMPStream.h"    
    
int main(int argc,char* argv[])    
{       
    
    bool bRet = Connect("rtmp://192.168.1.104/live/test");    
    
    SendH264File("E:\\video\\test.264");    
    
    Close();    
}  

