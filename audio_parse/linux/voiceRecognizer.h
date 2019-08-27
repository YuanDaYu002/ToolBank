#ifndef VOICE_RECOGNIZER_H
#define VOICE_RECOGNIZER_H

#include "base.h"
#include "common.h"
#include "thread.h"
#include "buffer.h"
#ifndef SUB_BAND_ANALYSER
#include "signalAnalyser.h"
#endif
#include "kiss_fftr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef float ffttype;
struct kiss_fft_out
{
	kiss_fft_cpx *out;
	int outSize;
};
struct kiss_fft_out *fft_out_init(struct kiss_fft_out *_this, kiss_fft_cpx *_data, int _size);
int fft_out_size(struct kiss_fft_out *_this);
ffttype fft_out_r(struct kiss_fft_out *_this, int _idx);
ffttype fft_out_i(struct kiss_fft_out *_this, int _idx);

struct kissFFT
{
	int fftSize;
	kiss_fftr_cfg cfg;
	kiss_fft_cpx *out;
};

struct kissFFT *kiss_fft_init(struct kissFFT *_this, int _fftSize);
void kiss_fft_finalize(struct kissFFT *_this);
struct kiss_fft_out kiss_fft_execute(struct kissFFT *_this, ffttype *_in);

const static int Status_Success = 0;
const static int Status_ECCError = -2;
const static int Status_WiFiCodeError = -3;

static const int Status_TooMuchSignal = 99;
const static int Status_NotEnoughSignal = 100;
const static int Status_NotHeaderOrTail = 101;
const static int Status_RecogCountZero = 102;

#ifdef FFT_AMPLITUDE_ANALYSER

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

#define Recog_RecogCountZero 120
static const int Recog_Success = 0;
static const int Recog_TooMuchSignal = 99;
static const int Recog_NotEnoughSignal = 100;
static const int Recog_NotHeaderOrTail = 101;

#endif

struct MyRecognitionListener
{
	struct RecognitionListener parent;
	bool (*onAfterECC)(struct MyRecognitionListener *_this, float _soundTime, int _recogStatus, int *_indexs, int _count);
};

struct MyRecognitionListener *mrl_init(struct MyRecognitionListener *_this);
void mrl_onStartRecognition(struct RecognitionListener *_this, float _soundTime);
bool mrl_onStopRecognition(struct RecognitionListener *_this, float _soundTime, int _recogStatus, int *_indexs, int _count);
#ifdef SEARCH_SIMILAR_SIGNAL
bool mrl_onStopRecognition2(struct RecognitionListener *this_, float _soundTime, int _recogStatus, struct TimeRangeSignal* _indexs, int _count
	, struct SignalBlock *_blocks, int _blockCount);
#endif
bool mrl_decode(struct MyRecognitionListener *_this, int *indexs, int signalLen);

typedef long long TPos;
struct CircleBuffer
{
	TPos pos;
	int bufSize;
	char *buf;
};

struct CircleBuffer *cb_init(struct CircleBuffer *_this, int _bufSize);
void cb_finalize(struct CircleBuffer *_this);

TPos cb_write(struct CircleBuffer *_this, char *_buf, int _bufLen);

int cb_read(struct CircleBuffer *_this, TPos _startPos, char *_buf, int _bufSize);
TPos cb_pos(struct CircleBuffer *_this);
TPos cb_size(struct CircleBuffer *_this);

enum WriteType{WriteTypeNone, WriteTypeShort, WriteTypeFloat};
struct BufferDataWriter
{
	int floatOverlap, floatStepSize;
	int byteOverlap, byteStepSize;
	int bufferSize, floatBufferSize, overlap, currBufferFilled;
	bool firstBuffer;
	bool fillFloatBuffer;
	struct BufferSource *bufferSource;
	struct BufferData *currBuffer;
	char * buffer;
	float * floatBuffer;
	struct AudioConverter *converter;	
	enum WriteType writeType;
};

struct BufferDataWriter *bdw_init(struct BufferDataWriter *_this, struct BufferSource *_bufferSource, int _channel, int _bits, int _bufferSize, int _overlap, bool _fillFloatBuffer);
void bdw_finalize(struct BufferDataWriter *_this);

int bdw_write(struct BufferDataWriter *_this, char *_data, int _dataLen);
void bdw_flush(struct BufferDataWriter *_this);

struct VoiceEvent
{
	struct BufferData *data;
	TPos eventStartPos;
	int byteStepSize;
	bool end;
};

void vevent_reset(struct VoiceEvent *_this, struct BufferData *_data);

struct VoiceProcessor
{
	void (*process)(struct VoiceProcessor *_this, struct VoiceEvent *_event);
	void (*setFreqs)(struct VoiceProcessor *_this, int _freqsIdx, int* _freqs);
	void (*setListener)(struct VoiceProcessor *_this, int _freqsIdx, struct RecognitionListener *_listener);
	struct RecognitionListener *(*getListener)(struct VoiceProcessor *_this, int _freqsIdx);
};

struct FFTAmplitudeAnalyser
{
	void (*analyFFT)(struct FFTAmplitudeAnalyser *_this, float *_amplitudes, int _ampsLen);
	int (*getMinFreqIdx)(struct FFTAmplitudeAnalyser *_this);
	int (*getMaxFreqIdx)(struct FFTAmplitudeAnalyser *_this);
	void (*setListener)(struct FFTAmplitudeAnalyser *_this, struct RecognitionListener *_listener);
	struct RecognitionListener *(*getListener)(struct FFTAmplitudeAnalyser *_this);
	void (*finalize)(struct FFTAmplitudeAnalyser *_this);
	bool signalOK;
};

struct SignalAnalyserProcessIndicator
{
	bool isAnalyserProcess;
};

struct SignalAnalyserVoiceProcessor
{
	struct VoiceProcessor parent;
	struct SignalAnalyserProcessIndicator *(*getAnalyserIndicator)(struct SignalAnalyserVoiceProcessor *_this, int _idx);
};

#define FreqAnalyserCount 2
typedef struct kissFFT tfft;
#ifdef FFT_AMPLITUDE_ANALYSER
typedef struct FFTAmplitudeAnalyser TFFTSignalAnalyser;
#else
typedef struct SignalAnalyser TFFTSignalAnalyser;
#endif
struct FFTVoiceProcessor
{
	struct SignalAnalyserVoiceProcessor parent;
	int FFTSize;
	int sampleRate;
	short channel, bits;
	int analyserCount;
	TFFTSignalAnalyser *analysers[FreqAnalyserCount];
	struct SignalAnalyserProcessIndicator analyserIndicators[FreqAnalyserCount];
	tfft fft;
	float *outAmptitudes;
	float *floatBuffer;
	int floatOverlap, floatStepSize, byteOverlap, byteStepSize;
	bool firstBuffer;
	struct AudioConverter *converter;
	struct FFTVoiceProcessorListener *listener;	
};

struct FFTVoiceProcessor *fvp_init(struct FFTVoiceProcessor *_this,
	int _sampleRate, int _channel, int _bits, int _bufferSize, int _overlap);
int fvp_addSignalAnalyser(struct FFTVoiceProcessor *_this, TFFTSignalAnalyser *_analyser);
void fvp_finalize(struct FFTVoiceProcessor *_this);
void fvp_process(struct VoiceProcessor *this_, struct VoiceEvent *_event);
void fvp_setFreqs(struct VoiceProcessor *this_, int _freqsIdx, int* _freqs);
void fvp_setListener(struct VoiceProcessor *this_, int _freqsIdx, struct RecognitionListener *_listener);
struct RecognitionListener *fvp_getListener(struct VoiceProcessor *this_, int _freqsIdx);
struct SignalAnalyserProcessIndicator *fvp_getAnalyserIndicator(struct SignalAnalyserVoiceProcessor *this_, int _idx);

struct MaybeSignal
{
	bool discoveryFinished, recognizeFinished;
	int signalIdx;
	int totalDetectCount, validDetectCount;
	TPos startPos, endPos;
	unsigned long startTime, endTime, recognizeStartTime;
	bool (*isDiscoveryFinished)(struct MaybeSignal *_this);
};

struct MaybeSignal *ms_init(struct MaybeSignal *_this);
void ms_reset(struct MaybeSignal *_this);
void ms_onDetect(struct MaybeSignal *_this, bool _hasSignal);
bool ms_isDiscoveryFinished(struct MaybeSignal *_this);

static int MaybeSignalQueue_globalSignalIdx;
#define MAX_QUEUE_NAME_LENGTH 64
struct MaybeSignalQueue
{
	struct CircularQueue signalQueue;
	struct MaybeSignal discardSignal;
	char name[MAX_QUEUE_NAME_LENGTH];	
	bool currDiscoveryDiscarded;
};

struct MaybeSignalQueue *msq_init(struct MaybeSignalQueue *_this);
void msq_finalize(struct MaybeSignalQueue *_this);
void msq_clear(struct MaybeSignalQueue *_this);

struct MaybeSignal *msq_startDiscoverySignal(struct MaybeSignalQueue *_this, TPos _startPos, TPos _endPos);
struct MaybeSignal *msq_currDiscoveryingSignal(struct MaybeSignalQueue *_this);
void msq_endDiscoverySignal(struct MaybeSignalQueue *_this);

struct MaybeSignal *msq_startRecognizeSignal(struct MaybeSignalQueue *_this);
void msq_endRecognizeSignal(struct MaybeSignalQueue *_this);

struct MultiMaybeSignalQueueWrapper
{
	struct MaybeSignal maybeSignal;
	int queueCount;
	TPos preEndPos, indicatePos;
	int signalIdx;
	bool inRecognizingSignal;
	bool recognizeSyned;
	mtx_t m;	
	struct MaybeSignal *recogningMaySignals[FreqAnalyserCount];
	struct MaybeSignalQueue *maybeSignalQueues[FreqAnalyserCount];
	struct SignalAnalyserProcessIndicator *analyserIndicators[FreqAnalyserCount];
};
struct MultiMaybeSignalQueueWrapper *mmsq_init(struct MultiMaybeSignalQueueWrapper *_this);
void mmsq_finalize(struct MultiMaybeSignalQueueWrapper *_this);
void mmsq_addQueue(struct MultiMaybeSignalQueueWrapper *_this, struct MaybeSignalQueue *_Queue, struct SignalAnalyserProcessIndicator *_analyserIndicator);
void mmsq_clear(struct MultiMaybeSignalQueueWrapper *this_);
bool mmsq_isDiscoveryFinished(struct MaybeSignal *_this);

void mmsq_indicate(struct MultiMaybeSignalQueueWrapper *_this, TPos _processDataStart, TPos _processDataEnd);
void mmsq_endAllDiscoveryingSignals(struct MultiMaybeSignalQueueWrapper *_this);

struct MaybeSignal *mmsq_startRecognizeSignal(struct MultiMaybeSignalQueueWrapper *_this);
void mmsq_endRecognizeSignal(struct MultiMaybeSignalQueueWrapper *_this);

void mmsq_synRecognizeSignal(struct MultiMaybeSignalQueueWrapper *_this);

enum DetectResult {DR_Ignore, DR_Yes, DR_No};
struct SignalDetector;
#define LONG_TERM_FSD_AVG_AMPLITUDE 120

struct FreqSignalDetector
{
	struct SignalDetector *parentDetector;
	int minFrequency, maxFrequency;
	int minFreqIdx, maxFreqIdx;
	int signalStartRelative;
	double minPeakAmplitudeTop2Avg;
	int minNormalAmplitudeCount, currNormalAmplitudeCount, currTempAmplitudeCount;
	float sumNormalAmplitude, sumTempAmplitude;
	float longTermAvgAmp;
	int longTermAvgCount;
	int signalIdx;
	struct MaybeSignalQueue maybeSignalQueue;
	enum DetectResult hasSignal_;
	bool normalAmplitudeInited;
	bool preHasSignal, prepreSignal, preprepreSignal, prepreprepreSignal;
	bool hasNewSignal;
};
struct FreqSignalDetector *fsd_init(struct FreqSignalDetector *_this, struct SignalDetector *_parentDetector, 
	int _sampleRate);
void fsd_finalize(struct FreqSignalDetector *_this);
void fsd_reset(struct FreqSignalDetector *_this);
void fsd_setFreqs(struct FreqSignalDetector *_this, int* _freqs);
void fsd_detect(struct FreqSignalDetector *_this, struct VoiceEvent *_event, struct kiss_fft_out *_fftOut, bool _ignore);
enum DetectResult fsd_hasSignal(struct FreqSignalDetector *_this);

#define SignalDetector_FFTSize_Default 256
const static int SignalDetector_DetectEventDistance = 8;
static int SignalDetector_FFTSize = SignalDetector_FFTSize_Default;
struct SignalDetector
{
	struct VoiceProcessor parent;	
	volatile bool hasNewSignal;
	TPos eventIdx;
	tfft fft;
	float outAmptitudes[SignalDetector_FFTSize_Default / 2 + 1];
	int sampleRate;
	int bufferSize, bufferDataSize;
	float *floatBuffer;
	struct AudioConverter *converter;
	
	bool normalAmplitudeInited;
	int detectorCount;
	struct SignalAnalyserVoiceProcessor *fvp;
	struct FreqSignalDetector *freqDetectors[FreqAnalyserCount];
	struct MultiMaybeSignalQueueWrapper multiMaybeSignalQueue;
};

struct SignalDetector *sd_init(struct SignalDetector *_this, struct SignalAnalyserVoiceProcessor *_fvp, int _sampleRate, int _channel, int _bits, int _bufferSize, int _overlap);
void sd_finalize(struct SignalDetector *_this);
void sd_reset(struct SignalDetector *_this);
void sd_process(struct VoiceProcessor *this_, struct VoiceEvent *_event);
void sd_setFreqs(struct VoiceProcessor *this_, int _freqsIdx, int* _freqs);
void sd_setListener(struct VoiceProcessor *_this, int _freqsIdx, struct RecognitionListener *_listener);
struct RecognitionListener *sd_getListener(struct VoiceProcessor *_this, int _freqsIdx);
void sd_addFreqSignalDetector(struct SignalDetector *_this, struct SignalAnalyserProcessIndicator *_analyserIndicator);

#define VD_LONGTERM_LONG 2 *60 * 8000
#define VD_LONGTERM_LONG_MIN 3 * 8000
#define VD_MIN_PEAK_LONG 1600
#define VD_MAX_PEAK_LONG 16000 * 8
#define VD_RECENTTERM_LONG 100
#define MIN_PEAK_RECENT_AVG_2_LONG_AVG 1.5
#define MIN_LONG_TERM_RATIO 2
#define MIN_PEAK_RECENT_AVG_2_LONG_AVG_WHEN_CHECK_DETAIL 1.5

struct VolumeDetector
{
	struct VoiceProcessor parent;
	
	long long longTermCount, recentTermCount, absCount;
	long long longTermSum, recentTermSum, absSum;
	long long eventIdx;
	int sampleRate;
	int sampleOverlap, sampleStepSize;
	int byteOverlap, byteStepSize;
	struct AudioConverter *converter;
	char isPrePeak, isInPeaks;
	long long prePeakSampleIdx;
	struct MaybeSignalQueue maybeSignalQueue;
	struct MultiMaybeSignalQueueWrapper multiMaybeSignalQueue;
	volatile bool hasNewSignal;
	struct MaybeSignal *currSignal;
	short *shortBuffer;
	float minRecentAvg2LongtermAvg;
};

struct VolumeDetector *vd_init(struct VolumeDetector *_this, int _sampleRate, int _channel, int _bits, 
	int _bufferSize, int _overlap, struct SignalAnalyserProcessIndicator *_analyserIndicator);
void vd_finalize(struct VolumeDetector *_this);
void vd_reset(struct VolumeDetector *_this);
void vd_process(struct VoiceProcessor *this_, struct VoiceEvent *_event);
void vd_setFreqs(struct VoiceProcessor *this_, int _freqsIdx, int* _freqs);
void vd_setListener(struct VoiceProcessor *_this, int _freqsIdx, struct RecognitionListener *_listener);
struct RecognitionListener *vd_getListener(struct VoiceProcessor *_this, int _freqsIdx);

struct PreprocessVoiceProcessor
{
	struct VoiceProcessor parent;
	int sampleRate, channel, bufferSize, bits, overlap;
	volatile bool stopped;
	struct MultiMaybeSignalQueueWrapper *maybeSignalQueue;
	struct SignalDetector signalDetector;
	struct SignalAnalyserVoiceProcessor *realProcessor;
	volatile bool realProcessing;
	mtx_t m;
	cnd_t cond;
	thrd_t realProcessThread;
	volatile bool realProcessNULL;
	volatile bool realProcessThreadQuited;
	struct CircleBuffer voiceHistory;
	struct RecognitionListener *listener;
	int floatOverlap, floatStepSize;
	int frameSize;
	int signalIdx;
	int byteOverlap, byteStepSize;
	bool firstBuffer;
	bool allowDiscardSignal;
};

struct PreprocessVoiceProcessor *pvp_init(struct PreprocessVoiceProcessor *_this, int _sampleRate, int _channel, int _bits, int _bufferSize, int _overlap);
void pvp_finalize(struct PreprocessVoiceProcessor *_this);
void pvp_process(struct VoiceProcessor *this_, struct VoiceEvent *_event);
void pvp_setFreqs(struct VoiceProcessor *this_, int _freqsIdx, int* _freqs);
void pvp_setListener(struct VoiceProcessor *this_, int _freqsIdx, struct RecognitionListener *_listener);
struct RecognitionListener *pvp_getListener(struct VoiceProcessor *this_, int _freqsIdx);
int pvp_realProcess(void *_p);
void pvp_signalStart(struct PreprocessVoiceProcessor *_this, TPos _startPos);
void pvp_signalEnd(struct PreprocessVoiceProcessor *_this, TPos _endPos);

enum ProcessorType
{
	FFTVoiceProcessorType = 1, PreprocessVoiceProcessorType = 2, CryDetectProcessorType = 3
};

struct VoiceRecognizer
{
	struct BufferSource parent;	
	struct RecognitionListener *listeners[FreqAnalyserCount];
	int freqsCount;
	struct Buffer buffer;
	struct BufferDataWriter writer;
	struct VoiceProcessor *voiceProcessor;
	enum ProcessorType processorType;
	volatile unsigned long signalContinueTime;
	volatile bool paused;
	bool stopped;
};

struct VoiceRecognizer *vrr_init(struct VoiceRecognizer *_this, enum ProcessorType _processorType, int _sampleRate, int _channel, int _bits, int _bufferSize, int _overlap);
void vrr_finalize(struct VoiceRecognizer *_this);

void vrr_setStopped(struct VoiceRecognizer *_this, bool _stopped);
struct BufferData *vrr_getBuffer(struct BufferSource *__this);
struct BufferData *vrr_tryGetBuffer(struct BufferSource *__this);
void vrr_freeBuffer(struct BufferSource *__this, struct BufferData *data);
struct BufferDataWriter *vrr_getBufferWriter(struct VoiceRecognizer *_this);
bool vrr_isStopped(struct VoiceRecognizer *_this);
void vrr_run(struct VoiceRecognizer *_this);
void vrr_pause(struct VoiceRecognizer *_this, int _microSeconds);
struct RecognitionListener* vrr_getListener(struct VoiceRecognizer *_this, int _idx);
void vrr_setListener(struct VoiceRecognizer *_this, int _idx, struct RecognitionListener* _listener);
void vrr_setFreqs(struct VoiceRecognizer *_this, int _idx, int* _freqs, int _freqCount);
int vrr_getFreqsCount(struct VoiceRecognizer *_this);

struct AudioConverter
{
	void (*toFloatArray)(char *in_buff, float* out_buff, int out_len);
	void (*toShort)(char *in_buff, short *out_buff, int out_len);
};

struct AudioConverter *getConverter(int _channel, int _bits, bool _signed, bool _bigEndian);

#ifdef __cplusplus
}
#endif

#endif
