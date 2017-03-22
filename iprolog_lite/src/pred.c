#include "prolog.h"


term
new_pred(int (*c_code)(), char *name)
{
	term a = intern(name);
	term cl = halloc(sizeof(pred));

	TYPE(cl) = PRED;
	FLAGS(a) |= PREDEF;
	ID(cl) = a;
	C_CODE(cl) = c_code;
	PROC(a) = cl;
	return(a);
}


term
new_fpred(int (*c_code)(), char *name)
{
	term a = intern(name);
	term cl = halloc(sizeof(pred));

	TYPE(cl) = FPRED;
	FLAGS(a) |= PREDEF;
	ID(cl) = a;
	C_CODE(cl) = c_code;
	PROC(a) = cl;
	return(a);
}


term
new_subr(term (*c_code)(), char *name)
{
	term a = intern(name);
	term cl = halloc(sizeof(pred));

	TYPE(cl) = SUBR;
	FLAGS(a) |= PREDEF;
	ID(cl) = a;
	S_CODE(cl) = c_code;
	PROC(a) = cl;
	return(a);
}


term
new_fsubr(term (*c_code)(), char *name)
{
	term a = intern(name);
	term cl = halloc(sizeof(pred));

	TYPE(cl) = FSUBR;
	FLAGS(a) |= PREDEF;
	ID(cl) = a;
	S_CODE(cl) = c_code;
	PROC(a) = cl;
	return(a);
}


/* print a friendly error message when arguments to a built in are wrong */

static void
arg_type_error(int n, int type)
{
	char *s1, *s2, buf[256];
	
	switch (n % 10)
	{
	   case 1:		s1 = "st";
				break;
	   case 2:		s1 = "nd";
				break;
	   case 3:		s1 = "rd";
				break;
	   default:		s1 = "th";
				break;
	}
	switch (type)
	{
	   case FN:		s2 = "a compound term";
				break;
	   case LIST:		s2 = "a list";
				break;
	   case ATOM:		s2 = "an atom";
				break;
	   case INT:		s2 = "an integer";
				break;
	   case REAL:		s2 = "a real number";
				break;
	   case NUMBER:		s2 = "a number";
				break;
	   case STREAM:		s2 = "a stream";
				break;
	   case FREE:		s2 = "a free variable";
				break;
	   case BOUND:		s2 = "a bound variable";
				break;
	   case ANY:		s2 = "any kind of term";
				break;
	}
	sprintf(buf, "%d%s argument must be %s", n, s1, s2);
	fail(buf);
}


/* checkargs - argmuent processing for builtin functions */

term
check_arg(int n, term goal, term *frame, int wanted_type, int mode)
{
	term x;

	if (TYPE(goal) != FN || n > ARITY(goal))
		fail("not enough arguments");

	x = ARG(n, goal);

L:	switch (TYPE(x))
	{
	  case REF:	if (POINTER(x) != NULL)
			{
				x = POINTER(x);
				goto L;
			}
	  case ANON:
	  case FREE:	if (mode == OUT)
				return(x);
	  		break;
	  case BOUND:	x = frame[OFFSET(x)];
			goto L;
	}

	if (mode == EVAL)
		x = eval(x, frame);

	switch (wanted_type)
	{
	   case ANY:
			break;
	   case LIST:
			if (x != _nil && TYPE(x) != LIST)
			{
				/* rprint(goal, frame); */
				arg_type_error(n, wanted_type);
			}
			break;
	   case NUMBER:
			if (TYPE(x) != INT && TYPE(x) != REAL)
				arg_type_error(n, wanted_type);
			break;
	   default:
			if (TYPE(x) != wanted_type)
				arg_type_error(n, wanted_type);
			break;
	}
	return(x);	
}


void
check_arity(term goal, int n)
{
	if (goal == NULL)	fail("Missing goal");
	if (TYPE(goal) != FN)	fail("arguments missing");
	if (ARITY(goal) != n)	fail("incorrect number of arguments");
}
