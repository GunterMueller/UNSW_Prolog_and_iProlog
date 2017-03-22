/******************************************************************************

			UNSW Prolog (version 4.2)

			  Written by Tony Grech
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/

#include "g.h"


FILE	*trace_output = NULL;
int	tlevel;

extern	environment	*parent;
extern	atom		*hashtable[];



trace(p, frame, head)		/* for PREDEF's */
pval p;
binding *frame;
predef *head;
{
	++tlevel;

#ifdef PROFILING
	if (head->tflags & TRACED)
	{
		trace_print(p, frame, CALL);
		trace_print(p, frame, EXIT);
	}
	if (profiling)
		prof_predef();
#else
	trace_print(p, frame, CALL);
	trace_print(p, frame, EXIT);
#endif

	--tlevel;
}



static
trace_print(p, frame, kind)
pval p;
binding *frame;
int kind;
{
	int i;
	FILE *old_output = output;

	static char trace_type[] = {'C', 'E', 'F', 'R'};
	static char arrow_head[] = {'>', '<', '<', '>'};

	if (trace_output != NULL) output = trace_output;

	putc(trace_type[kind], output);
	for (i = (tlevel < 80 ? tlevel : 80); i != 0; i--)
		putc('|', output);
	putc(arrow_head[kind], output);
	print(p, 1200, frame); 
	fflush(output);

	output = old_output;
}



watch(p, frame, kind, cl)
pval p;
binding *frame;
int kind;
clause *cl;
{
	atom *head;

	++tlevel;

#ifdef PROFILING
	if (isvariable(p))
	{
		unbind(p, frame);
		p = termb;
		frame = frameb;
	}
	if (iscompound(p)) head = (atom *)p->c.term[0];
	else		   head = (atom *)p;
	if (isvariable(head))
	{
		unbind(head, frame);
		head = (atom *)termb;
	}

	/*
	 * BUG - misses some tracing fails if
	 * tracing and profiling at same time
	 */
	if ((head->tflags & TRACED) || !profiling)
		trace_print(p, frame, kind);
	if (isatom(head) && profiling)
		profiler(cl, kind);
#else
	trace_print(p, frame, kind);
#endif
}




back_trace(depth)
{
	register environment	*p;
	static last_depth = 5;

	if (depth != -1) last_depth = depth;
	else depth = last_depth;

	p = parent;
	if ( p >= env_stack && depth != 0)
		fprintf(output, "\n\t*** BACK TRACE ***\n");

	while (p >= env_stack && depth-- != 0)
	{
		print(*(p->cl), 1200, p->parent < env_stack ? stack : p->parent->sp);
		p = p->parent;
	}
}
