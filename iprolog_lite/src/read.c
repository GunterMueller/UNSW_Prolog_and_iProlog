#include <setjmp.h>
#include "prolog.h"

extern int lex_type;

int syntax_error = FALSE;

term read_expr(int);

static jmp_buf start_env;
static term temp, saved_macro, _more_prompt;


void
syn_err(char *msg)
{
	extern int linen;
	term p;

	syntax_error = TRUE;

	clear_input();

	if (isatty(fileno(input)))
	{
		fprintf(stderr, "\nSYNTAX ERROR: %s.\n\n", msg);
		_prompt = _prolog_prompt;
		skip_line();
	}
	else
	{
#ifdef THINK_C
		d_error(input, (long) linen, msg);
#else
		fprintf(stderr, "\nSYNTAX ERROR on or near line %d: %s.\n", linen, msg);

		do
			p = get_atom();
		while (p != _end_of_file && p != _dot);
#endif
	}

	longjmp(start_env, 1);
}


static term
read_list(void)
{
	term p, expr, rval;

	expr = read_expr(999);
	p = get_atom();
	if (p == _bar)
	{
		rval = gcons(expr, read_expr(999));
		if (get_atom() != _rbrac)
			syn_err("malformed list end");
		return(rval);
	}
	else if (p == _comma)
		rval = gcons(expr, read_list());
	else if (p == _rbrac)
		rval = gcons(expr, _nil);
	else syn_err("malformed list");
	return(rval);
}


static term
list_macro(void)
{
	term p;

	if ((p = get_atom()) == _rbrac)
		return(_nil);
	else ungetatom(p);

	return read_list();
}


static term
paren_macro(void)
{
	term rval = read_expr(1201);

	if (get_atom() == _rpren)
		return rval;
	else syn_err(" missing )");
}

static term
brace_macro(void)
{
	term rval = read_expr(1201);

	if (get_atom() ==_rbrace)
		return g_fn1(_lbrace, rval);
	else syn_err(" missing }");
}


static term
read_arg_list(int nterms)
{
	term expr, rval;

	expr = read_expr(999);
	rval = get_atom();
	if (rval == _comma)
	{
		rval = read_arg_list(++nterms);
		ARG(nterms, rval) = expr;
	}
	else if (rval == _rpren)
	{
		rval = new_g_fn(++nterms);
		ARG(nterms, rval) = expr;
	}
	else syn_err(", not found");
	return(rval);
}


term
read_term(void)
{
	term rval, p;
	int hold_lex;

	rval = get_atom();

 	switch (hold_lex = lex_type)
 	{
	case INT_T:
 	case REAL_T:
 		return rval;
 	case QUOTED_T:
 	case STRING_T:
 		break;
 	default:
 		if (TYPE(rval) == ATOM && MACRO(rval) != NULL && !(FLAGS(rval) & OP))
			return (*MACRO(rval))();
	}

	p = get_atom();

 	if (rval == _plus && (TYPE(p) == INT || TYPE(p) == REAL))
		return(p);
	if (rval == _minus)
	{
		if (TYPE(p) == INT)
		{
			IVAL(p) = - IVAL(p);
			return(p);
		}
		if (TYPE(p) == REAL)
		{
			RVAL(p) = - RVAL(p);
			return(p);
		}
	}
	if (p == _lpren)
	{
		p = read_arg_list(0);
		ARG(0, p) = rval;
		return(p);
	}
	lex_type = hold_lex;
	ungetatom(p);
	return(rval);
}


term
read_expr(int given)
{
	term p, rval;

	rval = read_term();

/*	if (TYPE(rval) == ATOM && PREFIX(rval) && PRE_PREC(rval) < given)
*/	if (TYPE(rval) == ATOM && PREFIX(rval) && lex_type != QUOTED_T && lex_type != STRING_T)
	{
		p = read_expr(PRE_PREC(rval));

		if (TYPE(p) == ATOM &&
		   (POSTFIX(p) && POST_PREC(p) > PRE_PREC(rval)
		   || INFIX(p) && IN_PREC(p) > PRE_PREC(rval)))
			ungetatom(p);
		else if (TERM_EXPAND(rval) == NULL)
			rval = g_fn1(rval, p);
		else
			rval = (*TERM_EXPAND(rval))(g_fn1(rval, p));
	}

	repeat
	{
		p = get_atom();
		if (! IS_OP(p))
		{
			ungetatom(p);
			break;
		}
		if (POSTFIX(p) && POST_PREC(p) < given)
		{
			rval = g_fn1(p, rval);
			if (POST_PREC(p) >= 1200)
				break;
		}
		else if ((INFIX(p) & RIGHT) && (IN_PREC(p) <= given))
			rval = g_fn2(p, rval, read_expr(IN_PREC(p)));
		else if (INFIX(p) && IN_PREC(p) < given)
			rval = g_fn2(p, rval, read_expr(IN_PREC(p)));
		else {
			ungetatom(p);
			break;
		}
		if (TERM_EXPAND(p) != NULL)
			rval = (*TERM_EXPAND(p))(rval);
	}
	return rval;
}


term
p_read(void)
{
	extern term varlist;
	term p;

	varlist = _nil;

	if (setjmp(start_env))
		if (! isatty(fileno(input)))
			return _end_of_file;

	clear_input();

	if ((p = get_atom()) == _end_of_file)
		return p;
	else
		ungetatom(p);

	_prompt = _more_prompt;

	p = read_expr(1201);

	if (TYPE(p) != FN || ! POSTFIX(p) || ! POST_PREC(ARG(0, p)) == 1200)
	{
		print(p);
		fflush(output);
		syn_err("Input must be a question, command or assertion");
	}
	return p;
}


static int
read_built_in(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term rval = p_read();

	if (rval == _end_of_file)
		return(FALSE);
	else
		return(unify(x, frame, ARG(1, rval), frame));
}


static int
p_read_term(term goal, term *frame)
{
	term x, rval;

	if (ARITY(goal) == 2)
	{
		term old_input = current_input;

		x = check_arg(2, goal, frame, ANY, OUT);
		current_input = check_arg(1, goal, frame, STREAM, IN);
		input = FPTR(current_input);

		rval = p_read();

		current_input = old_input;
		input = FPTR(current_input);
	}
	else
	{
		x = check_arg(1, goal, frame, ANY, OUT);
		rval = p_read();
	}

	if (rval == _end_of_file)
		return(FALSE);
	else
		return(unify(x, frame, ARG(1, rval), frame));
}


void
read_init()
{
	temp = new_h_fn(1);
	_more_prompt = intern("> ");

	new_pred(read_built_in,	"read");
	new_pred(p_read_term,	"read_term");

	MACRO(_lbrac) = list_macro;
	MACRO(_lpren) = paren_macro;
	MACRO(_lbrace) = brace_macro;
}
