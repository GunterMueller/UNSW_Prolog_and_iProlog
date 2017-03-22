/******************************************************************************

			UNSW Prolog (version 4.2)

		     Written by Andrew R. Nicholson
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

		  Real Arithmetic added Sun Oct 19 1986.

******************************************************************************/





		/*	The basic Prolog predicates	*/


#include "pred.h"
#include <math.h>
#include <errno.h>

int errno;
double fuzz = 0.000000001;		/* Fuzzy float */

int arith_failure = FALSE;


	/* Issue warning when a predefined predicate fails */

warning(s)
char *s;
{
	FILE *old_output = output;

	output = stderr;
	fprintf(stderr, "\nWARNING - %s\n", s);
	back_trace(-1);
	output = old_output;
}


		/*	ARITH evaluates arithmetic expressions	*/


static
awarning(s)
char *s;
{
	if (!arith_failure)
	{
		warning(s);
		arith_failure = TRUE;
	}
}


_arith(term, frame, result)
pval term;
binding *frame;
pval result;
{
	int i;
	pobj  arg[4];

	arith_failure = FALSE;
START:
	switch (TYPE(term))
	{
	   case INT:	TYPE(result) = INT;
			result->i.int_val = (term == stack_int)
					? (int) frame
					: ((integer *) term)->int_val;
			return;

	   case REAL:	TYPE(result) = REAL;
			result->r.real_val = term->r.real_val;
			return;

	   case FN:	if (TYPE(term -> c.term[0]) != PREDEF)
			{
				awarning("Unrecognized arithmetic operator");
				return;
			}
			if (SIZE(term) > 4)
			{
				awarning("ARITH - TOO MANY ARGUMENTS");
				return;
			}
			TYPE(result) = INT;
			for (i = 0; i < SIZE(term); i++) {
				_arith(term -> c.term[i + 1], frame, &arg[i]);
				if (arg[i].c.type == REAL) TYPE(result) = REAL;
			}
			if (TYPE(result) == REAL)
			{
			    for (i = 0; i < SIZE(term); i++) 
				if (arg[i].c.type == INT)
				    arg[i].r.real_val = arg[i].i.int_val;

			    switch ((int)VAL(term -> c.term[0]))
			    	{
			      	case PLUS:	result->r.real_val = arg[0].r.real_val + arg[1].r.real_val;
						return;
			      	case MINUS:	result->r.real_val = arg[0].r.real_val - arg[1].r.real_val;
						return;
			      	case TIMES:	result->r.real_val = arg[0].r.real_val * arg[1].r.real_val;
						return;
			      	case DIV:	if (arg[1].r.real_val == 0.0)
					   	{
						   	awarning("Attempted divide by zero");
						   	return;
					   	}
					   	result->r.real_val = arg[0].r.real_val / arg[1].r.real_val;
						return;
			      	case POW:	result->r.real_val = pow(arg[0].r.real_val, arg[1].r.real_val);
						if (errno == EDOM)
							awarning("Illegal domain for ^");
						if (errno == ERANGE)
							awarning("Result out of range for ^");
						return;
			   	case UPLUS:	result->r.real_val = arg[0].r.real_val;
						return;
			   	case UMINUS:	result->r.real_val = - arg[0].r.real_val;
						return;

			   	default:	awarning("Unknown arithmetic operation for real");
						return;
				}
			}
			/* At this point result must be an integer */
			switch ((int)VAL(term -> c.term[0]))
			{
			case PLUS:	result->i.int_val = arg[0].i.int_val + arg[1].i.int_val;
					return;
			case MINUS:	result->i.int_val = arg[0].i.int_val - arg[1].i.int_val;
					return;
			case TIMES:	result->i.int_val = arg[0].i.int_val * arg[1].i.int_val;
					return;
			case DIV:	if (arg[1].i.int_val == 0)
				   	{
					   	awarning("Attempted divide by zero");
					   	return;
				   	}
				   	result->i.int_val = arg[0].i.int_val / arg[1].i.int_val;
					return;
			case MOD:	if (arg[1].i.int_val == 0)
				   	{
					   	awarning("Attempted mod by zero");
					   	return;
				   	}
					result->i.int_val = arg[0].i.int_val % arg[1].i.int_val;
					return;
			case POW:	result->i.int_val = 1;
					if (arg[1].i.int_val >= 0)
					{
						for (i = arg[1].i.int_val; i != 0; i--)
							result->i.int_val *= arg[0].i.int_val;
					}
					if ((result->i.int_val == 0 &&
					    arg[0].i.int_val != 0) ||
					    (result->i.int_val < 0 &&
					    arg[0].i.int_val > 0) ||
					    (arg[1].i.int_val < 0))
					{
						result->r.type = REAL;
			      			result->r.real_val =
						    pow((double) arg[0].i.int_val, (double) arg[1].i.int_val);
						if (errno == EDOM)
							awarning("Illegal domain for ^");
						if (errno == ERANGE)
							awarning("Result out of range for ^");
					}
					return;
			case UPLUS:	result->i.int_val = arg[0].i.int_val;
					return;
			case UMINUS:	result->i.int_val = - arg[0].i.int_val;
					return;
			case LAND:	result->i.int_val = arg[0].i.int_val & arg[1].i.int_val;
					return;
			case LOR:	result->i.int_val = arg[0].i.int_val | arg[1].i.int_val;
					return;
			case XOR:	result->i.int_val = arg[0].i.int_val ^ arg[1].i.int_val;
					return;
			case NEG:	result->i.int_val = ~ arg[0].i.int_val;
					return;
			case LSHIFT:	result->i.int_val = arg[0].i.int_val << arg[1].i.int_val;
					return;
			case RSHIFT:	result->i.int_val = arg[0].i.int_val >> arg[1].i.int_val;
					return;

			default:	awarning("Unknown arithmetic operation for integer");
					return;
			}
	   case VAR:
			unbind(term,frame);
			term  = termb;
			frame = frameb;
			if (!isvariable(term)) goto START;

			awarning("Unbound variable in arithmetic expression");
			return;

	    default:	awarning("Non-numeric term in arithmetic expression");
			return;
	}
}



static
is PREDICATE
{
	pobj result;

	arith(arg[1], frame[1], &result);
	if (arith_failure) return(FALSE);
	if (result.c.type == REAL)
		return(unify(arg[0], frame[0], stack_real(result.r.real_val), 0));
	return(unify(arg[0], frame[0], stack_int, result.i.int_val));
}


static
int compare(arg, frame)
pval arg[];
binding **frame;
{
	pobj x, y;
	register double tmp;

	if (isatom(arg[0]) && isatom(arg[1]))
		return(strcmp(NAME(arg[0]), NAME(arg[1])));
	arith(arg[0], frame[0], &x);
	if (!arith_failure)
		arith(arg[1], frame[1], &y);
	if (x.c.type == REAL && y.c.type == INT)
	{
		tmp = (double) y.i.int_val;
		y.r.real_val = tmp;
		y.r.type = REAL;
	}
	if (y.c.type == REAL && x.c.type == INT)
	{
		tmp = (double) x.i.int_val;
		x.r.real_val = tmp;
		x.r.type = REAL;
	}
	if (y.c.type == INT)
		return(x.i.int_val - y.i.int_val);
	x.r.real_val -= y.r.real_val;
	if (fabs(x.r.real_val) < fuzz) return(0);
	if (x.r.real_val < 0.0) return(-1);
	return(1);
}


static
lt PREDICATE
{
	return(compare(arg, frame) < 0 && ! arith_failure);
}


static
le PREDICATE
{
	return(compare(arg, frame) <= 0 && ! arith_failure);
}

static
gt PREDICATE
{
	return(compare(arg, frame) > 0 && ! arith_failure);
}

static
ge PREDICATE
{
	return(compare(arg, frame) >= 0 && ! arith_failure);
}

static
eq PREDICATE
{
	return(compare(arg, frame) == 0 && ! arith_failure);
}


static
neq PREDICATE
{
	return(compare(arg, frame) != 0 && ! arith_failure);
}



static
p_round PREDICATE
{
	int val;

	if (isreal(arg[0]))
		val = REAL_VAL(0) + 0.5;
	else if (isinteger(arg[0]))
		val = INT_VAL(0);
	else	fail("round first arg must be real or integer")

	return(unify(arg[1], frame[1], stack_int, val));
}



static
is_int PREDICATE
{
	if (isreal(arg[0]))
	{
		double val = REAL_VAL(0) + (fuzz*0.5);

		return((fabs(val - floor(val))) < fuzz);
	}
	return(isinteger(arg[0]));
}

static
is_number PREDICATE
{
	return(isreal(arg[0]) || isinteger(arg[0]));
}

static
is_atom PREDICATE
{
	return(isatom(arg[0]));
}


static
quoted PREDICATE
{
	return(isatom(arg[0]) && arg[0] -> a.op_t == QATOM);
}


static
atomic PREDICATE
{
	switch (TYPE(arg[0]))
	{
	   case ATOM:
	   case PREDEF:
	   case REAL:
	   case INT:	return(TRUE);
	   default: return(FALSE);
	}
}


static
is_var PREDICATE
{
	return(isvariable(arg[0]));
}


static
nonvar PREDICATE
{
	return(! isvariable(arg[0]));
}



extern stream *qstream();

static
stream_type	PREDICATE
{
	return(qstream(arg[0]) != NULL);
}


static
is_stream	PREDICATE
{
	stream *x = qstream(arg[0]);

	return(x != NULL && x->mode != CLOSED);
}

static
length PREDICATE
{
	register i;

	if (arg[0] == (pval) nil) i = 0;
	else if (TYPE(arg[0]) == LIST)
	{
		i = 0;
		termb = arg[0];
		frameb = frame[0];
		repeat
		{
			if (TYPE(termb) == VAR || TYPE(termb) != LIST)
				break;
			else i++;
			unbind(termb -> c.term[1], frameb);
		}
	}
	else	fail("Length - first argument must be a list")

	if (isinteger(arg[1])) return(i == INT_VAL(1));
	if (isvariable(arg[1]))
	{
		bind_num(1, i);
		return(TRUE);
	}
	else return(FALSE);
}


atom_table p_basic =
{
	SET_PRED(XFX, 700, 2, ">", gt),
	SET_PRED(XFX, 700, 2, "<", lt),
	SET_PRED(XFX, 700, 2, "<=", le),
	SET_PRED(XFX, 700, 2, ">=", ge),
	SET_PRED(XFX, 700, 2, "==", eq),
	SET_PRED(XFX, 700, 2, "<>", neq),
	SET_PRED(XFX, 700, 2, "is", is),
	SET_PRED(NONOP, 0, 2, "round", p_round),
	SET_PRED(NONOP, 0, 1, "integer", is_int),
	SET_PRED(NONOP, 0, 1, "number", is_number),
	SET_PRED(NONOP, 0, 1, "atom", is_atom),
	SET_PRED(NONOP, 0, 1, "quoted", quoted),
	SET_PRED(NONOP, 0, 1, "atomic", atomic),
	SET_PRED(NONOP, 0, 1, "nonvar", nonvar),
	SET_PRED(NONOP, 0, 2, "length", length),
	SET_PRED(NONOP, 0, 1, "var", is_var),
	SET_PRED(NONOP, 0, 1, "stream_type", stream_type),
	SET_PRED(NONOP, 0, 1, "stream", is_stream),
	END_MARK
};
