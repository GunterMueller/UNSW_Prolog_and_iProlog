/******************************************************************************

			UNSW Prolog (version 4.2)

			  Written by Tony Grech
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/

#include "g.h"

#ifdef PROFILING


typedef struct prof_info
{
	char locked;		/* need this so that recursive calls don't
				   get credited for the same time twice    */
	int calls;
	int s_calls;
	long time;
	int exits[1];		/* extensible array counting exits 
				 * from each clause and its subgoals */
} prof_info;


typedef struct pstack_entry
{
	clause *cl;
	environment *envp;
	struct	pstack_entry *P_sp;	/* parent stack_pointer */
	char	flags;
	long	entry_time;		/* 0 if call didn't lock */
	int	*pend_exits;
} pstack_entry;

/* pstack_entry flag values */

#define	EXITED		1
#define	SUCCESSFUL	2


#define	INFO_CHUNK	(2048 * MEM_AVAIL)

#define	PROF_INFO(cl)	((prof_info *)cl->prof_data)


char			profiling = FALSE;

static	pstack_entry	*pstack;
static	int		PROF_SIZE;
static	pstack_entry	*psp;		/* profiling stack pointer */
static	pstack_entry	*exit_psp;

static	char		*pheap;
static	char		*ip;
static	int		INFO_SIZE;


extern	char		run;
extern	char		*sbrk();
extern	long		get_time();


profiler(cl, kind)
clause *cl;
int kind;
{
	switch (kind)
	{
	    case REDO:	do_redo(cl);	return;
	    case CALL:	do_call(cl);	return;
	    case EXIT:	do_exit();	return;
	}
}



static
inc_exits(sp)
pstack_entry *sp;
{
	if (sp >= pstack)
	{
		sp->pend_exits[0]++;
		sp->pend_exits++;
	}
}




static
push_stack()
{
	if (++psp >= &pstack[PROF_SIZE])
		fatal("PROFILING STACK_OVERFLOW\n");

	if ((psp == pstack) || !((psp-1)->flags&SUCCESSFUL))
		psp->P_sp	= psp-1;
	else	psp->P_sp	= exit_psp->P_sp;

	psp->envp	= envp;
}




static
int		/* returns true if clause was locked */
_call(cl)
clause *cl;
{
	prof_info *info_p;

	info_p = PROF_INFO(cl);
	info_p->calls++;

	psp->cl		= cl;
	psp->pend_exits	= &info_p->exits[1];
	psp->flags	= 0;

	if (!info_p->locked)
	{
		info_p->locked = TRUE;
		return(FALSE);
	}
	else	return(TRUE);
}




static
do_call(cl)
clause *cl;
{
	push_stack();

	psp->entry_time = ( _call(cl) ? 0 : get_time() );
}



static
do_redo(cl)
clause *cl;
{
	pstack_entry *sp;
	long cur_time = get_time();

	for (; psp >= pstack && psp->envp >= envp; psp--)
	{
		if (psp->flags & SUCCESSFUL)
		{
			if (psp->P_sp >= pstack)
				psp->P_sp->pend_exits--;
		}
		else if (psp->entry_time)
		{
			prof_info *info_p = PROF_INFO(psp->cl);

			info_p->time += cur_time - psp->entry_time;
			info_p->locked = FALSE;
		}
	}

	++psp;
	for (sp = psp->P_sp; sp >= pstack && (sp->flags&SUCCESSFUL); sp = sp->P_sp)
	{
		sp->flags &= ~SUCCESSFUL;

		if (sp->P_sp >= pstack)
			sp->P_sp->pend_exits--;
		if (sp->entry_time)
		{
			sp->entry_time = cur_time;
			PROF_INFO(sp->cl)->locked = TRUE;
		}
	}

	psp->entry_time = ( _call(cl) ? 0 : cur_time );
}



static
do_exit()
{
	prof_info *info_p;

	if (!(psp->flags&SUCCESSFUL))
		exit_psp = psp;
	else	exit_psp = exit_psp->P_sp;

if (TYPE(exit_psp->cl) != CLAUSE)
{printf("PROFILE BUG ON EXIT %d\n", TYPE(exit_psp->cl)); return;}

	info_p = PROF_INFO(exit_psp->cl);

	if (!(exit_psp->flags & EXITED))
		info_p->s_calls++;

	inc_exits(exit_psp->P_sp);
	exit_psp->flags |= (EXITED|SUCCESSFUL);

	if (exit_psp->entry_time)
	{
		info_p->time += get_time() - exit_psp->entry_time;
		info_p->locked = FALSE;
	}
	info_p->exits[0]++;
}



profile_cut()
{
	if (psp < pstack)
		return;

	if (psp->flags&SUCCESSFUL)
		psp = exit_psp->P_sp;

	inc_exits(psp);
}


prof_predef()
{
	push_stack();

	psp->flags = (EXITED|SUCCESSFUL);
	psp->entry_time	= 0;

	inc_exits(psp->P_sp);

	exit_psp = psp;
}



static
each_clause(a, fn)
atom *a;
int (*fn)();
{
	clause *cl;
	int ngoals;

	for (cl = VAL(a); cl != NULL; cl = cl->rest)
	{
		for (ngoals = 0; cl->goal[ngoals+1]; ngoals++)
			;
		(*fn)(cl, ngoals);
		if (cl == cl->rest)	break;
	}
}



static
each_atom(fn)
int (*fn)();
{
	int	h;
	atom	*a;
	extern	atom	*private, *hashtable[];

	for (h = 0; h < HASHSIZE; h++)
		for (a = hashtable[h]; a != NULL; a = a->link)
			(*fn)(a);

	for (a = private; a != NULL; a = a->link)
		(*fn)(a);
}



prof_init_clause(cl, ngoals)
clause *cl;
{
	cl->prof_data	= ip;

	ip += sizeof(prof_info) + ngoals*sizeof(int);
	if (ip >= &pheap[INFO_SIZE])
	{
		pheap = cl->prof_data = sbrk(INFO_CHUNK);
		if ((int)pheap == -1)
		{
			fprintf(stderr, "OUT OF MEMORY FOR PROFILING\n");
			exit(1);
		}

		INFO_SIZE = INFO_CHUNK;
		ip = pheap + sizeof(prof_info) + ngoals*sizeof(int);
	}

	prof_reset_clause(cl, ngoals);
}



static
prof_reset_clause(cl, ngoals)
clause *cl;
{
	prof_info *info_p;

	info_p		= PROF_INFO(cl);
	info_p->calls	= info_p->s_calls = 0;
	info_p->locked	= info_p->time	= 0;

	do
		info_p->exits[ngoals] = 0;
	while (--ngoals >= 0);
}



static
prof_init_atom(a)
atom *a;
{
	a->tflags |= PROFILED;

	if (TYPE(a) != PREDEF)
		each_clause(a, prof_init_clause);
}



static
prof_reset_atom(a)
atom *a;
{
	if (TYPE(a) != PREDEF)
		each_clause(a, prof_reset_clause);
}




static
_profile1(cl, ngoals)
clause *cl;
{
	prof_info *info_p;
	int i, temp;

	info_p = PROF_INFO(cl);

	fprintf(output, "%d\t%d\t%d\t%ld.", info_p->calls,
					    info_p->calls-info_p->s_calls,
					    info_p->exits[0],
					    info_p->time/HZ);

	temp = 2*(info_p->time % HZ);
	if (temp < 10) putc('0', output);

	fprintf(output, "%lds\n", temp);

	if (!info_p->calls)
	{
		fprintf(output, "* ");
		_prin(cl->goal[0],1200);
		putc('\n', output);
		return;
	}
	_prin(cl->goal[0],1200);

	if (ngoals)
	{
		fprintf(output, " :-");
		for (i = 1; i != ngoals; i++)
		{
			fprintf(output, "\n\t%d\t", info_p->exits[i]);
			_prin(cl->goal[i], 999);
			putc(',', output);
		}
		fprintf(output, "\n\t%d\t", info_p->exits[i]);
		_prin(cl->goal[i], 999);
	}
	fprintf(output, ".\n\n");
}



profile1(proc)
pval proc;
{
	if (TYPE(proc) == PREDEF || proc->a.lib || !proc->a.val)
		return;

	run = FALSE;

	if (VAL(proc))  putc('\n', output);
	each_clause(proc, _profile1);

	run = TRUE;
}



profile()
{
	each_atom(profile1);
}


profile_reset()
{
	long cur_time = get_time();

	for (; psp >= pstack && psp->envp >= envp; psp--)
		if (!(psp->flags&SUCCESSFUL)&&(psp->entry_time))
		{
			prof_info *info_p = PROF_INFO(psp->cl);

			info_p->time += cur_time - psp->entry_time;
			info_p->locked = FALSE;
		}
}



start_profiling()
{
	extern	int	ENV_SIZE;

	if (!profiling)
	{
		PROF_SIZE = (11*ENV_SIZE/10);
		INFO_SIZE = 2*INFO_CHUNK;

		pstack = (pstack_entry *)sbrk(PROF_SIZE*sizeof(pstack_entry)+INFO_SIZE);

		if ((int)pstack == -1)
		{
			fprintf(stderr, "not enough memory for profiling\n");
			return;
		}

		ip = pheap = (char *)&pstack[PROF_SIZE];
		profiling = TRUE;
		each_atom(prof_init_atom);
	}
	else	each_atom(prof_reset_atom);

	psp	   = pstack - 1;
}
#endif PROFILING
