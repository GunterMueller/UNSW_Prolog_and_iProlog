#include "prolog.h"

#define BIND(x, y)	POINTER(x) = y;

static term dcg(term, term *);
static term lhs(term, term *);
static term rhs(term, term *, term);
static term tag(term, term, term);
static term append(term, term);
void assert_dcg(term, term *);
term read_atom(void);

static term start;
static term follow;

static term
dcg(term rule, term *frame)
{
	term x, y;

	if (ARITY(rule) != 2)
		fail("DCG must have left and right hand side");

	start = new_ref();
	follow = new_ref();

	x = lhs(ARG(1, rule), frame);
	y = rhs(ARG(2, rule), frame, NULL);
	BIND(start, follow);
	return (y == NULL ? x : g_fn2(_neck, x, y));
}

static term
lhs(term head, term *frame)
{
	if (TYPE(head) == FN && ARG(0, head) == _comma)
	{
		term NT = unbind(ARG(1, head), frame);
		term T = unbind(ARG(2, head), frame);
		term x;

		if (isvariable(NT))
			fail("Head of DCG is an unbound variable");
		if (TYPE(T) != LIST)
			fail("Terminal symbol in head of DCG must be given as list");
		x = append(T, follow);

		return tag(NT, start, x);
	}
	else
	{
		term NT = unbind(head, frame);

		if (isvariable(NT))
			fail("Head of DCG is an unbound variable");

		return tag(NT, start, follow);
	}
}


static term
rhs(term body, term *frame, term old_start)
{
	term rval, x, y;

	switch (TYPE(body))
	{
	case LIST:
		x = new_ref();
		rval = append(body, x);
		BIND(start, rval);
		start = x;
		return NULL;
	case FN:
		if (ARG(0, body) == _comma)
		{
			term x = rhs(ARG(1, body), frame, NULL);
			term y = rhs(ARG(2, body), frame, NULL);

			if (x == NULL && y == NULL)
				return NULL;
			if (x == NULL)
				return y;
			if (y == NULL)
				return x;

			return g_fn2(_comma, x, y);
		}
		if (ARG(0, body) == _semi_colon)
		{
			term x, y, new_start;
			term follow = new_ref();

			if (old_start == NULL)
				old_start = start;

			new_start = start = new_ref();
			x = rhs(ARG(1, body), frame, old_start);
			BIND(start, follow);
			if (x == NULL)
				x = g_fn2(_equal, old_start, new_start);
			else
				x = g_fn2(_comma, g_fn2(_equal, old_start, new_start), x);

			start = new_start = new_ref();
			y = rhs(ARG(2, body), frame, old_start);
			BIND(start, follow);
			if (y == NULL || TYPE(y) != FN || ARG(0, y) != _semi_colon)
				if (y == NULL)
					y = g_fn2(_equal, old_start, new_start);
				else
					y = g_fn2(_comma, g_fn2(_equal, old_start, new_start), y);

			start = follow;

			if (x == NULL && y == NULL)
				return NULL;
			if (x == NULL)
				return y;
			if (y == NULL)
				return x;

			return g_fn2(_semi_colon, x, y);
		}
		if (ARG(0, body) == _lbrace)
			return ARG(1, body);
	default:
		if (body == _nil)
			return NULL;
		
		rval = tag(body, (x = new_ref()), (y = new_ref()));
		BIND(x, start);
		start = y;
		return rval;
	}
}

static term
tag(term fn, term var0, term var1)
{
	term rval;
	int i;

	switch (TYPE(fn))
	{
	case ATOM:
		rval = new_g_fn(2);
		ARG(0, rval) = fn;
		ARG(1, rval) = var0;
		ARG(2, rval) = var1;
		return rval;
	case FN:
		rval = new_g_fn(ARITY(fn) + 2);
		for (i = 0; i <= ARITY(fn); i++)
			ARG(i, rval) = ARG(i, fn);
		ARG(i++, rval) = var0;
		ARG(i, rval) = var1;
		return rval;
	default:
		fail("Non-terminals must be atoms or compound terms");
	}
}


static term
append(term x, term y)
{
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

	return(rval);
}

void
assert_dcg(term rule, term *frame)
{
	extern term varlist;

	varlist = _nil;
	add_clause(mkclause(dcg(rule, frame), frame), FALSE);
}


static int
p_dcg(term goal, term *frame)
{
	term rule = check_arg(1, goal, frame, FN, IN);
	term cl = check_arg(2, goal, frame, ANY, OUT);

	return unify(cl, frame, dcg(rule, frame), frame);
}


static int
read_sentence(term goal, term *frame)
{
	term s = check_arg(1, goal, frame, LIST, OUT);
	term rval, *p = &rval;
	term punct = NULL;

	if (ARITY(goal) == 2)
		punct = check_arg(2, goal, frame, ATOM, OUT);

	repeat
	{
		term x = read_atom();
		
		if (x == _dot || x == _bang || x == _question)
		{
			if (punct != NULL)
				if (! unify(punct, frame, x, frame))
					return FALSE;
			return unify(s, frame, rval, frame);
		}

		*p = gcons(x, _nil);
		p = &CDR(*p);
	}
}


void
dcg_init(void)
{
	new_pred(p_dcg, "dcg");
	new_pred(read_sentence, "read_sentence");
}
