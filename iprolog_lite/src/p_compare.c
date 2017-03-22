#include "prolog.h"


static int
type(term goal, term *frame)
{
	goal = ARG(1, goal);

	repeat
		switch (TYPE(goal))
		{
		   case BOUND:
				goal = frame[OFFSET(goal)];
				break;
		   case REF:
				if (POINTER(goal) != NULL)
				{
					goal = POINTER(goal);
					break;
				}
		   default:
				return(TYPE(goal));
		}
}


static int
is_int(term goal, term *frame)
{
	return(type(goal, frame) == INT);
}


static int
is_float(term goal, term *frame)
{
	return(type(goal, frame) == REAL);
}


static int
is_num(term goal, term *frame)
{
	int t = type(goal, frame);

	return(t == INT || t == REAL);
}


static int
is_atom(term goal, term *frame)
{
	int t = type(goal, frame);

	return(t == ATOM);
}


static int
is_atomic(term goal, term *frame)
{
	int t = type(goal, frame);

	return(t != LIST && t != FN);
}


static int
is_var(term goal, term *frame)
{
	term t = check_arg(1, goal, frame, ANY, IN);

	repeat
	{
		switch (TYPE(t))
		{
		   case ANON:
		   case FREE:
				return(TRUE);
		   case BOUND:
				t = frame[OFFSET(t)];
				break;
		   case REF:
				if ((t = POINTER(t)) == NULL)
					return(TRUE);
				break;
		   default:
				return(FALSE);
		}
	}
}


static int
nonvar(term goal, term *frame)
{
	return(! is_var(goal, frame));
}


static int
eq(term goal, term *frame)
{
	check_arity(goal, 2);
	return(unify(ARG(1, goal), frame, ARG(2, goal), frame));
}


static int
neq(term goal, term *frame)
{
	check_arity(goal, 2);
	return(! unify(ARG(1, goal), frame, ARG(2, goal), frame));
}


int term_compare(term, term *, term, term *);

static int
cmp_fn_list(term t1, term *f1, term t2, term *f2)
{
	int result;

	switch (ARITY(t1))
	{
	case 1:
		return(-1);
	case 2:
		if ((result = term_compare(ARG(0, t1), f1, _dot, f2)) == 0
		&&  (result = term_compare(ARG(1, t1), f1, CAR(t2), f2)) == 0
		&&  (result = term_compare(ARG(2, t1), f1, CDR(t2), f2)) == 0)
			return(0);
		else
			return(result);
	default:
		return(1);
	}
}


int
term_compare(term t1, term *f1, term t2, term *f2)
{
	int i, result;

	DEREF(t1);
	DEREF(t2);

L:	if (t1 == t2) return(0);

	switch (TYPE(t1))
	{
	   case ANON:	
			if (TYPE(t2) == ANON)
				return(0);
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	   case FREE:
			copy(t1, f1);
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	   case BOUND:
			if (TYPE(t2) == BOUND)
				return(term_compare(PNAME(t1), f1, PNAME(t2), f2));
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	   case INT:
			if (TYPE(t2) == INT)
			{
				if (IVAL(t1) < IVAL(t2))
					return(-1);
				else if (IVAL(t1) == IVAL(t2))
					return(0);
				else
					return(1);
			}
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	   case REAL:
			if (TYPE(t2) == REAL)
			{
				if (RVAL(t1) < RVAL(t2))
					return(-1);
				else if (RVAL(t1) == RVAL(t2))
					return(0);
				else
					return(1);
			}
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	   case ATOM:
			if (TYPE(t2) == ATOM)
				return(strcmp(NAME(t1), NAME(t2)));
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	   case FN:
	   		if (TYPE(t2) == LIST)
	   			return(cmp_fn_list(t1, f1, t2, f2));
			if (TYPE(t2) != FN)
				return(TYPE(t1) < TYPE(t2) ? -1 : 1);
			if (ARITY(t1) < ARITY(t2))
				return(-1);
			if (ARITY(t1) > ARITY(t2))
				return(1);

			for (i = 0; i <= ARITY(t1); i++)
				if (result = term_compare(ARG(i, t1), f1, ARG(i, t2), f2))
					return(result);
			return(0);
	   case LIST:
	   		if (TYPE(t2) == FN)
	   			return(- cmp_fn_list(t2, f2, t1, f1));
			if (TYPE(t2) != LIST)
				return(TYPE(t1) < TYPE(t2) ? -1 : 1);
			if ((result = term_compare(CAR(t1), f1, CAR(t2), f2)) == 0)
			{
				t1 = CDR(t1);
				t2 = CDR(t2);
				goto L;
			}
			return(result);
	   default:
			return(TYPE(t1) < TYPE(t2) ? -1 : 1);
	}
}


static int
compare_eq(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);

	return(term_compare(x, frame, y, frame) == 0);
}


static int
compare_ne(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);

	return(term_compare(x, frame, y, frame));
}


static int
compare_lt(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);

	return(term_compare(x, frame, y, frame) == -1);
}


static int
compare_le(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);

	return(term_compare(x, frame, y, frame) <= 0);
}


static int
compare_gt(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);

	return(term_compare(x, frame, y, frame) == 1);
}


static int
compare_ge(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);

	return(term_compare(x, frame, y, frame) >= 0);
}


/************************************************************************/
/*				init					*/
/************************************************************************/

void
compare_init(void)
{
	new_pred(is_int,	"integer");
	new_pred(is_float,	"real");
	new_pred(is_num,	"number");
	new_pred(is_atom,	"atom");
	new_pred(is_atomic,	"atomic");
	new_pred(is_var,	"var");
	new_pred(nonvar,	"nonvar");
	new_pred(eq,		"=");
	
	defop(700, XFX,		new_pred(neq, "\\="));

	defop(700, XFX,		new_pred(compare_eq, "=="));
	defop(700, XFX,		new_pred(compare_ne, "\\=="));
	defop(700, XFX,		new_pred(compare_lt, "@<"));
	defop(700, XFX,		new_pred(compare_le, "@=<"));
	defop(700, XFX,		new_pred(compare_gt, "@>"));
	defop(700, XFX,		new_pred(compare_ge, "@>="));
}
