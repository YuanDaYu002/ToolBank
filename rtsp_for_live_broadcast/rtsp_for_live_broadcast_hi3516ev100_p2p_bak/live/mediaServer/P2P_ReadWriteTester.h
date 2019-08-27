

#ifndef P2P_READ_WRITE_TESTER_H
#define P2P_READ_WRITE_TESTER_H

int living_stream_init(void);

int living_stream_satrt(void);

int living_stream_get_frame_data(void**data,unsigned int* len);

int living_stream_exit(void);



#endif


