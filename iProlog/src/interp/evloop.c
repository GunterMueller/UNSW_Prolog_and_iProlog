#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "prolog.h"

extern term varlist, *global, mkbody();
extern query current_query;

static jmp_buf *ret_env;


/************************************************************************/
/*			Error handling routines				*/
/************************************************************************/


void
warning(char *msg)
{
	extern int linen;
	extern term current_input;

	if (! isatty(fileno(input)))
		fprintf(stderr, "%s: line %d: %s.\n", NAME(FILE_NAME(current_input)), linen, msg);
	else
		fprintf(stderr, "\nERROR: %s.\n", msg);
}


void
interrupt(int i)
{
	signal(SIGINT, interrupt);
	longjmp(*ret_env, 1);
}


void
fail(char *msg)
{
	warning(msg);
	backtrace();
	interrupt(0);
}


/************************************************************************/
/* Evaluate a predicate and only return FALSE if "fail" is called	*/
/************************************************************************/

int
trap_cond(term *cl, term *frame)
{
	term *old_global = global;
	jmp_buf local_env, *last_env;
	int rval;

	signal(SIGINT, interrupt);

	last_env = ret_env;
	ret_env = &local_env;
	if (setjmp(local_env))
	{
		global = old_global;
		rval = FALSE;
	}
	else
		rval = cond(cl, frame);

	ret_env = last_env;
	return rval;
}


/************************************************************************/
/*			The main read/execute/print loop		*/
/************************************************************************/

void
evloop(void)
{
	term x, *old_global = global;
	FILE *prog_input = input, *current_input = input;
	jmp_buf local_env, *last_env;

	signal(SIGINT, interrupt);

	last_env = ret_env;
	ret_env = &local_env;
	if (setjmp(local_env))
	{
		if (current_query != NULL)
			current_query = current_query -> previous_query;
		global = old_global;
		_prompt = _prolog_prompt;
		if (isatty(fileno(input)))
		{
			fseek(input, 0L, SEEK_END);
			fputc('\n', output);
		}
	}
	prompt();
//	ungetc('\n', stdin);

	while ((x = p_read()) != _end_of_file)
	{
		extern term _double_arrow;

		input = prog_input;
		_prompt = _user_prompt;

		if (ARG(0, x) == _dot)
		{
			x = ARG(1, x);
			if (iscompound(x) && ARG(0, x) == _double_arrow)
				assert_dcg(x, NULL);
			else
				add_clause(mkclause(x, NULL), FALSE);
		}
		else if (ARG(0, x) == _question)
			directive(x, varlist);
		else if (ARG(0, x) == _bang)
			directive(ARG(1, x), varlist);

//		if (is_socket(input))
			fflush(output);

		_prompt = _prolog_prompt;
		prog_input = input;
		input = current_input;
		global = old_global;
	}
	ret_env = last_env;
}
