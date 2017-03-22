/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





		/*	Meta-logical predicates		*/

#include "pred.h"


extern int argn;
extern atom *same_proc;


static
assert PREDICATE
{
	register pval rval;
	var **old_vc, *vc[MAXVAR];

	if (iscompound(arg[0]) || isatom(arg[0]))
	{
		argn = 0;
		old_vc = varcell;
		varcell = vc;
		rval = mkclause(arg[0], frame[0]);
		rval -> g.nvars = argn;
		add_clause(rval);
		varcell = old_vc;
		argn = 0;
		return(TRUE);
	}
	else fail("Assert - argument must be compound or atom")
}

static
asserta PREDICATE
{
	register pval rval, x;
	var **old_vc, *vc[MAXVAR];

	if (iscompound(arg[0]) || isatom(arg[0]))
	{
		argn = 0;
		old_vc = varcell;
		varcell = vc;
		rval = mkclause(arg[0], frame[0]);
		rval -> g.nvars = argn;
		check_clause(rval);
		x = rval -> g.goal[0];
		if (iscompound(x)) x = x -> c.term[0];
		rval -> g.rest = VAL(x);
		VAL(x) = (clause *) rval;
		same_proc = 0;
		varcell = old_vc;
		argn = 0;
		return(TRUE);
	}
	else fail("Asserta - argument must be compound or atom")
}


static
pterm PREDICATE
{
	register int i;

	if (TYPE(arg[0]) != INT || TYPE(arg[1]) == VAR)
		fail("Arg - bad argument")
	i = INT_VAL(0);
	if (isatom(arg[1]) && i == 0)
		return(unify(arg[2], frame[2], arg[1], 0));
	else if (iscompound(arg[1]) && i <= SIZE(arg[1]))
		return(unify(arg[2], frame[2], arg[1] -> c.term[i], frame[1]));
	else return(FALSE);
}


static
pval mkfn(functor, arity)
atom *functor;
int arity;
{
	register compterm *rval;
	register int i;
	register var *v;
	char buf[6];

	rval = record(arity);
	rval -> term[0] = (pval) functor;
	for (i = 0; i < arity; i++)
	{
		sprintf(buf, "_%d", i + 1);
		v = (var *) new(VAR);
		v -> offset = i;
		v -> pname = (atom *) intern(buf, strlen(buf));
		rval -> term[i + 1] = (pval) v;
	}
	trail_pointer(rval);
	return((pval) rval);
}


static
functor PREDICATE
{
	extern binding *sp;
	register pval rval;
	register int arity;

	if (isvariable(arg[0]) && isatom(arg[1]) && isinteger(arg[2]))
	{
		if ((arity = INT_VAL(2)) == 0)
		{
			bind(arg[0], frame[0], arg[1], 0);
			return(TRUE);
		}
		if ((int)(rval = mkfn(arg[1], arity)) == FALSE) return(FALSE);
		bind(arg[0], frame[0], rval, sp);
		clear_frame(arity);
		return(TRUE);
	}
	else if (isatom(arg[0]))
		return (unify(arg[1], frame[1], arg[0], 0)
		     && unify(arg[2], frame[2], stack_int, 0));
	else if (TYPE(arg[0]) == FN)
		return (unify(arg[1], frame[1], arg[0] -> c.term[0], frame[0])
		     && unify(arg[2], frame[2], stack_int, SIZE(arg[0])));
	else fail("Functor - bad arg")
}


#define	CONCATSIZ	256

static
concat PREDICATE
{
	char cat_buf[CONCATSIZ];
	char tmp_buf[64];	/* must be bigger than a %d or a %.5g */
	pval tl_term, rval;
	binding *tl_frame;
	char	*cp;
	
	cp = cat_buf;

	if (arg[0] == (pval) nil) return(FALSE);
	if (TYPE(arg[0]) != LIST)
		fail("Concat - first argument must be a list");

	termb = arg[0];
	frameb = frame[0];
	while (TYPE(termb) != VAR && termb != (pval) nil)
	{
		int	len;
		char	*cp2;

		tl_term = termb;
		tl_frame = frameb;
		unbind(termb->c.term[0], frameb);

		if (isatom(termb))
			cp2 = NAME(termb);
		else if (isinteger(termb))
			if (termb == stack_int)
				sprintf(cp2=tmp_buf, "%d", (int) frameb);
			else	sprintf(cp2=tmp_buf, "%d", termb->i.int_val);

		else if (isreal(termb))
			sprintf(cp2=tmp_buf, "%.5g", termb->r.real_val);

		else fail("Concat - list members must be atomic")

		len = strlen(cp2);
		if (cp + len - cat_buf >= CONCATSIZ)
			fail("Concat - string too long");

		strcpy(cp, cp2);
		unbind(tl_term -> c.term[1], tl_frame);
		cp += len;
	}
	rval = intern(cat_buf, cp - cat_buf);
	rval = nonop(atype(cat_buf), rval);
	if (isatom(arg[1])) return(arg[1] == rval);
	else if (isvariable(arg[1]))
	{
		bind(arg[1], frame[1], rval, 0);
		return(TRUE);
	}
	else fail("Concat - bad second argument")
}


typedef enum
{
  WORDCH , STRINGCH , SYMBOLCH , PUNCTCH , QUOTECH , DIGIT , WHITESP , 
 ILLEGALCH
} chartype;

static
p_char PREDICATE
{
	extern chartype chtype[];
	char buf[2];
	register i;
	register pval rval;

	if (isinteger(arg[0]) && isatom(arg[1]))
	{
		i = INT_VAL(0);
		if (i < 1 || i > strlen(NAME(arg[1]))) return(FALSE);
		buf[0] = NAME(arg[1])[i - 1];
		buf[1] = 0;
		rval = intern(buf, 1);
		rval = nonop(atype(buf), rval);
		if (isatom(arg[2])) return(rval == arg[2]);
		else if (isvariable(arg[2]))
		{
			bind(arg[2], frame[2], rval, 0);
			return(TRUE);
		}
	}
	fail("Char - bad argument")
}


extern var _1, _2;
static struct {itemtype type; var *_1, *_2;} _list = {LIST, &_1, &_2};


static
ancestors PREDICATE
{
	register pval t;
	register environment *p;
	binding *f, *s;

	t = arg[0]; f = frame[0];
	for (p = parent->parent; p >= env_stack; p = p->parent)
	{
		s = sp;
		clear_frame(2);
		if (! unify(t, f, &_list, s))
			fail("Ancestors - incorrect argument")
		s -> termv = (pval)(*(p -> cl));
		if (p -> parent < env_stack) s -> framev = stack;
		else s -> framev = p->parent->sp;
		t = (pval)(&_2); f = s;
	}
	bind(t, f, nil, 0);
	return(TRUE);
}


static int next_no = 0;

static
numv(x, f)
pval x;
binding *f;
{
	int i, limit;
	pval a;
	char buf[16];

	switch (TYPE(x))
	{
	   case LIST:
	   case FN:	limit = SIZE(x);
			break;
	   case VAR:	unbind(x, f);
			if (TYPE(termb) != VAR)
				numv(termb, frameb);
			else {
				sprintf(buf, "#%d", next_no++);
				
				a = intern(buf, strlen(buf));
				bind(termb, frameb, a, 0);
			}
	   default:	return;
	}

	for (i = 0; i <= limit; i++)
		numv(x -> c.term[i], f);
}


static
number_vars PREDICATE
{
	if (TYPE(arg[1]) != INT)
		fail("Numbervars - second argument must be an integer")
	if (TYPE(arg[2]) != INT && TYPE(arg[2]) != VAR)
		fail("Numbervars - third arg must be integer or variable")
	next_no = INT_VAL(1);
	numv(arg[0], frame[0]);
	return(unify(arg[2], frame[2], stack_int, next_no));
}



atom_table p_meta =
{
	SET_PRED(NONOP, 0, 3, "term", pterm),
	SET_PRED(NONOP, 0, 3, "arg", pterm),
	SET_PRED(NONOP, 0, 3, "functor", functor),
	SET_PRED(NONOP, 0, 1, "assert", assert),
	SET_PRED(NONOP, 0, 1, "asserta", asserta),
	SET_PRED(NONOP, 0, 1, "assertz", assert),
	SET_PRED(NONOP, 0, 2, "concat", concat),
	SET_PRED(NONOP, 0, 3, "char", p_char),
	SET_PRED(NONOP, 0, 1, "ancestors", ancestors),
	SET_PRED(NONOP, 0, 3, "numbervars", number_vars),
	END_MARK
};
