/************************************************************************/
/*     		 functions to copy terms and clauses onto heap		*/
/************************************************************************/

#include "prolog.h"

extern term _is;

term varlist;


term
new_var(int type, int offset, term name)
{
   	term rval = halloc(sizeof(var));

	TYPE(rval) = type;
	FLAGS(rval) = 0;
	OFFSET(rval) = offset;
	PNAME(rval) = name;
	return(rval);
}


term
new_ref(void)
{
	term rval = galloc(sizeof(ref));

	TYPE(rval) = REF;
	FLAGS(rval) = COPY;
	POINTER(rval) = NULL;
	TRAIL(rval) = NULL;
	return(rval);
}


term
new_clause(int n)
{
	term rval = halloc(sizeof(clause) + (n + 1) * WORD_LENGTH);

	TYPE(rval) = CLAUSE;
	FLAGS(rval) = 0;
	NVARS(rval) = 0;
	NEXT(rval) = NULL;
	LABEL(rval) = NULL;
	GOAL(0, rval) = GOAL(n + 1, rval) = NULL;
	return(rval);
}


term
new_h_fn(int n)
{
	term rval = halloc(sizeof(compterm) + n * WORD_LENGTH);

	TYPE(rval) = FN;
	FLAGS(rval) = 0;
	ARITY(rval) = n;
	return(rval);
}


term
new_g_fn(int n)
{
	term rval = galloc(sizeof(compterm) + n * WORD_LENGTH);

	TYPE(rval) = FN;
	FLAGS(rval) = 0;
	ARITY(rval) = n;
	return(rval);
}


term
h_fn1(term f, term arg)
{
	term rval = new_h_fn(1);

	ARG(0, rval) = f;
	ARG(1, rval) = arg;
	return(rval);
}


term
g_fn1(term f, term arg)
{
	term rval = new_g_fn(1);

	ARG(0, rval) = f;
	ARG(1, rval) = arg;
	return(rval);
}


term
h_fn2(term f, term arg1, term arg2)
{
	term rval = new_h_fn(2);

	ARG(0, rval) = f;
	ARG(1, rval) = arg1;
	ARG(2, rval) = arg2;
	return(rval);
}


term
g_fn2(term f, term arg1, term arg2)
{
	term rval = new_g_fn(2);

	ARG(0, rval) = f;
	ARG(1, rval) = arg1;
	ARG(2, rval) = arg2;
	return(rval);
}


term
new_unit(term head)
{
	term rval = new_clause(0);

	HEAD(rval) = head;
	return(rval);
}


term
lookup_var(term id)
{
	static term anon_struct = NULL;
	term *p, rval;

 	if (id == _anon)
	{
		if (anon_struct == NULL)
			anon_struct = new_var(ANON, 0, _anon);
		return(anon_struct);
	}

	for (p = &varlist; *p != _nil; p = &CDR(*p))
		if (id == CAR(CAR(*p)))
			return(CDR(CAR(*p)));

	*p = gcons(gcons(id, rval = new_ref()), _nil);

	return(rval);
}


static term
variable(term r)
{
	int i;
	term *p, rval;
	char buf[16];
 
 	for (p = &varlist, i = 0; *p != _nil; p = &CDR(*p), i++)
		if (r == CDR(CAR(*p)))
			switch (TYPE(CAR(CAR(*p))))
			{
			   case ATOM:
					rval = new_var(FREE, i, CAR(CAR(*p)));
					CAR(CAR(*p)) = rval;
					return(rval);
			   case FREE:
					rval = new_var(BOUND, i, PNAME(CAR(CAR(*p))));
					CAR(CAR(*p)) = rval;
					return(rval);
			   case BOUND:
					return(CAR(CAR(*p)));
			}
 
 	/* sprintf(buf, "_%ld", (long)(r)); */
 	sprintf(buf, "_%d", i);
	rval = new_var(FREE, i, intern(buf));
	*p = gcons(gcons(rval, r), _nil);
	return(rval);
}

 
term
make(term t, term *frame)
{
	term rval;
	int i;
 
	repeat
	{
		switch (TYPE(t))
		{
		   case FREE:
	   			frame[OFFSET(t)] = rval = galloc(sizeof(ref));
	   			TYPE(rval) = REF;
				FLAGS(rval) = COPY;
				POINTER(rval) = NULL;
				return(variable(rval));
		   case BOUND:
				if (frame == NULL)
		   			fail("Tried to ground an unbound variable");
		   		t = frame[OFFSET(t)];
		   		break;
		   case REF:
				if (POINTER(t) == NULL)
					return(variable(t));
				else
					t = POINTER(t);
				break;
		   case INT:
		   		rval =  halloc(sizeof(integer));
				TYPE(rval) = INT;
				FLAGS(rval) = 0;
				IVAL(rval) = IVAL(t);
				return(rval);
		   case REAL:
		   		rval = halloc(sizeof(real));
		   		TYPE(rval) = REAL;
				FLAGS(rval) = 0;
				RVAL(rval) = RVAL(t);
				return(rval);
		   case LIST:
		   case FN:
		   		rval = halloc(sizeof(compterm) + ARITY(t) * WORD_LENGTH);
				TYPE(rval) = TYPE(t);
				FLAGS(rval) = 0;
				ARITY(rval) = ARITY(t);

				for (i = 0; i <= ARITY(t); i++)
					ARG(i, rval) = make(ARG(i, t), frame);
				return(rval);
		   default:
				return(t);
		}
	}
}


static term
mkbody(term x, term *frame, int n)
{
	term rval, tmp;
 
 	DEREF(x);
	if (TYPE(x) != FN || ARG(0, x) != _comma)
	{
		tmp = make(x, frame);
		rval = new_clause(n);
		GOAL(n, rval) = tmp;
	}
	else
	{
		tmp = make(ARG(1, x), frame);
		rval = mkbody(ARG(2, x), frame, n + 1);
		GOAL(n, rval) = tmp;
	}
	return(rval);
}


int
n_vars(term v)
{
	int i;
	
	for (i = 0; v != _nil; v = CDR(v))
		i++;

	return(i);
}

term
mkclause(term x, term *frame)
{
	extern term *global;
 	term rval, *old_global = global;

	if (iscompound(x) && ARG(0, x) == _neck)
	{
		if (TYPE(ARG(1, x)) == FN && ARG(0, ARG(1, x)) == _is)
		{
			term tmp, tmp1, tmp2;

			tmp1 = make(ARG(1, ARG(1, x)), frame);
			rval = mkbody(ARG(2, x), frame, 1);
			tmp2 = make(ARG(2, ARG(1, x)), frame);
			tmp = new_h_fn(2);
			ARG(0, tmp) = _is;
			ARG(1, tmp) = tmp1;
			ARG(2, tmp) = tmp2;
			HEAD(rval) = tmp;
		}
		else
		{
			term tmp = make(ARG(1, x), frame);
			rval = mkbody(ARG(2, x), frame, 1);
			HEAD(rval) = tmp;
		}
	}
	else
	{
		rval = new_clause(0);
		HEAD(rval) = make(x, frame);
	}
	NVARS(rval) = n_vars(varlist);
	varlist = _nil;
	global = old_global;
	return(rval);
}



static term last_predicate = NULL, last_clause = NULL;

term
add_clause(term cl, int at_beginning)
{
	term a, head = HEAD(cl);

L:	switch (TYPE(head))
	{
	   case ATOM:	a = head;
			break;
	   case FN:	if (isatom(ARG(0, head)))
			{
				a = ARG(0, head);
				if (a == _is)
				{
					head = ARG(1, head);
					goto L;
				}
				break;
			}
	   default:	print(cl);
			warning("Bad principal functor in clause head");
			return(_nil);
	}

	if ((FLAGS(a) & PREDEF) || (FLAGS(a) & LOCK))
	{
		char buf[128];

		sprintf(buf, "\"%s\" is already defined by Prolog", NAME(a));
		warning(buf);
		FLAGS(a) &= ~PREDEF;
		PROC(a) = NULL;
	}

	if (at_beginning)
	{
		NEXT(cl) = PROC(a);
		PROC(a) = cl;
	}
	else if (a == last_predicate && NEXT(last_clause) == NULL)
		NEXT(last_clause) = cl;
	else
	{
		term *p;

		for (p = &PROC(a); *p != NULL; p = &NEXT(*p));
		*p = cl;
	}

	if (input != stdin)
		add_to_proc_list(a);

	/* Remember last clause to speed up adding new clauses to clause list */

	last_predicate = a;
	last_clause = cl;

	return(a);
}


void check_last_clause(term cl)
{
	if (last_clause == cl)
		last_predicate = last_clause = NULL;
}


term
cache_predicate(term cl)
{
	term a, head = HEAD(cl);

L:	switch (TYPE(head))
	{
	   case ATOM:	a = head;
			break;
	   case FN:	if (isatom(ARG(0, head)))
			{
				a = ARG(0, head);
				if (a == _is)
				{
					head = ARG(1, head);
					goto L;
				}
				break;
			}
	   default:	print(cl);
			warning("Bad principal functor in clause head");
			return(_nil);
	}

	NEXT(cl) = PROC(a);
	PROC(a) = cl;

	return(a);
}
