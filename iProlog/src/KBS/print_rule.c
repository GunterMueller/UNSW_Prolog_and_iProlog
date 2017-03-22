#include "prolog.h"

extern term _is, _if, _then, _else, _except, _because, _and, _or;

static term _unless;

/************************************************************************/
/*	   Print a goal and format it if it as "if" exression		*/
/************************************************************************/


static void
tab(int n)
{
	int i;

	fputs("\n\t", output);
	for (i = 0; i < n; i++)
		fputc('\t', output);
}


void
print_goal(term x, int n)
{
	if (TYPE(x) != FN)
		prin(x);
	else if (ARG(0, x) ==_if)
	{
		fputs("if ", output);
		print_goal(ARG(1, x), n);
	}
	else if (ARG(0, x) == _else)
	{
		print_goal(ARG(1, x), n);
		tab(n);
		fputs("else ", output);
		print_goal(ARG(2, x), n);
	}
	else if (ARG(0, x) == _bar)
	{
		char sep = '(';
		do {
			tab(n);
			fputc(sep, output);
			sep = '|';
			print_goal(ARG(1, x), n);
			x = ARG(2, x);
		}
		while (TYPE(x) == FN && ARG(0, x) == _bar);
		tab(n);
		fputc(sep, output);
		print_goal(x, n);
		fputc(')', output);
	}
	else if (ARG(0, x) == _semi_colon)
	{
		char sep = '(';
		do {
			tab(n);
			fputc(sep, output);
			sep = ' ';
			print_goal(ARG(1, x), n);
			fputc(';', output);
			x = ARG(2, x);
		}
		while (TYPE(x) == FN && ARG(0, x) == _semi_colon);
		tab(n);
		fputc(sep, output);
		print_goal(x, n);
		fputc(')', output);
	}
	else if (ARG(0, x) == _then)
	{
		prin(ARG(1, x));
		fputs(" then ", output);
		print_goal(ARG(2, x), n+1);
	}
	else if (ARG(0, x) == _arrow)
	{
		prin(ARG(1, x));
		fputs(" -> ", output);
/*
 * 		if (TYPE(ARG(2, x)) == FN || ARG(0, ARG(2, x)) == _semi_colon)
 * 			tab(n+1);
 */
		print_goal(ARG(2, x), n+1);
	}
	else if (ARG(0, x) == _comma)
	{
		tab(n);
		prin(ARG(1, x));
		fputc(',', output);
		if (TYPE(ARG(2, x)) != FN || ARG(0, ARG(2, x)) != _comma)
			tab(n);
		print_goal(ARG(2, x), n);
	}
	else if (ARG(0, x) == _except)
	{
		prin(ARG(1, x));
		fputs(" except", output);
		tab(n);
		print_goal(ARG(2, x), n);
	}
	else if (ARG(0, x) == _unless)
	{
		prin(ARG(1, x));
		fputs(" unless", output);
		tab(n+1);
		print_goal(ARG(2, x), n+1);
	}
	else if (ARG(0, x) == _is)
	{
		prin(ARG(1, x));
		fputs(" is ", output);
		print_goal(ARG(2, x), n);
	}
	else
		prin(x);
}

/************************************************************************/
/*			Print formatted expressions			*/
/************************************************************************/

static int
print_if(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);

	tab(0);
	print_goal(x, 0);
	fputc('\n', output);
	return TRUE;
}


/************************************************************************/
/*   Print each clause in a clause list and pretty print if-then-else	*/
/************************************************************************/

void
print_rule(term p)
{
	extern int display;
	term clist;
	int i;

	if (p == NULL)
		return;

	display = TRUE;

	if (TYPE(p) != CLAUSE)
	{
		prin(p);
		return;
	}

	putc('\n', output);
	for (clist = p; clist != NULL && TYPE(clist) == CLAUSE; clist = NEXT(clist))
	{
		print_goal(HEAD(clist), 0);
		if (GOAL(1, clist) != NULL)
		{
 			fprintf(output, " :-\n\t");
			for (i = 1; GOAL(i + 1, clist) != NULL; i++)
			{
				print_goal(GOAL(i, clist), 0);
				fprintf(output, ",\n\t");
			}
			print_goal(GOAL(i, clist), 0);
		}
		fprintf(output, ".\n");
	}
}


/************************************************************************/
/*		Pretty print clauses attached to an atom		*/
/************************************************************************/

static int
pp_rule(term goal, term *frame)
{
	term a = check_arg(1, goal, frame, ATOM, IN);
	term q = PROC(a);

	if (q != NULL && TYPE(q) == CLAUSE)
		print_rule(q);
	return TRUE;
}


/************************************************************************/
/*			Module initialisation				*/
/************************************************************************/

void
print_rule_init(void)
{
	_unless = intern("unless");

	new_pred(print_if, "print_if");
	defop(700, FX, new_fpred(pp_rule, "pp_rule"));
}
