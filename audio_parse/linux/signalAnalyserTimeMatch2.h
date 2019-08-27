#ifndef SIGNAL_ANALYSER_TIME_MATCH_H
#define SIGNAL_ANALYSER_TIME_MATCH_H

#include "common.h"
#ifdef FREQ_ANALYSE_TIME_MATCH2
#include "base.h"
#include "my_vector.h"
#include "my_poolAlloc.h"

typedef int tidx;
#define MAX_TIME_IDX MAX_INT
#define MAX_EVENT_COUNT 768
#define MAX_FI_MATCH_COUNT 4

#define SEARCH_SIMILAR_SIGNAL

struct FrequencyInfoSearcher;

struct FreqAmplitude
{
	float amplitude;
	short frequencyY;
	char peakType; 
	char order; 
};

struct FreqAmplitude *fa_init(struct FreqAmplitude *_this, int frequencyY, float amplitude);
struct FreqAmplitude *fa_init2(struct FreqAmplitude *_this, struct FreqAmplitude *_clone);

#define MAX_EVENT_AMP 3
struct EventInfo
{
	tidx timeIdx;
	union
	{
		struct FreqAmplitude points[MAX_EVENT_AMP];
		struct
		{
			struct FreqAmplitude p1;
			struct FreqAmplitude p2;
			struct FreqAmplitude p3;
			
		};
	};
};

#ifdef SEARCH_SIMILAR_SIGNAL
#define MAX_BLOCK_START_SIGNAL_MISS 2
#define MAX_RANGE_IDX_COUNT 2
struct TimeRangeSignal
{
	union
	{
		signed char idxes[MAX_RANGE_IDX_COUNT];
		struct
		{
			signed char idx1;
			signed char idx2;

		};
	};
	
	char flags;
};
inline void trs_reset(struct TimeRangeSignal *_this)
{
	memset(_this->idxes, -1, sizeof(_this->idxes));
	_this->flags = 0;
}
inline bool trs_maybeError(struct TimeRangeSignal *_this)
{
	return _this->flags;
}
inline void trs_setMaybeError(struct TimeRangeSignal *_this, bool _val)
{
	_this->flags = _val;
}

struct SignalBlock
{
	short startIdx;
	short signalCount;
};
#endif

struct EventInfo *ei_init2(struct EventInfo *_this, tidx timeIdx, struct FreqAmplitude *p1, struct FreqAmplitude *p2);
struct EventInfo *ei_init(struct EventInfo *_this);
struct RecognitionListener {
	void (*onStartRecognition)(struct RecognitionListener *_this, float _time);
	bool (*onStopRecognition)(struct RecognitionListener *_this, float _time, int _recogStatus, int *_indexs, int _count);
#ifdef SEARCH_SIMILAR_SIGNAL
	bool (*onStopRecognition2)(struct RecognitionListener *_this, float _time, int _recogStatus, struct TimeRangeSignal*_indexs, int _count, 
		struct SignalBlock *_blocks, int _blockCount);
#endif
	void (*onMatchFrequency)(struct RecognitionListener *_this, struct SignalAnalyser *_analyser, struct EventInfo *_event, 
	struct FrequencyInfoSearcher *_fiSearcher);
};

struct FrequencyMatch
{
	struct EventInfo *event;
	struct FreqAmplitude *frequency;	
};

#define maxSignalEventCount 7
#define maxFIEventCount maxSignalEventCount*2
struct FrequencyInfo
{
	short frequencyY;
	char preciseFreqCount;	
	char shortFreqCount; 
	char prePeakType;
	char preOrderType;	
	char realBigMatchCount;
	char bigMatchCount; 
	char orderMatchCount; 
	struct FrequencyMatch realTimes[maxFIEventCount];
	char realEventCount, timesStart, timesEnd;
	char maxAmplitudeIdx;
	char preDowning;

	char endReason;
	char blockSignalCount;
	tidx startTimeIdx;
	int idx;
};

struct FrequencyInfo *fi_init(struct FrequencyInfo *_this, int frequencyY, struct EventInfo *_event, struct FreqAmplitude *_freq);
struct FrequencyInfo *fi_init2(struct FrequencyInfo *_this, int frequencyY);

inline struct FrequencyMatch *fi_realTimes(struct FrequencyInfo *_this)
{
	return _this->realTimes;
}
inline int fi_realTimesCount(struct FrequencyInfo *_this)
{
	return _this->realEventCount;
}
inline struct FrequencyMatch *fi_realTimesFirst(struct FrequencyInfo *_this)
{
	assert(_this->realEventCount > 0);
	return _this->realTimes;
}
inline struct FrequencyMatch *fi_realTimesLast(struct FrequencyInfo *_this)
{
	return _this->realTimes + (_this->realEventCount - 1);
}
inline struct FrequencyMatch *fi_times(struct FrequencyInfo *_this)
{
	return _this->realTimes + _this->timesStart;
}
inline int fi_timesCount(struct FrequencyInfo *_this)
{
	return _this->timesEnd - _this->timesStart;
}
inline struct FrequencyMatch *fi_timesFirst(struct FrequencyInfo *_this)
{
	assert(_this->realEventCount > 0 && _this->timesStart >= 0 && _this->timesStart < _this->timesEnd && _this->timesStart < _this->realEventCount && _this->timesEnd <= _this->realEventCount);
	return _this->realTimes+_this->timesStart;
}
inline struct FrequencyMatch *fi_timesLast(struct FrequencyInfo *_this)
{
	assert(_this->realEventCount > 0 && _this->timesStart >= 0 && _this->timesStart < _this->timesEnd && _this->timesStart < _this->realEventCount && _this->timesEnd <= _this->realEventCount);
	return _this->realTimes+(_this->timesEnd-1);
}
inline void fi_timesPollFirst(struct FrequencyInfo *_this)
{
	assert(_this->timesStart < _this->timesEnd);
	_this->timesStart ++;
}
inline void fi_timesClear(struct FrequencyInfo *_this)
{
	_this->realEventCount = _this->timesStart = _this->timesEnd = 0;
}
tidx fi_howLongTime(struct FrequencyInfo *_this);
tidx fi_realHowLongTime(struct FrequencyInfo *_this);

struct SignalAnalyser;

bool fi_checkFreq(struct FrequencyInfo *_this, struct EventInfo *_event, int _minMatchFreqYDistance);

void fi_addTime2(struct FrequencyInfo *_this, struct EventInfo *_event, struct FreqAmplitude *_freq);
struct FrequencyInfo *fi_removeSmallMatch(struct FrequencyInfo *_this, int _maxMatchCount);
void fi_addTime(struct FrequencyInfo *_this, const struct FrequencyMatch *_fm);
void fi_InitializeInstanceFields(struct FrequencyInfo *_this);
int FrequencyInfo_compare( const void *p_data_left, const void *p_data_right );
float sumFIAmplitude(struct FrequencyInfo *_this);

struct Idx2EventInfo
{
	int size;
	tidx end;
	struct EventInfo *events;
};

struct Idx2EventInfo *iei_init(struct Idx2EventInfo *_this, int _size);
void iei_finalize(struct Idx2EventInfo *_this);

inline tidx iei_idx(struct Idx2EventInfo *_this)
{
	return _this->end;
}

inline void iei_add(struct Idx2EventInfo *_this, const struct EventInfo *_event)
{
	int idx = _this->end % _this->size;
	_this->events[idx] = *_event;
	_this->events[idx].timeIdx = ++_this->end;
}

inline struct EventInfo *iei_curr(struct Idx2EventInfo *_this)
{
	return _this->events + ((_this->end-1) % _this->size);
}

inline tidx iei_minIdx(struct Idx2EventInfo *_this)
{
	return _this->end - _this->size + 1;
}

inline tidx iei_currIdx(struct Idx2EventInfo *_this)
{
	return _this->end - 1;
}

inline struct EventInfo* iei_get(struct Idx2EventInfo *_this, tidx _idx)
{
	_idx = _idx - 1;
	if(_idx < _this->end && _idx >= (_this->end - _this->size))
	{
		return _this->events + (_idx % _this->size);
	}
	return NULL;
}

int FreqAmplitudeComparator(const void *_o1, const void *_o2);

struct FIMatch
{
	
	short headMatchCount;
	short bigMatchCount;
	short orderMatchCount;
	short tinyMatchCount;
	short topAmplitudeIdx;
	struct FrequencyMatch firstMatch;
	float sumAmplitude;
	float topAmplitude;
	float sumQuality;
};

struct FFTAmpsRange
{
	float *amplitudes;
	int fftSize;
	int minFreqIdx;
	int maxFreqIdx;
	struct FreqAmplitude *freqAmplitudes;
	char *freqAmplitudeBits;
};
void far_init(struct FFTAmpsRange *_this, int _fftSize, int _minFreqIdx, int _maxFreqIdx);
void far_setRange(struct FFTAmpsRange *_this, int _minFreqIdx, int _maxFreqIdx);
void far_finalize(struct FFTAmpsRange *_this);
void far_resetAmps(struct FFTAmpsRange *_this, float *_amplitudes);

#define LONG_TERM_AVG_AMPLITUDE 1000
struct FreqAmpsEventStat
{
	struct FreqAmplitude freqAmplitudes[MAX_EVENT_AMP];
	struct EventInfo event;
	float eventTotalAmplitude;
	float avgAmplitude;
	float longTermAvgAmplitude;
	int longTermAvgAmpCount;
	float amplitudeTop2Avg;
	struct FreqAmplitude *p1;
	struct SignalAnalyser *sa;
	bool hasValidAmplitude;
	
	char *freqY2FreqIdx;
	int freqY2FreqIdxSize;
};

void fats_init(struct FreqAmpsEventStat *_this, struct SignalAnalyser *_sa);
void fats_finalize(struct FreqAmpsEventStat *_this);
void fats_onFFTAmpsRange(struct FreqAmpsEventStat *_this, struct FFTAmpsRange *_amps, float _minPeak2Avg, float _minStatPeak2Avg, float _maxFreqYDistance);

struct FFTAmpsRangeTop2AvgIniter
{
	bool inited;
	int minNormalAmplitudeCount;
	int sumPeakAmplitudeTop2AvgCount;
	float sumPeakAmplitudeTop2Avg;
	float normalAmplitudeTop2Avg; 
	float minPeakAmplitudeTop2Avg; 
	float minRangePeakAmplitudeTop2Avg;
	float minLongRangePeakAmplitudeTop2Avg;
	float minWeakPeakAmplitudeTop2Avg;
	float rangAvgSNR;
	int rangeCurrAvgCount;
};

void fari_init(struct FFTAmpsRangeTop2AvgIniter *_this, int _minEventCount, float _initTop2Avg);

void fari_onFFTAmpsRange(struct FFTAmpsRangeTop2AvgIniter *_this, struct FFTAmpsRange *_amps, struct Idx2EventInfo *_idx2EventInfo, struct FreqAmpsEventStat *_eventStat);

struct FrequencyInfoSearcher
{
	struct myvector checkingFreq;
	struct myvector checkedFreqs;
	struct Idx2EventInfo *idx2Times;
	struct PoolAllocator *freqInfoAlloc;
};

void fis_init(struct FrequencyInfoSearcher *_this, struct Idx2EventInfo *_idx2Times, struct PoolAllocator *_freqInfoAlloc);
void fis_onFFTEvent(struct FrequencyInfoSearcher *_this, struct EventInfo *_ei, float _maxFreqYDistance);

void fis_truncateSignal(struct FrequencyInfoSearcher *_this, float _maxFreqYDistance);
void fis_finalize(struct FrequencyInfoSearcher *_this);

struct PeakInfo
{
	tidx timeIdx;
	int frequencyY;
};
struct PeakDetector
{
	struct SignalAnalyser *sa;
	int peakCount, preBlockPeakCount;
	int blockFreqY;
	tidx lastPosTimes[DEFAULT_CODE_FREQUENCY_SIZE];
	struct PeakInfo peakStartTimes[RS_CORRECT_BLOCK_SIZE+RS_CORRECT_SIZE+5];
	int nextPeakPos;
};
void pd_init(struct PeakDetector *_this, int _blockFreqY, struct SignalAnalyser *_sa);
void pd_reset2(struct PeakDetector *_this, int _blockFreqY);
void pd_onFFTEvent(struct PeakDetector *_this, struct EventInfo *_ei);
void pd_onBlock(struct PeakDetector *_this, struct FrequencyInfo *_blockFI);
void pd_reset(struct PeakDetector *_this);
int pd_blockPeakCount(struct PeakDetector *_this);
int pd_preBlockPeakCount(struct PeakDetector *_this);

#define BLOCKSEARCH_BLOCK_CACHE_COUNT 3
struct BlockSearcher
{
	struct Idx2EventInfo *idx2Times;
	struct FrequencyInfo blockFIs[BLOCKSEARCH_BLOCK_CACHE_COUNT];
	struct PeakDetector peakDetector;

	short blockFreqY;
	float maxFreqYDistance;
	signed char blockCount;
	signed char okBlockIdx;
	signed char unsedBlockIdx;
	bool allBlockFinished;
	void (*onFFTEvent)(struct BlockSearcher *_this, struct EventInfo *_ei);
};
void bs_init(struct BlockSearcher *_this, struct Idx2EventInfo *_idx2Times, short _blockFreqY, float _maxFreqYDistance, struct SignalAnalyser *_sa);
void bs_onFFTEvent(struct BlockSearcher *_this, struct EventInfo *_ei);

bool bs_hasBlock(struct BlockSearcher *_this, struct FrequencyInfo **_blockHead, struct FrequencyInfo **_blockTail, bool *_isEndBlock);
void bs_reset(struct BlockSearcher *_this);
void bs_reset2(struct BlockSearcher *_this, short _blockFreqY);

struct TimeRangeMatch
{	
	int peak;
	float sumAmplitude;
	float sumQuality;
	char bigMatchCount;
	char smallMatchCount;
	char tinyMatchCount;
	short rangeIdx;
	short preMatchIdx;
};

#define Recog_RecogCountZero 120
#define MAX_BLOCK_COUNT 10
typedef struct RecognitionListener * TRecognitionListener;
#define getHeight 1250

static const int Recog_Success = 0;
static const int Recog_TooMuchSignal = 99;
static const int Recog_NotEnoughSignal = 100;
static const int Recog_NotHeaderOrTail = 101;

struct SignalAnalyser
{
	TRecognitionListener listener;
	struct FFTAmpsRange ampsRange;
	struct FreqAmpsEventStat eventStat;
	struct FFTAmpsRangeTop2AvgIniter top2AvgIniter;
#ifdef MATCH_FI
	struct FrequencyInfoSearcher fiSearcher;
	struct PoolAllocator freqInfoAlloc;
#endif

	int fftSize;
	float sampleRate;
	int overlap;
	int pitchDistance;
	float maxFreqYDistance; 	
	int minFrequency;
	int maxFrequency;
	int header, tail;
	int *CODE_FREQUENCY;
	int *CODE_Y;
	char *freqY2IdxCaches;
	int freqY2IdxCacheCount;
	int freqY2IdxCacheMinIdx;

	struct FIMatch *freqRanges;
	float blockSignalAmp;
	struct BlockSearcher blockSearcher;
	struct myvector similarSignals;
	struct SignalBlock blocks[MAX_BLOCK_COUNT];
	int blockCount;
	float oneSignalLen;
	int preTargetIdx, prepreTargetIdx;
	int preTaretPeakType, prepreTaretPeakType;
	struct TimeRangeMatch preMatchs[MAX_FI_MATCH_COUNT], curMatchs[MAX_FI_MATCH_COUNT];
	struct Idx2EventInfo idx2Times;

	bool recoging;
	tidx preSignalTime; 
	tidx preValidFreqAmplitudeTime; 
	tidx firstValidFreqAmplitudeTime;
	tidx firstRangeValidFreqAmplitudeTime;
	tidx firstSignalTime;
	int minRegcoSignalCount;	
	bool signalOK;
	bool finished;
	bool newSignal;
	float rangAvgSNR;
	int rangeCurrAvgCount;
	tidx preValidRangeAvgSNRIdx;
	tidx preValidLongRangeAvgSNRIdx;

	int totalSignalCount;
	int okSignalCount;
	unsigned long preAnalyEndTime;
	unsigned long preAnalyStartTime;
	unsigned long preAnalyTotalTime;
	unsigned long analyFreqTotalTime;
	unsigned long anyseSignalTotalTime;
	unsigned long analyseFreqTop3TotalTime;
	unsigned long analyseFreqValidTotalTime;
};

struct SignalAnalyser *sa_init(struct SignalAnalyser *_this, int _sampleRate, int _channel, int _bits, int _fftSize, int _overlap);
void sa_finalize(struct SignalAnalyser *_this);

inline void sa_setMinRegcoSignalCount(struct SignalAnalyser *_this, int minRegcoSignalCount)
{
	_this->minRegcoSignalCount = minRegcoSignalCount;
}

void sa_analyFFT(struct SignalAnalyser *_this, float amplitudes[]);
void sa_setFreqs(struct SignalAnalyser *_this, int* _freqs);

inline int sa_getMinFreqIdx(struct SignalAnalyser *_this)
{
	return _this->ampsRange.minFreqIdx;
}

inline int sa_getMaxFreqIdx(struct SignalAnalyser *_this)
{
	return _this->ampsRange.maxFreqIdx;
}

inline TRecognitionListener sa_getListener(struct SignalAnalyser *_this)
{
	return _this->listener;
}

inline void sa_setListener(struct SignalAnalyser *_this, TRecognitionListener listener)
{
	_this->listener = listener;
}

#endif
#endif
