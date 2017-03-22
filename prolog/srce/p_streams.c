/******************************************************************************

			UNSW Prolog (version 4.2)

		  Written by Tony Grech/Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





/*		built-in predicates for I/O			*/


#include "pred.h"
#include "in.h"

FILE  *piport, *poport;
static pval cur_istream, cur_ostream;

extern	atom *_read, *_write, *_append;
extern	atom *_user_input, *_user_output, *_user_error;


set_streams()
{
	cur_istream = (pval) _user_input;
	cur_ostream = (pval) _user_output;
}


add_stream(strm, mode, file)
stream *strm;
pval  mode, file;
{
	register clause *p;
	register pval q;

	extern	atom	 *_current_stream;

	p = create(0,0);
	p->rest = VAL(_current_stream);
	VAL(_current_stream) = p;
	p->goal[0] = q = (pval) record(3);
	p->goal[1] = 0;

	q->c.term[0] = (pval) _current_stream;
	q->c.term[1] = file;
	q->c.term[2] = mode;
	q->c.term[3] = (pval) strm;
}


stream *
qstream(x)
pval x;
{
	static	stream	stdin_stream	= {STREAM, NULL, stdin,  R_MODE, 0};
	static	stream	stdout_stream	= {STREAM, NULL, stdout, W_MODE, 0};
	static	stream	stderr_stream	= {STREAM, NULL, stderr, W_MODE, 0};

	if (isstream(x))
		return((stream *)x);

	else if ( TYPE(x) == ATOM)
	{
		if	(x == (pval) _user_input)	return(&stdin_stream);
		else if (x == (pval) _user_output)	return(&stdout_stream);
		else if (x == (pval) _user_error)	return(&stderr_stream);
	}

	return(NULL);
}


_check_stream(x, mode)
pval *x;
{
	stream *temp;
	if ( (temp = qstream(*x)) == NULL )
		return(FALSE);

	switch (temp->mode)
	{
		case R_MODE:
			if (mode&INPUT)
				break;
			warning("can`t use input stream for output");
			return( -1);

		case A_MODE:
		case W_MODE:
			if (mode&OUTPUT)
				break;
			warning("can`t use output stream for input");
			return( -1);

		case CLOSED:
			warning("stream has been closed");
			return( -1);
	}
	*x = (pval) temp;
	return(TRUE);
}


check_stream(x, mode)
pval *x;
{
	switch (_check_stream(x , mode))
	{
		case TRUE:	return(TRUE);
		case FALSE:	fail("bad stream arg")
		default:	return(FALSE);
	}
}


static
set_input	PREDICATE
{
	if (!check_stream(arg, INPUT))
		return(FALSE);

	clear_buf();
	piport = arg[0]->s.file;
	cur_istream = (piport == stdin ? (pval)_user_input : arg[0]);
	return(TRUE);
}


static
read_in		PREDICATE
{
	stream *strm = (stream *)new(STREAM);
	extern char *cur_file;

	clear_buf();
	strm->mode  = R_MODE;
	strm->file  = piport = prog_file;
	strm->sname = cur_file;
	cur_istream = (pval)strm;

	return(TRUE);
}


static
set_output	PREDICATE
{
	if (! check_stream(arg, OUTPUT))
		return(FALSE);

	poport = output  = arg[0]->s.file;

	if (poport == stdout)
		cur_ostream = (pval)_user_output;

	else if (poport == stderr)
		cur_ostream = (pval)_user_error;

	else	cur_ostream = arg[0];
	return(TRUE);
}


static
flush_output	PREDICATE
{
	if (!check_stream(arg, OUTPUT))
		return(FALSE);

	fflush(arg[0]->s.file);
	return(TRUE);
}



static
open	PREDICATE
{
	FILE *filep;
	char mode , *type;
	stream	*newstream;
	extern pval getatom();

	if (!isatom(arg[0]) || !isatom(arg[1]) || !isvariable(arg[2]))
		fail("open - bad arg")

	if	((atom *)arg[1] == _read)	{ mode = R_MODE; type = "r"; }
	else if ((atom *)arg[1] == _append)	{ mode = A_MODE; type = "a"; }
	else if ((atom *)arg[1] == _write)	{ mode = W_MODE; type = "w"; }

	else	fail("open - bad mode")

	if ( (filep = fopen(NAME(arg[0]), type)) != NULL)
	{
		newstream = (stream *)new(STREAM);

		newstream->mode	 = mode;
		newstream->sname = NAME(arg[0]);
		newstream->file	 = filep;
		add_stream(newstream, arg[1], arg[0]);

		bind(arg[2], frame[2], newstream, 0);
		return(TRUE);
	}
	else	fail("open - couldn`t open file")
}

static
do_close	PREDICATE
{
	char can_close = TRUE;

	if (!check_stream(arg, INPUT|OUTPUT))
		return(FALSE);
	if (fileno(arg[0]->s.file) <= 2)
		fail("cannot close reserved stream")

	if (arg[0] == cur_istream)
	{
		clear_buf();
		if (piport == input)
		{
			trail_pointer(cur_istream);
			can_close = FALSE;
		}

		cur_istream = (pval) _user_input;
		piport = stdin;
	}
	else if (arg[0] == cur_ostream)
	{
		cur_ostream = (pval) _user_output;
		poport = output = stdout;
	}

	if (arg[0]->s.file == trace_output)
		trace_output = NULL;

	if (can_close) fclose(arg[0]->s.file);
	arg[0]->s.mode = CLOSED;
	return(TRUE);
}



infile(file_name, pname)
char *file_name;
char *pname;
{
	char old_term,   *old_cur_file;
	FILE *old_input, *old_prog;
	int  old_linen;

	extern char terminal, *cur_file;
	extern int linen;
	
	old_input	= input;
	old_prog	= prog_file;
	old_cur_file	= cur_file;
	old_linen	= linen;
	old_term	= terminal;

	if ((input = fopen(file_name, "r")) == NULL)
	{
		input = old_input;
		fail("Consult - cannot open file")
	}
	prog_file = input;
	cur_file  = (pname == NULL ? file_name : pname);
	linen	  = 1;
	terminal  = isatty(fileno(input));

	evloop();

	if (piport == input)
	{
		clear_buf();
		free_term(cur_istream);
		cur_istream->s.mode = CLOSED;
		piport = stdin;
		cur_istream = (pval) _user_input;
	}
	fclose(prog_file);

	input	  = old_input;
	prog_file = old_prog;
	cur_file  = old_cur_file;
	linen	  = old_linen;
	terminal  = old_term;
	return(TRUE);
}


static
consult PREDICATE
{
	extern pval proc_list;
	pval prev_proc_list;

	prev_proc_list = proc_list;
	if (! isatom(arg[0]))
		fail("Consult - first argument must be an atom")

	proc_list = (pval) nil;
	if (infile(NAME(arg[0]), (char *)NULL))
	{
		add_file(arg[0], proc_list);
		proc_list = prev_proc_list;
		return(TRUE);
	}
	return(FALSE);
}



static
curr_input	PREDICATE
{
	if (!isvariable(arg[0]))
		fail("current_input - argument must be unbound variable")

	bind(arg[0], frame[0], cur_istream, 0);
	return(TRUE);
}


static
curr_output	PREDICATE
{
	if (!isvariable(arg[0]))
		fail("current_output - argument must be unbound variable")

	bind(arg[0], frame[0], cur_ostream, 0);
	return(TRUE);
}

static
set_to	PREDICATE
{
	if (!check_stream(arg, OUTPUT))
		return(FALSE);

	trace_output = arg[0]->s.file;
	return(TRUE);
}


atom_table p_streams =
{
	SET_PRED(NONOP, 0, 1, "set_input", set_input),
	SET_PRED(NONOP, 0, 0, "read_from_this_file", read_in),
	SET_PRED(NONOP, 0, 1, "set_output", set_output),
	SET_PRED(NONOP, 0, 1, "flush_output", flush_output),
	SET_PRED(NONOP, 0, 3, "open", open),
	SET_PRED(NONOP, 0, 1, "do_close", do_close),
	SET_PRED(NONOP, 0, 1, "consult", consult),
	SET_PRED(NONOP, 0, 1, "current_input", curr_input),
	SET_PRED(NONOP, 0, 1, "current_output", curr_output),
	SET_PRED(NONOP, 0, 1, "set_to", set_to),
	END_MARK
};
