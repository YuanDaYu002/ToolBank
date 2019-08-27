#ifndef LINUX_H
#define LINUX_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h> 

#include "base.h"

#define size_t unsigned int

char* itoa(int value, char* result, int base);

inline char *_itoa_s(int _Value, char * _DstBuf, int _Size, int _Radix)
{
	return itoa(_Value, _DstBuf, _Radix);
}

inline char *___gcvt(double _val, int _numOfDigits, char *_result)
{
	sprintf(_result, "%f", _val);
	return _result;
}

inline char *_gcvt_s(char *_buf, int _bufSize, double _Value, int _NumOfDigits)
{
	return ___gcvt(_Value, _NumOfDigits, _buf);
	return _buf;
}

void Sleep(int _ms);

#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#define _tcscmp strcmp
#define _stricmp strcasecmp
#define scanf_s(_sformat, _DstBuf, _SizeInBytes, ...) scanf((_sformat), (_DstBuf), __VA_ARGS__)
#define fopen_s fopen
#define sprintf_s(_DstBuf, _SizeInBytes, ...) sprintf((_DstBuf), __VA_ARGS__)
#define strcpy_s(_dst, _size, _src) strcpy((_dst), (_src))

#endif

