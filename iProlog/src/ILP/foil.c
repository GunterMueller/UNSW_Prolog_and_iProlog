/************************************************************************/
/*    		    FOIL search used with refinements			*/
/************************************************************************/

#include "prolog.h"
#include <limits.h>

void mark_covered(term, term);
int count_cover(term, term);
int covers_neg(term, term);

#define COVERED_FLAG		128

#define MARK_COVERED(x)		FLAGS(x) |= COVERED_FLAG
#define UNMARK_COVERED(x)	FLAGS(x) &= COVERED_FLAG
#define COVERED(x)		(FLAGS(x) & COVERED_FLAG)


/************************************************************************/
/* Test all the unmarked positive examples against the given clause	*/
/* Mark the clauses that are covered.					*/
/************************************************************************/

static void
mark_covered_examples(term c, term pos)
{
	term p;

	for (p = pos; p != NULL; p = NEXT(p))
		if (! COVERED(p) && covered(p, c))
			MARK_COVERED(p);
}


/************************************************************************/
/* Select the literal in s that give the best information gain.		*/
/* Return NULL if no specialisation improves the gain.			*/
/************************************************************************/

static term
best_literal(term s, term neglist)
{
}


/************************************************************************/
/*	Call best_literal repeatedly to specialise a clause.		*/
/************************************************************************/


static term
find_clause(term e, term neglist)
{
	term *p, *q, rval, lit_list, *last;
	term *old_global = global;
	term old_trail = trail;
	int i, ngoals = 0;
/* 
	skip through list of positive examples looking for ones
	not yet marked as covered
*/
	while (e != NULL && COVERED(e))
		e = NEXT(e);

	if (e == NULL)
		return NULL;


	s = variablise(saturate(e));

	lit_list = gcons(HEAD(s), _nil);

	while ((lit = best_literal(s, neglist)) != NULL)
	{
		*last = gcons(lit, _nil);
		last = &CDR(*last);
		ngoals++;
	}

	untrail_vars();
	rval = new_clause(ngoals);
	NVARS(rval) = nvars;

	for (i = 0; i <= ngoals; i++, lit_list = CDR(lit_list))
		GOAL(i, rval) = make(CAR(lit_list), NULL);

	untrail(old_trail);
	global = old_global;
	return rval;
}


/************************************************************************/
/* Unmark all positive examples.					*/
/* Call find_clause as long as some positive examples remain uncovered	*/
/************************************************************************/

static term
foil(term pos, term neglist)
{
	term p, clause_list = NULL;
	term *last = &clause_list;

	for (p = pos; p != NULL; p = NEXT(p))
		UNMARK_COVERED(p);

	while ((p = find_clause(pos, neglist)) != NULL)
	{
		*last = p;
		last = &NEXT(p);

		mark_covered_examples(p, pos);
	}

	return clause_list;
}


/************************************************************************/
/*	Prolog hook for call to golem on a relation			*/
/*	Clauses are stored as usual in Prolog's database		*/
/************************************************************************/

static int
p_foil(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, ATOM, IN);
	term neglist = check_arg(2, goal, frame, LIST, IN);
	term rval, x;

	if (PROC(pos) == NULL)
		fail("Undefined relation");

	rval = foil(PROC(pos), neglist);

	list_proc(rval);
	return TRUE;
}


/************************************************************************/
/*			   Initialise Module				*/
/************************************************************************/

void
foil_init(void)
{
	new_pred(p_foil, "foil");
}
