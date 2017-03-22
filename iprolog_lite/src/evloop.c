#include <setjmp.h>
#include <signal.h>
#include "prolog.h"

extern term varlist, *local, *global, mkbody();
extern void *current_query;

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
#ifdef THINK_C1
		d_error(input, (long) linen, msg);
#else
		fprintf(stderr, "%s: line %d: %s.\n", NAME(FILE_NAME(current_input)), linen, msg);
#endif
	else
		fprintf(stderr, "\nERROR: %s.\n", msg);
}


void
interrupt(int i)
{
#ifdef THINK_C
	signal(SIGINT, (__sig_func) interrupt);
#else
	signal(SIGINT, interrupt);
#endif
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
	term *old_local = local, *old_global = global;
	jmp_buf local_env, *last_env;
	int rval;

#ifdef THINK_C
	signal(SIGINT, (__sig_func) interrupt);
#else
	signal(SIGINT, interrupt);
#endif
	last_env = ret_env;
	ret_env = &local_env;
	if (setjmp(local_env))
	{
		local = old_local;
		global = old_global;
		rval = FALSE;
	}
	else
		rval = cond(cl, frame);

	ret_env = last_env;
	return(rval);
}


/************************************************************************/
/*			The main read/execute/print loop		*/
/************************************************************************/

void
evloop(void)
{
	term x, *old_local = local, *old_global = global;
	FILE *prog_input = input, *current_input = input;
	jmp_buf local_env, *last_env;

#ifdef THINK_C
	signal(SIGINT, (__sig_func) interrupt);
#else
	signal(SIGINT, interrupt);
#endif
	last_env = ret_env;
	ret_env = &local_env;
	if (setjmp(local_env))
	{
		current_query = NULL;
		local = old_local;
		global = old_global;
		_prompt = _prolog_prompt;
		if (isatty(fileno(input)))
		{
			fseek(input, 0L, SEEK_END);
			fputc('\n', output);
		}
	}
	prompt();

	while ((x = p_read()) != _end_of_file)
	{
		extern term _eval_print, _double_arrow;
		FILE *old_output = output;

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

		_prompt = _prolog_prompt;
		prog_input = input;
		input = current_input;
		local = old_local;
		global = old_global;
	}
	ret_env = last_env;
}
