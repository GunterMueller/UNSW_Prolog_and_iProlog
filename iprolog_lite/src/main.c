#include <setjmp.h>
#include <signal.h>
#include "prolog.h"


/************************************************************************/
/*	Initialise all modules. Must be called before anything else	*/
/************************************************************************/

static void
init(void)
{
	mem_init();		/* must be called first */
	atom_init();
	prove_init();
	meta_init();
	file_init();
	lists_init();
	eval_init();
	math_init();
	out_init();
	compare_init();
	db_init();
	system_init();
	lex_init();
	read_init();
	dcg_init();

#ifdef UNIX
	dump_init();
	unix_init();
#endif
}


/************************************************************************/
/*	Initialise; read command line arguments and start main loop	*/
/************************************************************************/

#ifdef THINK_C

main(int argc, char **argv)
{
	mac_init("Prolog", 128000L);
	init();
	evloop();
}

#else

main(int argc, char **argv)
{
	extern int optind;
	extern char *optarg;
	extern size_t local_size, global_size, seg_size;
	int c;
	char *dump_file = NULL;

	while ((c = getopt(argc, argv, "g:l:r:")) != EOF)
		switch (c)
		{
		   case 'g':
				global_size = 1024 * atoi(optarg);
				break;
		   case 'l':
				local_size = 1024 * atoi(optarg);
				break;
		   case 'r':
				dump_file = optarg;
				break;
		   default:
				fprintf(stderr, "Incorrect argument\n");
				break;
		}

	if (isatty(fileno(stdin)))
		fprintf(stderr, "\niProlog/lite (9 April 2002)\n");

	if (dump_file)
	{
		if (! restore(dump_file))
			exit(1);
		reset_io();
		_prompt = _prolog_prompt;
	}
	else
	{
		init();

		while (optind < argc)
			read_file(intern(argv[optind++]));

	}


	evloop();
	exit(0);
}

#endif
