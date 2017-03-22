#include "prolog.h"

extern void (*set_printing)(term);
static term hash_table, attributes;


/************************************************************************/
/*			     Create a new set 				*/
/************************************************************************/

term
new_set(int n, term contents)
{
	int i;
	term p = (term) malloc(sizeof(set) + (sizeof(unsigned long) * (n - 1)));

	if (p == NULL)
		fail("Can't allocate space for new set\n");

	TYPE(p) = SET;
	FLAGS(p) = 0;
	SET_SIZE(p) = n;
	NEXT(p) = NULL;
	CONTENTS(p) = contents;
	SUBSUMES(p) = _nil;
	CLASS(p) = -1;
	NSEL(p) = NPOS(p) = NNEG(p) = 0;

	for (i = 0; i < n; i++)
		SELECTOR(i, p) = 0L;

	return p;
}


term
copy_set(term s)
{
	int i;
	int n = SET_SIZE(s);
	term p = (term) malloc(sizeof(set) + (sizeof(unsigned long) * (n - 1)));

	TYPE(p) = SET;
	FLAGS(p) = FLAGS(s);
	SET_SIZE(p) = n;
	NEXT(p) = NEXT(s);
	CONTENTS(p) = CONTENTS(s);
	SUBSUMES(p) = SUBSUMES(s);
	CLASS(p) = CLASS(s);
	NSEL(p) = NSEL(s);
	NPOS(p) = NPOS(s);
	NNEG(p) = NNEG(s);

	for (i = 0; i < n; i++)
		SELECTOR(i, p) = SELECTOR(i, s);

	return p;
}


term
new_complex(term s)
{
	int i;
	int n = SET_SIZE(s);
	term p = (term) malloc(sizeof(set) + (sizeof(unsigned long) * (n - 1)));

	if (p == NULL)
		fail("Can't allocate space for new set\n");

	TYPE(p) = SET;
	FLAGS(p) = 0;
	SET_SIZE(p) = n;
	NEXT(p) = SUBSUMES(p) = NULL;
	CONTENTS(p) = CONTENTS(s);
	CLASS(p) = CLASS(s);
	NSEL(p) = NPOS(p) = NNEG(p) = 0;

	for (i = 0; i < n; i++)
		SELECTOR(i, p) = ALL_BITS;

	return p;
}


/************************************************************************/
/*			Free a disjunction of sets			*/
/************************************************************************/

void
free_disj(term d)
{
	while (d != NULL)
	{
		term p = NEXT(d);
		free(d);
		d = p;
	}
}


/************************************************************************/
/*			    Bit set operations				*/
/************************************************************************/

void
clear_set(term s)
{
	int i;
	unsigned long *p = s -> z.sel;

	for (i = SET_SIZE(s); i != 0; i--, p++)
		*p = 0L;
}


void
all_ones(term s)
{
	int i;
	unsigned long *p = s -> z.sel;

	for (i = SET_SIZE(s); i != 0; i--, p++)
		*p = ALL_BITS;
}


void
set_add(term x, short posn)
{
	if (posn < 0 || posn >= (SET_SIZE(x) * BITS_IN_WORD))
		fail("bit position out of range");

	SELECTOR(posn/BITS_IN_WORD, x) |= 1L << (posn % BITS_IN_WORD);
}


int
set_intersection(term x, term y, term z)
{
	int i;
	unsigned long j = 0, *p = x -> z.sel, *q = y -> z.sel, *r = z -> z.sel;

	for (i = SET_SIZE(x); i != 0; i--, p++, q++, r++)
		j |= (*r = (*p & *q));

	return (j != 0);
}


int
set_union(term x, term y, term z)
{
	int i;
	unsigned long j = 0, *p = x -> z.sel, *q = y -> z.sel, *r = z -> z.sel;

	for (i = SET_SIZE(x); i != 0; i--, p++, q++, r++)
		j |= (*r = (*p | *q));

	return (j != 0);
}


int
set_diff(term x, term y, term z)
{
	int i;
	unsigned long j = 0, *p = x -> z.sel, *q = y -> z.sel, *r = z -> z.sel;

	for (i = SET_SIZE(x); i != 0; i--, p++, q++, r++)
		j |= (*r = ((*p) & ~(*q)));

	return (j != 0);
}


/************************************************************************/
/*			   Bit set conditionals				*/
/************************************************************************/

int
empty_set(term s)
{
	int i;
	unsigned long *p = s -> z.sel;

	for (i = SET_SIZE(s); i != 0; i--, p++)
		if (*p)
			return FALSE;
	return TRUE;
}


int
set_eq(term x, term y)
{
	int i;
	unsigned long *p = x -> z.sel, *q = y -> z.sel;

	for (i = SET_SIZE(x); i != 0; i--, p++, q++)
		if (*p != *q)
			return FALSE;
	return TRUE;
}


int
set_contains(term x, term y)
{
	int i;
	unsigned long *p = x -> z.sel, *q = y -> z.sel;

	for (i = SET_SIZE(x); i != 0; i--, p++, q++)
		if ((*p & *q) != *q)
			return FALSE;
	return TRUE;
}


int
disj_contains(term d, term s)
{
	while (d != NULL)
		if (set_contains(d, s))
			return(TRUE);
		else d = NEXT(d);
	return FALSE;
}


/************************************************************************/
/*		Find the number of 1's in a bit set			*/
/************************************************************************/

int
set_cardinality(term s)
{
	int i, count = 0;
	unsigned long sel;

	for (i = SET_SIZE(s) - 1; i >= 0; i--)
		for (sel = SELECTOR(i, s); sel != 0; sel >>= 1)
			if (sel & 1)
				count++;

	return count;
}


/************************************************************************/
/*	Calculate the cardinality of the intersection of two sets	*/
/************************************************************************/

int
intersection_size(term s1, term s2)
{
	int i, count = 0;

	for (i = SET_SIZE(s1) - 1; i >= 0; i--)
	{
		unsigned long sel1 = SELECTOR(i, s1);
		unsigned long sel2 = SELECTOR(i, s2);

		while (sel1 != 0 && sel2 != 0)
		{
			if ((sel1 & 1) && (sel2 & 1))
				count++;

			sel1 >>= 1;
			sel2 >>= 1;
		}
	}

	return count;
}


/************************************************************************/
/*		     Print raw bit set as 0's and 1's			*/
/************************************************************************/

void
print_bit_set(term s)
{
	int i, n = ARITY(CONTENTS(s));

	for (i = 0; i < n; i++)
		printf("%d ", ((SELECTOR(i/BITS_IN_WORD, s) & 1L << (i % BITS_IN_WORD)) != 0));
	printf("\n");
}


/************************************************************************/
/*			    Print bits in bit map			*/
/************************************************************************/

void
print_bit_map(term b)
{
	int i;

	printf("\n");
	for (i = 1; i <= ARITY(b); i++)
		print_bit_set(ARG(i, b));
	printf("\n");
}


/************************************************************************/
/* Print a set using contents field to get values assoc'd with a bit	*/
/************************************************************************/


void
print_set(term s)
{
	void print_complex(term);
	int i, j;
	unsigned long mask;
	char *sep = "";
	term contents = CONTENTS(s);
	int set_size = SET_SIZE(s);

	if (ARG(0, contents) != hash_table)
	{
		print_complex(s);
		return;
	}

	if (CLASS(s) != -1)
		fprintf(output, "%s :- ", NAME(ARG(CLASS(s) + 1, contents)));
	else
		fputc('{', output);

	for (i = 0; i < set_size; i++)
	{
		mask = 1;
		for (j = 0; j < BITS_IN_WORD; j++)
		{
			if (SELECTOR(i, s) & mask)
			{
				fputs(sep, output);
				sep = ", ";
				prin(ARG((int)(i * BITS_IN_WORD + j + 1), contents));
			}
			mask <<= 1;
		}
	}

	if (CLASS(s) != -1)
		fputc('.', output);
	else
		fputc('}', output);
}


/************************************************************************/
/*	Print a set, each element is described by its position		*/
/************************************************************************/

void
print_set_raw(term s)
{
	int i, j, k = 0;
	unsigned long mask;
	char *sep = "";
	int set_size = SET_SIZE(s);

	fputc('{', output);

	for (i = 0; i < set_size; i++)
		for (j = 0, mask = 1; j < BITS_IN_WORD; j++, mask <<= 1, k++)
			if (SELECTOR(i, s) & mask)
			{
				fprintf(output, "%s%d", sep, k);
				sep = ", ";
			}

	fputc('}', output);
}


/************************************************************************/
/*		Print each set in a disjunction of sets			*/
/************************************************************************/

void
print_disj(term d)
{
	fputc('{', output);
	while (d != NULL)
	{
		print_set(d);
		if (d = NEXT(d))
			fputs(", ", output);
	}
	fputc('}', output);
}


/************************************************************************/
/*			     Set initialisation				*/
/************************************************************************/

void
set_init(void)
{
	set_printing = print_set;
	hash_table = intern("hash_table");
	attributes = intern("attributes");
}
