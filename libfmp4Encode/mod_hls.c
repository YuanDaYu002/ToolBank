/*
 * main file for HLS apache module
 * Copyright (c) 2012-2013 Voicebase Inc.
 *
 * Author: Alexander Ustinov
 * email: alexander@voicebase.com
 *
 * This file is the part of the mod_hls apache module
 *
 * This program is free software, distributed under the terms of
 * the GNU Lesser General Public License version 3. See the LICENSE file
 * at the top of the source tree.
 *
 */

//#include "httpd.h"
//#include "http_config.h"
//#include "http_core.h"
//#include "http_log.h"
//#include "http_main.h"
//#include "http_protocol.h"
//#include "http_request.h"
//#include "util_script.h"
//#include "http_connection.h"
//#include "util_script.h"

/*
#ifdef TWOLAME
#include "twolame.h"
#else
#include "lame.h"
#endif
*/

//#include "apr_strings.h"
#include "hls_file.h"
#include "hls_media.h"
#include "hls_mux.h"
#include "mod_conf.h"

#include <stdio.h>
#include <sys/time.h>
#include <string.h>

double get_clock(void){
	struct timeval ts;
	gettimeofday(&ts, NULL);
	return ts.tv_sec + (double)(ts.tv_usec) / 1000000.0;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Data declarations.                                                       */
/*                                                                          */
/* Here are the static cells and structure declarations private to our      */
/* module.                                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                                                                          */
/* These routines are strictly internal to this module, and support its     */
/* operation.  They are not referenced by any external portion of the       */
/* server.                                                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/


static void remove_last_extension(char* filename){
    int len = strlen(filename);
    int pos = len - 1;
    while (pos >= 0 && filename[pos]!='.')
    	--pos;
    filename[pos] = 0;
}

int is_letter(char c){
	if (c >= '0' && c <= '9')
		return 1;
	if (c >= 'a' && c <= 'z')
		return 1;
	if (c >= 'A' && c <= 'Z')
		return 1;
	return 0;
}

char int_to_hex(int v){
	switch(v){
		case 0:
			return '0';
		case 1:
			return '1';
		case 2:
			return '2';
		case 3:
			return '3';
		case 4:
			return '4';
		case 5:
			return '5';
		case 6:
			return '6';
		case 7:
			return '7';
		case 8:
			return '8';
		case 9:
			return '9';
		case 10:
			return 'A';
		case 11:
			return 'B';
		case 12:
			return 'C';
		case 13:
			return 'D';
		case 14:
			return 'E';
		case 15:
			return 'F';

	}
	return '0';
}

void convert_to_hex(char* res, unsigned char c){
	int c1 = c >> 4;
	int c2 = (c & 0xF);

	res[0] = '%';
	res[1] = int_to_hex(c1);
	res[2] = int_to_hex(c2);

}



static int get_real_filename(char* filename){ //return segment number
//the filename ends at _%d.ts
    int len = strlen(filename);
    int pos = len - 1;
    int segment = 0;
    while (pos >= 0 && filename[pos]!='_')
    	--pos;
    filename[pos] = 0;

    sscanf(&filename[pos+1], "%d.ts", &segment);

    return segment;
}



int hex_to_int(char c){
	switch (c){
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;

		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;

		case '8': return 8;
		case '9': return 9;
		case 'a':
		case 'A': return 10;

		case 'b':
		case 'B': return 11;


		case 'c':
		case 'C': return 12;

		case 'd':
		case 'D': return 13;

		case 'e':
		case 'E': return 14;

		case 'f':
		case 'F': return 15;

	}
	return 0;
}

char convert_str_to_char(char c1, char c2){
	return (hex_to_int(c1) << 4) | hex_to_int(c2);
}












