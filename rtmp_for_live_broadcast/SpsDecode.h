
#ifndef _SPS_DECODE_H
#define _SPS_DECODE_H

UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit);   
  
  
int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit);
  
  
DWORD u(UINT BitCount,BYTE * buf,UINT &nStartBit); 
  
  
bool h264_decode_sps(BYTE * buf,unsigned int nLen,int &width,int &height);

#endif

