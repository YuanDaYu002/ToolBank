#ifndef READ_WAVE_H
#define READ_WAVE_H

struct WavData
{
	short channels, bits_per_sample;    
	unsigned long sample_rate, avg_bytes_sec; 
	char *data;
	unsigned long size;
};

void wd_reset(struct WavData *_wavData);
int readWave(const char *_fname, struct WavData *_wavData);

#endif