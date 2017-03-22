/*		C implementation of basic AQ algorithm			*/

#ifdef PROFILE
#include <profile.h>
#endif

#include "attr_info.h"
#include "set.h"
#include "heap.h"

#define MAX_STAR	15

static int MaxStar = 5;
static term star[MAX_STAR], new_star[MAX_STAR];
static term Examples, DataSet;
static data_struct *data_info;


/************************************************************************/
/* c1 is better than c2 if it covers more +ve examples			*/
/* or it has fewer literals						*/
/************************************************************************/


static int
simpler(term c1, term c2)
{
	if (NPOS(c1) > NPOS(c2))
		return 1;
	if (NPOS(c1) < NPOS(c2))
		return 0;

	return (NSEL(c1) < NSEL(c2));
}


/************************************************************************/
/* Measure distance between examples by number of differing attributes	*/
/************************************************************************/

int
distance(term x, term y)
{
	int i, count = 0;
	unsigned long *p = x -> z.sel, *q = y -> z.sel;

	for (i = SET_SIZE(x); i != 0; i--, p++, q++)
		if (*p != *q)
			count++;
	return count;
}


/************************************************************************/
/* Given a set representing an example, scan bit map for examples that	*/
/* match set. The result is a bit slice where a one represents a hit.	*/
/* This is complicated by the fact that in the attr/val representation,	*/
/* slices within a selector must be unioned and then all those are	*/
/* intersected.								*
/* Finally find size of interesection of cover with class for the rule	*/
/************************************************************************/

static void
count_cover(term rule)
{
	term tmp = new_set(data_info -> n_words, data_info -> index);
	term covered = copy_set(data_info -> data_set);
	int attr, n_attr = data_info -> n_attributes;
	attribute_struct *attribute = data_info -> attribute;

	for (attr = 0; attr < n_attr; attr++)
	{
		unsigned long sel = SELECTOR(attr, rule);
		int i, n_values = attribute[attr].n_values;

		if (sel == 0 || sel == ALL_BITS)
			continue;

		clear_set(tmp);
		for (i = 0; i < n_values; i++)
			if (sel & (1L << i))
				set_union(tmp, attribute[attr].value[i].instances, tmp);

		set_intersection(covered, tmp, covered);
	}

	NPOS(rule) = intersection_size(covered, data_info -> class[CLASS(rule)].instances);
	free_term(tmp);
	free_term(covered);
}


/************************************************************************/
/*   The star is stored in a fixed size array. This initialises it.	*/
/************************************************************************/

static void
init_star(term *s)
{
	int i;

	for (i = 0; i < MaxStar; i++)
		s[i] = NULL;
}


/************************************************************************/
/*			Check if star contains an example		*/
/************************************************************************/

static int
star_contains(term *s, term x)
{
	int i;

	for (i = 0; i < MaxStar && s[i] != NULL; i++)
		if (set_contains(s[i], x))
			return TRUE;

	return FALSE;
}


/************************************************************************/
/* Insert new clause into star keeping only MaxStar best complexes	*/
/************************************************************************/

static void 
insert(term c, term *s)
{
	int i, j;

	count_cover(c);

	for (i = 0; i < MaxStar; i++)
		if (s[i] == NULL || simpler(c, s[i]))
		{
			if (s[MaxStar - 1] != NULL)
				free(s[MaxStar - 1]);

			for (j = MaxStar - 1; j > i; j--)
				s[j] = s[j - 1];

			s[i] = c;
			return;
		}
	free(c);
}


/************************************************************************/
/* First clause in star buffer should be the best. Free the rest.	*/
/************************************************************************/

static term 
get_best(term *s)
{
	int i;
	term p = s[0];

	s[0] = NULL;
	for (i = 1; i < MaxStar; i++)
	{
		if (s[i] != NULL)
			free(s[i]);
		s[i] = NULL;
	}
	return p;
}


/************************************************************************/
/* Generate the set of all single variable complexes which cover e but	*/
/* not eminus and multiply with the current partial start		*/
/************************************************************************/

static void 
extend_multiply(term e, term eminus, term *star)
{
	unsigned long bitstring;
	int n, i, Nselectors = SET_SIZE(e);

	init_star(new_star);

	for (n = 0; n < Nselectors; n++)
	{
		if ((SELECTOR(n, e) & SELECTOR(n, eminus)) != 0)
			continue;
		if ((bitstring = ~SELECTOR(n, eminus)) == ALL_BITS)
			continue;

		for (i = 0; i < MaxStar && star[i] != NULL; i++)
		{
			term c = copy_set(star[i]);
			if (SELECTOR(n, c) == ALL_BITS)
				++NSEL(c);
			SELECTOR(n, c) &= bitstring;

			insert(c, new_star);
		}
	}

	if (*new_star != NULL)
		for (i = 0; i < MaxStar; i++)
		{
			if (star[i] != NULL)
				free(star[i]);
			star[i] = new_star[i];
		}
}


/************************************************************************/
/* Generate star by creating all the different complexes that can cover	*/
/* the positive events and exclude all negative events			*/
/* The negative events are sorted by distance from the seed		*/
/************************************************************************/

static term 
gen_star(term e)
{
	term new_complex(term);
	term q_remove(void);
	term Eminus;

	init_star(star);

	*star = new_complex(e);
	fill_q(e, Examples);
	while ((Eminus = q_remove()) != NULL)
		if (star_contains(star, Eminus))
			extend_multiply(e, Eminus, star);

	return get_best(star);
}


/************************************************************************/
/*	Add a new complex to a cover, orderered by simplicity		*/
/************************************************************************/

static void
add_to_cover(term complex, term *cover)
{
	while (*cover != NULL && ! simpler(complex, *cover))
		cover = &NEXT(*cover);

	NEXT(complex) = *cover;
	*cover = complex;
}


/************************************************************************/
/*				Append two covers			*/
/************************************************************************/

static void
append_to_cover(term complex, term *cover)
{
	while (*cover != NULL)
		cover = &NEXT(*cover);
	*cover = complex;
}


/************************************************************************/
/*		Check consistency of a cover wrt examples		*/
/************************************************************************/

static int
check_cover(term cover, term examples)
{
	term p;
	int missed_pos = 0, missed_neg = 0;

	for (p = examples; p != NULL; p = NEXT(p))
		if (CLASS(p) == CLASS(cover))
			missed_pos += (disj_contains(cover, p) == FALSE);
		else
			missed_neg += (disj_contains(cover, p) != FALSE);

	if (missed_pos != 0)
		fprintf(stderr, "%d positive examples misclassified\n", missed_pos);
	if (missed_neg != 0)
		fprintf(stderr, "%d negative examples misclassified\n", missed_neg);

	return (missed_pos || missed_neg);
}


/************************************************************************/
/* The main AQ algorithm scans through the list of positive examples	*/
/* building a complex from each by extending against negative examples	*/
/************************************************************************/

term 
aq(int class)
{
	term pos, best_complex, cover = NULL;

	for (pos = Examples; pos != NULL; pos = NEXT(pos))
		if (CLASS(pos) == class && ! disj_contains(cover, pos))
		{
			best_complex = gen_star(pos);
			add_to_cover(best_complex, &cover);
		}

	/* check_cover(cover, Examples); */
	return cover;
}


/************************************************************************/
/*		Find a cover for each class that declared		*/
/************************************************************************/

static term
find_cover(term goal, term *frame)
{
	int i, n_classes;
	term relation_name = check_arg(1, goal, frame, ATOM, IN);
	term rval, cover = NULL;
	double finish_time, start_time;

#ifdef PROFILE
	InitProfile(200, 200);
#endif

	data_info = new_data_set(TRUE, relation_name);
	n_classes = data_info -> n_classes;
	DataSet = data_info -> data_set;
	Examples = ARG(1, CONTENTS(DataSet));

	create_q(data_info -> n_examples);
	start_time = get_time();

	for (i = 0; i < n_classes; i++)
		append_to_cover(aq(i), &cover);

	finish_time = get_time();

	rval = build_plist("aq",
			"creator",	make(goal, frame),
			"date",		intern(date_time()),
			"n_examples",	new_h_int(data_info -> n_examples),
			"time",		new_h_real(finish_time - start_time),
			NULL
	);
	add_to_world(rval, cover_to_clause(cover));

	free_disj(cover);
#ifdef PROFILE
	DumpProfile();
#endif
	return rval;
}


void
aq_init(void)
{
	new_subr(find_cover, "aq");
}
