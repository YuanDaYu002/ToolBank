#ifndef UTIL_H
#define UTIL_H
#include <string.h>
#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const char hexChars[16];

#ifndef min
#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))
#endif

#define myabs(x) ((x)>0?(x):-(x))

void asign(void *_pdes, const void *_psource, int _sizeofEle);

int searchImin(int *_array, int _size);
int searchFmin(int *_array, int _size);

long long __llmin(long long _x, long long _y);
long long __llmax(long long _x, long long _y);
void swapi(int _left, int _right);
void swapui(unsigned int _left, unsigned int _right);
void swapp(void * _left, void * _right);
unsigned long getTickCount2(void);
void mysleep(int _ms);
const int myround(const double x);

int fint_compare(const void * m, const void* n);
int ffloat_compare(const void * m, const void* n);
int fshort_compare(const void * m, const void* n);

int mybinarySearch_(const void * _Key, const void * _Base, 
	unsigned int _NumOfElements, unsigned int _SizeOfElements,
	int (*_PtFuncCompare)(const void *, const void *));

int topNIdx(const void *_elements, int _numOfElements, int _sizeOfElements, 
	int (*_fCompare)(const void *, const void *), int *_results, int _resultN);
int topN(const void *_elements, int _numOfElements, int _sizeOfElements, 
	int (*_fCompare)(const void *, const void *), void **_results, int _resultN);

unsigned short calc_crc16(int*, unsigned short);

int hexChar2Int(char _c);

int char64ToInt(char _c);

char int2Char64(int _hex);

void bitSet(char *_desBits, int _start, int _end, char _sourceBits);

char bitGet(char _sourceBits, int _start, int _end);

void bitsSet(char *_bits, int _bitsStart, int _bitsEnd, char _c);

char bitsGet(char *_bits, int _bitsStart, int _bitsEnd);

void generateWave(char *pFileName, char *data, int size, int _sampleRate);
void saveFile(char *_fileName, char *_data, int _size);

#if 1

typedef int TopNType;
struct TopNSearcher
{
	TopNType *datas;
	int *topNIdx;
	int dataCount, topNCount;
	int topNMinIdx;
};
struct TopNSearcher *tns_init(struct TopNSearcher *_this, int _dataCount, int _topNCount);
void tns_finalize(struct TopNSearcher *_this);
void tns_add(struct TopNSearcher *_this, int _dataIdx, TopNType _addValue);
void tns_set(struct TopNSearcher *_this, int _dataIdx, TopNType _value);
int *tns_topNIdx(struct TopNSearcher *_this);
TopNType *tns_data(struct TopNSearcher *_this);

typedef short RangeTopNType;
struct RangeTopNSearcher
{
	RangeTopNType *datas;
	int *topNIdx;
	int rangeCount, topNCount, currDataCount;
	bool needDelMem;
};
struct RangeTopNSearcher *rtns_init(struct RangeTopNSearcher *_this, int _rangeCount, int _topNCount);
struct RangeTopNSearcher *rtns_init2(struct RangeTopNSearcher *_this, RangeTopNType *_datas, int _rangeCount, int *_topNIdx, int _topNCount);
void rtns_finalize(struct RangeTopNSearcher *_this);
void rtns_add(struct RangeTopNSearcher *_this, RangeTopNType _value);
int *rtns_topNIdx(struct RangeTopNSearcher *_this);
int rtns_idx(struct RangeTopNSearcher *_this);
RangeTopNType rtns_data(struct RangeTopNSearcher *_this, int _idx);

#endif

#ifdef __cplusplus
}
#endif

#endif

