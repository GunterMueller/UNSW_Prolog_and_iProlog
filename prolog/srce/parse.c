/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





#include <setjmp.h>
#include "g.h"
#include "in.h"


/* 	Token variables. Instantiated in lex.c		*/

extern int linen;
extern char terminal;
extern char *cur_file;

extern int library;

static atom dummy = {ATOM, FX, 999, 0, 0, "", 0, 0};


/*      memory mangement functions defined in mem.c	*/

extern atom *add_clause();
extern pval in_uniop(), in_biop();

/*	lexical analysis routine declared in lex.c	*/

extern pval getatom();
extern ungetatom();

/*	global flags and counters that are used outside this file	*/

int	argn = 0,
	p_read_on = FALSE;

/* 	error flag and error recovery buffers	*/

int	read_err = FALSE,
	load_err = FALSE;

jmp_buf env1, env2;



/*	SYNTAX   ANALYSIS   ROUTINES 		*/



error(msg, skip)
char *msg;
int skip;
{
	extern atom *prompt_string, *init_prompt;
	extern linen, pushed_back;

	load_err = read_err = TRUE;
	p_read_on = FALSE;
	pushed_back = -1;
	prompt_string = init_prompt;
	if (terminal)
	{
		fprintf(stderr, "SYNTAX ERROR: %s\n", msg);
		if (skip)
		{
			while (readch != '\n');
			prompt();
		}
	}
	else {
		fprintf(stderr, "%s: line %d: %s\n", cur_file, linen, msg);
		if (skip) while (readch != '.');
	}
	longjmp(env2,1);
}




pval scan()
{
	pval rval;

	rval = getatom();
	if (feof(input) && ! p_read_on)
	{
		fprintf(stderr, "EOF unexpected\n");
		longjmp(env1,1);
	}
	return(rval);
}



isvar(string)
char *string;
{
	register char c = *string;

	return((c >= 'A' && c <= 'Z') || c == '_');
}


prefix(a)
atom **a;
{
	register atom *p;

	for (p = *a; p && strcmp(NAME(*a), NAME(p)) == 0; p = p -> link)
	{
		if (p -> op_t == FX || p -> op_t == FY)
		{
			*a = p;
			return(TRUE);
		}
	}
	return(FALSE);
}



infix(a)
atom **a;
{
	register atom *p;

	for (p = *a; p && strcmp(NAME(*a), NAME(p)) == 0; p = p -> link)
		if (p -> op_t == XFX || p -> op_t == XFY || p -> op_t == YFX)
		{
			*a = p;
			return(TRUE);
		}
	return(FALSE);
}




postfix(a)
atom **a;
{
	register atom *p;

	for (p = *a; p && strcmp(NAME(*a), NAME(p)) == 0; p = p -> link)
		if (p -> op_t == XF || p -> op_t == YF)
		{
			*a = p;
			return(TRUE);
		}
	return(FALSE);
}




static
moreimportant(op1,op2)
atom *op1,*op2;
{
	if (strcmp(op1 -> name, op2 -> name) == 0)
	   	return(op1 -> op_t == XFY);
	else 	return(op1 -> pred < op2 -> pred);
}



static
pval in_list()
{
	extern pval expression();
	pval p, expr, rval;

	p = scan();

	if (p == (pval) _rbrac)
		return((pval) nil);
	else ungetatom(p);

	expr = expression(&dummy);
	p = scan();
	if (p == (pval) _comma)
	{
		p = scan();
		if (p == (pval) _dot_dot)
		{
			rval = new(LIST);
			rval -> c.term[0] = expr;
			rval -> c.term[1] = expression(&dummy);
			if (scan() != (pval) _rbrac)
				error("malformed list end", TRUE);
			return(rval);
		}
		else ungetatom(p);
		rval = new(LIST);
		rval -> c.term[0] = expr;
		rval -> c.term[1] = in_list();
	}
	else if (p == (pval) _bar)
	{
		rval = new(LIST);
		rval -> c.term[0] = expr;
		rval -> c.term[1] = expression(&dummy);
		if (scan() != (pval) _rbrac)
			error("malformed list end", TRUE);
		return(rval);
	}
	else if (p == (pval) _rbrac)
	{
		rval = new(LIST);
		rval -> c.term[0] = expr;
		rval -> c.term[1] = (pval) nil;
	}
	else error("malformed list", TRUE);
	return(rval);
}



static
pval expr_list(nterms)
char nterms;
{
	extern pval expression();
	pval expr, rval;

	expr = expression(&dummy);
	rval = scan();
	if (rval == (pval) _comma)
	{
		rval = expr_list(++nterms);
		rval -> c.term[nterms] = expr;
	}
	else if (rval == (pval) _rpren)
	{
		rval = (pval) record(++nterms);
		rval -> c.term[nterms] = expr;
	}
	else error(", not found", TRUE);
	return(rval);
}



pval expression(given)
pval given;
{
	pval rval, p;

	rval = scan();

	if (isinteger(rval)) goto OPERATOR;
	if (isreal(rval)) goto OPERATOR;
	if (rval -> a.op_t == QATOM) goto FUNCTION;
	if (rval == (pval) _lbrac)
	{
		rval = in_list();
		goto OPERATOR;
	}
	if (rval == (pval) _lpren)
	{
		rval = expression(_rpren);
		if (scan() == (pval) _rpren)
			goto OPERATOR;
		else error(" missing )", TRUE);
	}
	if (rval == (pval) _lbrace)
	{
		rval = expression(_rbrace);
		if (scan() == (pval) _rbrace)
		{
			p = (pval) record(1);
			p -> c.term[0] = (pval) _lbrace;
			p -> c.term[1] = rval;
			rval = p;
			goto OPERATOR;
		}
		else error(" missing }", TRUE);
	}

	if (isvar(NAME(rval)))
#ifdef PRINC_VAR
		goto FUNCTION;
#else
		goto OPERATOR;
#endif

	if (! prefix(&rval) || ungetc(getc(input), input) == '(')
	{
		rval = nonop(NONOP, rval);
		goto FUNCTION;
	}

	p = scan();
	if (isatom(p) && p -> a.pred > 1200)
	{
		ungetatom(p);
		return(nonop(NONOP, rval));
	}

	if (rval == (pval) _uplus)
	{
		if (isinteger(p) || isreal(p))
		{
			rval = p;
			goto OPERATOR;
		}
		ungetatom(p);
		rval = in_uniop(rval, expression(rval));
	}
	else if (rval == (pval) _uminus)
	{
		if (isinteger(p))
		{
			p -> i.int_val = -(p -> i.int_val);
			rval = p;
			goto OPERATOR;
		}
		if (isreal(p))
		{
			p -> r.real_val = -(p -> r.real_val);
			rval = p;
			goto OPERATOR;
		}
		ungetatom(p);
		rval = in_uniop(rval, expression(rval));
	}
	else if (rval == (pval) _prefix)
	{
		if (prefix(&p))
			rval = p;
		else ungetatom(p);
	}
	else if (rval == (pval) _infix)
	{
		if (infix(&p))
			rval = p;
		else ungetatom(p);
	}
	else if (rval == (pval) _postfix)
	{
		if (postfix(&p))
			rval = p;
		else ungetatom(p);
	}
	else {
		ungetatom(p);
		rval = in_uniop(rval, expression(rval));
	}

FUNCTION:
	p = scan();
	if (p == (pval) _lpren)
	{
		p = expr_list(0);
		p -> c.term[0] = rval;
		rval = p;
	}
	else ungetatom(p);

OPERATOR:
	repeat {
		p = scan();
		if (TYPE(p) != ATOM && TYPE(p) != PREDEF)
		{
			ungetatom(p);
			return(rval);
		}
		if (postfix(&p) && moreimportant(p, given))
		{
			rval = in_uniop(p, rval);
			if (p -> a.pred >= 1200) return(rval);
		}
		else if (infix(&p) && moreimportant(p, given))
			rval = in_biop(p, rval, expression(p));
		else {
			ungetatom(p);
			return(rval);
		}
	}
}




pval append1(l, a)
pval l;
atom *a;
{
	register pval p = l;

	if (p == (pval) nil)
	{
		p = new(LIST);
		p -> c.term[0] = (pval) a;
		p -> c.term[1] = (pval) nil;
		return(p);
	}
	else {
		repeat
		{
			if (p -> c.term[0] == (pval) a) return(l);
			if (p -> c.term[1] == (pval) nil) break;
			p = p -> c.term[1];
		}
		p -> c.term[1] = new(LIST);
		p = p -> c.term[1];
		p -> c.term[0] = (pval) a;
		p -> c.term[1] = (pval) nil;
		return(l);
	}
}


static
do_command(expr, ifquery)
pval expr;
{
	extern pval mkbody();
	pval cl;

	if (arith_op(expr))
		error("Query must be a predicate, not a function", FALSE);
	cl = mkbody(expr, 0, 1);
	execute(((clause *) cl), argn, ifquery);
	free_term(cl);
}


pval proc_list;

static
p_assert(expr)
pval expr;
{
	extern int library;
	register pval rval;
	atom *a;

	rval = mkclause(expr, 0);
	rval -> g.nvars = argn;
	a = add_clause(rval);
	if (! (terminal || library))
		proc_list = append1(proc_list, a);
	argn = 0;
}


#define principal(x) expr -> c.term[0] == (pval) x

static
parse()
{
	extern int pushed_back;
	pval expr;
	var **old_vc, *vc[MAXVAR];

	pushed_back = -1;
	argn = 0;
	old_vc = varcell;
	varcell = vc;
	read_err = FALSE;
	expr = expression(_rpren);
	if (principal(_bang))
		do_command(expr -> c.term[1], FALSE);
	else if (principal(_question))
		do_command(expr -> c.term[1], TRUE);
	else if (principal(_dot))
	{
		pval old_expr = expr;

		expr = expr -> c.term[1];
		if (principal(_command))
			do_command(expr -> c.term[1], FALSE);
		else if (principal(_query))
			do_command(expr -> c.term[1], TRUE);
		else
			p_assert(expr);
		expr = old_expr;
	}
	else
	{
		free_term(expr);
		error("Input must be a question, command or assertion",TRUE);
	}
	free_term(expr);
	varcell = old_vc;
	argn = 0;
}
	


evloop()
{
	load_err = FALSE;
	if (terminal) prompt();
	repeat
		if (! setjmp(env1))
		{
			if (setjmp(env2))
				/* syntax error and abort returns here */;
			skipd();
			if (feof(input)) break;
			parse();
			fflush(stdout);
		}
}



pval inchar()
{
	register pval rval;
	char buf[2];

	buf[0] = (char) readch;
	if (feof(input)) return((pval) _eof);
	buf[1] = 0;
	rval = intern(buf, 1);
	rval = nonop(atype(buf), rval);
	rval -> a.op_t = atype(buf);
	return(rval);
}
