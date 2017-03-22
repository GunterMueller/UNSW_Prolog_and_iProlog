#include "prolog.h"
#include "set.h"
#include "hash.h"

#define MAX_PROPS	1024

#define INTER(i, r)	((i - 1)*(r - 1) - 2)
#define INTRA(i, r)	(i*(r - 1) - 2)
#define ABSORP(i, r)	((i - 1)*(r - 1))
#define IDENNT(i, r)	(i*(r - 1))
#define TRUNC(t, i)	(t - i - 1)


typedef enum
{
	INTER_CONSTRUCTION, INTRA_CONSTRUCTION, ABSORPTION, IDENTIFICATION,
	DICHOTOMISATION, TRUNCATION
} operator;


static term hash_table = NULL, theory, *last, original_theory;
static term free_list = NULL;

static unsigned short max_props = 1024;


/************************************************************************/
/*		Create a new bit set representing a clause		*/
/************************************************************************/

static term
new_clause_set(void)
{
	if (free_list != NULL)
	{
		term rval = free_list;

		free_list = NEXT(free_list);
		NEXT(rval) = NULL;
		SUBSUMES(rval) = _nil;
		CLASS(rval) = -1;
		return(rval);
	}
	return new_set(max_props/BITS_IN_WORD, hash_table);
}


/************************************************************************/
/* Free a set and the list cells in the subsumption list.		*/
/* Don't free sets in subsumption list since they are in the theory	*/
/************************************************************************/

static void
free_clause_set(term s)
{
	term p, q;

	if (s == NULL)
		return;
	p = SUBSUMES(s);

	while (p != _nil)
	{
		q = p;
		p = CDR(p);
		dispose(q);
	}

	NEXT(s) = free_list;
	free_list = s;
}		


/************************************************************************/
/*	Remove all the clauses subsumed by s from the theory		*/
/************************************************************************/

static void
remove_from_theory(term s)
{
	term q = SUBSUMES(s);

	while (q != _nil)
	{
		term z;
		term c = CAR(q);
		term *p = &theory;

		while (*p != NULL)
			if (*p == c)
			{
				*p = NEXT(*p);
				break;
			}
			else
				p = &NEXT(*p);

		dispose(c);
		z = CDR(q);
		dispose(q);
		q = z;
	}

	SUBSUMES(s) = _nil;
}


/************************************************************************/
/*   Check if s results in the same operation as performed previously	*/
/************************************************************************/

static int
equivalent(term c1, term c2)
{
	term p, q;

	for (p = SUBSUMES(c1); p != _nil; p = CDR(p))
	{
		for (q = SUBSUMES(c2); q != _nil; q = CDR(q))
			if (set_eq(CAR(p), CAR(q)))
				break;
		if (q == _nil)
			return FALSE;
	}
	return TRUE;
}			


static term
redundant(term s)
{
	term p;

	for (p = theory; p != NULL; p = NEXT(p))
		if (equivalent(s, p))
		{
			fprintf(stderr, "FOUND ONE\n");
			return p;
		}
	return NULL;
}


/************************************************************************/
/*			     Set printing				*/
/************************************************************************/

static void
print_clause_set(term s)
{
	term p;

	if (SUBSUMES(s) != _nil)
	{
		for (p = SUBSUMES(s); p != _nil; p = CDR(p))
		{
			print_set(CAR(p));
			printf("\n");
		}
		printf("-------------------------------------\n");
	}

	print_set(s);

	if (SUBSUMES(s) != _nil)
	{
		fprintf(output, " (%d)\n", COMPRESSION(s));
		printf("=====================================\n");
	}
	else
		fprintf(output, "\n");
}


/************************************************************************/
/*		Print a collection of sets that form a theory		*/
/************************************************************************/

static void
print_theory(term theory)
{
	term p;

	for (p = theory; p != NULL; p = NEXT(p))
	{
		print_set(p);
		fputc('\n', output);
		fflush(output);
	}
}


static int
p_print_theory(term goal, term *frame)
{
	term theory = check_arg(1, goal, frame, SET, EVAL);

	print_theory(theory);
	return TRUE;
}


/************************************************************************/
/* Test if the result of truncation is consistent with rest of theory	*/
/************************************************************************/

static int
consistent(term s)
{
	term p;
	int c = CLASS(CAR(SUBSUMES(s)));

	for (p = original_theory; p != NULL; p = NEXT(p))
		if (CLASS(p) != c && set_contains(p, s))
			return FALSE;

	return TRUE;
}


/************************************************************************/
/*		Calculate compression of a DUCE operator		*/
/************************************************************************/

static int
compression(term s)
{
	term p;
	int I = set_cardinality(s);
	int r = 0, total = 0, uniform = TRUE;
	int pos_lit = CLASS(s) != -1 ? CLASS(s) : CLASS(CAR(SUBSUMES(s)));

	for (p = SUBSUMES(s); p != _nil; p = CDR(p))
	{
		int c = set_cardinality(CAR(p));

		r++;
		total += c + 1;
		if (CLASS(CAR(p)) != pos_lit)
			uniform = FALSE;
	}

	if (uniform)
	{
		if (consistent(s))
		{
			OPERATOR(s) = TRUNCATION;
			COMPRESSION(s) = TRUNC(total, I);
		}
		else
		{
			OPERATOR(s) = INTRA_CONSTRUCTION;
			COMPRESSION(s) = INTRA(r, I);
		}
	}
	else
	{
		if (CLASS(s) != -1)
		{
			OPERATOR(s) = ABSORPTION;
			COMPRESSION(s) = ABSORP(r, I);
		}
		else
		{
			OPERATOR(s) = INTER_CONSTRUCTION;
			COMPRESSION(s) = INTER(r, I);
		}
	}
	return COMPRESSION(s);
}


/************************************************************************/
/*			       Encode clause				*/
/************************************************************************/

static term
encode_clause(term cl)
{
	term *p, s = new_clause_set();

	CLASS(s) = new_hash_entry(HEAD(cl), hash_table);

	for (p = BODY(cl); *p != NULL; p++)
		set_add(s, new_hash_entry(*p, hash_table));

	*last = s;
	last = &NEXT(s);

	return s;
}


/************************************************************************/
/*		   Encode a theory into a bit table			*/
/************************************************************************/

static void
encode_theory(term goal, term *frame)
{
	int i;
	double start_time = get_time();

	theory = NULL;
	last = &theory;

	for (i = 1; i <= ARITY(goal); i++)
	{
		term p = check_arg(i, goal, frame, ATOM, IN);

		for (p = PROC(p); p != NULL; p = NEXT(p))
			encode_clause(p);
	}

	fprintf(stderr, "%6.2f seconds to encode clauses\n", get_time() - start_time);
}


/************************************************************************/
/* Build a clause from a set using contents field to get values		*/
/* assoc'd with a bit							*/
/************************************************************************/

static term
decode_set(term s)
{
	int i, j, n = 1;
	unsigned long mask;
	term contents = CONTENTS(s);
	int set_size = SET_SIZE(s);
	int cardinality = set_cardinality(s);
	term rval = new_clause(cardinality);

	HEAD(rval) = (CLASS(s) == -1) ? _false : ARG(CLASS(s) + 1, contents);

	for (i = 0; i < set_size; i++)
	{
		mask = 1;
		for (j = 0; j < BITS_IN_WORD; j++)
		{
			if (SELECTOR(i, s) & mask)
				GOAL(n++, rval) = make(ARG((int)(i * BITS_IN_WORD + j + 1), contents), NULL);

			mask <<= 1;
		}
	}
	return rval;
}


/************************************************************************/
/* Build a list of clauses from a collection of sets that form a theory	*/
/************************************************************************/

static term
decode_theory(term theory)
{
	term p;
	term rval = NULL, *R = &rval;

	for (p = theory; p != NULL; p = NEXT(p))
	{
		*R = decode_set(p);
		R = &NEXT(*R);
	}

	return rval;
}


/************************************************************************/
/*		  		Copy a theory				*/
/************************************************************************/

static term
copy_theory(term t)
{
	term p, rval = copy_set(t);

	for (p = rval; NEXT(p) != NULL; p = NEXT(p))
		NEXT(p) = copy_set(NEXT(p));

	return rval;
}


/************************************************************************/
/* Find intersection of two sets and add to list if not already there	*/
/************************************************************************/

static term
new_intersection(term p, term q, term *s)
{
	term x = new_clause_set();

	if (set_intersection(p, q, x))
	{
		for (; *s != NULL; s = &NEXT(*s))
			if (set_eq(x, *s))
			{
				free_clause_set(x);
				return (*s);
			}

		if (set_eq(x, p))
			CLASS(x) = CLASS(p);
		else if (set_eq(x, q))
			CLASS(x) = CLASS(q);

		return (*s = x);
	}
	free_clause_set(x);
	return NULL;
}


/************************************************************************/
/* s subsumes p. Add p to subsumption list of s if it not already there	*/
/************************************************************************/

static void
subsumes(term s, term p)
{
	term *x;

	for (x = &SUBSUMES(s); *x != _nil; x = &CDR(*x))
		if (CAR(*x) == p)
			return;
	*x = hcons(p, _nil);
}


/************************************************************************/
/*	Build a list of pairwise intersections of clauses in a theory	*/
/************************************************************************/

static term
pairwise_intersections(void)
{
	term x, p, q, int_set = NULL;
	double start_time = get_time();

	for (p = theory; p != NULL; p = NEXT(p))
		for (q = NEXT(p); q != NULL; q = NEXT(q))
			if ((x = new_intersection(p, q, &int_set)) != NULL)
			{
				subsumes(x, p);
				subsumes(x, q);
			}

	fprintf(stderr, "%6.2lf seconds to compute intersections\n", get_time() - start_time);

	return int_set;
}


/************************************************************************/
/* Scan list of intersections and return the one with best compression	*/
/* Free all the others							*/
/************************************************************************/

static term
find_best(term int_set)
{
	term p, q, best_transform = NULL;
	int best_compression = 0;

	for (p = int_set; p != NULL; p = q)
	{
		q = NEXT(p);

		if (compression(p) > best_compression)
		{
			free_clause_set(best_transform);
			best_compression = COMPRESSION(p);
			best_transform = p;
			NEXT(p) = NULL;
		}
		else
			free_clause_set(p);
	}
	return best_transform;
}


static int interactive = 0;

static int
new_symbol(term s)
{
	term x;

	if (interactive)
	{
		print_clause_set(s);
		fprintf(stderr, "What is the name of the new concept? ");
		x = get_atom();
	}
	else
		x = gensym("z");

	return new_hash_entry(x, hash_table);
}


/************************************************************************/
/*			     The DUCE operators				*/
/************************************************************************/

static void
inter_construction(term s)
{
	term p;

	CLASS(s) = new_symbol(s);

	for (p = SUBSUMES(s); p != _nil; p = CDR(p))
	{
		term q = CAR(p);

		set_diff(q, s, q);
		set_add(q, CLASS(s));
	}

	*last = s;
	last = &NEXT(s);
}


static void
intra_construction(term s)
{
	term p;
	short new = new_symbol(s);

	CLASS(s) = CLASS(CAR(SUBSUMES(s)));

	for (p = SUBSUMES(s); p != _nil; p = CDR(p))
	{
		term q = CAR(p);

		set_diff(q, s, q);
		CLASS(q) = new;
	}

/*	if ((p = redundant(s)) != NULL)
	{
		set_add(s, CLASS(CAR(SUBSUMES(p))));
		remove_from_theory(s);
	}
	else
*/		set_add(s, new);

	*last = s;
	last = &NEXT(s);
}


static void
absorption(term s)
{
	term p;

	for (p = SUBSUMES(s); p != _nil; p = CDR(p))
	{
		term q = CAR(p);

		if (CLASS(q) == CLASS(s))
			continue;

		set_diff(q, s, q);
		set_add(q, CLASS(s));
	}
	free_clause_set(s);
}


static void
truncation(term s)
{
	CLASS(s) = CLASS(CAR(SUBSUMES(s)));
	remove_from_theory(s);
	*last = s;
	last = &NEXT(s);
}


/************************************************************************/
/*		      Choose the operator and apply it			*/
/************************************************************************/

static void
apply(term s)
{
	switch (OPERATOR(s))
	{
	   case INTER_CONSTRUCTION:	inter_construction(s);
					return;
	   case INTRA_CONSTRUCTION:	intra_construction(s);
					return;
	   case ABSORPTION:		absorption(s);
					return;
	   case TRUNCATION:		truncation(s);
					return;
	}
}


/************************************************************************/
/*			     DUCE main routine				*/
/************************************************************************/

static int
set_hash_table_size(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, INT, IN);

	if (hash_table != NULL)
		dispose(hash_table);
	max_props = IVAL(x);
	hash_table = new_hash_table(max_props);
	
	return TRUE;
}


/************************************************************************/
/*			     DUCE main routine				*/
/************************************************************************/

static term
duce(term goal, term *frame)
{
	term rewrite, rval;
	double finish_time, start_time;

	start_time = get_time();

	encode_theory(goal, frame);
	original_theory = copy_theory(theory);

	while ((rewrite = find_best(pairwise_intersections())) != NULL)
		apply(rewrite);

	finish_time = get_time();

	rval = build_plist("duce",
		    "creator",		make(goal, frame),
		    "date",		intern(date_time()),
		    "time",		new_h_real(finish_time - start_time),
		    "old_theory",	original_theory,
		    "new_theory",	theory,
		    NULL
		);

	add_to_world(rval, decode_theory(theory));

	/* Free data structures */

	return rval;
}


/************************************************************************/
/*				Initialise DUCE				*/
/************************************************************************/

void
duce_init(void)
{
	hash_table = new_hash_table(max_props);

	new_pred(set_hash_table_size,	"max_props");
	new_pred(p_print_theory,	"print_theory");
	new_subr(duce,			"duce");
}
