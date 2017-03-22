/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





		/*	Structure sharing Prolog interpreter 	*/

#include "g.h"
#include <setjmp.h>
#include <math.h>


#define push_trail(f, p) \
	if (f < frame2) \
	{ \
		if (tp < TRAIL_END)  *tp++ = p;  \
		else fatal("TRAIL OVERFLOW"); \
	}

#define	sbase(p)	(p == BOTTOM ? S_BOTTOM : p->sp)

binding	*stack, *sp;			/* the global variable stack	*/

environment *env_stack, *envp, *parent;	/* the local control stack	*/

binding	**trail, **tp;			/* the trail stack */

binding *frame2;

int STACK_SIZE, TRAIL_SIZE, ENV_SIZE;

static binding		*S_BOTTOM, *STACK_END;
static environment	*BOTTOM, *ENV_END;
static binding		**TRAIL_END;

static	int tracing;

long	n_suc_unify = 0,
	n_uns_unify = 0;

pval termb;
binding *frameb;

jmp_buf env3;

char run = FALSE;


extern double fuzz;
extern tlevel;

/* trail_pointer is used by built-ins which get space from the heap
 * and which should be recovered on backtracking
 */

trail_pointer(p)
pval p;
{
	if (tp < TRAIL_END)  *tp++ = (binding *) p;
	else fatal("TRAIL OVERFLOW");
}


_bind(v, f1, term, f2)
var *v;
pval term;
binding *f1, *f2;
{
	register binding *p;

#ifdef DEBUG
	printf("BIND ");
	run = FALSE;
	prin(v, 1200, f1);
	run = TRUE;
	printf(" TO ");
	print(term, 1200, f2);
#endif

	p = f1 + v->offset;
	p->termv = term;
	p->framev = f2;
	if (f1 < frame2)
	{
#ifdef DEBUG
		printf("PUT ON TRAIL\n");
#endif
		if (tp < TRAIL_END)  *tp++ = p; 
		else fatal("TRAIL OVERFLOW");
	}
}


_unify(t1, f1, t2, f2)
register pval t1, t2;
binding *f1, *f2;
{
	register int i;
	register binding *p, *q;

#ifdef DEBUG
	run = FALSE;
	printf("***********\n");
	prin(t1, 1200, f1); printf("  %d\n", f1 - stack);
	prin(t2, 1200, f2); printf("  %d\n", f2 - stack);
	printf("===========\n");
	run = TRUE;
#endif

L1:	if (TYPE(t1) == VAR)
	{
		p = f1 + t1->v.offset;
		if (p->termv != 0)
		{
			t1 = p->termv;
			f1 = p->framev;
			goto L1;
		}
L2:		if (isvariable(t2))
		{
			q = f2 + t2->v.offset;
			if (q->termv != 0)
			{
				t2 = q->termv;
				f2 = q->framev;
				goto L2;
			}
			if (p == q) return(TRUE);
			if (f2 > f1)
			{
				q->termv = t1;
				q->framev = f1;
				push_trail(f2, q);
				return(TRUE);
			}
		}
		p->termv = t2;
		p->framev = f2;
		push_trail(f1, p);
		return(TRUE);
	}
	while (isvariable(t2))
	{
		p = f2 + t2->v.offset;
		if (p->termv == 0) /* bind term 2 */
		{
			p->termv = t1;
			p->framev = f1;
			push_trail(f2, p);
			return(TRUE);
		}
		else { /* get value of term 2*/
			t2 = p->termv;
			f2 = p->framev;
		}
	}
	switch (TYPE(t1))
	{
	case ATOM:
		return(t1 == t2);
	case INT:
		if (isinteger(t2))
			return((t1 == stack_int ? (int) f1 : t1->i.int_val)
				==
				(t2 == stack_int ? (int) f2 : t2->i.int_val));
		else return(isreal(t2) &&
			    (fabs((t1 == stack_int ? (int) f1 : t1->i.int_val)
					- t2->r.real_val) <= fuzz));
	case LIST:
		if (islist(t2) && unify(t1->c.term[0], f1, t2->c.term[0], f2))
		{
			t1 = t1->c.term[1];
			t2 = t2->c.term[1];
			goto L1;
		}
		return(FALSE);
	case FN:
		if (iscompound(t2) && SIZE(t1) == SIZE(t2))
		{
#ifdef PRINC_VAR	/* unify principal term as well as args */
			if (! unify(t1->c.term[0], f1, t2->c.term[0], f2))
				return(FALSE);
#else
			if (t1->c.term[0] != t2->c.term[0])
				return(FALSE);
#endif
			for (i = 1; i < SIZE(t1); i++)
				if (! unify(t1->c.term[i], f1,
					    t2->c.term[i], f2))
					return(FALSE);
			t1 = t1->c.term[i];
			t2 = t2->c.term[i];
			goto L1;
		}
		else return(FALSE);
	case REAL:
		if (isreal(t2))
			return(fabs(t1->r.real_val - t2->r.real_val) <= fuzz);
		else return(isinteger(t2) &&
			    (fabs((t2 == stack_int ? (int) f2 : t2->i.int_val)
					- t1->r.real_val) <= fuzz));
	default:
		return(t1 == t2);
	}
}


unbind(v, f)
register pval v;
register binding *f;
{
	register binding *p;

	while (TYPE(v) == VAR)
	{
		p = f + v->v.offset;
		if (p->termv == 0) break;
		v = p->termv;
		f = p->framev;
	}
	termb = v;
	frameb = f;
}


#define MAXARGS 16


static
eval(a, t, frame1)
register pval a;
compterm *t;
binding *frame1;
{
	register int i;
	int nargs;
	pval arg[MAXARGS];
	binding *frame[MAXARGS];

	nargs = a->p.nargs;
	if (arith_op(a))
	{
		warning("Arithmetic operations are not executable");
		return(FALSE);
	}
	if (t->size != nargs && nargs != NPRED)
	{
		warning("Incorrect number of arguments to built-in");
		return(FALSE);
	}
	if (t->size > MAXARGS)
	{
		warning("TOO MANY ARGUMENTS FOR BUILT IN PREDICATE");
		return(FALSE);
	}
	for (i = 1; i <= t->size; i++)
		if (isvariable(t->term[i]))
		{
			unbind(t->term[i], frame1);
			arg[i-1] = termb; frame[i-1] = frameb;
		}
		else {
			arg[i-1] = t->term[i];
			frame[i-1] = frame1;
		}
	if (nargs == NPRED)
		return FVAL(a)(arg, frame, t->size);
	else return FVAL(a)(arg, frame);
}

clear_frame(n)
register int n;
{
#ifdef DEBUG
	printf("CLEAR %d STARTING AT %d\n", n, (int)(sp-stack));
#endif
	if ((sp + n) >= STACK_END) fatal("STACK OVERFLOW");
	while (n-- != 0) sp++->termv = 0;
}


clean_trail(trail_mark)
binding **trail_mark;
{
	register binding *k;

	while (tp > trail_mark)
	{
		k = *--tp;
		if (k < stack || k >= STACK_END) free_term(k);
	}
}



static int successful = FALSE;

static
lush(c, argn, print_vars)
clause *c;
int argn;
int print_vars;
{
	register pval t;
	register clause *clist;
	register int n;
	register binding *k;
	binding **old_tp, *frame1, *old_frame;
	pval a, *cl;
	int kind = CALL;

#ifdef DEBUG
	printf("LUSH\n");
	parent = envp = env_stack -1;
	sp = stack;
	tp = trail;
#endif

	frame2 = sp;
	cl = &(c->goal[1]);
	n = argn;
	if ((sp + n) >= STACK_END) fatal("STACK OVERFLOW");
	while (n-- != 0) sp++->termv = 0;
NEW_CLAUSE:
#ifdef DEBUG
	printf("GO NEW_CLAUSE\n");
#endif
	frame1 = frame2;
	parent = envp;
NEW_GOAL:
	if (*cl == 0) goto SUCCEED;
	t = *cl;
	old_frame = frame1;
GO_GOAL:
#ifdef DEBUG
	printf("NEW GOAL\n");
#endif
	switch (TYPE(t))
	{
	   case FN:	a = t->c.term[0];
#ifdef PRINC_VAR
			if (isvariable(a))
			{
				unbind(a, frame1);
				if (isatom(termb)) a = termb;
				else {
					warning("Principal term must be an atom");
					goto FAIL;
			
				}
			}
#endif
			if (TYPE(a) == PREDEF)
			{
#ifdef DEBUG
				printf("*** ");   print(t, 1200, frame1); 
#endif
				if (eval(a, t, frame1))
				{
					if (a->p.tflags) trace(t, frame1, a);
					cl++;
					frame1 = old_frame;
					goto NEW_GOAL;
				}
				else{
					tracing = a->p.tflags || tracing;
					goto FAIL;
				}
			}
			else if (clist = a->a.val)
			{
				tracing = a->a.tflags;
				break;
			}
			else goto FAIL;
	   case ATOM:
			if (t == (pval) _cut)
			{
#ifdef DEBUG
				printf("CUT %d %d\n", (int)(parent-env_stack), (int)(env-env_stack));
#endif
				envp = parent;
				cl++;
				if (envp != BOTTOM) envp->clist = 0;
				frame1 = old_frame;
#ifdef PROFILING
				if (profiling) profile_cut();
#endif
				goto NEW_GOAL;
			}
			if (clist = t->a.val)
			{
				tracing = t->a.tflags;
				break;
			}
			else goto FAIL;
	   case PREDEF:
			if (t->p.nargs != 0 && t->p.nargs != NPRED)
			{
				warning("Incorrect number of arguments to built-in");
				goto FAIL;
			}
			if (FVAL(t)((pval *)0, (binding**)0, 0))
			{
				if (t->p.tflags) trace(t, frame1, t);
				cl++;
				frame1 = old_frame;
				goto NEW_GOAL;
			}
			else{
				tracing = t->p.tflags || tracing;
				goto FAIL;
			}
	    case VAR:
			unbind(t, frame1);
			t = termb;
			frame1 = frameb;
#ifdef DEBUG
			printf("\nFrame1 = %o; offset = %d\n", frame1, t->v.offset);
			print(t, 1200, frame1);
#endif
			if (TYPE(t) != VAR) goto GO_GOAL;

	   default:	warning("Cannot execute goal");
			goto FAIL;
	}
#ifdef DEBUG
	printf("MAKING NEW ENVIRONMENT AT %d\n", env);
#endif
	if (++envp >= ENV_END) fatal("ENVIRONMENT STACK FULL");
	envp->parent = parent;
	envp->cl = cl;
	envp->tp = old_tp = tp;
	envp->sp = frame2 = sp;
	envp->tracing = 0;
BACKTRACK_POINT:
#ifdef DEBUG
	printf("BACKTRACK POINT\n");
	print(t, 1200, frame1);
#endif
	n = clist->nvars;
	if ((sp + n) >= STACK_END) fatal("STACK OVERFLOW");
	while (n-- != 0) sp++->termv = 0;
	a = clist->goal[0];

	if (iscompound(a))
		if (iscompound(t) && SIZE(a) == SIZE(t))
		{
			n = 1;
			do {
				if (! unify(a->c.term[n], frame2,
					    t->c.term[n], frame1))
					goto SHALLOW_BACKTRACK;
			} while(++n <= SIZE(t));
		}
		else	goto SHALLOW_BACKTRACK;
	else if (a != t) /* a is an atom */
		goto SHALLOW_BACKTRACK;

	n_suc_unify++;
	if (tracing)
	{
		watch(a, frame2, kind, clist);
		kind = CALL;
		envp->tracing = tlevel;
	}

	envp->clist = clist->rest;
	cl = &(clist->goal[1]);
	goto NEW_CLAUSE;

SHALLOW_BACKTRACK:
	n_uns_unify++;
	if ((clist = clist->rest) != NULL)
	{
#ifdef DEBUG
		printf("TRAIL is %d,old TRAIL is %d\n", (int)(tp-trail),(int)(old_tp-trail));
		print(t, 1200, frame1);
#endif
		while (tp > old_tp)
			(*--tp)->termv = 0;
		sp = frame2;
		goto BACKTRACK_POINT;
	}
	--envp;
FAIL:	if (tracing && kind == CALL)
	{
		watch(t, frame1, FAILG, NULL);
		tlevel--;
	}
POPENV:	if (envp <= BOTTOM)
	{
		if (! successful && print_vars) printf("** no\n");
		return;
	}
#ifdef DEBUG
	printf("BACKTRACKING %d\n", (int)(env-env_stack));
#endif
	if ((clist = envp->clist) == NULL)
	{
		if (envp->tracing) tlevel = envp->tracing -1;
		--envp;
		goto POPENV;
	}
	cl = envp->cl;
	parent = envp->parent;
	old_tp = envp->tp;
	frame2 = sp = envp->sp;
	frame1 = sbase(parent);
	t = *cl;
	while (isvariable(t))
	{
		k = frame1 + t->v.offset;
		t = k->termv;
		frame1 = k->framev;
	}
	while (tp > old_tp)
	{
		k = *--tp;
		if (k >= stack && k < STACK_END) k->termv = 0;
		else free_term(k);
	}
	if (tracing = envp->tracing)
	{
		tlevel = tracing - 1;
		kind = REDO;
	}
	else	kind = CALL;
	goto BACKTRACK_POINT;
SUCCEED:
	while (parent > BOTTOM)
	{
#ifdef DEBUG
		printf("SUCCESS %d\n", (int)(parent-env_stack));
#endif
		if (tracing = parent->tracing)
		{
			tlevel = tracing - 1;
			watch(*(parent->cl), sbase(parent->parent), EXIT, NULL);
			--tlevel;
		}
		cl = parent->cl + 1;
		parent = parent->parent;
		if (*cl)
		{
			old_frame = frame1 = sbase(parent);
			t = *cl;
			goto GO_GOAL;
		}
	}
	if (print_vars) {
		prvars(argn);
		goto POPENV;
	}
	else return;
}



static
prvars(argn)
int argn;
{
	register i;
	FILE *old_output;

	successful = TRUE;
	old_output = output;
	output = stdout;
	if (argn)
	{
		putchar('\n');
		for (i = 0; i != argn; i++)
		{
			if (varcell[i]->pname == anon) continue;
			printf("%s = ", varcell[i]->pname->name);
			print(varcell[i], 1200, S_BOTTOM);
		}
	}
	else printf("** yes\n");
	output = old_output;
}


#ifdef DEBUG
dump_stack()
{
	register i, j;

	run = FALSE;
	fprintf(output, "\n------------- VARIABLE STACK  -------------\n");
	for (i = (int)(sp -1-stack); i >= 0; i--)
	{
		fprintf(output, "%3d :", i);
		if (stack[i].termv == stack_int)
			fprintf(output, "	    %d\n",
					(int) stack[i].framev);
		else {
			if ((j = (int) (stack[i].framev - stack)) || TYPE(stack[i].termv) == VAR)
				fprintf(output, "  %-3d  ", j);
			else fprintf(output, "       ");
			print(stack[i].termv, 1200 , stack[i].framev);
		}
	}
	run = TRUE;
}

dump_env()
{
	register i, base;

	fprintf(output, "\n------------- ENVIRONMENT STACK ------------\n");
	for (i = (int)(envp-env_stack); i != -1; i--)
	{
		if (env_stack[i].parent == BOTTOM)
			base = (int)(S_BOTTOM - stack);
		else	base = (int)(env_stack[i].parent->sp - stack);
		fprintf(output, "%3d :  %4d %4d %4d %4d    ", 
				i,
				(int)(env_stack[i].parent - env_stack),
				base,
				(int)(env_stack[i].sp - stack),
				(int)(env_stack[i].tp - trail)
		);
		print(*(env_stack[i].cl), 1200, &stack[base]);
	}
	fprintf(output, "============================================\n");
}
#endif


execute(c, argn, print_vars)
clause *c;
int argn;
int print_vars;
{
	extern atom *init_prompt, *read_prompt, *prompt_string;

	environment *O_BOTTOM = BOTTOM, *O_parent = parent;
	binding *O_S_BOTTOM = S_BOTTOM;
	binding **O_tp = tp, *O_frame2 = frame2;
	int	O_run = run;

	BOTTOM = envp;
	S_BOTTOM = sp;
#ifdef DEBUG
	printf("BEGIN EXECUTION\n");
	fprintf(stderr, "BOTTOM = %d, sp = %d\n", (int)(BOTTOM-env_stack), (int)(sp-stack));
#endif
	prompt_string = read_prompt;

	successful = FALSE;
	run = TRUE;
	if (envp == env_stack -1)
	{
		n_suc_unify = n_uns_unify = 0;
		tracing = FALSE;
		tlevel = 0;
	}
	if (! setjmp(env3))
		lush(c, argn, print_vars);

	run = O_run;
	clean_trail(O_tp);

	prompt_string = init_prompt;

	envp = BOTTOM;
	sp = S_BOTTOM;
	BOTTOM = O_BOTTOM;
	S_BOTTOM = O_S_BOTTOM;
	parent = O_parent;
	tp = O_tp;
	frame2 = O_frame2;
#ifdef PROFILING
	if (profiling) profile_reset();
#endif
#ifdef DEBUG
	fprintf(stderr, "BOTTOM = %d, sp = %d\n", (int)(BOTTOM-env_stack), (int)(sp-stack));
	printf("END EXECUTION\n");
#endif
}


set_stacks(n)
int n;
{
	extern char *sbrk();

	STACK_SIZE	= n;
	TRAIL_SIZE	= n/4;
	ENV_SIZE	= (2*n)/7;

	sp = stack = (binding *) sbrk(STACK_SIZE * sizeof(binding));
	tp = trail = (binding **) sbrk(TRAIL_SIZE* sizeof(binding *));
	env_stack = (environment *) sbrk(ENV_SIZE* sizeof(environment));

	if ((int)stack == -1 || (int)env_stack == -1 || (int)trail == -1)
		return(0);

	STACK_END = &stack[STACK_SIZE];
	TRAIL_END = &trail[TRAIL_SIZE];
	ENV_END	  = &env_stack[ENV_SIZE];

	S_BOTTOM = sp;
	BOTTOM   = envp = parent = env_stack -1;

	return(TRUE);
}
