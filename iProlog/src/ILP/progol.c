/************************************************************************/
/*			Naive implementation of Progol			*/
/************************************************************************/

#include "prolog.h"

#define MAX_ARG		8
#define MAX_DEPTH	8
#define MAX_GOALS	100
#define MAX_TERMS	20
#define MAX_TYPES	10
#define MAX_VAR		100

#define INPUT(i, mode)		(ARG(0, ARG(i, mode)) == _plus)
#define OUTPUT(i, mode)		(ARG(0, ARG(i, mode)) == _minus)
#define CONSTANT(i, mode)	(ARG(0, ARG(i, mode)) == _hash)
#define TYPE_INFO(i, mode)	ARG(1, ARG(i, mode))

extern term *global;
extern term _plus, _minus;
extern term varlist, *get_proc(), new_clause(), new_var();

static term _types, _modeh, _modeb, _hash;


static struct
{
	term type;
	int n_terms;
	struct {
		term value;
		term lit;
		term var;
	} t[MAX_TERMS];
} new_term[MAX_TYPES];

static struct
{
	term literal;
	term mode;
} new_goal[MAX_GOALS];

static int n_types;
static int n_goals;

/************************************************************************/
/* Print the contents of new_goals and new_terms - debugging only	*/
/************************************************************************/

static void
show_goals(void)
{
	int i;

	fputs("\nNEW GOALS:\n", output);

	for (i = 0; i < n_goals; i++)
	{
		prin(new_goal[i].literal);
		fprintf(output, "/");
		print(new_goal[i].mode);
	}
}

static void
show_terms(void)
{
	int i, j;

	fputs("\nFLATTENED TERMS:\n", output);

	for (i = 0; i < n_types; i++)
	{
		prin(new_term[i].type);
		fputs(":\n", output);

		for (j = 0; j < new_term[i].n_terms; j++)	
		{
			fputs("\t{", output);
			prin(new_term[i].t[j].var);
			fputc(' ', output);
			prin(new_term[i].t[j].lit);
			fputc(' ', output);
			prin(new_term[i].t[j].value);
			fputs("}\n", output);
		}
	}
}


/************************************************************************/
/* get_example(P/A) - looks for first example of P/A and returns it.	*/
/* This may have to be changed to make a copy of the example.		*/
/* May also be changed to find a random example, rather than the first	*/
/************************************************************************/

static term
get_example(term predicate)
{
	term princ = ARG(1, predicate);
	int arity = IVAL(ARG(2, predicate));
	term p;

	for (p = PROC(princ); p != NULL; p = NEXT(p))
	{
		term head = HEAD(p);

		if (ARITY(head) == arity)	
			return(head);
	}
	return(NULL);
}


/************************************************************************/
/* Initialise typing structures from the "types" predicate.		*/
/************************************************************************/

static void
init_types(void)
{
	term p = PROC(_types);

	if (p == NULL)
		fail("Types not declared");

	if (TYPE(p = HEAD(p)) != FN)
		fail("Types must be declared as a list of type names");

	for (p = ARG(1, p), n_types = 0; p != _nil; p = CDR(p), n_types++)
	{
		if (TYPE(p) != LIST)
			fail("Invalid list structure in type declaration");

		if (n_types == MAX_TYPES)
			fail("Exceeded maximum number of allowed types");
		if (TYPE(CAR(p)) != ATOM)
			fail("Incorrect type declaration");

		new_term[n_types].type = CAR(p);
		new_term[n_types].n_terms = 0;
	}
	show_terms();
}
	

/************************************************************************/
/* Apply 'f' to 'x'							*/
/* 'f' is assumed to be a unary predicate that checks the type of 'x'	*/
/************************************************************************/

static int
check_type(term f, term x)
{
	term q[2] = {NULL, NULL};
	term *old_global = global;
	int rval;

	*q = g_fn1(f, x);
	rval = cond(q, NULL);
	global = old_global;
	return(rval);
}


/************************************************************************/
/* Return the mode that conforms to this example, NULL if not found	*/
/* example - a +ve example which is assumed to be ground unit		*/
/************************************************************************/

static term
get_mode(term example)
{
	term p;

	for (p = PROC(_modeh); p != NULL; p = NEXT(p))
	{
		int i;
		term mode = ARG(2, HEAD(p));

		if (ARITY(mode) == ARITY(example))
			for (i = ARITY(mode); i != 0; i--)
				if (! check_type(TYPE_INFO(i, mode), ARG(i, example)))
					break;
		if (i == 0)
			return(mode);
	}
	return(NULL);
}


/************************************************************************/
/*	Find the type of an individual element in a structure		*/
/************************************************************************/

static term
get_type(term t)
{
	int i;

	for (i = 0; i < n_types; i++)
		if (check_type(new_term[i].type, t))
			return(new_term[i].type);

	show_terms();
	print(t); fflush(output);
	fail("Unknown type");
}


/************************************************************************/
/*		Lookup up type entry given the type name		*/
/************************************************************************/

static int
get_type_values(term type)
{
	int i;

	for (i = 0; i < n_types; i++)
		if (new_term[i].type == type)
			return(i);

	show_terms();
	print(type); fflush(output);
	fail("Unknown type");
}


/************************************************************************/
/* insert a term into terms list only if it's not already there		*/
/************************************************************************/

static term
insert(term x, term lit, term type)
{
	int i, j;

	for (i = 0; i < n_types; i++)
		if (new_term[i].type == type)
		{
			for (j = 0; j < new_term[i].n_terms; j++)
				if (term_compare(x, NULL, new_term[i].t[j].value, NULL) == 0)
					return(new_term[i].t[j].var);

			if (new_term[i].n_terms == MAX_TERMS)
				fail("Exceeded maximum number of allowed terms");

			new_term[i].t[j].value = x;
			new_term[i].t[j].lit = lit;
			new_term[i].t[j].var = new_ref();
			new_term[i].n_terms++;
			return(new_term[i].t[j].var);
		}
print(type);
	fail("A type appears in a mode that was not declared");
}


/************************************************************************/
/* insert a term into terms list and put all subtrees in too		*/
/************************************************************************/

static term
insert_term(term x, term type)
{
	int i;
	term rval, lit;
print(x);
print(type);
fprintf(output, "-------------\n");
	switch (TYPE(x))
	{
	case FN:
		lit = new_g_fn(ARITY(x));
		rval = insert(x, lit, type);
		
		for (i = 0; i < ARITY(x); i++)
			ARG(i, lit) = insert_term(ARG(i, x), get_type(ARG(i, x)));
		return(rval);
	case LIST:
		lit = gcons(_nil, _nil);
		rval = insert(x, lit, type);
		CAR(lit) = insert_term(CAR(x), get_type(CAR(x)));
		CDR(lit) = insert_term(CDR(x), get_type(CDR(x)));
		return(rval);
	default:
		return(insert(x, x, type));
	}
}


/************************************************************************/
/* flatten example according to mode decl and store terms in terms list	*/
/************************************************************************/

static void
flatten(term example, term mode)
{
	int i;

	for (i = 1; i <= ARITY(example); i++)
		insert_term(ARG(i, example), TYPE_INFO(i, mode));
}


/************************************************************************/
/* Invoke background knowledge to generate new literals for bottom	*/
/************************************************************************/

static term current_mode;

static void
add1(term *result, term x, term *frame)
{
	int i;

	for (i = 0; i < n_goals; i++)
		if (term_compare(x, frame, new_goal[i].literal, NULL) == 0)
			return;

	if (n_goals == MAX_GOALS)
		fail("Exceeded maximum number of allowed goals");


	new_goal[n_goals].literal = x = make(x, frame);
	new_goal[n_goals++].mode = current_mode;
	flatten(x, current_mode);
}


static void
gen_goal(term new_goal)
{
	term q[2] = {NULL, NULL};

	*q = new_goal;
	call_prove(q, NULL, new_goal, -1, add1, TRUE);
}


/************************************************************************/
/* Fill in arguments for new goal.					*/
/* If all arguments present and executing it succeeds, add to bottom	*/
/************************************************************************/

static void
fill(int first_arg, term new_goal, term mode)
{
	if (first_arg > ARITY(mode))
	{
		current_mode = mode;
		gen_goal(new_goal);
	}
	else if (OUTPUT(first_arg, mode))
	{
		ARG(first_arg, new_goal) = new_ref();
		fill(first_arg+1, new_goal, mode);
	}
	else if (INPUT(first_arg, mode))
	{
		int i = get_type_values(TYPE_INFO(first_arg, mode));
		int j, n_terms = new_term[i].n_terms;

		for (j = 0; j < n_terms; j++)
		{
			ARG(first_arg, new_goal) = new_term[i].t[j].value;
			fill(first_arg+1, new_goal, mode);
		}
	}
}


/************************************************************************/
/*		gen_layer - find literals in bottom clause		*/
/************************************************************************/

static void
gen_layer(void)
{
	term p;

	for (p = PROC(_modeb); p != NULL; p = NEXT(p))
	{
		term mode = ARG(2, HEAD(p));
		term new_goal = new_h_fn(ARITY(mode));

		ARG(0, new_goal) = ARG(0, mode);
		fill(1, new_goal, mode);
		dispose(new_goal);
	}
}


/************************************************************************/
/* saturate(example) - find first matching mode				*/
/* saturate example and return list of new goals			*/
/************************************************************************/

static int
saturate(term goal, term *frame)
{
	term example = check_arg(1, goal, frame, ANY, IN);
	term head_mode = get_mode(example);
	int i;

	if (head_mode == NULL)
		fail("Couldn't find mode to match example");

	init_types();

	n_goals = 1;

	new_goal[0].literal = make(example, frame);
	new_goal[0].mode = head_mode;
	flatten(example, head_mode);

	for (i = 0; i < MAX_DEPTH; i++)
	{
		int old_n_goals = n_goals;

		gen_layer();
		if (n_goals == old_n_goals)
		{
			printf("Stopped at depth %d\n", i);
			break;
		}
	}

	show_terms();
	show_goals();
	return(TRUE);
}
		

/************************************************************************/
/* Turns a term into a clause and checks its cover.			*/
/* Returns number of +ve and -ve examples covered			*/
/* -ve examples have the form false :- ...				*/
/************************************************************************/

static int
check_cover(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, FN, IN);
	term y = check_arg(2, goal, frame, INT, OUT);
	term z = check_arg(3, goal, frame, INT, OUT);
	term cl, *proc, examples, p;
	int pos = 0, neg = 0;

	varlist = _nil;
	cl = mkclause(x, frame);
	proc = get_proc(HEAD(cl), frame);
	examples = *proc;
	*proc = cl;

	for (p = examples; p != NULL; p = NEXT(p))
		pos += cond(&HEAD(p), frame);

	for (p = PROC(_false); p != NULL; p = NEXT(p))
		neg += cond(BODY(p), frame);

	dispose(cl);
	*proc = examples;
	unify(y, frame, new_int(pos), frame);
	unify(z, frame, new_int(neg), frame);
	return(TRUE);
}


/************************************************************************/
/*	   Create variablised terms for clause construction		*/
/************************************************************************/

static struct
{
	term val;
	term v;
} xvar[MAX_VAR];

static int n_vars;

static term
variable(term t)
{
	int i;
	char buf[16];

	for (i = 0; i < n_vars; i++)
		if (term_compare(xvar[i].val, NULL, t, NULL) == 0)
		{
			if (TYPE(xvar[i].v) == FREE)
				TYPE(xvar[i].v) = BOUND;
			return(xvar[i].v);
		}

	if (n_vars++ == MAX_VAR)
		fail("Too many variables needed in bottom clause");

	sprintf(buf, "X%d", i);
	xvar[i].val = t;
	xvar[i].v = new_var(FREE, i, intern(buf));
	return(xvar[i].v);
}


static term
variablise(int n)
{
	term lit = new_goal[n].literal;
	term mode = new_goal[n].mode;
	term new = new_h_fn(ARITY(mode));
	int i;

	ARG(0, new) = ARG(0, mode);
	for (i = ARITY(mode); i != 0; i--)
		if (CONSTANT(i, mode))
			ARG(i, new) = ARG(i, lit);
		else
			ARG(i, new) = variable(ARG(i, lit));
	return(new);
}


/************************************************************************/
/* search for subset of saturated clause that is an acceptable cover	*/
/************************************************************************/

static int n_vars;

static void
fill_clause(int first_goal, term new_clause)
{
	int i, old_nvars = n_vars;

	GOAL(first_goal, new_clause) = NULL;
	NVARS(new_clause) = n_vars;
	list_proc(new_clause);

	for (i = first_goal; i < n_goals; i++)
	{
		GOAL(first_goal, new_clause) = variablise(i);
		fill_clause(i+1, new_clause);
		n_vars = old_nvars;
		/* free_term(GOAL(first_goal, new_clause)); */
	}
	GOAL(first_goal, new_clause) = NULL;
}


static int
search(term goal, term *frame)
{
	term cl = new_clause(MAX_GOALS);

	saturate(goal, frame);
	n_vars = 0;
	HEAD(cl) = variablise(0);
	fill_clause(1, cl);
}


/************************************************************************/
/*			   Initialise Module				*/
/************************************************************************/

void
progol_init(void)
{
	_types = intern("types");
	_modeh = intern("modeh");
	_modeb = intern("modeb");
	_hash = intern("#");

	new_pred(saturate, "saturate");
	new_pred(check_cover, "check_cover");
	new_pred(search, "search");
}
