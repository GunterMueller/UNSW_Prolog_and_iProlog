/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/

/* Andrew N - added reals Wed Oct 8 1986 */

		/* MEMORY ALLOCATION ROUTINES */

#include "g.h"



	/*
	 * stack_real is used by builtins that create a real data structure
	 * the space is recovered on backtracking
	 */

pval
stack_real(val)
double val;
{
	pval ret;

	ret = new(REAL);
	ret->r.real_val = val;
	trail_pointer(ret);

	return(ret);
}


pval new(t)
register itemtype t;
{
	register pval rval; 

	switch(t)
	{
	   case ATOM	: rval = (pval) halloc(sizeof(atom));
#ifdef PROFILING
			  rval -> a.tflags = (profiling ? PROFILED : 0);
#else
			  rval -> a.tflags = 0;
#endif
			  rval -> a.lib = 0;
			  rval -> a.val = 0;
			  rval -> a.pred = 0;
			  rval -> a.op_t = NONOP;
			  rval -> a.link = 0;
			  break;
	   case VAR	: rval = (pval) halloc(sizeof(var));
			  rval -> v.offset = 0;
			  break;
	   case INT	: rval =  (pval) halloc(sizeof(integer));
			  rval -> i.int_val = 0;
			  break;
	   case LIST	: rval = (pval) record(1);
			  rval -> c.type = LIST;
			  break;
	   case	STREAM	: rval = (pval) halloc(sizeof(stream));
			  rval -> s.ref_cnt = 1;
			  break;
	   case REAL	: rval =  (pval) halloc(sizeof(real));
			  rval -> r.real_val = (float) 0.0;
			  break;
	}

	TYPE(rval) = t;
	return(rval);
}


compterm *record(n)
char n;
{
	register compterm *r;

	r =  (compterm *) halloc(sizeof(compterm) + n * WORD_LENGTH);
	r -> type = FN;
	r -> size = n;
	return(r);
}



clause *create(ngoals, nvars)
int ngoals;
int nvars;
{
	register clause *r;

	r = (clause *) halloc(sizeof(clause) + (1 + ngoals) * WORD_LENGTH);
	r -> type = CLAUSE;
	r -> nvars = nvars;
	r -> rest = 0;
	r -> goal[0] = r -> goal[ngoals + 1] = 0;

#ifdef PROFILING
	if (profiling)
		prof_init_clause(r, ngoals);
#endif
	return(r);
}


/*	Hash table use to uniquely store atoms	*/

atom *hashtable[HASHSIZE];

hash(string)
register char *string;
{
	register h = 0;

	while (*string)
		h += *string++;
	return(h & 0177);
}




pval intern(string, size)
char *string;
int size;
{
	register h;
	register atom *p;

	h = hash(string);
	for (p = hashtable[h]; p != 0; p = p -> link)
		if (strcmp(string, NAME(p)) == 0)
			return((pval) p);
	p = (atom *) new(ATOM);
	p -> link = hashtable[h];
	hashtable[h] = p;
	p -> name = (char *)halloc(size+1);
	strcpy(p -> name, string);
	return((pval) p);
}



/*   Data structures to compute the offset for each variable	*/


var **varcell;


var *variable(id)
register atom *id;
{
	extern int argn;
	register i;
	register var *rval;

	if (id != anon)
		for (i = 0; i < argn; i++)
			if (id == varcell[i] -> pname) return(varcell[i]);
	if (++argn > MAXVAR) error("TOO MANY VARIABLES IN CLAUSE", FALSE);
	varcell[argn-1] = rval = (var *) new(VAR);
	rval -> offset = argn - 1;
	rval -> pname = id;
	return(rval);
}




pval in_uniop(oper,opand)
pval oper,opand;
{
	pval rval;

	rval = (pval) record(1);
	rval -> c.term[0] = oper;
	rval -> c.term[1] = opand;
	return(rval);
}





pval in_biop(oper, opand1, opand2)
pval oper, opand1, opand2;
{
	pval rval;

	rval = (pval) record(2);
	rval -> c.term[0] = oper;
	rval -> c.term[1] = opand1;
	rval -> c.term[2] = opand2;
	return(rval);
}


check_clause(cl)
clause *cl;
{
	atom *a;
	extern int library;
	extern atom *_fail;

	if (isatom(cl -> goal[0]))
		a = (atom *) cl -> goal[0];
	else if (iscompound(cl->goal[0]) && isatom(cl->goal[0]->c.term[0]))
		a = (atom *) (cl -> goal[0] -> c.term[0]);
	else	error("Bad principal functor in clause head", FALSE);

	if (TYPE(a) == PREDEF || (a->lib && ! library && a->val) || a == _fail)
	{
		fprintf(stderr, "*** %s ***\n", NAME(a));
		error("can't redefine a predefined function", FALSE);
	}
}


clause 	*Q;
atom 	*same_proc = 0;

atom *add_clause(cl)
register clause *cl;
{
	register atom *a;
	register clause *p;
	extern int library, read_err;

	if (read_err) return(0);
	check_clause(cl);
	a = (atom *) cl -> goal[0];
	if (iscompound(a)) a = (atom *) ((compterm *)a)->term[0];

	if (a == same_proc)
	{
		Q -> rest = cl;
		Q = Q -> rest;
	}
	else {
		same_proc = a;
		if (a -> val == 0) Q = a -> val = cl;
		else {
			p = a -> val;
			while (p -> rest != 0) p = p -> rest;
			Q = p -> rest = cl;
		}
		a -> lib = library;
	}
	return(a);
}



	/*		Garbage disposal		*/


free_term(t)
pval t;
{
	register i, limit;

	if (t == 0) return;
	switch(TYPE(t))
	{
	   case ATOM	: return;
	   case INT	: hfree(t, sizeof(integer)); return;
	   case REAL	: hfree(t, sizeof(real)); return;
	   case	VAR	: hfree(t, sizeof(var)); TYPE(t) = FREE;
			  return;
	   case FN	:
#ifndef PRINC_VAR
			  i = 1;
			  break;
#endif
	   case LIST	: i = 0;
			  break;
	   case CLAUSE	: free_term(t->g.goal[0]); /* head may be zero */
			  for (i = 1; t -> g.goal[i]; i++)
				free_term(t -> g.goal[i]);
			  hfree(t, sizeof(clause) + i * WORD_LENGTH);
			  same_proc = (atom *) 0;
			  return;
	   case STREAM	: if (--t->s.ref_cnt == 0) hfree(t, sizeof(stream));
	   case PREDEF	:
	   case FREE	: return;
	   default	: fprintf(stderr,
				  "\nProlog error: FREE - Unknown type %d\n",
				 TYPE(t));
			  exit(1);
	}

	limit = SIZE(t);
	while (i <= limit)
		free_term(t -> c.term[i++]);
	hfree(t, sizeof(compterm) + limit * WORD_LENGTH);
}


free_proc(p)
clause *p;
{
	register clause *q;
	register i;

	while (p)
	{
		q = p;
		p = p -> rest;
		for (i = 0; q -> goal[i]; i++)
			free_term(q -> goal[i]);

		hfree(q, sizeof(clause) + i * WORD_LENGTH);
	}
	same_proc = (atom *) 0;
}
