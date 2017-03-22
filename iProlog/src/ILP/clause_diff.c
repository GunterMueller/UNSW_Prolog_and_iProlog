/************************************************************************/
/*    		Find the difference between two clauses			*/
/* 		WARNING!!!! This program is incomplete			*/
/************************************************************************/

#include "prolog.h"

static term _eq, substitution;


/************************************************************************/
/* Print members of a list in separate lines - debugging only		*/
/************************************************************************/

static void
print_list(term L)
{
	term p = L;

	fputs("[\n", output);
	while (p != _nil)
	{
		fputc('\t', output);
		print(CAR(p));
		p = CDR(p);
		DEREF(p);
	}
	fputs("]\n", output);
}		


static int
p_print_list(term goal, term *frame)
{
	term L = check_arg(1, goal, frame, LIST, IN);

	print_list(L);

	return TRUE;
}		


/************************************************************************/
/* Lookup inverse substitution or create a new one if it doesn't exist	*/
/************************************************************************/

static int
lookup(term t1, term t2)
{
	term p;

	for (p = substitution; p != _nil; p = CDR(p))
	{
		term antisub = CAR(p);
		int cmp1 = term_compare(t1, NULL, ARG(1, (antisub)), NULL);
		int cmp2 = term_compare(t2, NULL, ARG(2, (antisub)), NULL);

		if (cmp1 == 0 && cmp2 == 0)
			return(TRUE);
		if (cmp1 == 0 || cmp2 == 0)
			return(FALSE);
	}

	substitution = gcons(g_fn2(_eq, t1, t2), substitution);
	return TRUE;
}


/************************************************************************/
/*		     		match_term				*/
/************************************************************************/

static int
match_term(term t1, term t2)
{
L1:	if (t1 == t2) return(TRUE);

	switch (TYPE(t1))
	{
	   case ATOM:
	 		break;
	   case INT:
			if (TYPE(t2) == INT && IVAL(t1) == IVAL(t2)
			||  TYPE(t2) == REAL && ((double) IVAL(t1)) == RVAL(t2))
				return(TRUE);
			break;
	   case REAL:
			if (TYPE(t2) == REAL && RVAL(t1) == RVAL(t2)
			|| TYPE(t2) == INT && RVAL(t1) == ((double) IVAL(t2)))
				return(TRUE);
			break;
	   case FN:
			if (TYPE(t2) == FN && ARITY(t1) == ARITY(t2))
			{
				int i;

				for (i = 0; i <= ARITY(t1); i++)
					if (! match_term(ARG(i, t1), ARG(i, t2)))
						return(FALSE);
				return(TRUE);
			}
			break;
	   case LIST:
			if (TYPE(t2) == LIST && match_term(CAR(t1), CAR(t2)))
			{
				t1 = CDR(t1);
				t2 = CDR(t2);
				goto L1;
			}
			break;
	   default:
			fail("Can only take the difference of ground clauses");
	}

	return lookup(t1, t2);
}


/************************************************************************/
/* Call lgg_lit to find lgg of two literals in a clause			*/
/* An LGG is added to a clause unless it satisfies a refinement rule	*/
/************************************************************************/

static int
match_lit(term t1, term t2)
{
	if (t1 == t2) return(TRUE);
	if (TYPE(t1) != FN || TYPE(t2) != FN)
		return(FALSE);
	if (ARG(0, t1) != ARG(0, t2))
		return(FALSE);

	return match_term(t1, t2 );
}


/************************************************************************/
/*	Try to match a literal in the body of another clause		*/
/************************************************************************/
/*
static term
body_diff(term c1, term c2, int dirn)
{
	term diff = _nil, *dp = &diff;
	term *p, *q;

	for (p = BODY(c1); *p != NULL; p++)
	{
		term old_subst = substitution;

		for (q = BODY(c2); *q != NULL; q++)
			if (dirn == 0 && match_lit(*p, *q)
			 || dirn == 1 && match_lit(*q, *p))
			{
				print(*p);
				print(*q);
				fprintf(output, "------------\n");
				break;
			}
			else
				substitution = old_subst;

		if (*q == NULL)
		{
			*dp = gcons(*p, _nil);
			dp = &CDR(*dp);
		}
	}

	return(diff);
}
*/

static term
body_diff(term *c1, term *c2, int dirn)
{
	term *q;

	if (*c1 == NULL)
	{
/*		print_list(substitution);
 */		return(_nil);
	}

	for (q = c2; *q != NULL; q++)
	{
		term old_subst = substitution;

		if (dirn == 0 && match_lit(*c1, *q)
		 || dirn == 1 && match_lit(*q, *c1))
		 {
		 	prin(*c1); fprintf(output, " <=> "); print(*q);
			print_list(body_diff(c1+1, c2, dirn));
//			fflush(output);
//			getchar();
		}

		substitution = old_subst;
	}

	return gcons(*c1, body_diff(c1+1, c2, dirn));
}


/************************************************************************/
/* Return two lists - the differences between the two input clauses	*/
/************************************************************************/

static int
clause_diff(term c1, term c2, term *d1, term *d2)
{
	substitution = _nil;

	if (! match_lit(HEAD(c1), HEAD(c2)))
		return FALSE;

	*d1 = body_diff(BODY(c1), BODY(c2), 0);
 	*d2 = body_diff(BODY(c2), BODY(c1), 1);

	return TRUE;
}


/************************************************************************/
/*	This is the hook to call the lgg procedure from Prolog		*/
/************************************************************************/

static int
p_clause_diff(term goal, term *frame)
{
	term c1 = check_arg(1, goal, frame, CLAUSE, IN);
	term c2 = check_arg(2, goal, frame, CLAUSE, IN);
	term d1 = check_arg(3, goal, frame, LIST, OUT);
	term d2 = check_arg(4, goal, frame, LIST, OUT);
	term s = check_arg(5, goal, frame, LIST, OUT);
	term diff1, diff2;

	if (! clause_diff(c1, c2, &diff1, &diff2))
		return FALSE;
/*
	print(diff1);
	getchar();
	print(diff2);
	getchar();
	print(substitution);
	getchar();
*/
	return
		unify(d1, frame, diff1, NULL) &&
		unify(d2, frame, diff2, NULL) &&
		unify(s, frame, substitution, NULL);
}


/************************************************************************/
/*			   Initialise Module				*/
/************************************************************************/

void
clause_diff_init(void)
{
	_eq = intern("=");

	new_pred(p_clause_diff,	"clause_diff");
	new_pred(p_print_list,	"print_list");
}
