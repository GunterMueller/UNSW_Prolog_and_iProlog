/************************************************************************/
/*    Naive implementation of Plotkin's Least General Generalisation	*/
/************************************************************************/

#include "prolog.h"
#include "refine.h"

extern term *global, trail;
extern term varlist;

static term substitution, _constraint, var_trail, _subst, current_literal;
static int nvars, nvar_names;

#define SUB_FLAG	128

#define SET_SUB(x)	FLAGS(x) |= SUB_FLAG
#define UNSET_SUB(x)	FLAGS(x) &= ~SUB_FLAG
#define SUBSUMED(x)	(FLAGS(x) & SUB_FLAG)


/************************************************************************/
/* Inverse substitution is stored in a list as:				*/
/*			[$subst(V, T1, T2, [LitList]]			*/
/* where LitList is the list of literals containing the new variable	*/
/************************************************************************/
/* add_lit - put a new literal into the LitList of substiution		*/
/************************************************************************/

static void
add_lit(term lit, term *lit_list)
{
	term *p;

	for (p = lit_list; *p != _nil; p = &CDR(*p))
		if (CAR(*p) == lit)
			return;

	*p = gcons(lit, _nil);
}


/************************************************************************/
/*			Create new substitution strucuture		*/
/************************************************************************/

term
new_subst(term var, term t1, term t2, term lit)
{
	term rval = new_g_fn(4);

	nvars++;
	ARG(0, rval) = _subst;
	ARG(1, rval) = var;
	ARG(2, rval) = t1;
	ARG(3, rval) = t2;
	ARG(4, rval) = gcons(lit, _nil);
	return rval;
}

/************************************************************************/
/* Variables created for inverse substitution are always bound to dummy	*/
/* constants initially. The variables are recorded in separate trail.	*/
/************************************************************************/

static term
lgg_new_var(void)
{
	char buf[16];
	term rval = new_ref();

	sprintf(buf, "#%d", nvar_names++);
	POINTER(rval) = intern(buf);
	TRAIL(rval) = var_trail;
	var_trail = rval;
	return rval;
}


/************************************************************************/
/* Unbind all the variables in the variable trail.			*/
/************************************************************************/

void
untrail_vars()
{
	while (var_trail != NULL)
	{
		POINTER(var_trail) = NULL;
		var_trail = TRAIL(var_trail);
	}
}



/************************************************************************/
/* Print all the variables substitutions - debugging only		*/
/************************************************************************/

static void print_subst()
{
	term p;

	for (p = substitution; p != _nil; p = CDR(p))
		print(CAR(p));
}		


/************************************************************************/
/* Lookup inverse substitution or create a new one if it doesn't exist	*/
/************************************************************************/

static term
lookup(term t1, term *f1, term t2, term *f2)
{
	term rval, *p;

	for (p = &substitution; *p != _nil; p = &CDR(*p))
	{
		term antisub = CAR(*p);

		if (term_compare(t1, f1, ARG(2, antisub), NULL) == 0 &&
		    term_compare(t2, f2, ARG(3, antisub), NULL) == 0)
		{
			add_lit(current_literal, &ARG(4, antisub));
			return ARG(1, antisub);
		}
	}

/* 	*p = gcons(new_subst(rval = lgg_new_var(), t1, t2, current_literal), _nil);
 */	substitution = gcons(new_subst(rval = lgg_new_var(), t1, t2, current_literal), substitution);
	return rval;
}


/************************************************************************/
/*	Older and simple inverse substitution lookup			*/
/*	Inverse substitution is stored in a list as [V/(T1, T2)]	*/
/************************************************************************/
/*
static term
lookup(term t1, term t2)
{
	term rval, *p;

	for (p = &substitution; *p != _nil; p = &CDR(*p))
	{
		term antisub = CAR(*p);
		term pair = ARG(2, antisub);

		if (term_compare(t1, NULL, ARG(1, pair), NULL) == 0 &&
		    term_compare(t2, NULL, ARG(2, pair), NULL) == 0)
			return(ARG(1, antisub));
	}

	*p = gcons(g_fn2(intern("/"), rval = new_ref(), g_fn2(_comma, t1, t2)), _nil);
	nvars++;
	return(rval);
}
*/

/************************************************************************/
/* Reduce a list of literals by eliminating all those that contain	*/
/* variables not referenced elsewhere.					*/
/* Uses LitList in substitution to find out if variable appears in more	*/
/* than one literal, then marks literal by replacing ARG(0) by _nil	*/
/************************************************************************/

static void
reduce(term *lit_list, int *nvars, int *ngoals)
{
	term p, *q;

	for (p = *lit_list; p != _nil; p = CDR(p))
		FLAGS(CAR(p)) |= MARK;

	for (p = substitution; p != _nil; p = CDR(p))
	{
		term lit_list = ARG(4, CAR(p));
		term marked = NULL;

		for (;; lit_list = CDR(lit_list))
			if (lit_list == _nil)
			{
				FLAGS(marked) &= ~MARK;
				break;
			}
			else if (FLAGS(CAR(lit_list)) & MARK)
				if (marked == NULL)
					marked = CAR(lit_list);
				else
					break;
	}

	for (q = lit_list; *q != _nil;)
		if (FLAGS(CAR(*q)) & MARK)
		{
			FLAGS(CAR(*q)) &= ~MARK;
			q = &CDR(*q);
		}
		else
		{
			*q = CDR(*q);
			(*nvars)--;
			(*ngoals)--;
		}
}		


/************************************************************************/
/*		     lgg_term - find the lgg of two terms		*/
/************************************************************************/

static term
lgg_term(term t1, term *f1, term t2, term *f2)
{
L2:	if (t1 == t2) return t1;

	switch (TYPE(t2))
	{
	   case ANON:
			fail("Can't find lgg of anonymous variable");
	   case FREE:
			f2[OFFSET(t2)] = new_ref();
	   case BOUND:
			t2 = f2[OFFSET(t2)];
			goto L2;
	   case REF:
			if (POINTER(t2) == NULL)
				break;
			t2 = POINTER(t2);
			goto L2;
	}

L1:	if (t1 == t2) return t1;

	switch (TYPE(t1))
	{
	   case ATOM:
	 		break;
	   case ANON:
			fail("Can't find lgg of anonymous variable");
	   case FREE:
			f1[OFFSET(t1)] = new_ref();
	   case BOUND:
			t1 = f1[OFFSET(t1)];
			goto L1;
	   case REF:
			if (POINTER(t1) == NULL)
				break;
			t1 = POINTER(t1);
			goto L1;
	   case INT:
			if (TYPE(t2) == INT)
				if (IVAL(t1) == IVAL(t2))
					return copy(t1, f1);
			if (TYPE(t2) == REAL)
				if ((double) IVAL(t1) == RVAL(t2))
					return copy(t1, f1);
			break;
	   case REAL:
			if (TYPE(t2) == REAL)
				if (RVAL(t1) == RVAL(t2))
					return(copy(t1, f1));
			if (TYPE(t2) == INT)
				if (RVAL(t1) == (double) IVAL(t2))
					return copy(t1, f1);
			break;
	   case FN:
			if (TYPE(t2) == FN && ARITY(t1) == ARITY(t2) && ARG(0, t1) == ARG(0, t2))
			{
				int i, a = ARITY(t1);
		   		term rval = galloc(sizeof(compterm) + a * WORD_LENGTH);

				TYPE(rval) = TYPE(t1);
				FLAGS(rval) = COPY;
				ARITY(rval) = a;
				ARG(0, rval) = ARG(0, t1);

				if (current_literal == NULL)
					current_literal = rval;

				for (i = 1; i <= a; i++)
					ARG(i, rval) = lgg_term(ARG(i, t1), f1, ARG(i, t2), f2);

				return rval;
			}
			break;
	   case LIST:
			if (TYPE(t2) == LIST)
				return gcons(lgg_term(CAR(t1), f1, CAR(t2), f2),
					     lgg_term(CDR(t1), f1, CDR(t2), f2));
			break;
	   default:
			fail("Can't find lgg of that term");
	}

	return lookup(t1, f1, t2, f2);
}


/************************************************************************/
/* Call lgg_lit to find lgg of two literals in a clause			*/
/* An LGG is added to a clause unless it satisfies a refinement rule	*/
/************************************************************************/

static term
lgg_lit(term t1, term *f1, term t2, term *f2)
{
	term rval, constraint = NULL;
	int old_nvars = nvars;
	term old_subst = substitution;

	t1 = unbind(t1, f1);
	t2 = unbind(t2, f2);

	if (TYPE(t1) == FN && ARG(0, t1) == _constraint
	&&  TYPE(t2) == FN && ARG(0, t2) == _constraint)
		if (term_compare(ARG(2, t1), f1, ARG(2, t2), f2) == 0)
		{
			constraint = ARG(2, t1);
			t1 = ARG(1, t1);
			t2 = ARG(1, t2);
		}
		else
			return NULL;

	if (t1 == t2) return t1;
	if (TYPE(t1) != FN) return NULL;
	if (TYPE(t2) != FN) return NULL;
	if (ARG(0, t1) != ARG(0, t2)) return NULL;

	current_literal = NULL;
	rval = lgg_term(t1, f1, t2, f2);

	if (constraint != NULL)
		rval = g_fn2(_constraint, rval, constraint);

	if (refine_lgg(rval))
		return rval;

	FLAGS(rval) |= MARK;
	nvars = old_nvars;
	substitution = old_subst;
	return NULL;
}


/************************************************************************/
/*	Find the LGG of two clauses as stored in Prolog's data base	*/
/************************************************************************/


static void
fill_clause(term rval, term substitutions, term lit_list, int ngoals)
{
	term p, q;
	int i = 1;

	for (p = lit_list; p != _nil; p = CDR(p))
		FLAGS(CAR(p)) &= ~MARK;

	GOAL(0, rval) = make(CAR(lit_list), NULL);
	FLAGS(CAR(lit_list)) |= MARK;

	for (p = substitutions; p != _nil; p = CDR(p))
		for (q = ARG(4, CAR(p)); q != _nil; q = CDR(q))
			{
				if (FLAGS(CAR(q)) & MARK)
					continue;

				if (i > ngoals)
				{
					fprintf(stderr, "Wrong number of goals\n");
					exit(1);
				}

				GOAL(i++, rval) = make(CAR(q), NULL);
				FLAGS(CAR(q)) |= MARK;
			}

	for (p = lit_list; p != _nil; p = CDR(p))
		FLAGS(CAR(p)) &= ~MARK;
}


term
lgg_clause(term c1, term c2)
{
	extern term temporary_clauses;
	term f1[NVARS(c1)];
	term f2[NVARS(c2)];
	term *p, *q, rval, lit_list, *last;
	term old_varlist = varlist;
	term *old_global = global;
	term old_trail = trail;
	int i, ngoals = 0;

	temporary_clauses = _nil;
	substitution = _nil;
	var_trail = NULL;
	varlist = _nil;
	nvar_names = 0;
	nvars = 0;

	if (HEAD(c1) == HEAD(c2))
		lit_list = gcons(HEAD(c1), _nil);
	else if (TYPE(HEAD(c1)) == FN && TYPE(HEAD(c2)) == FN && ARG(0, HEAD(c1)) == ARG(0, HEAD(c2)))
	{
		term lit = lgg_lit(HEAD(c1), f1, HEAD(c2), f2);

		if (lit == NULL)
			fail("Can't find the LGG if clause heads");

		lit_list = gcons(lit, _nil);
	}
	else
		return NULL;

	last = &CDR(lit_list);

	for (p = BODY(c1); *p != NULL; p++)
		for(q = BODY(c2); *q != NULL; q++)
		{
			term lit = lgg_lit(*p, f1, *q, f2);
	
			if (lit != NULL)
			{
				*last = gcons(lit, _nil);
				last = &CDR(*last);
				ngoals++;
			}
/* 			print(lit_list);
*/		}
/* 
	reduce(&CDR(lit_list), &nvars, &ngoals);
 */
	untrail_vars();
	rval = new_clause(ngoals);
	NVARS(rval) = nvars;

#ifdef REORDER
	fill_clause(rval, substitution, lit_list, ngoals);
#else
	for (i = 0; i <= ngoals; i++, lit_list = CDR(lit_list))
		GOAL(i, rval) = make(CAR(lit_list), NULL);
#endif


	untrail(old_trail);
	varlist = old_varlist;
	global = old_global;
/*
 	fputc('\n', output);
	print(c1);
	print(c2);
	fprintf(output, "======================\n");
	print(rval);
 */
	retract_temporary_clauses();
	return rval;
}


/************************************************************************/
/*	This is the hook to call the lgg procedure from Prolog		*/
/************************************************************************/

static int
p_lgg_clause(term goal, term *frame)
{
	term c1 = check_arg(1, goal, frame, CLAUSE, IN);
	term c2 = check_arg(2, goal, frame, CLAUSE, IN);
	term c3 = check_arg(3, goal, frame, CLAUSE, OUT);
	term rval = NULL;

	if ((rval = lgg_clause(c1, c2)) == NULL)
		return FALSE;

	return unify(c3, frame, rval, frame);
}


/************************************************************************/
/* Unify arguments of two clause heads. Predicate symbol is ignore so	*/
/* that heads of positive and negative examples can be matched.		*/
/************************************************************************/

int
match_head(term h1, term *f1, term h2, term *f2)
{
	int i;

	if (ARITY(h1) != ARITY(h2))
		fail("positive and negative examples differ in the number of arguments");;

	for (i = 1; i <= ARITY(h1); i++)
		if (! unify(ARG(i, h1), f1, ARG(i, h2), f2))
			return FALSE;
	return TRUE;
}


/************************************************************************/
/* A call to numbervars is needed to prevent unwanted variable matches	*/
/************************************************************************/

void
numvars(term t, term *f)
{
	static int count = 0;
	term *p;

	for (p = &HEAD(t); *p != NULL; p++)
	{
		make_ref(*p, f);
		numbervars(*p, f, &count);
	}
}


/************************************************************************/
/************************************************************************/

static term
copy_clause(term cl, term *frame)
{
	int i, n = 0;
	term *p, rval;

	for (p = &HEAD(cl); *p != NULL; p++)
		n++;

	rval = new_clause(n-1);

	for (i = 0; i < n; i++)
		GOAL(i, rval) = copy(GOAL(i, cl), frame);

	return rval;
}

/************************************************************************/
/* Called from "subsumes"						*/
/* tests if body of first clause is contained in second			*/
/************************************************************************/

static int
subset(term *b1, term *f1, term *b2, term *f2)
{
	term *q;

	if (*b1 == NULL)
		return(TRUE);

	for (q = b2; *q != NULL; q++)
	{
		term *old_global = global;
		term old_trail = trail;

		if (unify(*b1, f1, *q, f2))
			if (subset(b1+1, f1, b2, f2))
				return TRUE;

		untrail(old_trail);
		global = old_global;
	}
	return FALSE;
}


/************************************************************************/
/* Test if one clause subsumes another.					*/
/* First match heads of clause and then check if body of first clause	*/
/* is a subset of body of second clause.				*/
/************************************************************************/

static int
subsumes(term c1, term c2)
{
	term f1[NVARS(c1)];
	term f2[NVARS(c2)];
	term *old_global = global;
	term old_trail = trail;
	int rval = FALSE;
/* 
	fprintf(output, "*** SUBSUMPTION TEST ***\n");
	print(c1);
	print(c2);
	fprintf(output, "*************************\n");
 */
	c2 = copy_clause(c2, f2);
	numvars(c2, f2);

	if (match_head(HEAD(c1), f1, HEAD(c2), f2))
		rval = subset(BODY(c1), f1, BODY(c2), f2);

	untrail(old_trail);
	global = old_global;
	dispose(c2);
	return rval;
}


/************************************************************************/
/*	   	 Check if example is covered by a new LGG		*/
/************************************************************************/

static void
succeed(term *result, term varlist, term *frame)
{
	*result = _true;
}

static int
covered(term c1, term c2)
{
	term f1[NVARS(c1)];
	term f2[NVARS(c2)];
	int rval = FALSE;

	if (match_head(HEAD(c1), f1, HEAD(c2), f2))
		rval = call_prove(BODY(c2), f2, _nil, 1, succeed, TRUE) != _nil;
/* 
	fflush(output);
	fprintf(output, "%d: ", rval);
	rprint(c1, f1);
	fflush(output);
 */
	return rval;
}


/************************************************************************/
/* Mark positive examples that are covered				*/
/* Examples may be ground unit clauses (call "covered")			*/
/*	or non-unit clauses (call "subsumes").				*/
/************************************************************************/

int
mark_covered(term c, term clause_list)
{
	term q ;
	int count = 0;

	for (q = clause_list; q != NULL; q = NEXT(q))
		if (! SUBSUMED(q))
			if (GOAL(1, q) == NULL)
			{
				if (covered(q, c))
				{
					SET_SUB(q);
					count++;
				}
			}
			else if (subsumes(c, q))
			{
				SET_SUB(q);
				count++;
			}
	return count;
}


/************************************************************************/
/* Loop through list of examples counting clauses that are covered	*/
/* Examples may be ground unit clauses (call "covered")			*/
/*	or non-unit clauses (call "subsumes").				*/
/************************************************************************/

int
count_cover(term c, term clause_list)
{
	term q ;
	int count = 0;

	for (q = clause_list; q != NULL; q = NEXT(q))
		if (GOAL(1, q) == NULL)
		{
			if (covered(q, c))
				count++;
		}
		else if (subsumes(c, q))
			count++;

	return count;
}


int
count_neg_cover(term c, term neglist)
{
	term p = neglist;
	int sum = 0;

	while (p != _nil)
	{
		term q = CAR(p);

		if (TYPE(q) != ATOM)
			fail("List of negative predicates must only contain atoms");
		if ((q = PROC(q)) == NULL)
			fail("Undefined negative relation");

		sum += count_cover(c, q);

		p = CDR(p);
		DEREF(p);
	}
	return sum;
}


/************************************************************************/
/* Loop through list of negative examples to check if clause covers one	*/
/* Negative examples may be ground unit clauses (call "covered")	*/
/*	or non-unit clauses (call "subsumes").				*/
/************************************************************************/

int
covers_neg(term c, term neglist)
{
	term p = neglist;

	while (p != _nil)
	{
		term q = CAR(p);

		if (TYPE(q) != ATOM)
			fail("List of negative predicates must only contain atoms");
		if ((q = PROC(q)) == NULL)
			fail("Undefined negative relation");

		while (q != NULL)
		{
			if (GOAL(1, q) == NULL)
			{
				if (covered(q, c))
				{
					fflush(output);
					fprintf(output, "Covers neg: ");
					print(q);
					return TRUE;
				}
			}
			else if (subsumes(c, q))
				{
					fflush(output);
					fprintf(output, "Covers neg: ");
					print(q);
					return TRUE;
				}

			q = NEXT(q);
		}

		p = CDR(p);
		DEREF(p);
	}
	return FALSE;
}


/************************************************************************/
/* Insert a new LGG into clause list.					*/
/* If a clause already subsumes new one, forget it			*/
/* Remove from list any clauses subsumed by new one			*/
/************************************************************************/

static void
insert(term new_clause, term *clause_list)
{
	term *last = clause_list;

	while (*last != NULL)
		if (subsumes(*last, new_clause))
			return;
		else if (subsumes(new_clause, *last))
			*last = NEXT(*last);
		else
			last = &NEXT(*last);

	*last = new_clause;
}


/************************************************************************/
/*	Find the LGG of a set of clauses stored in a single relation	*/
/************************************************************************/

static term
lgg_reln(term clause_list, term neglist)
{
	term p, q, lgg;
	term rval = NULL;
	int changed = FALSE;

	for (p = clause_list; p != NULL; p = NEXT(p))
		UNSET_SUB(p);
		
/*
 * 	Find all pairwise LGG's of clauses in relation.
 * 	Skip LGG's that cover negative examples.
 */
	
	for (p = clause_list; p != NULL; p = NEXT(p))
		for (q = NEXT(p); q != NULL; q = NEXT(q))
			if ((lgg = lgg_clause(p, q)) != NULL && ! covers_neg(lgg, neglist))
			{
/* 				fprintf(output, "LGG: "); list_proc(lgg); fflush(output);
*/ 				if (subsumes(p, lgg) || subsumes(q, lgg))
				{
					fprintf(output, "SKIP\n");
					continue;
				}
 				changed = TRUE;
/* 				fprintf(output, "INSERT: "); list_proc(lgg); fflush(output);
 */				SET_SUB(p);
				SET_SUB(q);
				UNSET_SUB(lgg);
				insert(lgg, &rval);
			}
			else
				free_term(lgg);

	if (! changed)
		return NULL;

	p = clause_list;
	while (p != NULL)
	{
		q = NEXT(p);
		if (! SUBSUMED(p))
		{
			NEXT(p) = NULL;
			insert(p, &rval);
		}
		p = q;
	}

	return rval;
}


/************************************************************************/
/*	Prolog hook for call to lgg of a relation			*/
/*	Clauses are stored as usual in Prolog's database		*/
/************************************************************************/

static int
p_lgg_reln(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, ATOM, IN);
	term neglist = check_arg(2, goal, frame, LIST, IN);
	term rval, x;

	if (PROC(pos) == NULL)
		fail("Undefined relation");

	rval = lgg_reln(PROC(pos), neglist);

	while (x  = lgg_reln(rval, neglist))
		rval = x;

	list_proc(rval);
	return TRUE;
}


/************************************************************************/
/*	Prolog hook for call to lgg of a relation			*/
/*	Clauses are stored in a frame as the result of a refinement	*/
/************************************************************************/

static int
p_lgg_frame(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, ATOM, IN);
	term neglist = check_arg(2, goal, frame, LIST, IN);
	term rval, x;

	if ((pos = getprop(pos, intern("rule"))) == NULL)
		fail("Undefined relation");

	rval = lgg_reln(pos, neglist);
	list_proc(rval);
	fprintf(output, "______________________\n");
	fflush(output);

	while (x = lgg_reln(rval, neglist))
	{
		rval = x;
		list_proc(rval);
		fprintf(output, "______________________\n");
		fflush(output);
	}

	list_proc(rval);
	return TRUE;
}


/************************************************************************/
/* These procedures are for clauses represented as comma-separated	*/
/* compound terms.							*/
/************************************************************************/

static term *
append(term *conjunction, term lgg)
{
	if (lgg != NULL)
		if (*conjunction == NULL)
			*conjunction = lgg;
		else
		{
			*conjunction = g_fn2(_comma, *conjunction, lgg);
			conjunction = &ARG(2, *conjunction);
		}
	return conjunction;
}


static term *
lgg_body_lit(term lit, term *lit_frame, term body, term *body_frame, term *rval)
{
	term x;

	for (x = unbind(body, body_frame); ARG(0, x) == _comma; x = unbind(ARG(2, x), body_frame))
		rval = append(rval, lgg_lit(lit, lit_frame, ARG(1, x), body_frame));

	rval = append(rval, lgg_lit(lit, lit_frame,  x, body_frame));

	return rval;
}


static term
lgg_body(term t1, term *f1, term t2, term *f2)
{
	term x, rval = NULL, *p = &rval;

	t1 = unbind(t1, f1);
	t2 = unbind(t2, f2);

	for (x = t1; ARG(0, x) == _comma; x = unbind(ARG(2, x), f1))
		p = lgg_body_lit(ARG(1, x), f1, t2, f2, p);

	lgg_body_lit(x, f1, t2, f2, p);
	return rval;
}


static term
lgg_comma(term t1, term *f1, term t2, term *f2)
{
	if (ARG(0, t1) == _neck && ARG(0, t2))
	{
		term lgg_h, lgg_b;

		if ((lgg_h = lgg_lit(ARG(1, t1), f1, ARG(1, t2), f2)) == NULL)
			return NULL;

		if ((lgg_b = lgg_body(ARG(2, t1), f1, ARG(2, t2), f2)) == NULL)
			return(lgg_h);

		return g_fn2(_neck, lgg_h, lgg_b);
	}
	return lgg_lit(t1, f1, t2, f2);
}


static int
lgg(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, FN, IN);
	term y = check_arg(2, goal, frame, FN, IN);
	term l = check_arg(3, goal, frame, FN, OUT);
	term s = check_arg(4, goal, frame, LIST, OUT);
	term rval;

	substitution = _nil;
	nvars = 0;

	if ((rval = lgg_comma(x, frame, y, frame)) == NULL)
		return FALSE;

	unify(l, frame, rval, frame);
	unify(s, frame, substitution, frame);

	return TRUE;
}


/************************************************************************/
/*			   Initialise Module				*/
/************************************************************************/

void
lgg_init(void)
{
	_constraint = intern("\\");
	_subst = intern("$subst");

	new_pred(p_lgg_clause,		"lgg_clause");
	new_pred(p_lgg_reln,		"lgg_reln");
	new_pred(p_lgg_frame,		"lgg_frame");
	new_pred(lgg,			"lgg");
}
