#include "prolog.h"

#define WEIGHT		5				/* number of bits to encode per term */
#define MAX_LEVELS	4				/* number of levels of tree position recorded */
#define WIDTH		6				/* Generate random no. up to 2**WIDTH */

#define SCW_WORDS 	2				/* number of words in scw register */
#define SCW_BITS 	(SCW_WORDS * BITS_IN_WORD)	/* maximum width of scw word */

#define SLICE_WORDS 	2				/* number of words in a bit slice */
#define SLICE_BITS 	(SLICE_WORDS * BITS_IN_WORD)	/* maximum width of a bit slice */

#define	COM_POSN	8
#define	COM_VAR_START	0
#define	COM_DOT_START	(COM_VAR_START + COM_POSN * (COM_POSN + 1)) /* 72 */
#define	COM_INT_START	(COM_DOT_START + COM_POSN * (COM_POSN + 1)) /* 144 */

#define	LEVEL1(addr)	(addr.level[1] == 0)		/* Tree address is at level 1 */
#define LEVEL1or2(addr)	(addr.level[2] == 0)		/* Tree address is at level 1 or 2 */


typedef unsigned long scw[SCW_WORDS];
typedef unsigned long bit_slice[SLICE_WORDS];


/************************************************************************/
/* Tree addesses are stored in byte positions in a long word allowing	*/
/* encoding to four levels in a 4 byte word.				*/
/************************************************************************/

typedef union
{
	unsigned long long_addr;
	unsigned char level[WORD_LENGTH];
} tree_address;

extern term varlist, *local, *global;

static unsigned long slice[SCW_BITS][SLICE_WORDS];
static term index[SLICE_BITS];
static term *next_free;					/* index free list pointer */

static unsigned long bit[BITS_IN_WORD];			/* scw mask */
static unsigned long seed;				/* seed register for rand_num */

static int variables_in_db = FALSE;

/************************************************************************/
/*		Print bits in one bit slice - debugging only		*/
/************************************************************************/

static void
print_scw(scw r)
{
	int i;

	for (i = 0; i < SCW_BITS; i++)
		printf("%d", ((r[i/BITS_IN_WORD] & 1L << (i % BITS_IN_WORD)) != 0));
	printf("\n");
}


static void
print_slice(bit_slice b)
{
	int i;

	for (i = 0; i < SLICE_BITS; i++)
		printf("%d", ((b[i/BITS_IN_WORD] & 1L << (i % BITS_IN_WORD)) != 0));
	printf("\n");
}


/************************************************************************/
/*			Set all bits in an SCW to 0			*/
/************************************************************************/

static void
clear_scw(scw r)
{
 	int i;
 
	for (i = 0; i < SCW_WORDS; i++)
		r[i] = 0;
}


/************************************************************************/
/*	    		Set all bits in a slice to 1			*/
/************************************************************************/

static void
all_ones(bit_slice b)
{
	int i;

	for (i = 0; i < SLICE_WORDS; i++)
		b[i] = ALL_BITS;
}


/************************************************************************/
/* Find the union of two bit slices. Put the result in the third	*/
/************************************************************************/

static void
slice_union(bit_slice x, bit_slice y, bit_slice z)
{
	int i;
	unsigned long *p = x, *q = y, *r = z;

	for (i = SLICE_WORDS; i != 0; i--, p++, q++, r++)
		*r = (*p | *q);
}


/************************************************************************/
/* Find the intersection of two bit slices. Put the result in the third	*/
/************************************************************************/

static void
intersect(bit_slice x, bit_slice y, bit_slice z)
{
	int i;
	unsigned long *p = x, *q = y, *r = z;

	for (i = SLICE_WORDS; i != 0; i--, p++, q++, r++)
		*r = (*p & *q);
}


/************************************************************************/
/*	   add scw in to bit slice array at clause position pos		*/
/************************************************************************/

static void
add_bit_slice(int pos, scw r)
{
	int i = pos / BITS_IN_WORD;
	int j = pos % BITS_IN_WORD;
	int k;

	for (k = 0; k < SCW_BITS; k++)
		if (r[k / BITS_IN_WORD] & bit[k % BITS_IN_WORD])
			slice[k][i] |= bit[j];
}


/************************************************************************/
/*	    Return a bit slice containing hits for a query		*/
/************************************************************************/


static void
get_hits(scw r, bit_slice result)
{
	int i, j, s = 0;

	all_ones(result);
	for (i = 0; i < SCW_WORDS; i++)
	{
		unsigned long word = r[i];

		for (j = 0; j < BITS_IN_WORD; j++, s++)
			if (word & bit[j])
				intersect(result, slice[s], result);
	}
}


/************************************************************************/
/*	       Fast shift register random number generator		*/
/************************************************************************/

static unsigned long
ran_num(unsigned long w)
{
	unsigned long  i, v = 0, mask = 0x40000008;

	for (i = w; i != 0; i--)
	{
		unsigned long x = seed & mask;

		if (x == 0 || x == mask)
		{
			seed <<= 1;
			v <<= 1;
		}
		else
		{
			seed = (seed << 1) | 1;
			v = (v << 1) | 1;
		}
	}
	return v;
}


/************************************************************************/
/*	XOR every Nth char and OR the result - N = WORD_LENGTH		*/
/************************************************************************/

static unsigned long
hash_text(char *text)
{
	char h[WORD_LENGTH];
	unsigned long ans;
	int i;

	for (i = 0; i < WORD_LENGTH; i++)
		h[i] = 0;

	for (i = 0; text[i] != '\0'; i++)
		h[i % WORD_LENGTH] ^= text[i];

	ans = h[0];
	for (i = 1; i < WORD_LENGTH; i++)
		ans = (ans << 8) | h[i];

	return ans;
}


/************************************************************************/
/* A tree address is just the place of a term stored to four levels	*/
/************************************************************************/

static unsigned long
tree_addr(unsigned long old, unsigned long new)
{
	int i;
	tree_address p;

	p.long_addr = old;

	for (i = 0; i < WORD_LENGTH; i++)
		if (p.level[i] == 0)
			break;

	if (i < WORD_LENGTH)
		p.level[i] = new;

	return p.long_addr;
}


/************************************************************************/
/*	Generate the bits for a term and OR them into the code word	*/
/************************************************************************/

static unsigned long
scw_encode(unsigned long hashed_value, scw s)
{
	int i;
	scw r;

	seed = hashed_value;

	clear_scw(r);

	for (i = 0; i < WEIGHT;)
	{
		unsigned long n = ran_num(WIDTH);
		int j = n % BITS_IN_WORD;
		int k = n / BITS_IN_WORD;

		if ((r[k] & bit[j]) == 0)
		{
			r[k] |= bit[j];
			i++;
		}
	}

	for (i = 0; i < SCW_WORDS; i++)
		s[i] |= r[i];
}


/************************************************************************/
/*	Produce a short encoding based on sh.				*/
/*	The result is that only two bit positions are used		*/
/************************************************************************/

void
short_encode(int sh, scw r)
{
	int b1 = 0;
	int b2 = sh + SCW_BITS;
	int n = BITS_IN_WORD - 1;

	while (b2 >= SCW_BITS)
	{
		b2 -= n--;
		b1++;
	}
	b2 += b1;

	r[b1 / BITS_IN_WORD] |= bit[b1 % BITS_IN_WORD];
	r[b2 / BITS_IN_WORD] |= bit[b2 % BITS_IN_WORD];
}


/************************************************************************/
/* Try short encoding if 0 <= i <= 9 and only if at top level		*/
/* and position of arg is in the first 8				*/
/************************************************************************/

static void
encode_int(unsigned long i, unsigned long addr, scw r)
{
	int L1;
	tree_address a;

	a.long_addr = addr;
	L1 = a.level[0];

	if (0 <= i && i <= 9 && LEVEL1(a) && L1 <= COM_POSN)
		short_encode(COM_INT_START + (L1 - 1) * 10 + i + 1, r);
	else
		scw_encode(i ^ addr, r);
}


/************************************************************************/
/* Try short of dot only if in top two levels				*/
/* and position of arg in bothe levels is in the first 8		*/
/************************************************************************/

static void
encode_dot(unsigned long addr, scw r)
{
	int L1, L2;
	tree_address a;

	a.long_addr = addr;
	L1 = a.level[0];
	L2 = a.level[1];

	if (LEVEL1or2(a) && L1 <= COM_POSN && L2 <= COM_POSN)
		short_encode(COM_DOT_START + L1 + L2 * COM_POSN, r);
	else
		scw_encode(hash_text(".") ^ addr, r);
}


/************************************************************************/
/*			Compute the SCW for a term			*/
/************************************************************************/

static void
calculate(int query, term x, unsigned long addr, scw r)
{
	int i;

	switch (TYPE(x))
	{
		case REF:
		case ANON:
		case FREE: 
		case BOUND:
			if (! query)
			{
				scw_encode(addr, r);
				variables_in_db = TRUE;
			}
			return;
		case ATOM: 
			scw_encode(hash_text(NAME(x)) ^ addr, r);
			break;
		case INT: 
			/* scw_encode(IVAL(x) ^ addr, r); */
			encode_int(IVAL(x), addr, r);
			break;
		case LIST:
			/* scw_encode(hash_text(".") ^ addr, r); */
			encode_dot(addr, r);
			calculate(query, CAR(x), tree_addr(addr, 1), r);
			calculate(query, CDR(x), tree_addr(addr, 2), r);
			break;
		case FN: 
			scw_encode(hash_text(NAME(ARG(0, x))) ^ ARITY(x) ^ addr, r);

			for (i = 1; i <= ARITY(x); i++)
				calculate(query, ARG(i, x), tree_addr(addr, i), r);
			break;
	}
}


/************************************************************************/
/* Find hits for a query when DB is not ground.				*/
/* Requires two passes for each term in query (var or nonvar)		*/
/************************************************************************/

static void
search(term x, unsigned long addr, bit_slice result)
{
	int i;
	scw r;
	bit_slice b;

	clear_scw(r);

	switch (TYPE(x))
	{
		case REF:
		case ANON:
		case FREE: 
		case BOUND:
			all_ones(result);
			return;
		case ATOM: 
			scw_encode(hash_text(NAME(x)) ^ addr, r);
			get_hits(r, result);
			break;
		case INT: 
			/* scw_encode(IVAL(x) ^ addr, r); */
			encode_int(IVAL(x), addr, r);
			get_hits(r, result);
			break;
		case LIST:
			/* scw_encode(hash_text(".") ^ addr, r); */
			encode_dot(addr, r);
			get_hits(r, result);
			search(CAR(x), tree_addr(addr, 1), b);
			intersect(result, b, result);
			search(CDR(x), tree_addr(addr, 2), b);
			intersect(result, b, result);
			break;
		case FN: 
			scw_encode(hash_text(NAME(ARG(0, x))) ^ ARITY(x) ^ addr, r);
			get_hits(r, result);

			for (i = 1; i <= ARITY(x); i++)
			{
				search(ARG(i, x), tree_addr(addr, i), b);
				intersect(result, b, result);
			}
			break;
	}
	if (addr)
	{
		clear_scw(r);
		scw_encode(addr, r);
		get_hits(r, b);
		slice_union(result, b, result);
	}
}


/************************************************************************/
/*	      Find matches in index corresponding to hits		*/
/************************************************************************/

static int
unify_matches(term query, term *frame, term cl)
{
	term *old_global = global;
	term new_frame[NVARS(cl)];

	if (unify(HEAD(cl), new_frame, query, frame))
		if (rest_of_clause())
			return FALSE;

	_untrail();
	global = old_global;
	return TRUE;
}


static int
find_matches(term x, term *frame)
{
	int i, j, k = 0;
	bit_slice hits;

	if (variables_in_db)
		search(x, 0L, hits);
	else
	{
		scw r;
		clear_scw(r);
		calculate(TRUE, x, 0L, r);
		get_hits(r, hits);
	}

	for (i = 0; i < SLICE_WORDS; i++)
	{
		unsigned long word = hits[i];

		for (j = 0; j < BITS_IN_WORD; j++, k++)
			if (word & bit[j])
			{
				/* printf("=> %d: ", k); print(index[k]); */
				if (! unify_matches(x, frame, index[k]))
					break;
			}
	}
	return FALSE;
}


/************************************************************************/
/*		    Add a clause to the SCW data base			*/
/************************************************************************/

static void
scw_add_clause(term x)
{
	int pos;
	scw r;
	term *p;

	if (next_free == NULL)
		fail("SCW index is full");

	pos = next_free - index;
	p = (term *)(*next_free);
	*next_free = x;
	next_free = p;

 	clear_scw(r);

	calculate(FALSE, HEAD(x), 0L, r);
/*
	print(index[pos]);
	print_scw(r);
*/
	add_bit_slice(pos, r);
}


/************************************************************************/
/*			Prolog hooks to SCW module			*/
/************************************************************************/

static int
p_scw(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term *old_global = global;

	varlist = _nil;
	scw_add_clause(mkclause(copy(x, frame), frame));
	global = old_global;
	return TRUE;
}


static int
scw_query(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
 
	return find_matches(x, frame);
}



/************************************************************************/
/* Initialise scw module - must be called before anything else here	*/
/************************************************************************/

static void
init_map()
{
	int i, j;

	for (i = 0; i < SCW_BITS; i++);
		for (j = 0; j < SLICE_WORDS; j++)
			slice[i][j] = 0;
}

void
scw_init(void)
{
	int i;

	init_map();
	for (i = 0; i < BITS_IN_WORD; i++)
		bit[i] = 1L << i;

	index[SLICE_BITS - 1] = NULL;
	for (i = SLICE_BITS - 1; i > 0; i--)
		index[i-1] = (term)(&index[i]);
	next_free = index;

	new_pred(p_scw, "scw");
	new_pred(scw_query, "scw_query");
}
