/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





/*			Output routines				*/

#include "g.h"


extern char run;

static binding *frame = 0;

static
prints(str)
register char *str;
{
	while (*str) putc(*str++, output);
}

sprints(str)
register char *str;
{
	register char c;

	while (c = *str++)
		switch (c)
		{
		   case 07:	fprintf(output, "\\b");
				break;
		   case '\n':	fprintf(output, "\\n");
				break;
		   case '\t':	fprintf(output, "\\t");
				break;
		   case '\'':
		   case '"':	fprintf(output, "\\%c", c);
				break;
		   default:	if (c < 32)
					fprintf(output, "\\%d", c);
				else putc(c, output);
				break;
		}
}

static
print_list(l)
pval l;
{
	register pval x = l;
	binding *old_frame = frame;

	putc('[', output);
	while(TYPE(x) == LIST)
	{
		_prin(x -> c.term[0], 999);
		x = x -> c.term[1];

		if (TYPE(x) == VAR)
			if (run)
			{
				unbind(x, frame);
				if (TYPE(termb) == VAR)
				{
					fprintf(output, " | _%d",
						(int)(frameb-stack) + termb -> v.offset);
					break;
				}
				else {
					x = termb;
					frame = frameb;
				}
			}
			else if (x -> v.pname)
			{
				fprintf(output, " | %s", NAME(x -> v.pname));
				break;
			}
			else {
				fprintf(output, "_%d", x -> v.offset);
				break;
			}
		if (x != (pval) nil)
		{
			if (TYPE(x) != LIST)
			{
				fprintf(output, " | ");
				_prin(x, 999);
				break;
			}
			else	fprintf(output, ", ");
		}
	}
	putc(']', output);
	frame = old_frame;
}

static
print_op(x,p_pred)
register compterm *x;
register short p_pred;
{
	short left_pred, right_pred;
	register precedence;
	
	precedence = x -> term[0] -> a.pred;
	if (p_pred < precedence) putc('(', output);
	switch(x -> term[0] -> a.op_t)
	{

    	    case FX	: 
	    case FY	: prints(NAME(x -> term[0]));
			  putc(' ', output);
			  _prin(x -> term[1], x -> term[0] -> a.pred);
			  break;
	    case XFX	: left_pred = right_pred = (x->term[0]->a.pred) - 1;
			  goto L;
	    case XFY	: right_pred = x -> term[0] -> a.pred;
			  left_pred = right_pred - 1;
			  goto L;
	    case YFX	: left_pred = x -> term[0] -> a.pred;
			  right_pred = left_pred - 1;
	    L		: _prin(x -> term[1], left_pred);
			  putc(' ', output);
			  prints(NAME(x -> term[0]));
			  putc(' ', output);
			  _prin(x -> term[2], right_pred);
			  break;
	    case XF	:
	    case YF	: _prin(x -> term[1],x -> term[0] -> a.pred);
			  putc(' ', output);
			  prints(NAME(x -> term[0]));
			  break;
		
	    default	: fprintf(output, "UNKNOWN OP");
	}
	
	if ( p_pred < (x -> term[0] -> a.pred)) putc(')', output);
}



static
print_strip(t, n)
register pval t[];
register char n;
{
	register i;

	for (i = 1; i < n; i++)
	{
		_prin(t[i], 999); putc(',', output); putc(' ', output);
	}
	if (n != 0) _prin(t[i], 999);
}

_prin(x,p_pred)
register pval x;
register short p_pred;
{
	register i;
	register pval t;
	char *mode;


	if (x == 0) {putc('_', output); return;}
	if (x == (pval) nil) {fprintf(output, "[]"); return;}
	switch(TYPE(x))
	{
	   case PREDEF	:
	   case ATOM	: switch (x -> a.op_t)
			  {
			     case FX:
			     case FY:
				fprintf(output, "prefix %s", NAME(x));
				break;
			     case XFY:
			     case YFX:
			     case XFX:
				fprintf(output, "infix %s", NAME(x));
				break;
			     case XF:
			     case YF:
				fprintf(output, "postfix %s", NAME(x));
				break;
			     case QATOM:
				putc('\'', output); sprints(NAME(x));
				putc('\'', output);
				break;
			     default:
				fprintf(output, NAME(x));
				break;
			  }
			  break;
	   case VAR	: if (run)
			  {
				unbind(x,frame);
				if (TYPE(termb) == VAR)
					fprintf(output, "_%d",
						(int)(frameb-stack) + termb -> v.offset);
				else prin(termb, p_pred, frameb);
			  }
			  else if (x -> v.pname)
				 fprintf(output, NAME(x -> v.pname));
			  else fprintf(output, "_%d", x -> v.offset);
			  break;
	   case INT	: if (x == stack_int)
				fprintf(output, "%d", frame);
			  else fprintf(output, "%d", x -> i.int_val);
			  break;
	   case REAL	: fprintf(output, "%.16g", x -> r.real_val);
			  break;
	   case LIST	: print_list(x); break;
	   case FN	: t = x -> c.term[0];
			  if (t == (pval) _lbrace)
			  {
				putc('{', output);
				_prin(x -> c.term[1], 1200);
				putc('}', output);
				break;
			  }
#ifdef PRINC_VAR
			  if (isvariable(t))
			  {
				_prin(t, p_pred);
				putc('(', output);
				print_strip(x -> c.term, x -> c.size);
				putc(')', output);
				break;
			  }
#endif
			  if (t -> a.op_t == NONOP || t -> a.op_t == QATOM)
			  {
				if (t -> a.op_t == QATOM)
				{
					putc('\'', output);
					sprints(NAME(t));
					putc('\'', output);
				}
				else
					fprintf(output, NAME(t));
				putc('(', output);
				print_strip(x -> c.term, x -> c.size);
				putc(')', output);
			  }
			  else print_op(x,p_pred);
			  break;
	   case CLAUSE	: _prin(x -> g.goal[0], p_pred);
			  i = 1;
			  if (x -> g.goal[i])
			  {
				fprintf(output, " :- ");
				repeat
				{
					_prin(x -> g.goal[i++], 999);
					if (x -> g.goal[i])
						fprintf(output, ", ");
					else break;
				}
			  }
			  putc('.', output);
			  break;
	   case	STREAM	: switch (x->s.mode)
			  {
				case CLOSED:	mode = "closed"; break;
				case R_MODE:	mode = "read";   break;
				case W_MODE:	mode = "write";  break;
				case A_MODE:	mode = "append"; break;
			  }
			  fprintf(output,"# %s STREAM- %s #",mode,x->s.sname);
			  break;
	   default	: fprintf(output, "# %d #", TYPE(x));
	}
}


prin(x, p_pred, i)
pval x;
short p_pred;
binding *i;
{
	binding *old_frame = frame;
	frame = i;
	_prin(x,p_pred);
	frame = old_frame;
}

print(x, p_pred, i)
pval x;
short p_pred;
binding *i;
{
	prin(x, p_pred, i);
	putc('\n', output);
	fflush(stdout);
}
