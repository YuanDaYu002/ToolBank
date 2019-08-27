
#include "readWav.h"
#include <stdio.h>
#include <stdlib.h>

void wd_reset(struct WavData *_wavData)
{
	_wavData->data = NULL;
	_wavData->size = 0;
	_wavData->avg_bytes_sec = 0;
	_wavData->bits_per_sample = 0;
	_wavData->channels = 0;
	_wavData->sample_rate = 0;
}

int readWave(const char *_fname, struct WavData *_wavData)
{
	FILE *fp;
	size_t readed;

	fp = fopen(_fname,"rb");
	if (fp)
	{
		char id[5];          
		unsigned long size;  
		short format_tag, channels, block_align, bits_per_sample;    
		unsigned long format_length, sample_rate, avg_bytes_sec, data_size; 

		readed = fread(id, sizeof(char), 4, fp); 
		id[4] = '\0';
		if (!strcmp(id, "RIFF"))
		{ 
			readed = fread(&size, sizeof(unsigned long), 1, fp); 
			readed = fread(id, sizeof(char), 4, fp);         
			id[4] = '\0';

			if (!strcmp(id,"WAVE"))
			{ 
				readed = fread(id, sizeof(char), 4, fp);     
				readed = fread(&format_length, sizeof(unsigned long),1,fp);
				readed = fread(&format_tag, sizeof(short), 1, fp); 
				readed = fread(&channels, sizeof(short),1,fp);    
				readed = fread(&sample_rate, sizeof(unsigned long), 1, fp);   

				readed = fread(&avg_bytes_sec, sizeof(unsigned long), 1, fp); 
				readed = fread(&block_align, sizeof(short), 1, fp);     
				readed = fread(&bits_per_sample, sizeof(short), 1, fp);       
				readed = fread(id, sizeof(char), 4, fp);                      
				readed = fread(&data_size, sizeof(unsigned long), 1, fp);     
				_wavData->size = data_size;
				_wavData->data = (char *) malloc(sizeof(char)*data_size); 
				_wavData->channels = channels;
				_wavData->avg_bytes_sec = avg_bytes_sec;
				_wavData->bits_per_sample = bits_per_sample;
				_wavData->sample_rate = sample_rate;
				readed = fread(_wavData->data, sizeof(char), data_size, fp);       

			}
			else
			{
				printf("Error: RIFF file but not a wave file\n");
			}
		}
		else
		{
			printf("Error: not a RIFF file\n");
		}
		fclose(fp);
		return 1;
	}
	return 0;
}

