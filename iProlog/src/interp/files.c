/************************************************************************/
/*		Routines for file and stream handling			*/
/************************************************************************/

#include "prolog.h"

FILE *input, *output;
FILE *dialog_in, *dialog_out;
FILE *alert;

term current_input, current_output;
term p_stdin, p_stdout;

static term open_streams = NULL;


/************************************************************************/
/*		Allocate and initialise stream structure		*/
/************************************************************************/

static term
new_stream(term fname, term mode, FILE *fptr)
{
	term rval = halloc(sizeof(stream));
	TYPE(rval) = STREAM;
	FLAGS(rval) = 0;
	MODE(rval) = mode;
	FILE_NAME(rval) = fname;
	NEXT_STREAM(rval) = NULL;
	FPTR(rval) = fptr;
	return rval;
}


/************************************************************************/
/*	Add a new stream to the list of currently opened streams	*/
/************************************************************************/

term
add_stream(term a, term mode, FILE *f)
{
	term p = new_stream(a, mode, f);
	setbuf(f, IOBUF(p));

	NEXT_STREAM(p) = PROC(open_streams);	/* Add entry to stream list */
	PROC(open_streams) = p;			/* most recent stream */
	return p;				/* first */
}


/************************************************************************/
/* Return a stream with the given name.					*/
/*	- if new create one						*/
/*	- if already exists, return associated stream			*/
/************************************************************************/

term
get_stream(term a, term mode)
{
	term p;
	FILE *f;
	
	/* see if file is open */

	for (p = PROC(open_streams); p != NULL; p = NEXT_STREAM(p))
		if (FILE_NAME(p) == a)
			return (MODE(p) == mode ? p : _nil);

	/* open file */

	if ((f = fopen(NAME(a), NAME(mode))) == NULL)
		return _nil;

	/* initialise stream structure */

	return p = add_stream(a, mode, f);
}


/************************************************************************/
/* Open named file with given mode, return associated stream		*/
/*	- doesn't have the options of the ISO standard			*/
/************************************************************************/

static int
p_open(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);
	term mode  = check_arg(2, goal, frame, ATOM, IN);
	term strm  = check_arg(3, goal, frame, STREAM, OUT);
	term p = get_stream(fname, mode);

	if (p == _nil)
		return FALSE;
	return unify(strm, frame, p, frame);
}


/************************************************************************/
/*		close a stream with the given file name			*/
/************************************************************************/

int
close_stream(term strm)
{
	term *p;

	for (p = &PROC(open_streams); *p != 0; p = &NEXT_STREAM(*p))
		if (*p == strm)
		{
			*p = NEXT_STREAM(*p);
			fclose(FPTR(strm));
			free_term(strm);
			return TRUE;
		}
	return FALSE;
}


/************************************************************************/
/*		close a stream, given the stream structure		*/
/************************************************************************/

static int
p_close(term goal, term *frame)
{
	term strm = check_arg(1, goal, frame, STREAM, IN);

	return close_stream(strm);
}


/************************************************************************/
/*	set the current input stream to be the given stream		*/
/************************************************************************/

int
set_input(term streamp)
{
	current_input = streamp;
	input = FPTR(streamp);
	return TRUE;
}


static int
p_set_input(term goal, term *frame)
{
	term streamp = check_arg(1, goal, frame, STREAM, IN);

	return set_input(streamp);
}


static int
p_current_input(term goal, term *frame)
{
	term streamp = check_arg(1, goal, frame, STREAM, OUT);

	return unify(streamp, frame, current_input, frame);
}


/************************************************************************/
/*	set the current output stream to be the given stream		*/
/************************************************************************/

int
set_output(term streamp)
{
	current_output = streamp;
	output = FPTR(streamp);
	return TRUE;
}


static int
p_set_output(term goal, term *frame)
{
	term streamp = check_arg(1, goal, frame, STREAM, IN);

	return set_output(streamp);
}


static int
p_current_output(term goal, term *frame)
{
	term streamp = check_arg(1, goal, frame, STREAM, OUT);

	return unify(streamp, frame, current_output, frame);
}


/************************************************************************/
/*			flush the output stream				*/
/************************************************************************/


static int
flush_output(term goal, term *frame)
{
	FILE *fptr = output;

	if (TYPE(goal) == FN && ARITY(goal) == 1)
		fptr = FPTR(check_arg(1, goal, frame, STREAM, IN));

	fflush(fptr);
	return TRUE;
}


/************************************************************************/
/*	see	- make the named file the current input stream		*/
/*	seeing	- unify with the name of the current input file		*/
/*	seen	- close the current input stream and revert to stdin	*/
/*	These routines are not in ISO standard				*/
/************************************************************************/

static int
see(term a)
{
	term p = get_stream(a, intern("r"));

	if (p == _nil)
		return(FALSE);
	input = FPTR(p);
	current_input = p;
	return TRUE;
}


static int
_see(term goal, term *frame)
{
	term f = check_arg(1, goal, frame, ATOM, IN);

	if (see(f))
		return(TRUE);
	fail("Could not open file for reading");
}


static int
seeing(term goal, term *frame)
{
	term p = check_arg(1, goal, frame, STREAM, OUT);

	return unify(p, frame, FILE_NAME(current_input), frame);
}


static int
seen(term goal, term *frame)
{
	if (input != stdin && close_stream(current_input))
	{
		input = stdin;
		current_input = p_stdin;
		return TRUE;
	}
	else return FALSE;
}


/************************************************************************/
/*	tell	- make the named file the current output stream		*/
/*	telling	- unify with the name of the current ouptut file	*/
/*	told	- close the current output stream and revert to stdout	*/
/*	These routines are not in ISO standard				*/
/************************************************************************/

static int
p_tell(term goal, term *frame)
{
	term a = check_arg(1, goal, frame, ATOM, IN);
	term p = get_stream(a, intern("w"));

	if (p == _nil)
		fail("Can't open output stream");
	output = FPTR(p);
	current_output = p;
	return TRUE;
}


static int
telling(term goal, term *frame)
{
	term p = check_arg(1, goal, frame, STREAM, OUT);

	return unify(p, frame, FILE_NAME(current_output), frame);
}


static int
told(term goal, term *frame)
{
	if (output != stdout && close_stream(current_output))
	{
		output = stdout;
		current_output = p_stdout;
		return(TRUE);
	}
	else return FALSE;
}


/************************************************************************/
/*		Add the name of a procedure to the proc list		*/
/************************************************************************/

static term proc_list;

void
add_to_proc_list(term a)
{
	term *p;

	for (p = &proc_list; *p != _nil; p = &CDR(*p))
		if (CAR(*p) == a)
			return;

	*p = hcons(a, _nil);
}


/************************************************************************/
/* Create "file" clause for storing the proc list associated with file	*/
/************************************************************************/

static void
create_proc_entry(term a)
{
	term p = new_unit(h_fn2(_file, a, proc_list));

	NEXT(p) = PROC(_file);
	PROC(_file) = p;
}


/************************************************************************/
/* Get the proc list of a file and retract all the procedures defined	*/
/************************************************************************/

static void
unload(term a)
{
	term p, *q;

	proc_list = _nil;		

	for (q = &PROC(_file); *q != NULL; q = &NEXT(*q))
		if (ARG(1, HEAD(*q)) == a)
		{
			for (p = ARG(2, HEAD(*q)); p != _nil; p = CDR(p))
				free_proc(CAR(p));
		
			p = NEXT(*q);
			free_term(*q);
			*q = p;
			break;
		}
}


/************************************************************************/
/* Read a file, unloading all procedure previously defined in that file	*/
/************************************************************************/

int
read_file(term fname)
{
	extern int linen;
	int old_linen = linen, rval = FALSE;
	term old_input = current_input, old_proc_list = proc_list;

	if (see(fname))
	{
		linen = 0;
		unload(fname);
		evloop();
		create_proc_entry(fname);

		if (close_stream(current_input))
			rval = TRUE;
		else
			fprintf(stderr, "Cannot close file %s\n", NAME(fname));

		current_input = old_input;
		input = FPTR(current_input);
		linen = old_linen;
		proc_list = old_proc_list;
	}
	else
		fprintf(stderr, "Cannot open file %s\n", NAME(fname));
}


/************************************************************************/
/*	Variation on read_file that is used by the "ed" built-in	*/
/************************************************************************/

int
input_file(char *fname)
{
	extern int linen, syntax_error;
	term old_input = current_input;
	term p = get_stream(intern(fname), intern("r"));

	if (p == _nil)
		fail("Can't open input file");

	input = FPTR(p);
	current_input = p;
	proc_list = _nil;
	syntax_error = FALSE;
	linen = 0;

	evloop();

	free_term(proc_list);
	close_stream(current_input);
	current_input = old_input;
	input = FPTR(current_input);

	return (! syntax_error);
}


/************************************************************************/
/*	Another variation on read_file used by Mac editor		*/
/************************************************************************/

#ifdef THINK_C

void
process_input(char *str, FILE *fp)
{
	extern int linen;
 	term old_input = current_input;
	term fname = intern(str);

	linen = 0;
	/* ifile = fname; */
	unload(fname);

	input = fp;
	evloop();
	current_input = old_input;
	input = FPTR(current_input);

	create_proc_entry(fname);
}

#endif

/************************************************************************/
/* consult built-in predicate accepts any number of file name arguments	*/
/************************************************************************/

static int
consult(term goal, term *frame)
{
	int i, a = ARITY(goal);

	for (i = 1; i <= a; i++)
	{ 
		term x = check_arg(i, goal, frame, ATOM, IN);
		read_file(x);
	}
	return TRUE;
}


/************************************************************************/
/*	unconsult removes all the procedures consulted in a file	*/
/************************************************************************/

static int
unconsult(term goal, term *frame)
{
	int i, a = ARITY(goal);

	for (i = 1; i <= a; i++)
	{ 
		term x = check_arg(i, goal, frame, ATOM, IN);
		unload(x);
	}
	return TRUE;
}


/************************************************************************/
/*	load non-standard built-in accepts single or list argument	*/
/************************************************************************/

static int
load(term goal, term *frame)
{
	term files = unbind(ARG(1, goal), frame);

	if (TYPE(files) == ATOM)
	{
		read_file(files);
		return TRUE;
	}
	if (TYPE(files) != LIST)
		fail("Argument must be a file name or list of file names");

	while (files != _nil)
	{
		term x = CAR(files);

		DEREF(x);
		if (TYPE(x) != ATOM)
			fail("File name must be an atom");
		read_file(x);
		files = CDR(files);
		DEREF(files);
	}
	return TRUE;
}


/************************************************************************/
/*	unload removes all the procedures loaded froma file		*/
/************************************************************************/

static int
_unload(term goal, term *frame)
{
	term files = unbind(ARG(1, goal), frame);

	if (TYPE(files) == ATOM)
	{
		unload(files);
		return TRUE;
	}
	if (TYPE(files) != LIST)
		fail("Argument must be a file name or list of file names");

	while (files != _nil)
	{
		term x = CAR(files);

		DEREF(x);
		if (TYPE(x) != ATOM)
			fail("File name must be an atom");
		unload(x);
		files = CDR(files);
		DEREF(files);
	}
	return TRUE;
}


/************************************************************************/
/*	Remove all clauses asserted into database by user		*/
/************************************************************************/

static int
clear(void)
{
	extern term hashtable[];
	int i;
	term p;

	for (i = 0; i < HASHSIZE; i++)
		for (p = hashtable[i]; p != NULL; p = LINK(p))
		{
			term q = PROC(p);

			if (FLAGS(p) & PREDEF)
				continue;

			while (q != NULL)
			{
				term n = NEXT(q);

				free_term(q);
				q = n;
			}
			PROC(p) = NULL;
		}

	return TRUE;
}


/************************************************************************/
/*		Restore state of data base to starting state		*/
/************************************************************************/

static int
reset(void)
{
	extern term *global;
	term p, files = _nil, *old_global = global;

	for (p = PROC(_file); p != NULL; p = NEXT(p))
		files = gcons(ARG(1, HEAD(p)), files);

	clear();

	for (p = files; p != _nil; p = CDR(p))
	{
		print(CAR(p));
		read_file(CAR(p));
	}
	global = old_global;
	return TRUE;
}


/************************************************************************/
/* When under Unix, set stdin and stdout to use buffers allocated by us	*/
/************************************************************************/

static void
set_buffers(void)
{
	setbuf(stdin, IOBUF(current_input));
	setbuf(stdout, IOBUF(current_output));
}


/************************************************************************/
/* Resets all the I/O to startup state. Called when reloading a dump	*/
/************************************************************************/

void
reset_io(void)
{
	if (current_input != p_stdin)
	{
		close_stream(current_input);
		current_input = p_stdin;
		input = FPTR(current_input);
		clear_input();
	}
	if (current_output != p_stdout)
	{
		close_stream(current_output);
		current_output = p_stdout;
		output = FPTR(current_output);
	}
	set_buffers();
}


/************************************************************************/
/*			     Module initialisation			*/
/************************************************************************/

void
file_init(void)
{
	open_streams = intern("open_streams");
	proc_list = _nil;

	input = dialog_in = stdin;
	output = dialog_out = stdout;
	alert = stderr;

	current_input = p_stdin = new_stream(intern("stdin"), intern("r"), stdin);
	current_output = p_stdout = new_stream(intern("stdout"), intern("w"), stdout);

	set_buffers();

	FLAGS(_file) |= DYNAMIC;

	new_pred(p_open,		"open");
	new_pred(p_close,		"close");
	new_pred(p_set_input,		"set_input");
	new_pred(p_current_input,	"current_input");
	new_pred(p_set_output,		"set_output");
	new_pred(p_current_output,	"current_output");
	new_pred(flush_output,		"flush_output");
	new_pred(_see,			"see");
	new_pred(seeing,		"seeing");
	new_pred(seen,			"seen");
	new_pred(p_tell,		"tell");
	new_pred(telling,		"telling");
	new_pred(told,			"told");
	new_pred(consult,		"consult");
	new_pred(unconsult,		"unconsult");
	new_pred(clear,			"clear_db");
	new_pred(reset,			"reset");

	defop(700,	FX,	new_pred(load, "load")); 
	defop(700,	FX,	new_pred(_unload, "unload")); 
}
