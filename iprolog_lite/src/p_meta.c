#include "prolog.h"
#include "env.h"


static void
succeed(term *result, term varlist, term *frame)
{
	*result = _true;
}

/************************************************************************/
/*			Collect results in a bag			*/
/************************************************************************/

static void
nconc(term *x, term y)
{
	while (*x != _nil)
		x = &(CDR(*x));
	*x = y;
}


static void
collect(term *result, term vars, term *frame)
{
	extern term varlist;

 	varlist = _nil;
 	nconc(result, hcons(make(vars, frame), _nil));
}


/************************************************************************/
/*				Find all solutions			*/
/************************************************************************/

static int
findall(term goal, term *frame)
{
	int rval;
	term fn, tmp, q[2] = {NULL, NULL};

	check_arity(goal, 3);
	fn = copy(ARG(1, goal), frame);
	*q = copy(ARG(2, goal), frame);
	DEREF(*q);
	tmp = call_prove(q, local, fn, -1, collect, TRUE);
	rval = unify(ARG(3, goal), frame, tmp, local);
	free_term(tmp);
	return(rval);
}


term
all(term goal, term *frame)
{
	term fn, tmp, rval, q[2] = {NULL, NULL};

	check_arity(goal, 2);
	fn = copy(ARG(1, goal), frame);
	*q = eval(ARG(2, goal), frame);
/*	DEREF(*q);
*/
	tmp = call_prove(q, local, fn, -1, collect, TRUE);
	rval = copy(tmp, local);
	free_term(tmp);
	return(rval);
}


/************************************************************************/
/*			Apply a given clause to a term			*/
/************************************************************************/

static int
apply_clause(term goal, term *frame)
{
	term t = check_arg(1, goal, frame, FN, IN);
	term cl = check_arg(2, goal, frame, CLAUSE, IN);
	term *current_frame = local;

	MAKE_FRAME(NVARS(cl));
	if (! unify(t, frame, HEAD(cl), current_frame))
	{
		_untrail();
		return(FALSE);
	}
	return(call_prove(BODY(cl), current_frame, _nil, 1, succeed, FALSE) != _nil);
}



/************************************************************************/
/*			User callable meta predicates			*/
/************************************************************************/

static int
p_cut(term goal, term *frame)
{
	cut(top_of_stack);
	return(TRUE);
}


static int
true(term t, term *frame)
{
	return(TRUE);
}


static int
_fail(term t, term *frame)
{
	return(FALSE);
}


static int
l_brace(term goal, term *frame)
{
	term q[2];

	q[0] = check_arg(1, goal, frame, ANY, IN);
	q[1] = NULL;

	prove(q, top_of_stack);
	if (top_of_stack -> cut)
		cut(top_of_stack);
	return(FALSE);
}


static int
not(term goal, term *frame)
{
	int rval;
	term q[2] = {NULL, NULL};

	q[0] = check_arg(1, goal, frame, ANY, IN);

	return(call_prove(q, frame, _nil, 1, succeed, TRUE) == _nil);
}


static int
and(term goal, term *frame)
{
	term q[3];

	q[0] = check_arg(1, goal, frame, ANY, IN);
	q[1] = check_arg(2, goal, frame, ANY, IN);
	q[2] = NULL;

	prove(q, top_of_stack);
	return(FALSE);
}


static int
arrow(term goal, term *frame)
{
	env current_env = top_of_stack;
	term q[2] = {NULL, NULL};

	*q = check_arg(1, goal, frame, ANY, IN);
	if (cond(q, frame))
	{
		*q = check_arg(2, goal, frame, ANY, IN);
		return(prove(q, current_env));
	}
	return(FALSE);
}


static int
or(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);
	term q[2] = {NULL, NULL};
	env current_env = top_of_stack;

	make_ref(x, frame);
	make_ref(y, frame);

	if (TYPE(x) == FN && ARG(0, x) == _arrow && ARITY(x) == 2)
	{
		*q = ARG(1, x);
		if (cond(q, frame))
		{
			*q = ARG(2, x);
			prove(q, current_env);
			return(FALSE);
		}
	}
	else
	{
		*q = x;
		prove(q, current_env);
		if (current_env -> cut)
			return(FALSE);
	}

	*q = y;
	prove(q, current_env);
	return(FALSE);
}


static int
unless(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, IN);
	term q[2] = {NULL, NULL};

	if (TYPE(x) == FN && ARITY(x) == 2 && ARG(0, x) == _arrow)
	{
		*q = ARG(1, x);
		make_ref(*q, frame);
		if (! cond(q, frame))
			return(FALSE);
	
		*q = y;
		make_ref(*q, frame);
		if (cond(q, frame))
			return(TRUE);
	
		*q = ARG(2, x);
		make_ref(*q, frame);
		return(cond(q, frame));
	}
	else
	{
		*q = y;
		make_ref(*q, frame);
		if (cond(q, frame))
			return(TRUE);
	
		*q = ARG(1, x);
		make_ref(*q, frame);
		return(cond(q, frame));
	}
}


static int
_repeat(term goal, term *frame)
{
	term *old_global = global;
	while (! rest_of_clause())
	{
		_untrail();
		global = old_global;
	}
	return(FALSE);
}


/************************************************************************/
/*				init					*/
/************************************************************************/

term _unless;

void
meta_init()
{
	new_pred(findall, "findall");
	new_fsubr(all, "all");
	defop(800, XFX, new_pred(apply_clause, "wrt"));

	new_pred(p_cut, "!");
	new_pred(true, "true");
	new_pred(_fail, "fail");
  	new_pred(not, "not");
	new_pred(and, ",");
	new_pred(l_brace, "{");
	defop(950, XFX, new_pred(l_brace, "\\"));
	new_pred(arrow, "->");
	new_pred(or, "|");
	new_pred(or, ";");
	defop(1075, XFY, _unless = new_pred(unless, "unless"));
	new_pred(_repeat, "repeat");
	defop(1140, XFX, new_pred(and, "checking"));
}
