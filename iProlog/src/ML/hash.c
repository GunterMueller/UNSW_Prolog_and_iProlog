#include "prolog.h"
#include "p_compare.h"

#define HASH(x, n)		(unsigned int)((((unsigned int) hash_fn(x)) >> 2) % n)
#define EQ(x, y)		(term_compare(x, NULL, y, NULL) == 0)


static unsigned int
hash_fn(term p)
{
	int i;
	unsigned int h = 0;

	switch (TYPE(p))
	{
	case FN:	for (i = ARITY(p); i >= 0; i--)
				h ^= hash_fn(ARG(i, p));
			return h;
	case LIST:	return hash_fn(CAR(p)) ^ hash_fn(CDR(p));
	case INT:	return ((unsigned int) IVAL(p));
	case REAL:	return ((unsigned int) RVAL(p));
	default:	return ((unsigned int) p);
	}
}


/************************************************************************/
/*	Linear probing hash table used to uniquely store set entries	*/
/************************************************************************/

short
new_hash_entry(term key, term hash_table)
{
	short hash_size = ARITY(hash_table);
	short h = HASH(key, hash_size);
	short hold = h;


	do {
		if (ARG(h+1, hash_table) == _nil)
		{
			ARG(h+1, hash_table) = key;
			return h;
		}

		if (EQ(ARG(h+1, hash_table), key))
			return h;

		h = (h + 1) % hash_size;
	}
	while (h != hold);

	fprintf(stderr, "Hash table is full!!!\n");
	return -1;
}


/************************************************************************/
/*			  Get existing hash entry			*/
/************************************************************************/

short
get_hash_entry(term key, term hash_table)
{
	card hash_size = ARITY(hash_table);
	card h = HASH(key, hash_size);
	card hold = h;

	do {
		if (ARG(h+1, hash_table) == _nil)
			return -1;

		if (EQ(ARG(h+1, hash_table), key))
			return h;

		h = (h + 1) % hash_size;
	}
	while (h != hold);

	return -1;
}


/************************************************************************/
/*			  Get existing hash entry			*/
/************************************************************************/

void
delete_hash_entry(int h, term hash_table)
{
	ARG(h+1, hash_table) = _nil;
}

/************************************************************************/
/*		  Dump hash table for debugging				*/
/************************************************************************/

void
dump_hash(term hash_table)
{
	int i, n = ARITY(hash_table);

	for (i = 1; i <= n; i++)
		if (ARG(i, hash_table) != _nil)
		{
			printf("%2d: %2d  ", i, HASH(ARG(i, hash_table), n) + 1);
			print(ARG(i, hash_table));
		}
}


/************************************************************************/
/*		  Initialise hash table module				*/
/************************************************************************/

term
new_hash_table(int n)
{
	int i;
	term p = new_h_fn(n);

	ARG(0, p) = intern("hash_table");

	for (i = 1; i <= n ; i++)
		ARG(i, p) = _nil;

	return p;
}
