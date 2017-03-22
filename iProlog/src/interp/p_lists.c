/************************************************************************/
/*				Built-in list predicates		*/
/************************************************************************/

#include <string.h>
#include "prolog.h"

/************************************************************************/
/*		Build list terms in the global stack or the heap	*/
/************************************************************************/

term
gcons(term car, term cdr)
{
	term rval = galloc(sizeof(compterm) + WORD_LENGTH);

	TYPE(rval) = LIST;
	FLAGS(rval) = COPY;
	ARITY(rval) = 1;
	CAR(rval) = car;
	CDR(rval) = cdr;
	return rval;
}


term
hcons(term car, term cdr)
{
	term rval = halloc(sizeof(compterm) + WORD_LENGTH);

	TYPE(rval) = LIST;
	FLAGS(rval) = 0;
	ARITY(rval) = 1;
	CAR(rval) = car;
	CDR(rval) = cdr;
	return rval;
}


/************************************************************************/
/*  			Return head and tail of a list			*/
/************************************************************************/

static term
f_car(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, EVAL);

	if (x == _nil)
		fail("Can't get the head of an empty list");

	return CAR(x);
}


static term
f_cdr(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, EVAL);

	if (x == _nil)
		fail("Can't get the tail of an empty list");

	return CDR(x);
}


/************************************************************************/
/*	"in" does a simple membership test without backtracking		*/
/************************************************************************/

int
member(term m, term x, term *frame)
{
	while (x != _nil)
	{
		if (TYPE(x) != LIST)
			fail("in - malformed list in second argument");
		if (unify(m, frame, CAR(x), frame))
			return(TRUE);
		x = CDR(x);
		DEREF(x);
	}
	return FALSE;
}


static int
p_in(term goal, term *frame)
{
	term m = check_arg(1, goal, frame, ANY, IN);
	term x = check_arg(2, goal, frame, LIST, EVAL);

	return member(m, x, frame);
}


/************************************************************************/
/*			member - backtracks through lists		*/
/************************************************************************/

static int
p_member(term goal, term *frame)
{
	term m = check_arg(1, goal, frame, ANY, OUT);
	term x = check_arg(2, goal, frame, LIST, IN);

	while (x != _nil)
	{
		if (TYPE(x) != LIST)
			return(FALSE);
		if (unify(m, frame, CAR(x), frame))
			if (rest_of_clause())
				break;
		_untrail();
		x = CDR(x);
		DEREF(x);
	}
	return FALSE;
}


/************************************************************************/
/*  Find the length of a list, dereferencing the tail, if necessary	*/
/************************************************************************/

int
length(term x, term *frame)
{
	int i = 0;

	while (x != _nil)
	{
		if (TYPE(x) != LIST)
			fail("length - malformed list in first argument");
		i++;
		x = CDR(x);
		x = unbind(x, frame);
	}
	return i;
}


static term
f_length(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);

	switch (TYPE(x))
	{
	  case ATOM:	if (x != _nil)
				return new_int((long) strlen(NAME(x)));
	  case LIST:	return new_int((long) length(x, frame));
	  default:	fail("Argument must be a list or atom");
	}
}


/************************************************************************/
/*  	If list is unbound, build a list of specified length		*/
/************************************************************************/

static term
build_list(long L)
{
	term rval = _nil;

	while (L--)
		rval = gcons(new_ref(), rval);

	return rval;
}


static int
p_length(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, OUT);
	term y = check_arg(2, goal, frame, INT, OUT);

	if (x == _nil || TYPE(x) == LIST)
		return unify(y, frame, new_int((long) length(x, frame)), frame);

	if (TYPE(y) == INT)
		return unify(x, frame, build_list(IVAL(y)), frame);

	fail("At least one of the arguments must be bound");
}


/************************************************************************/
/*		function  - concatenate two lists			*/
/************************************************************************/

static term
f_concatenate(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, IN);
	term y = check_arg(2, goal, frame, LIST, IN);
	term *p, rval;

	if (x == _nil)
		return(y);

	for (p = &rval; x != _nil; p = &CDR(*p))
	{
		if (TYPE(x) != LIST)
			fail("first argument is not a proper list");
		*p = gcons(CAR(x), _nil);
		x = CDR(x);
		DEREF(x);
	}
	*p = y;

	return rval;
}


/************************************************************************/
/*	predicate  - concatenate two lists producing a third		*/
/************************************************************************/

static int
p_concatenate(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, IN);
	term y = check_arg(2, goal, frame, LIST, IN);
	term z = check_arg(3, goal, frame, LIST, OUT);
	term *p, rval = _nil;

	for (p = &rval; x != _nil; p = &CDR(*p))
	{
		if (TYPE(x) != LIST)
			fail("first argument is not a proper list");

		*p = gcons(copy(CAR(x), frame), _nil);
		x = CDR(x);
		DEREF(x);
	}
	*p = copy(y, frame);

	return unify(z, frame, rval, frame);
}


/************************************************************************/
/*			difference between two lists			*/
/************************************************************************/

static int
p_diff(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, IN);
	term y = check_arg(2, goal, frame, LIST, IN);
	term z = check_arg(3, goal, frame, LIST, OUT);
	term rval = _nil, *p = &rval;

	while (x != _nil)
	{
		if (TYPE(x) != LIST)
			fail("first argument is not a proper list");
		if (! member(CAR(x), y, frame))
		{
			*p = gcons(CAR(x), _nil);
			p = &CDR(*p);
		}
		x = CDR(x);
		DEREF(x);
	}

	return unify(z, frame, rval, frame);
}


/************************************************************************/
/*				init					*/
/************************************************************************/

void
lists_init(void)
{
	new_subr(f_car, "car");
	new_subr(f_car, "hd");
	new_subr(f_car, "first");
	new_subr(f_cdr, "cdr");
	new_subr(f_cdr, "tl");
	new_subr(f_cdr, "rest");

	defop(100, FX, new_subr(f_length, "#"));
	defop(300, XFX, new_pred(p_in, "in"));
	defop(600, YFX, new_subr(f_concatenate, "++"));

	new_pred(p_length, "length");
	new_pred(p_member, "member");
	new_pred(p_concatenate, "cat");
	new_pred(p_concatenate, "append");
	new_pred(p_diff, "diff");
}
