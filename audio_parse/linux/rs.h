#ifndef RS2_H
#define RS2_H

typedef unsigned char rstype;

void rsInit();
int rsEncode(rstype _data[], int _dataLen, rstype _bb[]);
int rsDecode(rstype _data[], int _eras_pos[], int _no_eras);

#endif


