/************************************************************************/
/*		A small expression evaluator for Prolog			*/
/************************************************************************/

#include <unistd.h>
#include "prolog.h"

typedef struct det_env_struct *det_env;

typedef struct det_env_struct
{
	env parent;
	term *goals;
	term *frame;
	term *global;
} det_env_struct;

extern env top_of_stack;
//extern term *local, *local_end, *global;
extern term *global;

term _eval_print, _is, _of;

static term
apply(term eqn, term expr, term *frame)
{
//	term rval = expr, *new_frame = local;
	term rval = expr;
	det_env_struct environment;

	environment.parent = top_of_stack;
	environment.goals = &expr;
	environment.frame = frame;
	environment.global = global;
	top_of_stack = (env)(&environment);

	switch (TYPE(eqn))
	{
	  case SUBR:
	  case FSUBR:	trace_print("CALL", (env)(&environment), NULL);
			rval = (* S_CODE(eqn))(expr, frame);
			trace_print("EXIT", (env)(&environment), rval);
			break;
	  case PRED:
	  case FPRED:	trace_print("CALL", (env)(&environment), NULL);
			rval = (* C_CODE(eqn))(expr, frame) ? _true : _false;
			trace_print("CALL", (env)(&environment), NULL);
			break;
	  default:
			while (eqn != NULL)
			{
				term lhs, rhs, head = HEAD(eqn);
				term new_frame[NVARS(eqn)];
		
				if (TYPE(head) == FN && ARG(0, head) == _is)
				{
					lhs = ARG(1, head);
					rhs = ARG(2, head);
				}
				else
				{
					lhs = head;
					rhs = NULL;
				}
		
//				MAKE_FRAME(NVARS(eqn));
		
				if (unify(expr, frame, lhs, new_frame) && cond(BODY(eqn), new_frame))
				{
					if (rhs)
					{
						trace_print("CALL", (env)(&environment), NULL);
						rval = eval(rhs, new_frame);
						trace_print("EXIT", (env)(&environment), rval);
					}
					else
						rval = _true;
//					local = new_frame;
					break;
				}
//				local = new_frame;
				global = environment.global;
				eqn = NEXT(eqn);
			}
	}

	top_of_stack = environment.parent;
	return rval;
}


term
eval(term expr, term *frame)
{
	term fn, p;

L1:	switch (TYPE(expr))
	{
	   case REF:	if (POINTER(expr) != NULL)
			{
				expr = POINTER(expr);
				goto L1;
			}
			return expr;	/* this line added for use of "all" in infobot */
	   case ANON:
	   case FREE:	/* rprint(expr, frame); */
			fail("Unbound variable in expression evaluation");
	   case BOUND:	expr = frame[OFFSET(expr)];
			goto L1;
	   case ATOM:
			if ((p = PROC(expr)) != NULL)
			{
	   			if (expr == _bang)
	   				fail("Can't cut inside an evaluable expression");
				return apply(p, expr, frame);
			}
			return expr;
	   case LIST:	{
	   			term rval = galloc(sizeof(compterm) + WORD_LENGTH);
	   			TYPE(rval) = LIST;
	   			FLAGS(rval) = COPY;
	   			ARITY(rval) = 1;
	   			CAR(rval) = eval(CAR(expr), frame);
	   			CDR(rval) = eval(CDR(expr), frame);
	   			return rval;
	   		}
	   case FN:
			p = PROC(fn = unbind(ARG(0, expr), frame));
			if (p == NULL || (TYPE(p) != FSUBR && TYPE(p) != FPRED))
			{
				int i, a = ARITY(expr);
				term rval = galloc(sizeof(compterm) + a * WORD_LENGTH);

				TYPE(rval) = FN;
				FLAGS(rval) = COPY;
				ARITY(rval) = a;

				ARG(0, rval) = fn;
				for (i = 1; i <= a; i++)
					ARG(i, rval) = eval(ARG(i, expr), frame);

				if (p != NULL)
					rval = apply(p, rval, frame);

				return rval;
			}
			return apply(p, expr, frame);
	   default:
			return expr;
	}
}


/************************************************************************/
/*	Print the value of an expression - for interactive use		*/
/************************************************************************/

static int
p_eval_print(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, EVAL);

	if (x == NULL)
		return FALSE;
	if (! isatty(fileno(input)))
		return TRUE;
	if (x == _true)
		fputs("** yes\n", stderr);
	else if( x == _false)
		fputs("** no\n", stderr);
	else
	{
		list_proc(x);
		fputc('\n', output);
	}

	return TRUE;
}


void
eval_print(term expr, term varlist)
{
	ARG(0, expr) = _eval_print;
	lush(expr, varlist, 1);
}


/************************************************************************/
/*		Hook for Prolog to call function evaluator		*/
/************************************************************************/

static int
is(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term y = check_arg(2, goal, frame, ANY, EVAL);

	if (y == NULL)
		return FALSE;
	if (TYPE(x) == FN && ARG(0, x) == _of)
	{
		if (putprop(ARG(2, x), ARG(1, x), make(y, frame)) == NULL)
			fail("Property already exists");
		return TRUE;
	}
	return unify(x, frame, y, frame);
}


/************************************************************************/
/*			Return true of argument is defined		*/
/************************************************************************/

static term
is_defined(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);

	return (x == NULL ? _false : _true);
}


/************************************************************************/
/*		       Return the expression unevaluated		*/
/************************************************************************/

static term
quote(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);

	return x;
}


/************************************************************************/
/*	Return a copy of the expression checking for unquote		*/
/************************************************************************/

term _unquote;

static term
quasiquote(term t, term *frame)
{
	switch (TYPE(t))
	{
	   case FN:
			if (ARG(0, t) == _unquote)
				return eval(ARG(1, t), frame);
	   case LIST:
			if (! (FLAGS(t) & COPY))
			{
				int i, a = ARITY(t);
		   		term rval = galloc(sizeof(compterm) + a * WORD_LENGTH);

				TYPE(rval) = TYPE(t);
				FLAGS(rval) = COPY;
				ARITY(rval) = a;

				for (i = 0; i <= a; i++)
					ARG(i, rval) = quasiquote(ARG(i, t), frame);

				return rval;
			}
	   default:
			return copy(t, frame);
	}
}


static term
_quasiquote(term expr, term *frame)
{
	term x = check_arg(1, expr, frame, ANY, IN);

	return quasiquote(x, frame);
}


/************************************************************************/
/*		Evaluate a sequence of expressions			*/
/************************************************************************/

term
progn(term expr, term *frame)
{
	while (TYPE(expr) == FN && ARG(0, expr) == _comma)
	{
		eval(ARG(1, expr), frame);
		expr = ARG(2, expr);
	}
	
	return eval(expr, frame);
}


/************************************************************************/
/*				Initialisation				*/
/************************************************************************/

void
eval_init(void)
{
	defop(900, FX, _unquote = intern("$"));
	defop(900, FX, new_fsubr(_quasiquote, "`"));

	new_fsubr(quote, "quote");
	new_subr(is_defined, "is_defined");

	defop(1200, XF, _eval_print = new_pred(p_eval_print, "??"));
	defop(700, XFX, _is = new_fpred(is, "is"));
	_of = intern("of");
}
