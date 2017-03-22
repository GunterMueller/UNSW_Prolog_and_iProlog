/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





#include <setjmp.h>
#include <signal.h>
#include "g.h"

#define MIN_SIZE 1000

#define DEBUG

jmp_buf	start_env;
char	terminal, *cur_file;
char	*temp_file = "/tmp/plXXXXXX";
char	*EDITOR = 0;
char	*SHELL	= 0;
FILE	*input,
	*output,
	*prog_file;
int	library;

extern int linen;

static
ferr(fname)
char *fname;
{
	fprintf(stderr, "\nCannot open %s\n", fname);
	exit(1);
}


#ifndef DEBUG
static
p_panic()
{
	fprintf(stderr, "\nPANIC!! - Internal Prolog error!\n");
	exit(1);
}
#endif


static
catch_kill()
{
	signal(SIGTERM, SIG_IGN);
	kill(0, SIGTERM);
	exit(1);
}


static
_reset()
{
	extern char run;
	extern argn, readatom, p_read_on;
	extern atom *prompt_string, *init_prompt;

	clean_trail(trail);
	tp = trail;
	sp = stack;
	parent = envp = env_stack-1;
	argn = run = 0;
	readatom = p_read_on = FALSE;
	input = stdin; output = poport;
	prompt_string = init_prompt;
	terminal = isatty(fileno(input));
#ifdef PROFILING
	if (profiling) profile_reset();
#endif
	longjmp(start_env, 1);
}


static
interrupt()
{
	signal(SIGINT, interrupt);
	putc(07,stderr); putc('\n',stderr);

	_reset();
}


fatal(msg)
char *msg;
{
	fprintf(stderr,"\n%s\n", msg);

	back_trace(-1);
	_reset();
}


add_file(name, proc_list)
pval name, proc_list;
{
	extern atom *_file;
	register clause *p;
	register pval q;

	p = create(0, 0);
	p -> rest = VAL(_file);
	VAL(_file) = p;
	p -> goal[0] = q = (pval) record(2);
	p -> goal[1] = 0;
	q -> c.term[0] = (pval) _file;
	q -> c.term[1] = name;
	q -> c.term[2] = proc_list;
}




static
get_file(name, lib)
char *name;
int lib;
{
	extern pval proc_list;
	register pval p;

	proc_list = (pval) nil;
	if ((input = fopen(name, "r")) == NULL)
		ferr(name);
	library = lib;
	linen = 1;
	cur_file = name;
	prog_file = input;
	if (! setjmp(start_env)) evloop();
	else {
		fprintf(stderr, "Fatal error while loading %s\n", name);
		fprintf(stderr, "Prolog execution aborted\n");
		exit(1);
	}
	fclose(input);
	if (! library)
	{
		p = intern(name, strlen(name));
		p = nonop(atype(name), p);
		add_file(p, proc_list);
	}
}


main(argc, argv)
register argc;
register char **argv;
{
	extern char *getenv();
	register i;
	int stack_size = MIN_SIZE;

#ifndef DEBUG
	signal(SIGSEGV, p_panic);
	signal(SIGBUS, p_panic);
	signal(SIGILL, p_panic);
#endif
	signal(SIGTERM, catch_kill);

		/* read in command line */

	for (i = 1; i < argc; i++)
		if (argv[i][0] == '-')
			switch (argv[i][1])
			{
			    case 's' :
				stack_size = atoi(&(argv[i][2]));
				if (stack_size < MIN_SIZE)
				{
					fprintf(stderr, "\nStack too small\n");
					exit(1);
				}
				break;
			    default  :
				fprintf(stderr, "usage : prolog [-sSIZE] [ file1 .. filen ]\n");
				exit(1);
			}

	if (!set_stacks(stack_size) || !init_heap())
	{
		fprintf(stderr, "Not enough memory for Prolog\n");
		exit(1);
	}

	setup();

	piport = stdin;
	poport = output = stdout;

	SHELL = getenv("SHELL");
	if (! SHELL) SHELL = DFLT_SHELL;

	EDITOR = getenv("EDITOR");
	if (! EDITOR) EDITOR = DFLT_ED;

	set_streams();
	mktemp(temp_file);	/* temporary file name for editor */

	get_file(LIB, 1);

	for (i = 1; i < argc; i++)
		if (argv[i][0] != '-')
			get_file(argv[i], 0);

		/* start interactive interpreter loop */

	prog_file = input = piport = stdin;
	library = FALSE;
	linen = 1;
	if (terminal = isatty(fileno(input)))
	{
		fprintf(stderr, "\nUNSW - PROLOG V4.2\n");

		cur_file = "input";
		if (signal(SIGINT, interrupt) == SIG_IGN)
			signal(SIGINT, SIG_IGN);
	}
	else {
		cur_file = "input file";
		signal(SIGINT, catch_kill);
	}
L1:	if (! setjmp(start_env)) evloop();
	else goto L1;

	exit(0);
}
