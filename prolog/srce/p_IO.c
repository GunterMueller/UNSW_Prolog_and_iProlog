/******************************************************************************

			UNSW Prolog (version 4.2)

		   Written by Claude Sammut/Tony Grech
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





/*		built-in predicates for simple I/O			*/

#include "pred.h"
#include "in.h"


static FILE *old_input, *old_output;
extern int readatom;
extern pval getatom();


choose_stream(port,argv, argc, mode, fullargc)
FILE **port;
pval *argv;
int argc, mode;
{
	if (argc == fullargc)
	{
		if (!check_stream(argv, mode))
			return(-1);
		*port = argv[0]->s.file;
		return(1);
	}
	if (argc == fullargc -1)
	{
		*port = (mode == INPUT ? piport : output);
		return(0);
	}

	warning("incorrect number of arguments to built-in");
	return(-1);
}

static
p_write NPREDICATE
{
	int i;

	old_output = output;
	if ((i = choose_stream(&output,arg, argc, OUTPUT, 2) ) == -1)
	    	return(FALSE);

	prin(arg[i],1200,frame[i]);
	output = old_output;

	return(TRUE);
}


static
p_getc NPREDICATE
{
	pval rval;
	int i;
	extern pval inchar();

	old_input = input;
	if ((i = choose_stream(&input,arg, argc, INPUT, 2) ) == -1)
		return(FALSE);

	rval  = inchar();
	input = old_input;
	if (isvariable(arg[i]))
	{
		bind(arg[i], frame[i], rval, 0);
		return(TRUE);
	}
	else	return(arg[i] == rval);
}


static
p_eof NPREDICATE
{
	FILE *istream;
	if (choose_stream(&istream,arg,argc, INPUT, 1) == -1)
		return(FALSE);

	return(feof(istream));
}


static
p_ungetc NPREDICATE
{
	int i;
	FILE *istream;

	if ((i = choose_stream(&istream,arg,argc,  INPUT, 2)) == -1)
		return(FALSE);

	if (TYPE(arg[i]) != ATOM)
		fail("Ungetc - argument must be an atom")

	if (ungetc(NAME(arg[i])[0], istream) == EOF)
		fail("ungetc - couldn't push character back")
	return(TRUE);
}

static
p_skip NPREDICATE
{
	register ch;
	int i;
	FILE *istream;

	if ((i = choose_stream(&istream,arg,argc,  INPUT, 2 )) == -1)
		return(FALSE);

	if (!isatom(arg[i]))
		 fail("Skip - argument must be atom")

	ch = NAME(arg[i])[0];
	while (ch != getc(istream))
		if (feof(istream))
			return(FALSE);
	return(TRUE);
}



static
p_read NPREDICATE
{
	register pval p, expr;
	int i;

	extern pval expression();
	extern int pushed_back;
	extern atom *_dot, *_rpren;
	extern int p_read_on;

	old_input = input;
	if ((i = choose_stream(&input,arg, argc, INPUT,2 )) == -1)
		return(FALSE);

	p_read_on = TRUE;
	p = expression(_rpren);
	p_read_on = FALSE;
	if (p -> c.term[0] == (pval) _dot)
	{
		expr = p -> c.term[1];
		p -> c.term[1] = 0;
		free_term(p);
	}
	else expr = p;
	trail_pointer(expr);
	if (feof(input)) pushed_back = -1;

	input = old_input;
	return(unify(arg[i], frame[i], expr, frame[i]));
}

static
noquote NPREDICATE
{
	int i;
	old_output = output;
	if ((i = choose_stream(&output,arg,argc,  OUTPUT,2 )) == -1)
		return(FALSE);

	if (!isatom(arg[i]))
	{
		output = old_output;
		fail("Noquote - argument must be an atom");
	}
	sprints(NAME(arg[i]));
	output = old_output;
	return(TRUE);
}

static
p_putc NPREDICATE
{
	int i;
	FILE *ostream;
	if ((i = choose_stream(&ostream,arg,argc,  OUTPUT , 2)) == -1)
		return(FALSE);


	if (! isatom(arg[i]))
		fail("putc - argument must be an atom")

	putc(NAME(arg[i])[0], ostream);
	return(TRUE);
}


static
p_nputs NPREDICATE
{
	register i;
	char *string;
	FILE *ostream;
	pobj result;
	extern arith_failure;

	if ((i = choose_stream(&ostream, arg, argc, OUTPUT , 3)) == -1)
	    return(FALSE);

	if (! isatom(arg[i])) fail("nputs - bad arg")
	string = NAME(arg[i]);

	arith(arg[i+1], frame[i+1], &result);
	if (arith_failure) return(FALSE);

	if (result.c.type == REAL)
		i = (int) result.r.real_val;
	else	i = result.i.int_val;

	while (i-- > 0) fprintf(ostream, string);

	return(TRUE);
}


static
newline NPREDICATE
{
	FILE *ostream;
	if (choose_stream(&ostream,arg,argc,  OUTPUT ,1) == -1)
	    return(FALSE);

	putc('\n', ostream);
	return(TRUE);
}

_list_proc(proc)
pval proc;
{
	clause *clist;
	int i;
	extern char run;

	if (TYPE(proc) != ATOM)
		return;
	run = FALSE;
	putc('\n', output);
	for (clist = VAL(proc); clist != 0; clist = clist -> rest)
	{
		_prin(clist -> goal[0],1200);
		if (clist -> goal[1])
		{
			if (clist -> goal[2]) fprintf(output, " :-\n\t");
			else fprintf(output, " :- ");
			for (i = 1; clist -> goal[i + 1]; i++)
			{
				_prin(clist -> goal[i], 999);
				fprintf(output, ",\n\t");
			}
			_prin(clist -> goal[i], 999);
		}
		fprintf(output, ".\n");
	}
	run = TRUE;
}


static
listing NPREDICATE
{
	register h;
	register atom *p;
	extern atom *hashtable[];

	old_output = output;
	if (choose_stream(&output,arg,argc,  OUTPUT, 1) == -1)
		 return(FALSE);

	for (h = 0; h < HASHSIZE; h++)
	{
		p = hashtable[h];
		while (p)
		{
			if (! p -> lib && p -> val) _list_proc((pval)p);
			p = p -> link;
		}
	}
	output = old_output;
	return(TRUE);
}



static
msg(arg, frame, argc, newline)
pval arg[];
binding **frame;
int argc;
{
	register  i = 0;

	old_output	= output;
	if (argc != 0)
		switch (i =  _check_stream(arg, OUTPUT))
		{
			case  TRUE: output	= arg[0]->s.file;
			case FALSE: break;
			default:    return(FALSE);
		}
	for (;i < argc;i++)
	{
		if (arg[i] == (pval) nil)
			fprintf(output, "[]");
		else if (isatom(arg[i]))
			fprintf(output, "%s", NAME(arg[i]));
		else
			prin(arg[i], 1200, frame[i]);
	}
	if (newline)	putc('\n', output);
	output = old_output;

	return( TRUE );
}


static
mess_ln NPREDICATE
{
	return( msg(arg, frame, argc,TRUE));
}

static
message NPREDICATE
{
	return( msg(arg, frame, argc,FALSE));
}


static pval push_buf[LOOKAHEAD];
static int next_atom;
static int can_unratom = 0;

clear_buf()
{
	can_unratom = 0;
}


static
buf_ratom PREDICATE
{
	pval a;

	old_input = input;
	input	  = piport;

	readatom = TRUE;
	a = push_buf[next_atom] = getatom();
	input	  = old_input;
	readatom = FALSE;

	next_atom = (next_atom + 1) % LOOKAHEAD;
	if (can_unratom < LOOKAHEAD)
		can_unratom++;

	if (isvariable(arg[0]))
	{
		bind(arg[0], frame[0], a, 0);
		return(TRUE);
	}
	else	return(arg[0] == a);
}



static
unratom PREDICATE
{
	if (can_unratom <= 0)
		fail("# - atom has been lost from buffer")

	can_unratom--;
	if (--next_atom == -1) next_atom = LOOKAHEAD - 1;
	ungetatom(push_buf[next_atom]);
	return(FALSE);
}


static
ratom NPREDICATE
{
	int i;
	pval a;

	old_input = input;
	if ((i = choose_stream(&input,arg,argc,  INPUT, 2)) == -1)
		return(FALSE);

	readatom = TRUE;
	a = getatom();
	input = old_input;
	readatom = FALSE;

	if (isvariable(arg[i]))
	{
		bind(arg[i], frame[i], a , 0);
		return(TRUE);
	}
	else	return(arg[i] == a);
}


static
list_proc PREDICATE
{
	_list_proc(arg[0]);
	return(TRUE);
}


static
ascii PREDICATE
{
	register i;
	register pval rval;
	char buf[2];

	if (isatom(arg[0]))
	{
		if (isinteger(arg[1]))
			return(((int) NAME(arg[0])[0]) == INT_VAL(1));
		else if (isvariable(arg[1]))
		{
			bind_num(1, ((int) NAME(arg[0])[0]));
			return(TRUE);
		}
	}
	else if (isvariable(arg[0]) && isinteger(arg[1]))
	{
		i = INT_VAL(1);
		if (i < 0 || i > 127)
			fail("Ascii - number out of range")
		buf[0] = (char) i;
		buf[1] = 0;
		rval = intern(buf, 1);
		rval -> a.op_t = atype(buf);
		bind(arg[0], frame[0], rval, 0);
		return(TRUE);
	}
	fail("Ascii - bad argument")
}


static
p_strlen PREDICATE
{
	if (! isatom(arg[0]))
		fail("Strlen - first argument must be an atom");
	return unify(arg[1], frame[1], stack_int, strlen(NAME(arg[0])));
}

static
p_tolower PREDICATE
{
	register char *s, *r;
	char buf[128];
	char changed = FALSE;

	if (! isatom(arg[0]))
		fail("tolower - first argument must be an atom");
	for (s = NAME(arg[0]), r = buf; *s != 0; s++, r++)
		if ('A' <= *s && *s <= 'Z')
		{
			*r = *s + 'a' - 'A';
			changed = TRUE;
		}
		else
			*r = *s;
	*r = (char) 0;
	if (changed)
		return unify(arg[1], frame[1], intern(buf, r-buf), 0);
	else
		return unify(arg[1], frame[1], arg[0], frame[0]);
}





atom_table p_IO =
{
	SET_PRED(NONOP, 0, NPRED, "read", p_read),	/* keep this first */
	SET_PRED(NONOP, 0, NPRED, "write", p_write),	/* and this second */
	SET_PRED(NONOP, 0, NPRED, "getc", p_getc),
	SET_PRED(NONOP, 0, NPRED, "ungetc", p_ungetc),
	SET_PRED(NONOP, 0, NPRED, "eof", p_eof),
	SET_PRED(NONOP, 0, NPRED, "skip", p_skip),
	SET_PRED(NONOP, 0, NPRED, "putc", p_putc),
	SET_PRED(NONOP, 0, NPRED, "nputs", p_nputs),
	SET_PRED(NONOP, 0, NPRED, "noquote", noquote),
	SET_PRED(NONOP, 0, NPRED, "listing", listing),
	SET_PRED(NONOP, 0, NPRED, "nl", newline),
	SET_PRED(NONOP, 0, NPRED, "print", mess_ln),
	SET_PRED(NONOP, 0, NPRED, "prin", message),
	SET_PRED(NONOP, 0, NPRED, "ratom", ratom),
	SET_PRED(NONOP, 0,  1,    "buf_ratom", buf_ratom),
	SET_PRED(FX,  700,  1,	  "pp", list_proc),
	SET_PRED(NONOP, 0,  0,    "unratom", unratom),
	SET_PRED(NONOP, 0,  2,    "ascii", ascii),
	SET_PRED(NONOP, 0,  2,    "strlen", p_strlen),
	SET_PRED(NONOP, 0,  2,    "tolower", p_tolower),
	END_MARK
};
