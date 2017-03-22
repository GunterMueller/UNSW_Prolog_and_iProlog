/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





#include "g.h"

#define PLUS	1
#define MINUS	2
#define TIMES	3
#define DIV	4
#define MOD	5
#define POW	6
#define UPLUS	7
#define UMINUS	8
#define LAND	9
#define LOR	10
#define XOR	11
#define NEG	12
#define LSHIFT	13
#define RSHIFT	14


#define PREDICATE	(arg, frame)			/* ARGSUSED */ \
			pval arg[]; binding **frame;

#define NPREDICATE	(arg, frame, argc) 		/* ARGSUSED */ \
			pval arg[]; binding **frame; int argc;


#define bind_num(v, n) bind(arg[v], frame[v], stack_int, (int)n)


#define INT_VAL(n)\
(\
	(arg[n] == stack_int) ?\
		  (int) frame[n]\
		: ((integer *) arg[n]) -> int_val\
)

#define REAL_VAL(n) (((real *) arg[n]) -> real_val)

typedef atom atom_table[];


#define SET_ATOM(x1, x2, x3)\
	{ATOM, x1, x2, 0, -1, x3, ((clause *) 0), ((atom *) 0)}

#define SET_PRED(x1, x2, x3, x4, x5)\
	{PREDEF, x1, x2, 0, x3, x4, ((clause *) x5), ((atom *) 0)}

#define END_MARK {ATOM, 0, 0, 0, 0, 0, 0, 0}


#define fail(x) {warning(x); return(FALSE);}

#define arith(pval, frame, result)\
	(arith_failure = FALSE, _arith(pval, frame, result))

extern pval	stack_real();
extern int	arith_failure;
