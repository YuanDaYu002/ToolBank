#include <assert.h>
#include <stdio.h>
#include "util.h"
#include "common.h"
#include "memory.h"

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#endif

#ifdef NO_INLINE
#define inline
#endif

const char hexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

unsigned long prePerforceTime = 0;
unsigned long currPerforceTime = 0;

inline void asign(void *_pdes, const void *_psource, int _sizeofEle)
{
	if (_pdes == NULL)return;
	if (_sizeofEle == sizeof(void *))
	{
		if (_psource == NULL)
			*((void **)_pdes) = NULL;
		else
		{
			*((void **)_pdes) = *((void **)_psource);
			
#ifdef WIN32
			assert(memcmp(_pdes, _psource, _sizeofEle) == 0);
#endif
		}
	}
	else
	{
		memcpy(_pdes, _psource, _sizeofEle);
	}
}

inline long long __llmin(long long _x, long long _y)
{
	long long x = _x, y = _y;
	return (x < y)?x:y;
}

inline long long __llmax(long long _x, long long _y)
{
	long long x = _x, y = _y;
	return (x > y)?x:y;
}

inline void swapi(int _left, int _right)
{
	int tmp = _left;
	_left = _right;
	_right = tmp;
}

inline void swapui(unsigned int _left, unsigned int _right)
{
	unsigned int tmp = _left;
	_left = _right;
	_right = tmp;
}

inline void swapp(void * _left, void * _right)
{
	void * tmp = _left;
	_left = _right;
	_right = tmp;
}
inline const int myround(const double x) {
	return ((int)(x + 0.5));
}

int searchImin(int *_array, int _size)
{
	int minpos = -1;
	while (--_size>=0)
	{
		if(minpos < 0 || _array[_size] < _array[minpos])minpos = _size;
	}
	return minpos;
}

int searchFmin(int *_array, int _size)
{
	int minpos = -1;
	while (--_size>=0)
	{
		if(minpos < 0 || _array[_size] < _array[minpos])minpos = _size;
	}
	return minpos;
}

unsigned long getTickCount2(void)
{
	unsigned long currentTime;
#ifdef WIN32
	currentTime = GetTickCount();
#else
	struct timeval current;
	gettimeofday(&current, NULL);
	currentTime = ((unsigned long)current.tv_sec) * 1000 + current.tv_usec/1000;
#endif
	return currentTime;
}

void mysleep(int _ms)
{
#ifdef WIN32
	Sleep(_ms);
#else
	usleep(_ms * 1000);
#endif
}

int fint_compare(const void * m, const void* n){  
	return *(int *)m - *(int *)n; 
}  

int ffloat_compare(const void * m, const void* n)
{
	float r = *(float *)m - *(float *)n; 
	return (r > 0.00001f)?1:((r < -0.00001f)?-1:0);
}

int fshort_compare(const void * m, const void* n)
{
	return *(short *)m - *(short *)n; 
}

int mybinarySearch_(const void * _Key, const void * _Base, 
	unsigned int _NumOfElements, unsigned int _SizeOfElements,
	int (*_PtFuncCompare)(const void *, const void *))
{
	int lo = 0;
	int hi = _NumOfElements - 1;

	while (lo <= hi) {
		int mid = (lo + hi) >> 1;
		int compareVal = _PtFuncCompare(_Key, ((char *)_Base + _SizeOfElements * mid));

		if (compareVal > 0) {
			lo = mid + 1;
		} else if (compareVal < 0) {
			hi = mid - 1;
		} else {
			return mid;  
		}
	}
	return ~lo;  
}

int topNIdx(const void *_elements, int _numOfElements, int _sizeOfElements, 
	int (*_fCompare)(const void *, const void *), int *_results, int _resultN)
{
	int resultCount = 0;
	int i, j;
	char *element = NULL;
	for (i = 0; i < _numOfElements; i ++)
	{
		element = (char *)_elements + _sizeOfElements * i;
		for (j = resultCount - 1; j >= 0; j --)
		{
			if (_fCompare(element, (char *)_elements+_sizeOfElements*_results[j]) > 0)
			{
				if (j+1 < _resultN)
				{
					_results[j+1] = _results[j];
				}
			}
			else
				break;
		}
		
		if(j+1 < _resultN)
		{
			_results[j+1] = i;
			if (resultCount < _resultN)
			{
				resultCount ++;
			}
		}
	}

	return resultCount;
}

int topN(const void *_elements, int _numOfElements, int _sizeOfElements, 
	int (*_fCompare)(const void *, const void *), void **_results, int _resultN)
{

	int *results = (int *)_results;
	int r = topNIdx(_elements, _numOfElements, _sizeOfElements, _fCompare, results, _resultN);
	
	int i;
	for (i = 0; i < r; i ++)
	{
		_results[i] = (char *)_elements + _sizeOfElements*results[i];
	}
	return r;
}

static unsigned short      crctab[] =    
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0

};

unsigned short calc_crc16(int *buf, unsigned short len)
{
    unsigned short crc = 0xFFFF;
    unsigned short i;

    for (i=0; i<len; i++) {
        crc = ((crc << 8) & 0xFFFF) ^ crctab[(crc >> 8) ^ (buf[i] & 0xFF)];
    }

    return crc;
}

int hexChar2Int(char _c)
{
	if(_c >= '0' && _c <= '9')
	{
		return _c - '0';
	}
	else if(_c >= 'a' && _c <= 'z')
	{
		return _c - 'a' + 10;
	}
	else if(_c >= 'A' && _c <= 'Z')
	{
		return _c - 'A' + 10;
	}
	return -1;
}

int char64ToInt(char _c)
{
	if (_c >= 'A' && _c <= 'Z')
	{
		return _c - 'A';
	}
	else if (_c >= 'a' && _c <= 'z')
	{
		return _c - 'a' + 26;
	}
	else if(_c >= '0' && _c <= '9')
	{
		return _c - '0' + 52;
	}
	else if(_c == '_')
	{
		return 62;
	}
	else if(_c == '-')
	{
		return 63;
	}
	return -1;
}

char int2Char64(int _hex)
{
	if (_hex >= 0 && _hex < 26)
	{
		return 'A' + _hex;
	}
	else if (_hex >= 26 && _hex < 52)
	{
		return 'a' + _hex - 26;
	}
	else if (_hex >= 52 && _hex < 62)
	{
		return '0' + _hex - 52;
	}
	else if(_hex == 62)
	{
		return '_';
	}
	else if(_hex == 63)
	{
		return '-';
	}
	return -1;
}

inline void bitSet(char *_desBits, int _start, int _end, char _sourceBits)
{
	char x = (_start > 0?(0xff >> _start):0xff) & (_end < 8?(0xff << (8 - _end)):0xff);
	*_desBits = (*_desBits & ~x) | ((_sourceBits << (8 - _end)) & x);
}

inline char bitGet(char _sourceBits, int _start, int _end)
{
	return (_end < 8?(_sourceBits >> (8 - _end)):_sourceBits) & (0xff >> (8 - (_end - _start)));
}

void bitsSet(char *_bits, int _bitsStart, int _bitsEnd, char _c)
{
	int byteStart, byteEnd;
	int startPos, endPos;
	char startSource, endSource;
	if ((byteStart = _bitsStart/8) == (byteEnd = _bitsEnd/8))
	{
		bitSet(_bits + byteStart, _bitsStart%8, _bitsEnd%8, _c);
	}
	else
	{
		assert(byteEnd == byteStart + 1);
		startPos = _bitsStart%8;
		endPos = _bitsEnd%8;
		startSource = _c >> endPos;
		endSource = _c & (0xff >> (8 - endPos));
		bitSet(_bits+byteStart, startPos, 8, startSource);
		bitSet(_bits+byteEnd, 0, endPos, endSource);
	}
}

char bitsGet(char *_bits, int _bitsStart, int _bitsEnd)
{
	int byteStart, byteEnd;
	int startPos, endPos;
	char startSource, endSource;
	if ((byteStart = _bitsStart/8) == (byteEnd = _bitsEnd/8))
	{
		return bitGet(_bits[byteStart], _bitsStart%8, _bitsEnd%8);
	}
	else
	{
		assert(byteEnd == byteStart + 1);
		startPos = _bitsStart%8;
		endPos = _bitsEnd%8;
		startSource = bitGet(_bits[byteStart], startPos, 8);
		endSource = bitGet(_bits[byteEnd], 0, endPos);
		return startSource << endPos | endSource;
	}
}

typedef unsigned int UInt32;
typedef unsigned short UInt16;

#define WAVE_FORMAT_PCM     1
#define PCM_CHANNEL 1
#define PCM_BITPERSEC 16
#define PCM_SAMPLE DEFAULT_SAMPLE_RATE

typedef struct tWAVEFORMATEXX
{
    UInt16        wFormatTag;         
    UInt16        nChannels;          
    UInt32       nSamplesPerSec;     
    UInt32       nAvgBytesPerSec;    
    UInt16        nBlockAlign;        
    UInt16        wBitsPerSample;     
    UInt16        cbSize;             
				    
} WAVEFORMATEXX, *PWAVEFORMATEXX;

void generateWave(char *pFileName, char *data, int size, int _sampleRate)
{
	FILE *pFile = fopen(pFileName, "wb");
	char waveHeader[64];
	WAVEFORMATEXX fmt;
	if (NULL == pFile)
		return;
	memset(waveHeader, 0, sizeof(waveHeader));
	strncpy(waveHeader, "RIFF", 4);
	*((int *)&waveHeader[4]) = 36 + size;
	strncpy(&waveHeader[8], "WAVE", 4);
	strncpy(&waveHeader[12], "fmt ", 4);
	*((int *)&waveHeader[16]) = 16;
	fmt.wFormatTag = WAVE_FORMAT_PCM;
	fmt.nChannels = PCM_CHANNEL;
	fmt.nSamplesPerSec = _sampleRate;
	fmt.wBitsPerSample = PCM_BITPERSEC;
	fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
	fmt.nAvgBytesPerSec = fmt.nBlockAlign * fmt.nSamplesPerSec;
	memcpy(&waveHeader[20], &fmt, 16);
	strncpy(&waveHeader[36], "data", 4);
	*((int *)&waveHeader[40]) = size;
	fwrite(waveHeader, 1, 44, pFile);
	fwrite(data, 1, size, pFile);
	fclose(pFile);
}

void saveFile(char *_fileName, char *_data, int _size)
{
	FILE *pFile = fopen(_fileName, "wb");
	fwrite(_data, 1, _size, pFile);
	fclose(pFile);
}

