/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





/*		predicates for manipulating clauses			*/


#include "pred.h"

atom *private = NULL;

extern binding *sp;

static
definition PREDICATE
{
	register clause *rval;
	register binding *rframe;

	if (TYPE(arg[0]) != ATOM)
		fail("Definition - first argument must be an atom")
	if (TYPE(arg[1]) != VAR)
		fail("Definition - second argument must be unbound")
	if ((rval = VAL(arg[0])) == 0)
		return(FALSE);
	rframe = sp;
	clear_frame(rval -> nvars);
	bind(arg[1], frame[1], rval, rframe);
	return(TRUE);
}


static
next_clause PREDICATE
{
	register clause *rval;
	register binding *rframe;

	if (TYPE(arg[0]) != CLAUSE)
		fail("Next_clause - first argument must be a clause")
	if (TYPE(arg[1]) != VAR)
		fail("Next_clause - second argument must be unbound")
	if ((rval = arg[0] -> g.rest) == 0)
		return(FALSE);
	rframe = sp;
	clear_frame(rval -> nvars);
	bind(arg[1], frame[1], rval, rframe);
	return(TRUE);
}


static
head PREDICATE
{
	if (TYPE(arg[0]) != CLAUSE)
		fail("Head - first argument must be a clause")
	return(unify(arg[0] -> g.goal[0], frame[0], arg[1], frame[1]));
}


extern var _1, _2;

static
struct {
	itemtype type;
	card size;
	atom *fun;
	var *left, *right;
} _conjunction =
{
	FN,
	2,
	0,
	&_1,
	&_2
};

static
body PREDICATE
{
	register pval *b;
	register binding *s, *s1, *s2;

	if (TYPE(arg[0]) != CLAUSE)
		fail("Body - first argument must be a clause")
	_conjunction.fun = _comma;
	b = &(arg[0] -> g.goal[1]);
	if (*b == 0)
		return(unify(arg[1], frame[1], _true, 0));
	if (*(b + 1) == 0)
		return(unify(arg[1], frame[1], *b, frame[0]));
	s1 = s = sp;
	clear_frame(2);
	s1[0].termv = *b++;
	s1[0].framev = frame[0];
	while (*(b + 1))
	{
		s2 = sp;
		clear_frame(2);
		s1[1].termv = (pval) (&_conjunction);
		s1[1].framev = s2;
		s2[0].termv = *b++;
		s2[0].framev = frame[0];
		s1 = s2;
	}
	s1[1].termv = *b;
	s1[1].framev = frame[0];
	return(unify(arg[1], frame[1], &_conjunction, s));
}


static
unl_clause PREDICATE
{
	extern atom *same_proc;
	pval head, p_name;
	register clause **p;

	if (TYPE(arg[0]) != CLAUSE)
		fail("retract - argument must be clause")
	head = arg[0] -> g.goal[0];
	if (isatom(head)) p_name = head;
	else p_name = head -> c.term[0];
	p = &VAL(p_name);
	while (*p)
	{
		if (*p == (clause *) arg[0])
		{
			trail_pointer(*p);
			*p = (*p) -> rest;
			same_proc = 0;
			return(TRUE);
		}
		else p = &((*p) -> rest);
	}
	fail("retract - clause not found")
}


static
p_free PREDICATE
{
	if (isvariable(arg[0]))
		fail("Cannot free unbound term")
	free_term(arg[0]);
	return(TRUE);
}

static
p_free_proc PREDICATE
{
	if (isatom(arg[0]))
	{
		free_proc(VAL(arg[0]));
		VAL(arg[0]) = 0;
		return(TRUE);
	}
	fail("Free proc - argument must be an atom")
}



static
remob PREDICATE
{
	extern atom *hashtable[];
	atom **p;

	if (isatom(arg[0]))
	{
		p = &hashtable[hash(NAME(arg[0]))];
		while (*p)
		{
			if (*p == (atom *) arg[0])
			{
				atom *temp = (*p)->link;
				(*p)->link = private;
				private = *p;

				*p =  temp;
				return(TRUE);
			}
			else p = &((*p) -> link);
		}
		fail("Remob - atom not found")
	}
	else fail("Remob - argument must be an atom")
}


static
member1 PREDICATE
{
	extern binding *frame2;
	binding *old_frame2;
	binding **old_tp;

	if (arg[1] == (pval) nil) return(FALSE);

	old_tp = tp;
	old_frame2 = frame2;
	frame2 = sp;	/* force all bindings on to trail */

	termb = arg[1];
	frameb = frame[1];

	do {
		if (TYPE(termb) != LIST)
			fail("member1 - bad list arg");

		if (unify(arg[0], frame[0], termb->c.term[0], frameb))
		{
			frame2 = old_frame2;
			return(TRUE);
		}

		clean_trail(old_tp);
		unbind(termb->c.term[1], frameb);

	} while (termb != (pval) nil);

	frame2 = old_frame2;
	return(FALSE);
}


static
nvars PREDICATE
{
	if (TYPE(arg[0]) != CLAUSE)
		fail("Nvars - first argument must be a clause")
	return(unify(arg[1], frame[1], stack_int, arg[0] -> g.nvars));
}


atom_table p_clause =
{
	SET_PRED(NONOP, 0, 1, "unlink_clause", unl_clause),
	SET_PRED(NONOP, 0, 1, "free", p_free),
	SET_PRED(NONOP, 0, 1, "free_proc", p_free_proc),
	SET_PRED(NONOP, 0, 1, "remob", remob),
	SET_PRED(NONOP, 0, 2, "head", head),
	SET_PRED(NONOP, 0, 2, "body", body),
	SET_PRED(NONOP, 0, 2, "definition", definition),
	SET_PRED(NONOP, 0, 2, "next_clause", next_clause),
	SET_PRED(NONOP, 0, 2, "member1", member1),
	SET_PRED(NONOP, 0, 2, "nvars", nvars),
	END_MARK
};
