#include "prolog.h"

extern term _is, varlist, *local, *global;


term *
get_proc(term x, term *frame)
{
	repeat
	{
		switch (TYPE(x))
		{
		   case ATOM:	return(&PROC(x));
		   case FN:	if (ARG(0, x) != _is)
					return(&PROC(unbind(ARG(0, x), frame)));
		   		x = unbind(ARG(1, x), frame);
		   		break;
		   default:	print(x);
				fail("argument not a valid clause head");
		}
	}
}


/************************************************************************/
/*				clause					*/
/************************************************************************/

static term
mkbody(term *body)
{
	if (*body == NULL)
		return(_true);
	if (body[1] == NULL)
		return(*body);
	return(g_fn2(_comma, *body, mkbody(body + 1)));
}


static int
_clause(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);
	term y = check_arg(2, goal, frame, ANY, OUT);
	term *first_clause = get_proc(x, frame);

	while (*first_clause != NULL)
	{
		term *new_frame = new_local_frame(NVARS(*first_clause));
		term *old_global = global;

		if (unify(HEAD(*first_clause), new_frame, x, frame)
		&&  unify(mkbody(BODY(*first_clause)), new_frame, y, frame))
			if (rest_of_clause())
				break;

		_untrail();
		local = new_frame;
		global = old_global;
		first_clause = &NEXT(*first_clause);
	}
	return(FALSE);
}


/************************************************************************/
/*	add_clause structure - adds a clause to the clause list		*/
/************************************************************************/

static int
p_add_clause(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, CLAUSE, IN);

	add_clause(x, FALSE);
	return(TRUE);
}


/************************************************************************/
/*				assert					*/
/************************************************************************/

static int
assert(term goal, term *frame)
{
	term x = unbind(ARG(1, goal), frame);

	if (TYPE(x) != ATOM && TYPE(x) != FN)
		fail("assert - argument must be a valid clause structure");

	varlist = _nil;
	add_clause(mkclause(x, frame), FALSE);
	return(TRUE);
}


/************************************************************************/
/*				assert					*/
/************************************************************************/

static int
asserta(term goal, term *frame)
{
	term x = unbind(ARG(1, goal), frame);

	if (TYPE(x) != ATOM && TYPE(x) != FN)
		fail("assert - argument must be a valid clause structure");

	varlist = _nil;
	add_clause(mkclause(x, frame), TRUE);
	return(TRUE);
}


/************************************************************************/
/*				retract					*/
/************************************************************************/

static int
unify_body(term x, term *f1, term *body, term *f2)
{
	term goal;

	if (x == _true)
		return(*body == NULL);

	while (*body != NULL)
	{
		if (x == NULL)
			return(FALSE);

		if (TYPE(x) == FN && ARG(0, x) == _comma)
		{
			goal = ARG(1, x);
			x = ARG(2, x);
		}
		else
		{
			goal = x;
			x = NULL;
		}
		if (! unify(goal, f1, *body, f2))
			return(FALSE);
		body++;
	}
	return(TRUE);	
}


static int
retract(term goal, term *frame)
{
	term *first_clause, body, fn;
	term head = unbind(ARG(1, goal), frame);

	if (TYPE(head) == FN && ARG(0, head) == _neck)
	{
		body = ARG(2, head);
		head = ARG(1, head);
	}
	else
		body = _true;

	first_clause = get_proc(head, frame);

	if (*first_clause != NULL && TYPE(*first_clause) != CLAUSE)
		fail("Can't retract a built-in predicate");

	while (*first_clause != NULL)
	{
		term *new_frame = new_local_frame(NVARS(*first_clause));
		term *old_global = global;

		if (unify(HEAD(*first_clause), new_frame, head, frame)
		&&  unify(mkbody(BODY(*first_clause)), new_frame, body, frame))
		{
			term p = *first_clause;

			check_last_clause(p);
			*first_clause = NEXT(p);
			free_term(p);

			if (rest_of_clause())
				break;
			_untrail();
		}
		else
			first_clause = &NEXT(*first_clause);

		local = new_frame;
		global = old_global;
	}
	return(FALSE);
}


/************************************************************************/
/*				abolish					*/
/************************************************************************/

static term _slash;

static int
abolish(term goal, term *frame)
{
	term *p, x = check_arg(1, goal, frame, FN, IN);
	int i;

	if (ARG(0, x) != _slash
	||  ARITY(x) != 2
	||  TYPE(ARG(1, x)) != ATOM
	||  TYPE(ARG(2, x)) != INT
	   )
		fail("argument should have the form \"atom/arity\"");

	p = &PROC(ARG(1, x));
	i = IVAL(ARG(2, x));

	while (*p != NULL)
	{
		if (ARITY(HEAD(*p)) == i)
		{
			term q = *p;
			*p = NEXT(q);
			free_term(q);
		}
		else
			p = &NEXT(*p);
	}
	return(TRUE);
}


/************************************************************************/
/*		univ - turn lists into terms and vice versa		*/
/************************************************************************/

static term
list_copy(term t, term *frame)
{
	term rval;

	switch (TYPE(t))
	{
	   case BOUND:
	   		return(frame[OFFSET(t)]);
	   case LIST:
			return(gcons(_dot, gcons(copy(CAR(t), frame), gcons(copy(CDR(t), frame), _nil))));
	   case FN:
			{
				register int i, a = ARITY(t);
				register term *p = &rval;
				
				for (i = 0; i <= a; i++)
				{
					*p = gcons(copy(ARG(i, t), frame), _nil);
					p = &CDR(*p);
				}

				return(rval);
			}
	   default:
			return(t);
	}
}


static term
fn_create(term x, term *frame)
{
	if (CAR(x) == _dot)
	{
		term first;

		if (TYPE(x = CDR(x)) != LIST)
			fail("=.. - malformed list in second argument");
		first = copy(CAR(x), frame);
		if (TYPE(x = CDR(x)) != LIST)
			fail("=.. - malformed list in second argument");
		return(gcons(first, copy(CAR(x), frame)));
	}
	else
	{
		int i = 0, L = length(x, frame);
		term rval = new_g_fn(L - 1);

		while (x != _nil)
		{
			if (TYPE(x) != LIST)
				fail("=.. - malformed list in second argument");
			ARG(i++, rval) = copy(CAR(x), frame);
			x = CDR(x);
			x = unbind(x, frame);
		}
		return(rval);	
	}
}


static int
univ(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term y = check_arg(2, goal, frame, LIST, OUT);

	switch (TYPE(x))
	{
	case FREE:
	case BOUND:
	case REF:
		return(unify(x, frame, fn_create(y, frame), frame));
	default:
		return(unify(list_copy(x, frame), frame, y, frame));
	}
}


/************************************************************************/
/*				functor					*/
/************************************************************************/

static int
functor(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term y = check_arg(2, goal, frame, ATOM, OUT);
	term z = check_arg(3, goal, frame, INT, OUT);

	switch(TYPE(x))
	{
	case ATOM:
		return(unify(y, frame, x, frame) && unify(z, frame, new_int(0), frame));
	case LIST:
		return(unify(y, frame, _dot, frame) && unify(z, frame, new_int(2), frame));
	case FN:
		return(unify(y, frame, ARG(0, x), frame) && unify(z, frame, new_int(ARITY(x)), frame));
	case ANON:
	case FREE:
	case BOUND:
	case REF:
		if (TYPE(y) == ATOM && TYPE(z) == INT)
		{
			int i = IVAL(z);

			if (i != 0)
			{
				term rval = new_g_fn(i);

				ARG(0, rval) = y;
				while (i != 0)
					ARG(i--, rval) = new_ref();

				return(unify(x, frame, rval, frame));
			}
			return(unify(x, frame, y, frame));
		}
		fail("2nd argument must be an atom and 3rd argument must be an integer");
	default:
		fail("1st argument must be a compound term, list or atom");
	}
}


/************************************************************************/
/*				arg					*/
/************************************************************************/

static int
arg(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, INT, IN);
	term y = check_arg(2, goal, frame, ANY, IN);
	term z = check_arg(3, goal, frame, ANY, OUT);
	int i = IVAL(x);

	switch(TYPE(y))
	{
	case LIST:
			if (i < 0 || i > 2)
				fail("A list has only two components");
			if (i == 0)
				return(unify(z, frame, _dot, frame));
			else
				return(unify(z, frame, ARG(i-1, y), frame));
	case FN:
			if (i < 0 || i > ARITY(y))
				fail("index out of range");
			return(unify(z, frame, ARG(i, y), frame));
	default:
			fail("1st argument must be c compund term or list");
	}
}


/************************************************************************/
/*				numbervars				*/
/************************************************************************/


static term
new_var_name(int *i)
{
	char buf[16];

	sprintf(buf, "$%d", (*i)++);
	return(intern(buf));
}

void
numbervars(term t, term *frame, int *count)
{
	extern term trail;

	repeat
		switch (TYPE(t))
		{
		   case FREE:
		   		frame[OFFSET(t)] = new_var_name(count);
				return;
		   case BOUND:
				t = frame[OFFSET(t)];
				break;
		   case REF:
				if (POINTER(t) == NULL)
				{
					POINTER(t) = new_var_name(count);
					TRAIL(t) = trail;
					trail = t;
					return;
				}
				t = POINTER(t);
				break;
		   case LIST:
		   		numbervars(CAR(t), frame, count);
		   		t = CDR(t);
		   		break;
		   case FN:
				{
					int i, a = ARITY(t);
	
					for (i = 0; i <= a; i++)
						numbervars(ARG(i, t), frame, count);
					return;
				}
		   default:
				return;
		}
}


static int
_numbervars(term goal, term *frame)
{
	term t = check_arg(1, goal, frame, ANY, IN);
	term n = check_arg(2, goal, frame, INT, IN);
	term m = check_arg(3, goal, frame, INT, OUT);
	int count = IVAL(n);

	numbervars(t, frame, &count);
	return(unify(m, frame, new_int(count), frame));
}


/************************************************************************/
/*		   Get clause list attached to an atom			*/
/************************************************************************/

static int
clause_list(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);
	term y = check_arg(2, goal, frame, CLAUSE, OUT);

	if (PROC(x) == NULL)
		return(FALSE);
	return(unify(y, frame, PROC(x), frame));
}


/************************************************************************/
/*		     Get next clause in clause list			*/
/************************************************************************/

static int
next_clause(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, CLAUSE, IN);
	term y = check_arg(2, goal, frame, CLAUSE, OUT);

	if (NEXT(x) == NULL)
		return(FALSE);
	return(unify(y, frame, NEXT(x), frame));
}


/************************************************************************/
/*		   unlink a clause from a clause list			*/
/************************************************************************/

static int
unlink_clause(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, CLAUSE, IN);
	term *p, a = HEAD(x);

	if (TYPE(a) == FN)
		a = ARG(0, a);

	for (p = &PROC(a); *p != NULL; p = &NEXT(*p))
		if (*p == x)
		{
			*p = NEXT(*p);
			return(TRUE);
		}
	return(FALSE);
}


/************************************************************************/
/*				init					*/
/************************************************************************/

void
db_init(void)
{
	_slash = intern("/");

	new_pred(p_add_clause, "add_clause");
	new_pred(assert, "assert");
	new_pred(assert, "assertz");
	new_pred(asserta, "asserta");
	new_pred(retract, "retract");
	new_pred(abolish, "abolish");
	new_pred(_clause, "clause");
	defop(700, XFX, new_pred(univ, "=.."));
	new_pred(functor, "functor");
	new_pred(arg, "arg");
	new_pred(_numbervars, "numbervars");

	new_pred(clause_list, "clause_list");
	new_pred(next_clause, "next_clause");
	new_pred(unlink_clause, "unlink_clause");
}

