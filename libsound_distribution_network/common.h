#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

	#define MYVERSION 104
	#define FREQ_ANALYSE_TIME_MATCH2

	#define FREQ_DISTANCE 150

	static const int DEFAULT_START_TOKEN = 0;
	static const int DEFAULT_STOP_TOKEN = 0;
	static const int DEFAULT_HEADER_TOKEN = 9;
	static const int DEFAULT_OVERLAP_TOKEN = 17; 
	static const int DEFAULT_OVERLAP2_TOKEN = 18; 
	static const int DEFAULT_BUFFER_SIZE = 4096;
	static const int DEFAULT_OVERLAP_SIZE = 768 * 4;
	static const int DEFAULT_BUFFER_COUNT = 4;
	static const int DEFAULT_SAMPLE_RATE = 44100;
	#define DEFAULT_FFT_SIZE 1024

	const static int DEFAULT_signalDuration = 65;
	const static int DEFAULT_blockDuration = 65;

	static const int DEFAULT_ECCodeLen = 5;

	extern const char DEFAULT_CODE_BOOK[16];
	#define VOICE_CODE_MIN 0
	#define VOICE_CODE_MAX 15

	extern int *DEFAULT_CODE_FREQUENCY;
	extern int *DEFAULT_CODE_FREQUENCY2;
	#define DEFAULT_CODE_FREQUENCY_SIZE 19
	extern int DEFAULT_CODE_BOOK_SIZE;
	extern int DEFAULT_minFrequency;
	extern int DEFAULT_maxFrequency;

	#define VOICE_BLOCK_ALIGN
	#define VOICE_BLOCK_SIZE 15
	#define RS_CORRECT
	#define RS_CORRECT_BLOCK_SIZE 13
	#define RS_CORRECT_SIZE 2

	#define MULTI_TONE 4

	#define MUTE_START_MS 10
	#define MUTE_END_MS 100

#ifdef __cplusplus
}
#endif

#endif

