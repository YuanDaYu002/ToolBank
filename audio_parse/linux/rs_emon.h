

#include "common.h"

typedef unsigned char dtype;
#define MM 4
#define KK RS_CORRECT_BLOCK_SIZE
#define NN 15

int             init_rs();

void            generate_gf(void);	
void            gen_poly(void);	

int             encode_rs(dtype data[], dtype bb[]);
int             encode_rs8_step(dtype data[], dtype bb[], int step);	
int             encode_rs4_step(dtype data[], dtype bb[], int step);	

#if 1
int             eras_dec_rs(dtype data[], int eras_pos[], int no_eras);
#endif
int             eras_dec_rs8_step(dtype data[], int step, int eras_pos[], int no_eras);
int             eras_dec_rs4_step(dtype data[], int step, int eras_pos[], int no_eras);

