/*
 * Bitstream operations header file
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

typedef struct PutBitContext {
    unsigned int bit_pos;
    unsigned char *buf;
    int size;
} PutBitContext;

static inline void init_put_bits(PutBitContext *s, unsigned char *buffer, int buffer_size)
{
    s->size 	= buffer_size;
    s->buf 		= buffer;
    s->bit_pos 	= 0;
    memset(s->buf, 0, s->size);
}

static inline int put_bits_count(PutBitContext *s)
{
    return (s->bit_pos);
}

static inline void flush_put_bits(PutBitContext *s)
{

}

static inline void put_bit(PutBitContext *s, unsigned int value){
	int bitpos   = s->bit_pos & 0x07;
	int byte_pos = s->bit_pos >> 3;
	s->buf[byte_pos] |= ((value & 0x01) << (7 - bitpos));
	++s->bit_pos;
}

static inline void put_bits(PutBitContext *s, unsigned int n, unsigned int value)
{
	int i;
	for(i = 0; i < n; ++i){
		put_bit(s, (value >> (n - i - 1)) & 1);
	}
}

static inline void put_sbits(PutBitContext *pb, int n, int value)
{

    put_bits(pb, n, value & ((1<<n)-1));
}

static inline void put_bits64(PutBitContext *s, int n, unsigned long long value)
{
	int i;
	for(i = 0; i < n; ++i){
		put_bit(s, (value >> (n - i - 1)) & 1);
	}
}
