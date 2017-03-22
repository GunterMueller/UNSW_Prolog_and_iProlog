/******************************************************************************

			UNSW Prolog (version 4.2)

		  Written by Tony Grech/Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





/*		predicates which rely in Unix			*/


#include "pred.h"
#include "in.h"
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/times.h>

extern char *SHELL ,*EDITOR;



static
relink	PREDICATE
{
	if (!isatom(arg[0]) || !isatom(arg[1]))
		fail("rename arguments must be atoms")

	if (arg[1] != (pval)nil)
		if (link(NAME(arg[0]), NAME(arg[1])) == -1)
			goto failed;

	if (unlink(NAME(arg[0])) == -1)
failed:		fail("rename unsucessful")

	return(TRUE);
}



static
sys_cmd(file, arg1, arg2)
char *file,*arg1, *arg2;
{
	int (*x)(), pid, ret;

	switch (pid = fork())
	{
	   case -1:	fail("Cannot fork")

	   case  0:	execlp(file, file, arg1, arg2, (char *)0);
			fprintf(stderr, "\nexec fail\n");
 			perror("");
			exit(1);

	   default:	x = signal(SIGINT, SIG_IGN);

			do {
				ret = wait((char *)NULL);
			} while (ret != pid && ret != -1);

			signal(SIGINT, x);
			return(TRUE);
	}
}



static
ed PREDICATE
{
	extern char *temp_file;
	extern int load_err;
	extern atom *same_proc;
	extern pval getatom();
	clause *p;
	FILE *old_output;
	char ret = FALSE;

	if (isatom(arg[0]))
	{
		old_output = output;
		if ((output = fopen(temp_file, "w")) == NULL)
		{
			output = old_output;
			fail("Ed - open failure")
		}
		_list_proc(arg[0]);
		fclose(output);
		output = old_output;
		repeat {
			if (!sys_cmd(EDITOR, temp_file, (char *)NULL)) break;
			p = VAL(arg[0]);
			VAL(arg[0]) = 0;
			same_proc = 0;
			if (! infile(temp_file, NAME(arg[0])))
				break;
			if (load_err)
			{
				fprintf(stderr,"Re-edit?(y/n) ");
				if (getatom() == (pval) no)
				{
					free_proc(VAL(arg[0]));
					VAL(arg[0]) = p;
					printf("Old definition restored\n");
					ret = TRUE;
					break;
				}
			}
			else {
				free_proc(p);
				ret = TRUE;
				break;
			}
		}
		unlink(temp_file);
		return(ret);
	}
	else fail("Ed - argument must be predicate name")
}



static
ef PREDICATE
{
	if (isatom(arg[0]))
		return(sys_cmd(EDITOR, NAME(arg[0]), (char *)NULL));

	else fail("Ef - argument must be file name")
}



static
sh PREDICATE
{
	return(sys_cmd(SHELL, (char *)NULL, (char *)NULL));
}



p_getenv PREDICATE
{
	extern char *getenv();
	register pval rval;
	char *p;

	if (TYPE(arg[0]) != ATOM)
		fail("Getenv - first argument must be an an atom");
	if ((p = getenv(NAME(arg[0]))) == 0)
		fail("Getenv - environment variable does not exist");
	rval = intern(p, strlen(p));
	rval = nonop(atype(p), rval);
	return(unify(arg[1], frame[1], rval, 0));
}



long
get_time()
{
	struct tms t;

	times(&t);
	return(t.tms_utime);
}


static
cputime PREDICATE
{
	if (TYPE(arg[0]) != VAR)
		fail("cputime - argument must be an unbound variable");
	bind_num(0, (int)get_time());
	return(TRUE);
}



static
p_system PREDICATE
{
	if (isatom(arg[0]))
		return(sys_cmd("sh","-c",NAME(arg[0])));

	else fail("System - argument must be atom")
}



/*
 * I had some trouble getting an fstat working (for pipes) on 4.2 bsd,
 * The manual gave a vague sort of warning so I use an ioctl instead
 */

#ifdef BSD
#include <sys/ioctl.h>

static
input_ready(io_stream)
FILE *io_stream;
{
	long	ch_count;

	if (io_stream->_cnt > 0) return(TRUE);
	ioctl(fileno(io_stream), FIONREAD, &ch_count);
	return(ch_count != 0);
}

#else
#include <sys/stat.h>

static
input_ready(io_stream)
FILE *io_stream;
{
	struct stat buf;

	if (io_stream->_cnt > 0) return(TRUE);
	fstat(fileno(io_stream), &buf);
	return(buf.st_size != 0);
}

#endif BSD



static
p_input_ready NPREDICATE
{
	FILE	*istream;

	if (choose_stream(&istream, arg, argc, INPUT, 1) == -1)
		return(FALSE);

	return(input_ready(istream));
}



#ifdef BSD

#define MAX_FDS	(sizeof(int)*8)


#include <sys/time.h>
static
p_select PREDICATE
{
	extern atom *_user_input;

	pval stream_buf[MAX_FDS], tl_term;
	binding *tl_frame;
	int readset, dummy, i;
	struct timeval timeout, *tp;

	if (!isstream(arg[2]) && !isvariable(arg[2]) || ! isinteger(arg[1]))
		fail("select - bad argument")

	i = INT_VAL(1);
	if (i == -1)
		tp = NULL;
	else if (i >= 0) {
		timeout.tv_sec  =  i / 1000;
		timeout.tv_usec = (i % 1000) * 1000;
		tp = &timeout;
	}
	else	fail("select - bad timeout");

	readset = i = dummy = 0;

	if (TYPE(arg[0]) == LIST)
	{
		termb  = arg[0];
		frameb = frame[0];
		while (TYPE(termb) != VAR && termb != (pval) nil)
		{
			tl_term  = termb;
			tl_frame = frameb;
			unbind(termb->c.term[0], frameb);
			stream_buf[i] = termb;
			if (!check_stream(&stream_buf[i], INPUT))
				return(FALSE);

			readset |= 1 << fileno(stream_buf[i]->s.file);
			if (++i >= MAX_FDS)
				fail("select - list too long")

			unbind(tl_term->c.term[1], tl_frame);
		}
	}
	else if (arg[0] != (pval) nil)
		fail("select - first argument must be a list");

	if (select(MAX_FDS, &readset, &dummy, &dummy, tp) == 0)
		return(arg[0] == (pval) nil);

	for(i = 0; !(readset&(1 << fileno(stream_buf[i]->s.file))); i++)
		;
	if (stream_buf[i]->s.file == stdin)
		stream_buf[i] = (pval)_user_input;
	return(unify(arg[2], frame[2], stream_buf[i], 0));
}

#endif



/*
 * replace old file descriptor by new file descriptor
 */
replace(oldfd, newfd)
{
	if (oldfd == newfd) return;

	close(oldfd);
	fcntl(newfd, F_DUPFD, oldfd);
	close(newfd);
}


#define	READ 0
#define	WRITE 1



static
close_pipe(bufs)
int bufs[2];
{
	close(bufs[READ]);
	close(bufs[WRITE]);
}



int
pipe_process(name, readp, writep, child_i, child_o)
char *name;
int *readp, *writep;
int child_i, child_o;
{
	int ibufs[2], obufs[2];
	int pid;
	static char *file_msg = "couldn't open pipe";

	if (writep && pipe(obufs) < 0)
		fail(file_msg)

	if (readp && pipe(ibufs) < 0)
	{
		if (writep) close_pipe(obufs);
		fail(file_msg)
	}

	switch (pid = fork())
	{
		case 0 : /* child */
			signal(SIGINT, SIG_IGN);

			if (writep) /* child will write */
			{
				close(obufs[READ]);
				replace(1, obufs[WRITE]);
			}
			else if (child_o != -1)
				replace(1, child_o);

			if (readp) /* child will read */
			{
				close(ibufs[WRITE]);
				replace(0, ibufs[READ]);
			}
			else if (child_i != -1)
				replace(0, child_i);

			execlp("/bin/sh", "sh", "-c", name, (char *)0);
			fprintf(stderr, "\nexec fail\n");
			exit(1);

		case -1 : /* fork failed */
			if (readp)	close_pipe(ibufs);
			if (writep)	close_pipe(obufs);
			fail("fork failed")

		default : /* parent */

			if (readp)
			{
				close(ibufs[READ]);
				*readp = ibufs[WRITE];
			}
			if (writep)
			{
				close(obufs[WRITE]);
				*writep = obufs[READ];
			}

			return(pid);
	}
}




static
p_process	PREDICATE
{
	int *writep, *readp;
	int write, read;
	int child_i, child_o;

	child_i = child_o = -1;
	writep = readp = NULL;

	if (!isatom(arg[0]))
		fail("process - command must be atom")

	if (isvariable(arg[2])) {
		if (arg[2]->v.pname != anon)
			writep = &write;
	}
	else if (!check_stream(&arg[2], OUTPUT))
		return(FALSE);
	else child_o = fileno(arg[2]->s.file);

	if (isvariable(arg[1])) {
		if (arg[1]->v.pname != anon)
		{
			if (arg[1]->v.pname == arg[2]->v.pname)
				fail("process - i/o streams must be different")
			readp = &read;
		}
	}
	else if (!check_stream(&arg[1], INPUT))
		return(FALSE);
	else child_i = fileno(arg[1]->s.file);

	if (!pipe_process(NAME(arg[0]), readp, writep, child_i, child_o))
		return(FALSE);

	if (writep)
	{
		stream *istream = (stream *) new(STREAM);

		istream->file  = fdopen(write, "r");
		istream->sname = NAME(arg[0]);
		istream->mode  = R_MODE;
		add_stream(istream, (pval) _read, arg[0]);

		bind(arg[2], frame[2], istream, 0);
	}

	if (readp)
	{
		stream *ostream = (stream *) new(STREAM);

		ostream->file  = fdopen(read, "w");
		ostream->sname = NAME(arg[0]);
		ostream->mode  = W_MODE;
		add_stream(ostream, (pval) _write, arg[0]);

		bind(arg[1], frame[1], ostream, 0);
	}

	return(TRUE);
}



atom_table p_unix =
{
	SET_PRED(NONOP, 0,  2,	"relink", relink),
	SET_PRED(FX,  700,  1,	"ed", ed),
	SET_PRED(FX,  700,  1,	"ef", ef),
	SET_PRED(NONOP, 0,  0,	"sh", sh),
	SET_PRED(NONOP, 0,  1,	"cputime", cputime),
	SET_PRED(NONOP, 0,  2,	"getenv", p_getenv),
	SET_PRED(NONOP, 0,  1,	"system",  p_system),
	SET_PRED(NONOP, 0,NPRED,"input_ready", p_input_ready),
#ifdef BSD
	SET_PRED(NONOP, 0,  3,	"select", p_select),
#endif
	SET_PRED(NONOP, 0,  3,	"process", p_process),
	END_MARK
};
