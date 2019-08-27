
#include <stdio.h>
#include "rs_emon.h"

#define inline __inline

typedef int     gf;

#define MM_MAX 16
#define KK_MAX 254
#define NN_MAX 255

static int     *Pp;
static int      Pp_array[][MM_MAX + 1] = {
	{1, 1, 1},
	{1, 1, 0, 1},
	{1, 1, 0, 0, 1},
	{1, 0, 1, 0, 0, 1},
	{1, 1, 0, 0, 0, 0, 1},
	{1, 0, 0, 1, 0, 0, 0, 1},
	{1, 0, 1, 1, 1, 0, 0, 0, 1},
	{1, 0, 0, 0, 1, 0, 0, 0, 0, 1},
	{1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
	{1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1},
	{1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
	{1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
	{1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
{1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1}};

#define B0	1

gf              Alpha_to[NN_MAX + 1];

gf              Index_of[NN_MAX + 1];

static int      A0;

gf              Gg[NN_MAX - 1 + 1];

static inline   gf
modnn(int x)
{
	while (x >= NN) {
		x -= NN;
		x = (x >> MM) + (x & NN);
	}
	return x;
}

#define	min(a,b)	((a) < (b) ? (a) : (b))

#define	CLEAR(a,n) {\
	int ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = 0;\
	}

#define CLEAR_STEP(a,n,s) {\
	int ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[(ci)*(s)] = 0;\
	}

#if 1
#define	COPY(a,b,n) {\
	int ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = (b)[ci];\
	}
#define	COPYDOWN(a,b,n) {\
	int ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = (b)[ci];\
	}
#else
#define COPY(a,b,n){\
	memcpy((a), (b), (sizeof(a))*(n));\
}
#define COPYDOWN(a,b,n){\
	memcpy((a), (b), (sizeof(a))*(n));\
}
#endif

int 
init_rs()

{
	int mm = MM, kk = KK;
	int             nn = ((1 << mm) - 1);

	if (mm != 0 && kk >= nn) {
		fprintf(stderr, "KK must be less than 2**MM - 1\n");
		return -1;
	}
	if (mm < 2 || 16 < mm) {
		fprintf(stderr, "MM must be in range 2-16\n");
		return -1;
	}

	Pp = Pp_array[mm - 2];
	A0 = NN;
	generate_gf();
	gen_poly();
	return 0;
}

void
generate_gf(void)
{
	register int    i, mask;

	mask = 1;
	Alpha_to[MM] = 0;
	for (i = 0; i < MM; i++) {
		Alpha_to[i] = mask;
		Index_of[Alpha_to[i]] = i;
		
		if (Pp[i] != 0)
			Alpha_to[MM] ^= mask;	
		mask <<= 1;	
	}
	Index_of[Alpha_to[MM]] = MM;
	
	mask >>= 1;
	for (i = MM + 1; i < NN; i++) {
		if (Alpha_to[i - 1] >= mask)
			Alpha_to[i] = Alpha_to[MM] ^ ((Alpha_to[i - 1] ^ mask) << 1);
		else
			Alpha_to[i] = Alpha_to[i - 1] << 1;
		Index_of[Alpha_to[i]] = i;
	}
	Index_of[0] = A0;
	Alpha_to[NN] = 0;
}

void
gen_poly(void)
{
	register int    i, j;

	Gg[0] = Alpha_to[B0];
	Gg[1] = 1;		
	for (i = 2; i <= NN - KK; i++) {
		Gg[i] = 1;
		
		for (j = i - 1; j > 0; j--)
			if (Gg[j] != 0)
				Gg[j] = Gg[j - 1] ^ Alpha_to[modnn((Index_of[Gg[j]]) + B0 + i - 1)];
			else
				Gg[j] = Gg[j - 1];
		
		Gg[0] = Alpha_to[modnn((Index_of[Gg[0]]) + B0 + i - 1)];
	}
	
	for (i = 0; i <= NN - KK; i++)
		Gg[i] = Index_of[Gg[i]];
}

int
encode_rs(dtype data[], dtype bb[])
{
	register int    i, j;
	gf              feedback;

	CLEAR(bb, NN - KK);
	for (i = KK - 1; i >= 0; i--) {
#if (MM != 8)
		if (data[i] > NN)
			return -1;	
#endif
		feedback = Index_of[data[i] ^ bb[NN - KK - 1]];
		if (feedback != A0) {	
			for (j = NN - KK - 1; j > 0; j--)
				if (Gg[j] != A0)
					bb[j] = bb[j - 1] ^ Alpha_to[modnn(Gg[j] + feedback)];
				else
					bb[j] = bb[j - 1];
			bb[0] = Alpha_to[modnn(Gg[0] + feedback)];
		} else {	
			for (j = NN - KK - 1; j > 0; j--)
				bb[j] = bb[j - 1];
			bb[0] = 0;
		}
	}
	return 0;
}

int
encode_rs8_step(dtype data[], dtype bb[], int step)

{
	register int    i, j;
	gf              feedback;

	CLEAR_STEP(bb, NN - KK, step);
	for (i = KK - 1; i >= 0; i--) {
		feedback = Index_of[data[i * step] ^ bb[(NN - KK - 1) * step]];
		if (feedback != A0) {	
			for (j = NN - KK - 1; j > 0; j--)
				if (Gg[j] != A0)
					bb[j * step] =
						bb[(j - 1) * step]
						^ Alpha_to[modnn(Gg[j] + feedback)];
				else
					bb[j * step] = bb[(j - 1) * step];
			bb[0] = Alpha_to[modnn(Gg[0] + feedback)];
		} else {	
			for (j = NN - KK - 1; j > 0; j--)
				bb[j * step] = bb[(j - 1) * step];
			bb[0] = 0;
		}
	}
	return 0;
}

int
encode_rs4_step(dtype data[], dtype bb[], int step)
{
	register int    i, j;
	gf              feedback;
	dtype           data4;
	dtype           bb2[NN - KK];

	CLEAR(bb2, NN - KK);
	CLEAR_STEP(bb, NN - KK, step);

	for (i = KK - 1; i >= 0; i--) {
		data4 = data[i * step] & 0x0f;
		feedback = Index_of[data4 ^ bb[(NN - KK - 1) * step]];
		if (feedback != A0) {	
			for (j = NN - KK - 1; j > 0; j--)
				if (Gg[j] != A0)
					bb[j * step] = bb[(j - 1) * step]
						^ Alpha_to[modnn(Gg[j]
							       + feedback)];
				else
					bb[j * step] = bb[(j - 1) * step];
			bb[0] = Alpha_to[modnn(Gg[0] + feedback)];
		} else {	
			for (j = NN - KK - 1; j > 0; j--)
				bb[j * step] = bb[(j - 1) * step];
			bb[0] = 0;
		}
	}

	for (i = KK - 1; i >= 0; i--) {
		data4 = data[i * step] >> 4;
		feedback = Index_of[data4 ^ bb2[NN - KK - 1]];
		if (feedback != A0) {	
			for (j = NN - KK - 1; j > 0; j--)
				if (Gg[j] != A0)
					bb2[j] = bb2[(j - 1)]
						^ Alpha_to[modnn(Gg[j]
							       + feedback)];
				else
					bb2[j] = bb2[j - 1];
			bb2[0] = Alpha_to[modnn(Gg[0] + feedback)];
		} else {	
			for (j = NN - KK - 1; j > 0; j--)
				bb2[j] = bb2[j - 1];
			bb2[0] = 0;
		}
	}

	for (j = NN - KK - 1; j >= 0; j--) {
		bb[j * step] |= bb2[j] << 4;
	}
	return 0;
}

#if 1
int
eras_dec_rs(dtype data[], int eras_pos[], int no_eras)
{
	int             deg_lambda, el, deg_omega;
	int             i, j, r;
	gf              u, q, tmp, num1, num2, den, discr_r;
	gf              recd[NN];
	gf              lambda[NN - KK + 1], s[NN - KK + 1];	
	gf              b[NN - KK + 1], t[NN - KK + 1], omega[NN - KK + 1];
	gf              root[NN - KK], reg[NN - KK + 1], loc[NN - KK];
	int             syn_error, count;

	for (i = NN - 1; i >= 0; i--) {
#if (MM != 8)
		if (data[i] > NN)
			return -1;	
#endif
		recd[i] = Index_of[data[i]];
	}
	
	syn_error = 0;
	for (i = 1; i <= NN - KK; i++) {
		tmp = 0;
		for (j = 0; j < NN; j++)
			if (recd[j] != A0)	
				tmp ^= Alpha_to[modnn(recd[j] + (B0 + i - 1) * j)];
		syn_error |= tmp;	
		
		s[i] = Index_of[tmp];
	}
	if (!syn_error) {
		
		return 0;
	}
	CLEAR(&lambda[1], NN - KK);
	lambda[0] = 1;
	if (no_eras > 0) {
		
		lambda[1] = Alpha_to[eras_pos[0]];
		for (i = 1; i < no_eras; i++) {
			u = eras_pos[i];
			for (j = i + 1; j > 0; j--) {
				tmp = Index_of[lambda[j - 1]];
				if (tmp != A0)
					lambda[j] ^= Alpha_to[modnn(u + tmp)];
			}
		}
#ifdef ERASURE_DEBUG
		
		for (i = 1; i <= no_eras; i++)
			reg[i] = Index_of[lambda[i]];
		count = 0;
		for (i = 1; i <= NN; i++) {
			q = 1;
			for (j = 1; j <= no_eras; j++)
				if (reg[j] != A0) {
					reg[j] = modnn(reg[j] + j);
					q ^= Alpha_to[reg[j]];
				}
			if (!q) {
				
				root[count] = i;
				loc[count] = NN - i;
				count++;
			}
		}
		if (count != no_eras) {
			printf("\n lambda(x) is WRONG\n");
			return -1;
		}
#ifndef NO_PRINT
		printf("\n Erasure positions as determined by roots of Eras Loc Poly:\n");
		for (i = 0; i < count; i++)
			printf("%d ", loc[i]);
		printf("\n");
#endif
#endif
	}
	for (i = 0; i < NN - KK + 1; i++)
		b[i] = Index_of[lambda[i]];

	r = no_eras;
	el = no_eras;
	while (++r <= NN - KK) {
		
		discr_r = 0;
		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i] != A0)) {
				discr_r ^= Alpha_to[modnn(Index_of[lambda[i]] + s[r - i])];
			}
		}
		discr_r = Index_of[discr_r];	
		if (discr_r == A0) {
			
			COPYDOWN(&b[1], b, NN - KK);
			b[0] = A0;
		} else {
			
			t[0] = lambda[0];
			for (i = 0; i < NN - KK; i++) {
				if (b[i] != A0)
					t[i + 1] = lambda[i + 1] ^ Alpha_to[modnn(discr_r + b[i])];
				else
					t[i + 1] = lambda[i + 1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				
				for (i = 0; i <= NN - KK; i++)
					b[i] = (lambda[i] == 0) ? A0 : modnn(Index_of[lambda[i]] - discr_r + NN);
			} else {
				
				COPYDOWN(&b[1], b, NN - KK);
				b[0] = A0;
			}
			COPY(lambda, t, NN - KK + 1);
		}
	}

	deg_lambda = 0;
	for (i = 0; i < NN - KK + 1; i++) {
		lambda[i] = Index_of[lambda[i]];
		if (lambda[i] != A0)
			deg_lambda = i;
	}
	
	COPY(&reg[1], &lambda[1], NN - KK);
	count = 0;		
	for (i = 1; i <= NN; i++) {
		q = 1;
		for (j = deg_lambda; j > 0; j--)
			if (reg[j] != A0) {
				reg[j] = modnn(reg[j] + j);
				q ^= Alpha_to[reg[j]];
			}
		if (!q) {
			
			root[count] = i;
			loc[count] = NN - i;
			count++;
		}
	}

#ifdef DEBUG
#ifndef NO_PRINT
	printf("\n Final error positions:\t");
	for (i = 0; i < count; i++)
		printf("%d ", loc[i]);
	printf("\n");
#endif
#endif
	if (deg_lambda != count) {
		
		return -1;
	}
	
	deg_omega = 0;
	for (i = 0; i < NN - KK; i++) {
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for (; j >= 0; j--) {
			if ((s[i + 1 - j] != A0) && (lambda[j] != A0))
				tmp ^= Alpha_to[modnn(s[i + 1 - j] + lambda[j])];
		}
		if (tmp != 0)
			deg_omega = i;
		omega[i] = Index_of[tmp];
	}
	omega[NN - KK] = A0;

	for (j = count - 1; j >= 0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != A0)
				num1 ^= Alpha_to[modnn(omega[i] + i * root[j])];
		}
		num2 = Alpha_to[modnn(root[j] * (B0 - 1) + NN)];
		den = 0;

		for (i = min(deg_lambda, NN - KK - 1) & ~1; i >= 0; i -= 2) {
			if (lambda[i + 1] != A0)
				den ^= Alpha_to[modnn(lambda[i + 1] + i * root[j])];
		}
		if (den == 0) {
#ifdef DEBUG
			printf("\n ERROR: denominator = 0\n");
#endif
			return -1;
		}
		
		if (num1 != 0) {
			data[loc[j]] ^= Alpha_to[modnn(Index_of[num1] + Index_of[num2] + NN - Index_of[den])];
		}
	}
	return count;
}
#endif

int
eras_dec_rs8_step(dtype data[], int step, int eras_pos[], int no_eras)
{
	int             deg_lambda, el, deg_omega;
	int             i, j, r;
	gf              u, q, tmp, num1, num2, den, discr_r;
	gf              recd[NN];
	gf              lambda[NN - KK + 1], s[NN - KK + 1];	
	gf              b[NN - KK + 1], t[NN - KK + 1], omega[NN - KK + 1];
	gf              root[NN - KK], reg[NN - KK + 1], loc[NN - KK];
	int             syn_error, count;

	for (i = NN - 1; i >= 0; i--) {
		recd[i] = Index_of[data[i * step]];
	}
	
	syn_error = 0;
	for (i = 1; i <= NN - KK; i++) {
		tmp = 0;
		for (j = 0; j < NN; j++)
			if (recd[j] != A0)	
				tmp ^= Alpha_to[modnn(recd[j] + (B0 + i - 1) * j)];
		syn_error |= tmp;	
		
		s[i] = Index_of[tmp];
	}
	if (!syn_error) {
		
		return 0;
	}
	CLEAR(&lambda[1], NN - KK);
	lambda[0] = 1;
	if (no_eras > 0) {
		
		lambda[1] = Alpha_to[eras_pos[0]];
		for (i = 1; i < no_eras; i++) {
			u = eras_pos[i];
			for (j = i + 1; j > 0; j--) {
				tmp = Index_of[lambda[j - 1]];
				if (tmp != A0)
					lambda[j] ^= Alpha_to[modnn(u + tmp)];
			}
		}
#ifdef ERASURE_DEBUG
		
		for (i = 1; i <= no_eras; i++)
			reg[i] = Index_of[lambda[i]];
		count = 0;
		for (i = 1; i <= NN; i++) {
			q = 1;
			for (j = 1; j <= no_eras; j++)
				if (reg[j] != A0) {
					reg[j] = modnn(reg[j] + j);
					q ^= Alpha_to[reg[j]];
				}
			if (!q) {
				
				root[count] = i;
				loc[count] = NN - i;
				count++;
			}
		}
		if (count != no_eras) {
			printf("\n lambda(x) is WRONG\n");
			return -1;
		}
#ifndef NO_PRINT
		printf("\n Erasure positions as determined by roots of Eras Loc Poly:\n");
		for (i = 0; i < count; i++)
			printf("%d ", loc[i]);
		printf("\n");
#endif
#endif
	}
	for (i = 0; i < NN - KK + 1; i++)
		b[i] = Index_of[lambda[i]];

	r = no_eras;
	el = no_eras;
	while (++r <= NN - KK) {
		
		discr_r = 0;
		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i] != A0)) {
				discr_r ^= Alpha_to[modnn(Index_of[lambda[i]] + s[r - i])];
			}
		}
		discr_r = Index_of[discr_r];	
		if (discr_r == A0) {
			
			COPYDOWN(&b[1], b, NN - KK);
			b[0] = A0;
		} else {
			
			t[0] = lambda[0];
			for (i = 0; i < NN - KK; i++) {
				if (b[i] != A0)
					t[i + 1] = lambda[i + 1] ^ Alpha_to[modnn(discr_r + b[i])];
				else
					t[i + 1] = lambda[i + 1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				
				for (i = 0; i <= NN - KK; i++)
					b[i] = (lambda[i] == 0) ? A0 : modnn(Index_of[lambda[i]] - discr_r + NN);
			} else {
				
				COPYDOWN(&b[1], b, NN - KK);
				b[0] = A0;
			}
			COPY(lambda, t, NN - KK + 1);
		}
	}

	deg_lambda = 0;
	for (i = 0; i < NN - KK + 1; i++) {
		lambda[i] = Index_of[lambda[i]];
		if (lambda[i] != A0)
			deg_lambda = i;
	}
	
	COPY(&reg[1], &lambda[1], NN - KK);
	count = 0;		
	for (i = 1; i <= NN; i++) {
		q = 1;
		for (j = deg_lambda; j > 0; j--)
			if (reg[j] != A0) {
				reg[j] = modnn(reg[j] + j);
				q ^= Alpha_to[reg[j]];
			}
		if (!q) {
			
			root[count] = i;
			loc[count] = NN - i;
			count++;
		}
	}

#ifdef DEBUG
#ifndef NO_PRINT
	printf("\n Final error positions:\t");
	for (i = 0; i < count; i++)
		printf("%d ", loc[i]);
	printf("\n");
#endif
#endif
	if (deg_lambda != count) {
		
		return -1;
	}
	
	deg_omega = 0;
	for (i = 0; i < NN - KK; i++) {
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for (; j >= 0; j--) {
			if ((s[i + 1 - j] != A0) && (lambda[j] != A0))
				tmp ^= Alpha_to[modnn(s[i + 1 - j] + lambda[j])];
		}
		if (tmp != 0)
			deg_omega = i;
		omega[i] = Index_of[tmp];
	}
	omega[NN - KK] = A0;

	for (j = count - 1; j >= 0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != A0)
				num1 ^= Alpha_to[modnn(omega[i] + i * root[j])];
		}
		num2 = Alpha_to[modnn(root[j] * (B0 - 1) + NN)];
		den = 0;

		for (i = min(deg_lambda, NN - KK - 1) & ~1; i >= 0; i -= 2) {
			if (lambda[i + 1] != A0)
				den ^= Alpha_to[modnn(lambda[i + 1] + i * root[j])];
		}
		if (den == 0) {
#ifdef DEBUG
			printf("\n ERROR: denominator = 0\n");
#endif
			return -1;
		}
		
		if (num1 != 0) {
			data[loc[j] * step] ^= Alpha_to[modnn(Index_of[num1] + Index_of[num2] + NN - Index_of[den])];
		}
	}
	return count;
}

int
eras_dec_rs4H_step(dtype data[], int step, int eras_pos[], int no_eras)
{
	int             deg_lambda, el, deg_omega;
	int             i, j, r;
	gf              u, q, tmp, num1, num2, den, discr_r;
	gf              recd[NN];
	gf              lambda[NN - KK + 1], s[NN - KK + 1];	
	gf              b[NN - KK + 1], t[NN - KK + 1], omega[NN - KK + 1];
	gf              root[NN - KK], reg[NN - KK + 1], loc[NN - KK];
	int             syn_error, count;

	for (i = NN - 1; i >= 0; i--) {
		recd[i] = Index_of[data[i * step] >> 4];
	}
	
	syn_error = 0;
	for (i = 1; i <= NN - KK; i++) {
		tmp = 0;
		for (j = 0; j < NN; j++)
			if (recd[j] != A0)	
				tmp ^= Alpha_to[modnn(recd[j] + (B0 + i - 1) * j)];
		syn_error |= tmp;	
		
		s[i] = Index_of[tmp];
	}
	if (!syn_error) {
		
		return 0;
	}
	CLEAR(&lambda[1], NN - KK);
	lambda[0] = 1;
	if (no_eras > 0) {
		
		lambda[1] = Alpha_to[eras_pos[0]];
		for (i = 1; i < no_eras; i++) {
			u = eras_pos[i];
			for (j = i + 1; j > 0; j--) {
				tmp = Index_of[lambda[j - 1]];
				if (tmp != A0)
					lambda[j] ^= Alpha_to[modnn(u + tmp)];
			}
		}
#ifdef ERASURE_DEBUG
		
		for (i = 1; i <= no_eras; i++)
			reg[i] = Index_of[lambda[i]];
		count = 0;
		for (i = 1; i <= NN; i++) {
			q = 1;
			for (j = 1; j <= no_eras; j++)
				if (reg[j] != A0) {
					reg[j] = modnn(reg[j] + j);
					q ^= Alpha_to[reg[j]];
				}
			if (!q) {
				
				root[count] = i;
				loc[count] = NN - i;
				count++;
			}
		}
		if (count != no_eras) {
			printf("\n lambda(x) is WRONG\n");
			return -1;
		}
#ifndef NO_PRINT
		printf("\n Erasure positions as determined by roots of Eras Loc Poly:\n");
		for (i = 0; i < count; i++)
			printf("%d ", loc[i]);
		printf("\n");
#endif
#endif
	}
	for (i = 0; i < NN - KK + 1; i++)
		b[i] = Index_of[lambda[i]];

	r = no_eras;
	el = no_eras;
	while (++r <= NN - KK) {
		
		discr_r = 0;
		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i] != A0)) {
				discr_r ^= Alpha_to[modnn(Index_of[lambda[i]] + s[r - i])];
			}
		}
		discr_r = Index_of[discr_r];	
		if (discr_r == A0) {
			
			COPYDOWN(&b[1], b, NN - KK);
			b[0] = A0;
		} else {
			
			t[0] = lambda[0];
			for (i = 0; i < NN - KK; i++) {
				if (b[i] != A0)
					t[i + 1] = lambda[i + 1] ^ Alpha_to[modnn(discr_r + b[i])];
				else
					t[i + 1] = lambda[i + 1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				
				for (i = 0; i <= NN - KK; i++)
					b[i] = (lambda[i] == 0) ? A0 : modnn(Index_of[lambda[i]] - discr_r + NN);
			} else {
				
				COPYDOWN(&b[1], b, NN - KK);
				b[0] = A0;
			}
			COPY(lambda, t, NN - KK + 1);
		}
	}

	deg_lambda = 0;
	for (i = 0; i < NN - KK + 1; i++) {
		lambda[i] = Index_of[lambda[i]];
		if (lambda[i] != A0)
			deg_lambda = i;
	}
	
	COPY(&reg[1], &lambda[1], NN - KK);
	count = 0;		
	for (i = 1; i <= NN; i++) {
		q = 1;
		for (j = deg_lambda; j > 0; j--)
			if (reg[j] != A0) {
				reg[j] = modnn(reg[j] + j);
				q ^= Alpha_to[reg[j]];
			}
		if (!q) {
			
			root[count] = i;
			loc[count] = NN - i;
			count++;
		}
	}

#ifdef DEBUG
#ifndef NO_PRINT
	printf("\n Final error positions:\t");
	for (i = 0; i < count; i++)
		printf("%d ", loc[i]);
	printf("\n");
#endif
#endif
	if (deg_lambda != count) {
		
		return -1;
	}
	
	deg_omega = 0;
	for (i = 0; i < NN - KK; i++) {
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for (; j >= 0; j--) {
			if ((s[i + 1 - j] != A0) && (lambda[j] != A0))
				tmp ^= Alpha_to[modnn(s[i + 1 - j] + lambda[j])];
		}
		if (tmp != 0)
			deg_omega = i;
		omega[i] = Index_of[tmp];
	}
	omega[NN - KK] = A0;

	for (j = count - 1; j >= 0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != A0)
				num1 ^= Alpha_to[modnn(omega[i] + i * root[j])];
		}
		num2 = Alpha_to[modnn(root[j] * (B0 - 1) + NN)];
		den = 0;

		for (i = min(deg_lambda, NN - KK - 1) & ~1; i >= 0; i -= 2) {
			if (lambda[i + 1] != A0)
				den ^= Alpha_to[modnn(lambda[i + 1] + i * root[j])];
		}
		if (den == 0) {
#ifdef DEBUG
			printf("\n ERROR: denominator = 0\n");
#endif
			return -1;
		}
		
		if (num1 != 0) {
			data[loc[j] * step] ^= ((Alpha_to[modnn(Index_of[num1] + Index_of[num2] + NN - Index_of[den])] << 4) & 0xf0);
		}
	}
	return count;
}

int
eras_dec_rs4L_step(dtype data[], int step, int eras_pos[], int no_eras)
{
	int             deg_lambda, el, deg_omega;
	int             i, j, r;
	gf              u, q, tmp, num1, num2, den, discr_r;
	gf              recd[NN];
	gf              lambda[NN - KK + 1], s[NN - KK + 1];	
	gf              b[NN - KK + 1], t[NN - KK + 1], omega[NN - KK + 1];
	gf              root[NN - KK], reg[NN - KK + 1], loc[NN - KK];
	int             syn_error, count;

	for (i = NN - 1; i >= 0; i--) {
		recd[i] = Index_of[data[i * step] & 0x0f];
	}
	
	syn_error = 0;
	for (i = 1; i <= NN - KK; i++) {
		tmp = 0;
		for (j = 0; j < NN; j++)
			if (recd[j] != A0)	
				tmp ^= Alpha_to[modnn(recd[j] + (B0 + i - 1) * j)];
		syn_error |= tmp;	
		
		s[i] = Index_of[tmp];
	}
	if (!syn_error) {
		
		return 0;
	}
	CLEAR(&lambda[1], NN - KK);
	lambda[0] = 1;
	if (no_eras > 0) {
		
		lambda[1] = Alpha_to[eras_pos[0]];
		for (i = 1; i < no_eras; i++) {
			u = eras_pos[i];
			for (j = i + 1; j > 0; j--) {
				tmp = Index_of[lambda[j - 1]];
				if (tmp != A0)
					lambda[j] ^= Alpha_to[modnn(u + tmp)];
			}
		}
#ifdef ERASURE_DEBUG
		
		for (i = 1; i <= no_eras; i++)
			reg[i] = Index_of[lambda[i]];
		count = 0;
		for (i = 1; i <= NN; i++) {
			q = 1;
			for (j = 1; j <= no_eras; j++)
				if (reg[j] != A0) {
					reg[j] = modnn(reg[j] + j);
					q ^= Alpha_to[reg[j]];
				}
			if (!q) {
				
				root[count] = i;
				loc[count] = NN - i;
				count++;
			}
		}
		if (count != no_eras) {
			printf("\n lambda(x) is WRONG\n");
			return -1;
		}
#ifndef NO_PRINT
		printf("\n Erasure positions as determined by roots of Eras Loc Poly:\n");
		for (i = 0; i < count; i++)
			printf("%d ", loc[i]);
		printf("\n");
#endif
#endif
	}
	for (i = 0; i < NN - KK + 1; i++)
		b[i] = Index_of[lambda[i]];

	r = no_eras;
	el = no_eras;
	while (++r <= NN - KK) {
		
		discr_r = 0;
		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i] != A0)) {
				discr_r ^= Alpha_to[modnn(Index_of[lambda[i]] + s[r - i])];
			}
		}
		discr_r = Index_of[discr_r];	
		if (discr_r == A0) {
			
			COPYDOWN(&b[1], b, NN - KK);
			b[0] = A0;
		} else {
			
			t[0] = lambda[0];
			for (i = 0; i < NN - KK; i++) {
				if (b[i] != A0)
					t[i + 1] = lambda[i + 1] ^ Alpha_to[modnn(discr_r + b[i])];
				else
					t[i + 1] = lambda[i + 1];
			}
			if (2 * el <= r + no_eras - 1) {
				el = r + no_eras - el;
				
				for (i = 0; i <= NN - KK; i++)
					b[i] = (lambda[i] == 0) ? A0 : modnn(Index_of[lambda[i]] - discr_r + NN);
			} else {
				
				COPYDOWN(&b[1], b, NN - KK);
				b[0] = A0;
			}
			COPY(lambda, t, NN - KK + 1);
		}
	}

	deg_lambda = 0;
	for (i = 0; i < NN - KK + 1; i++) {
		lambda[i] = Index_of[lambda[i]];
		if (lambda[i] != A0)
			deg_lambda = i;
	}
	
	COPY(&reg[1], &lambda[1], NN - KK);
	count = 0;		
	for (i = 1; i <= NN; i++) {
		q = 1;
		for (j = deg_lambda; j > 0; j--)
			if (reg[j] != A0) {
				reg[j] = modnn(reg[j] + j);
				q ^= Alpha_to[reg[j]];
			}
		if (!q) {
			
			root[count] = i;
			loc[count] = NN - i;
			count++;
		}
	}

#ifdef DEBUG
#ifndef NO_PRINT
	printf("\n Final error positions:\t");
	for (i = 0; i < count; i++)
		printf("%d ", loc[i]);
	printf("\n");
#endif
#endif
	if (deg_lambda != count) {
		
		return -1;
	}
	
	deg_omega = 0;
	for (i = 0; i < NN - KK; i++) {
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for (; j >= 0; j--) {
			if ((s[i + 1 - j] != A0) && (lambda[j] != A0))
				tmp ^= Alpha_to[modnn(s[i + 1 - j] + lambda[j])];
		}
		if (tmp != 0)
			deg_omega = i;
		omega[i] = Index_of[tmp];
	}
	omega[NN - KK] = A0;

	for (j = count - 1; j >= 0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != A0)
				num1 ^= Alpha_to[modnn(omega[i] + i * root[j])];
		}
		num2 = Alpha_to[modnn(root[j] * (B0 - 1) + NN)];
		den = 0;

		for (i = min(deg_lambda, NN - KK - 1) & ~1; i >= 0; i -= 2) {
			if (lambda[i + 1] != A0)
				den ^= Alpha_to[modnn(lambda[i + 1] + i * root[j])];
		}
		if (den == 0) {
#ifdef DEBUG
			printf("\n ERROR: denominator = 0\n");
#endif
			return -1;
		}
		
		if (num1 != 0) {
			data[loc[j] * step] ^= (Alpha_to[modnn(Index_of[num1] + Index_of[num2] + NN - Index_of[den])] & 0x0f);
		}
	}
	return count;
}

int
eras_dec_rs4_step(dtype data[], int step, int eras_pos[], int no_eras)
{
	int             countH, countL;

	countH = eras_dec_rs4H_step(data, step, eras_pos, no_eras);
	countL = eras_dec_rs4L_step(data, step, eras_pos, no_eras);

	if (countH < countL) {
		return countL;
	} else {
		return countH;
	}
}

void main27()
{
	dtype data[NN] = {1, 2, 3, 4, 5, 6, 7, 8};
	dtype dataCopy[NN];
	int erasedPos[2] = {0, 1};
	int i, recoveryed;
	init_rs();
	generate_gf();
	gen_poly();
	encode_rs(data, &data[13]);
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	data[0] = 0;

	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	
	memcpy(dataCopy, data, sizeof(data));
	recoveryed = eras_dec_rs(data, NULL, 0);
	printf("try error recoveryed data len:%d\n", recoveryed);
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	
	recoveryed = eras_dec_rs(dataCopy, erasedPos, 2);
	for (i = 0; i < sizeof(dataCopy); i ++)
	{
		printf("%d ", dataCopy[i]);
	}
	printf("\n");
	printf("recoveryed data len:%d\n", recoveryed);
}

void main28()
{
	
	dtype data[NN] =   {2, 3, 4, 5, 6, 7, 8, 12, 0, 11, 1, 5, 14, 2, 13};
	dtype dataCopy[NN];
	
	int erasedPos[4] = {7, 9, 12, 14};
	int i, recoveryed;
	init_rs();

	data[7] = 0;
	data[9] = 6;
	data[12] = 3;
	data[14] = 0;
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	
	memcpy(dataCopy, data, sizeof(data));
	recoveryed = eras_dec_rs(data, NULL, 0);
	printf("try error recoveryed data len:%d\n", recoveryed);
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	
	recoveryed = eras_dec_rs(dataCopy, erasedPos, 4);
	for (i = 0; i < sizeof(dataCopy); i ++)
	{
		printf("%d ", dataCopy[i]);
	}
	printf("\n");
	printf("recoveryed data len:%d\n", recoveryed);
}

#include <assert.h>
#include "base.h"
#include "rs.h"

bool rsInited = false;
void rsInit()
{
	if (!rsInited)
	{
		init_rs();
		generate_gf();
		gen_poly();
	}
	rsInited = true;
}

void printData(rstype _data[], int _dataLen)
{
	int i;
	for (i = 0; i < _dataLen; i ++)
	{
		assert(_data[i] >= 0 && _data[i] <= NN);
		
	}
}

int rsEncode(rstype _data[], int _dataLen, rstype _bb[])
{
	int r;
#ifdef _DEBUG
	printData(_data, _dataLen);
	
#endif
	assert(_dataLen <= KK);
	r = encode_rs(_data, _bb);

	return r;
}

int rsDecode(rstype _data[], int _eras_pos[], int _no_eras)
{
	
	int recoveryed;
#ifdef _DEBUG
	printData(_data, NN);
	
#endif

	recoveryed = eras_dec_rs(_data, _eras_pos, _no_eras);

	return recoveryed;
}

void testEncode()
{
	dtype data[NN] = {6, 7, 8, 9, 0, 9, 7, 10, 10, 0, 0, 0, 0, 0, 0};
	int i;
	init_rs();
	rsEncode(data, 13, data+13);	
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
}

void testrsDecode()
{

	dtype data[NN] =   {2, 3, 4, 5, 6, 7, 8, 12, 0, 11, 1, 5, 14, 2, 13};
	int i, recovery;
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
	init_rs();
	recovery = rsDecode(data, NULL, 0);
	printf("recovery:%d\n", recovery);
	for (i = 0; i < sizeof(data); i ++)
	{
		printf("%d ", data[i]);
	}
	printf("\n");
}

void main32()
{

	testrsDecode();
}

