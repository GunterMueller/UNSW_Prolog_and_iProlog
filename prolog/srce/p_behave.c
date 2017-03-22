/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





	/*	Predicates to modify Prolog's behaviour		*/


#include "pred.h"
#include <setjmp.h>


static
p_abort PREDICATE
{
	extern jmp_buf env3;

	longjmp(env3, 1);
}


static
p_halt PREDICATE
{
	exit(1);
}


static
p_prompt PREDICATE
{
	extern atom *read_prompt, *prompt_string;

	if (unify(arg[0], frame[0], read_prompt, 0))
	{
		if (isatom(arg[1]))
		{
			prompt_string = read_prompt = (atom *) arg[1];
			return(TRUE);
		}
		else if (arg[0] == arg[1]) return(TRUE);
		fail("Prompt - second argument must be atom")
	}
	return(FALSE);
}


static
trace PREDICATE
{
	if (isatom(arg[0]))
	{
		arg[0] -> a.tflags |= TRACED;
		return(TRUE);
	}
	else fail("Cannot trace non-atom!")
}

static
untrace PREDICATE
{
	if (isatom(arg[0]))
	{
		arg[0] -> a.tflags &= ~TRACED;
		return(TRUE);
	}
	else fail("Tried to untrace non-atom")
}


static char *ops[] =
	{"xfx", "xfy", "yfx", "fx", "fy", "xf", "yf", "nonop", "qatom"};

static
optype atopt(buff)
char *buff;
{
	register i;

	for (i = XFX; i <= YF; i++)
		if (strcmp(ops[i], buff) == 0) return(i);
	return(-1);
}

static
insert_op(a, precedence, type)
atom *a;
char type;
{
	extern atom* hashtable[];
	register atom *p, **q;

	/*	new define must appear BEFORE existing definition in	*/
	/*	hash bucket. NONOP's must be last in chain.		*/

	for (q = &(hashtable[hash(NAME(a))]); *q != a; q = &((*q) -> link));
	p = (atom *) new(ATOM);
	p -> link = a;
	*q = p;
	p -> name = a -> name;
	p -> pred = precedence;
	p -> op_t = type;
}

static
define_op PREDICATE
{
	optype dtype;
	pval p;

	if (! isatom(arg[2]))
		fail("op: 3rd argument must be atom")
	if (arg[2] -> a.op_t != NONOP)
	{
		p = intern(ops[arg[2]->a.op_t], strlen(ops[arg[2]->a.op_t]));

		return(unify(arg[0], frame[0], stack_int, arg[2] -> a.pred)
		    && unify(arg[1], frame[1], p, 0));
	}
	if (! isinteger(arg[0]) || ! isatom(arg[1]))
		return(FALSE);
	if ((dtype = atopt(NAME(arg[1]))) == -1)
		fail("Defop - bad operator type")

	p = arg[2];
	if (((dtype == FX || dtype == FY) && prefix(&p))
	||  ((dtype == XFX || dtype == XFY || dtype == YFX) && infix(&p))
	||  ((dtype == XF || dtype == YF) && postfix(&p)))
	{
		p -> a.op_t = INT_VAL(0);
		return(TRUE);
	}
	else insert_op(p, INT_VAL(0), dtype);
	return(TRUE);
}


static
statistics PREDICATE
{
	long tot_unif;
	int s_index, e_index, t_index;

	extern binding *sp, **tp;
	extern environment *envp;
	extern int STACK_SIZE, TRAIL_SIZE, ENV_SIZE;
	extern long n_suc_unify, n_uns_unify;

	s_index = (int)(sp - stack);
	e_index = (int)(envp - env_stack + 1);
	t_index = (int)(tp - trail);

	printf("Global stack: %d\t(%d %%)\n",s_index, (s_index*100)/STACK_SIZE);
	printf("Local stack:  %d\t(%d %%)\n",e_index, (e_index*100)/ENV_SIZE);
	printf("Trail:        %d\t(%d %%)\n",t_index, (t_index*100)/TRAIL_SIZE);

	tot_unif = n_suc_unify + n_uns_unify;

	printf("unifications %ld\n", tot_unif);
	printf("successful unifications %ld", n_suc_unify);

	if (tot_unif != 0)
		printf(" (%ld %%)", (n_suc_unify*100)/tot_unif);

	printf("\n\n");

	return(TRUE);
}


static
protect PREDICATE
{
	if (isatom(arg[0]))
	{
		arg[0] -> a.lib = TRUE;
		return(TRUE);
	}
	else fail("Protect - argument must be an atom");
}

static
unprotect PREDICATE
{
	if (isatom(arg[0]))
	{
		arg[0] -> a.lib = FALSE;
		return(TRUE);
	}
	else fail("Unprotect - argument must be an atom");
}


static
p_break PREDICATE
{
	evloop();
	clearerr(stdin);
	return(TRUE);
}

static
n_unify PREDICATE
{
	extern long n_suc_unify, n_uns_unify;

	bind_num(0, n_suc_unify);
	bind_num(1, n_uns_unify);
	return(TRUE);
}

static
p_backtrace NPREDICATE
{
	int depth = -1;

	if (argc == 1)
		if (isinteger(arg[0]) && INT_VAL(0) >= 0)
			depth = INT_VAL(0);
		else fail("backtrace - bad arg")

	else if (argc != 0)
		fail("backtrace - bad arg number")

	back_trace(depth);
	return(TRUE);
}


#ifdef PROFILING

static
check_profiling	PREDICATE
{
	if (!profiling)
		fail("PROFILE NOT ON")

	return(TRUE);
}


static
p_profiling	PREDICATE
{
	start_profiling();

	return(TRUE);
}


static
p_profile1	PREDICATE
{
	if (TYPE(arg[0]) != ATOM)
		fail("profile1 - arg must be atom")

	if (check_profiling())
	{
		profile1(arg[0]);
		return(TRUE);
	}
	else	return(FALSE);
}


static
do_profile	PREDICATE
{
	profile();
	return(TRUE);
}

#endif PROFILING


atom_table p_behave =
{
	SET_PRED(NONOP, 0, 3, "defop", define_op),
	SET_PRED(FX, 700, 1, "spy", trace),
	SET_PRED(FX, 700, 1, "unspy", untrace),
	SET_PRED(NONOP, 0, 2, "prompt", p_prompt),
	SET_PRED(NONOP, 0, 0, "abort", p_abort),
	SET_PRED(NONOP, 0, 0, "halt", p_halt),
	SET_PRED(NONOP, 0, 0, "statistics", statistics),
	SET_PRED(NONOP, 0, 1, "protect", protect),
	SET_PRED(NONOP, 0, 1, "unprotect", unprotect),
	SET_PRED(NONOP, 0, 0, "break", p_break),
	SET_PRED(NONOP, 0, 2, "unifications", n_unify),
	SET_PRED(NONOP, 0, NPRED, "backtrace", p_backtrace),
#ifdef PROFILING
	SET_PRED(NONOP, 0, 0, "profiling", p_profiling),
	SET_PRED(NONOP, 0, 0, "check_profiling", check_profiling),
	SET_PRED(NONOP, 0, 0, "do_profile", do_profile),
	SET_PRED(NONOP, 0, 1, "profile1", p_profile1),
#endif
	END_MARK
};
