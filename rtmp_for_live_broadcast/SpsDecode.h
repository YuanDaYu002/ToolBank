
#ifndef _SPS_DECODE_H
#define _SPS_DECODE_H

unsigned int Ue(char *pBuff, unsigned int nLen, unsigned int nStartBit);   
  
  
int Se(char *pBuff, unsigned int nLen, unsigned int nStartBit);
  
  
int u(unsigned int BitCount,char * buf,unsigned int nStartBit); 
  
  
char h264_decode_sps(char * buf,unsigned int nLen,int width,int height);

#endif




