/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





		/*	functions to make term and clauses	*/

#include "g.h"
#include <stdio.h>



pval make(x, ind)
pval x;
binding *ind;
{
	register pval rval;
	int i, limit;
	char buf[16];

	switch(TYPE(x))
	{
	   case ATOM	: if (isvar(NAME(x)) && x -> a.op_t != QATOM)
				return((pval) variable((atom *) x));
	   case PREDEF	: return(x);
	   case INT	: rval = new(INT);
			  rval -> i.int_val =
			  	(x == stack_int ? (int) ind : x -> i.int_val);
			  return(rval);
	   case VAR	: unbind(x, ind);
			  if (TYPE(termb) == VAR)
			  {
				sprintf(buf, "_%d",
					(int)(frameb-stack) + termb->v.offset);
				rval = (pval) intern(buf, strlen(buf));
				if (varcell) return((pval) variable(rval));
				else return(rval);
			  }
			  else return(make(termb, frameb));
	   case FN	: rval = (pval) record(x -> c.size);
#ifdef PRINC_VAR
			  rval -> c.term[0] = make(x -> c.term[0], ind);
#else
			  rval -> c.term[0] = x -> c.term[0];
#endif
			  i = 1;
			  limit = x -> c.size;
			  break;
	   case LIST	: rval = (pval) record(1);
			  i = 0; limit = 1;
			  break;
	   case STREAM  : x->s.ref_cnt++;
			  return(x);
	   case REAL	: rval = new(REAL);
			  rval -> r.real_val = x -> r.real_val;
			  return(rval);
	   default	: fprintf(stderr,"HERE");
			  return((pval)NULL);
	}

	TYPE(rval) = TYPE(x);
	while (i <= limit)
	{
		rval -> c.term[i] = make(x -> c.term[i], ind);
		i++;
	}
	return(rval);
}


pval mkbody(x, f, n)
pval x;
binding *f;
int n;
{
	register pval rval;

	if (isvariable(x))
	{
		unbind(x, f);
		x = termb;
		f = frameb;
	}
	if ((TYPE(x) != FN) || (x -> c.term[0] != (pval) _comma))
	{
		rval = (pval) create(n, 0);
		rval -> g.goal[n] = make(x, f);
	}
	else {
		rval = mkbody(x -> c.term[2], f, n + 1);
		rval -> g.goal[n] = make(x -> c.term[1], f);
	}
	return(rval);
}

pval mkclause(x, f)
pval x;
binding *f;
{
	register pval rval;

	if (iscompound(x) && (x -> c.term[0] == (pval) _neck))
	{
		rval = mkbody(x -> c.term[2], f, 1);
		rval -> g.goal[0] = make(x -> c.term[1], f);
	}
	else {
		rval = (pval) create(0, 0);
		rval -> g.goal[0] = make(x, f);
	}
	return(rval);
}
