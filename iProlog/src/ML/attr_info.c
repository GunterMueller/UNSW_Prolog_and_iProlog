/************************************************************************/
/*	This module contains attribute/value processing routines.	*/
/*									*/
/* Attr/val pairs are encoded as bit sets. One attribute per word	*/
/* Bit slices are used to store instances of attr/value pairs		*/
/************************************************************************/

#include <ctype.h>
#include "attr_info.h"
#include "set.h"

#define FAIL(msg)	{data_error = TRUE; fail(msg);}

extern term _unless;

/************************************************************************/
/*			Local variable declarations			*/
/************************************************************************/

static int n_words, n_classes, n_attributes, n_examples;
static term names, index, data_set;
static attribute_struct *attribute;
static class_struct *class;
static int data_error = FALSE;

static long example_number;

/************************************************************************/
/*		Set printing when using attribute encoding		*/
/************************************************************************/

void
print_complex(term s)
{
	term names = CONTENTS(s);
	int n_attr = ARITY(names)-1;
	char *name, *sep = "";
	int attr;

	for (attr = 0; attr < n_attr; attr++)
	{
		int i, n_values = NVALS(attr, names);
		term *value = VALUES(attr, names);
		unsigned long sel = SELECTOR(attr, s);
		char *or = "";

		if (sel == 0 || sel == ALL_BITS)
			continue;

		name = NAME(A_NAME(attr, names));
		fprintf(output, "%s%c%s = ", sep, toupper(name[0]), name+1);
		sep = ", ";

		for (i = 0; i < n_values; i++)
			if (sel & (1 << i))
			{
				fprintf(output, "%s%s", or, NAME(value[i]));
				or = " or ";
			}
	}

	if (CLASS(s) == -1)
	{
		fputc('\n', output);
		return;
	}
	if (*sep != '\0')
		fprintf(output, " -> ");
	name = NAME(ARG(0, CLASS_NAMES));
	fprintf(output, "%c%s = %s", toupper(name[0]), name+1, NAME(CLASS_NAME(CLASS(s))));
}



/************************************************************************/
/*		Routines for outputting rules as if-statements		*/
/************************************************************************/

static void
tab(int n)
{
	int i;

	fputc('\n', output);
	for (i = 0; i < n; i++)
		fputs("    ", output);
}


static void
print_rule_tree(term rule, int depth)
{
	char sep = '(';

	while (rule != NULL && rule != _nil)
	{
		tab(depth);
		fputc(sep, output);
		sep = '|';
		print_complex(rule);
		if (EXCEPT(rule) != NULL)
		{
			fputs(" unless", output);
			print_rule_tree(EXCEPT(rule), depth+1);
		}
		rule = NEXT(rule);
	}
	if (sep == '|')
		fputc(')', output);
}


void
print_cover(term rule)
{
	int i;
	char *sep = "(";
	term names = CONTENTS(rule);

	fprintf(output, "%s", NAME(ARG(0, names)));
	for (i = 1; i <= ARITY(names); i++)
	{
		char *name = NAME(ARG(0, ARG(i, names)));

		fprintf(output, "%s%c%s", sep, toupper(name[0]), name+1);
		sep = ", ";
	}
	fprintf(output, ") :-");
	print_rule_tree(rule, 1);
	fprintf(output, ".\n");
}


/************************************************************************/
/*			   Turn a set into a clause			*/
/************************************************************************/

static term head, rule_to_clauses(term);
static term *bound_var;

static term
complex_to_clause(term s)
{
	term rval = NULL, *q = &rval, names = CONTENTS(s);
	int attr, n_attr = ARITY(names)-1;

	for (attr = 0; attr < n_attr; attr++)
	{
		int i, count, n_values = NVALS(attr, names);
		term t, *p, *value = VALUES(attr, names);
		unsigned long sel = SELECTOR(attr, s);

		if (sel == 0 || sel == ALL_BITS)
			continue;

		t = h_fn2(_equal, bound_var[attr], NULL);
		p = &ARG(2, t);
		for (i = 0, count = 0; i < n_values; i++)
			if (sel & (1 << i))
 				switch (count++)
 				{
 				case 0:
 					*p = value[i];
 					break;
 				case 1:
 					*p = hcons(*p, hcons(value[i], _nil));
 					p = &CDR(CDR(*p));
 					break;
 				default:
 					*p = hcons(value[i], _nil);
 					p = &CDR(*p);
 					break;
 				}
 
 		if (count > 1)
 			ARG(0, t) = intern("in");
 
		if (*q == NULL)
			*q = t;
		else
		{
			*q = h_fn2(_comma, *q, t);
			q = &ARG(2, *q);
		}
	}

	if (CLASS(s) == -1)
		return rval;

	if (rval == NULL)
		rval = h_fn2(_equal, bound_var[n_attr], CLASS_NAME(CLASS(s)));
	else
		rval = h_fn2(_arrow, rval, h_fn2(_equal, bound_var[n_attr], CLASS_NAME(CLASS(s))));

	if (EXCEPT(s) != NULL)
		rval = h_fn2(_unless, rval, rule_to_clauses(EXCEPT(s)));

	return rval;
}


static term
rule_to_clauses(term rule)
{
	term rval = NULL, *p = &rval;

	while (rule != NULL && rule != _nil)
	{
		if (*p != NULL)
		{
			*p = h_fn2(_bar, *p, complex_to_clause(rule));
			p = &ARG(2, *p);
		}
		else
			*p = complex_to_clause(rule);

		rule = NEXT(rule);
	}
	return rval;
}


term
cover_to_clause(term rule)
{
	extern term new_var(int, int, term), new_clause(int);
	int i;
	term cl;
	term names = CONTENTS(rule);

	head = new_h_fn(ARITY(names));
	ARG(0, head) = ARG(0, names);
	bound_var = malloc(ARITY(names) * sizeof(term));

	for (i = 0; i < N_ATTR(names); i++)
	{
		char *name = NAME(A_NAME(i, names));
		char buf[128];

		sprintf(buf, "%c%s", toupper(name[0]), name+1);
		ARG(i+1, head) = new_var(FREE, i, intern(buf));
		bound_var[i] = new_var(BOUND, i, intern(buf));
	}

	cl = new_clause(1);
	NVARS(cl) = ARITY(names);
	HEAD(cl) = head;
	GOAL(1, cl) = rule_to_clauses(rule);
	free(bound_var);
	return cl;
}


/************************************************************************/
/*	find the type declarations for this induction task		*/
/************************************************************************/

term
get_table(term name)
{
	if (PLIST(name) == NULL)
		FAIL("table not declared");

	return PLIST(name);
}


static int
put_table(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, FN, IN);
	term r = ARG(0, x);

	if (TYPE(r) != ATOM)
		fail("Relation name must be an atom");
	PLIST(r) = make(x, frame);
	return TRUE;
}


/************************************************************************/
/*		Lookup up a name in the table declaration 	   	*/
/************************************************************************/

static int
lookup(term key, int a)
{
	int i, n = NVALS(a, names);
	term *table = VALUES(a, names);

	for (i = 0; i < n; i++, table++)
		if (key == *table)
			return i;

	fprintf(stderr, "Example %ld: attribute \"%s\"\n", example_number+1, NAME(A_NAME(a, names)));
	FAIL("Illegal value for an attribute or class");
}


/************************************************************************/
/*		   Free the space for all structures  			*/
/************************************************************************/

static void
free_data(data_struct *data)
{
	int a, v;
	int n_classes = data -> n_classes;
	int n_attributes = data -> n_attributes;
	class_struct *class = data -> class;
	attribute_struct *attribute = data -> attribute;
	term index = data -> index;

	if (attribute != NULL)
	{
		for (a = 0; a < n_attributes; a++)
		{
			if (attribute[a].value == NULL)
				continue;

			for (v = 0; v < attribute[a].n_values; v++)
			{
				free(attribute[a].value[v].class);
				free_term(attribute[a].value[v].instances);
			}
			free(attribute[a].value);
		}
		free(attribute);
	}

	if (class != NULL)
	{
		for (a = 0; a < n_classes; a++)
			free_term(class[a].instances);
		free(class);
	}

	if (index != NULL)
	{
		if (TYPE(ARG(1, index)) == SET)
			free_disj(ARG(1, index));
		dispose(index);
	}

	free(data);
}


/************************************************************************/
/*   Count the number of examples in data set and create index array 	*/
/************************************************************************/

static void
count_examples(void)
{
	term examples = PROC(ARG(0, names));
	term p;

	for (p = examples, n_examples = 0; p != NULL; p = NEXT(p))
		n_examples++;

	n_words = n_examples/BITS_IN_WORD + 1;
	index = new_h_fn(n_examples);
	ARG(0, index) = intern("index");
}


/************************************************************************/
/*	   Allocate and initialise space for all structures  		*/
/************************************************************************/

static void
new_attributes(void)
{
	int a, v, c;
	term attr;

	n_attributes = ARITY(names) - 1;

	attr = CLASS_NAMES;

	if (TYPE(attr = CLASS_NAMES) != FN)
		FAIL("Incorrect class specification");

	n_classes = ARITY(attr);
	class = calloc((size_t) n_classes, sizeof (class_struct));

	for (c = 0; c < n_classes; c++)
	{
		class[c].freq = 0;
		class[c].prob = 0.0;
		class[c].instances = NEW_SLICE;
	}

	attribute = calloc((size_t) n_attributes, sizeof (attribute_struct));

	for (a = 0; a < n_attributes; a++)
	{
		if (TYPE(attr = ARG(a+1, names)) != FN)
			FAIL("Incorrect attribute specification");

		attribute[a].n_values = ARITY(attr);
		attribute[a].value = calloc((size_t) ARITY(attr), sizeof (value_struct));

		for (v = 0; v < attribute[a].n_values; v++)
		{
			attribute[a].value[v].freq = 0;
			attribute[a].value[v].instances = NEW_SLICE;
			attribute[a].value[v].class = calloc((size_t) n_classes, sizeof (class_struct));

			for (c = 0; c < n_classes; c++)
			{
				attribute[a].value[v].class[c].freq = 0;
				attribute[a].value[v].class[c].prob = 0.0;
			}
		}
	}
}


/************************************************************************/
/*	 	Enter the training data for an induction task		*/
/************************************************************************/

static term
enter_data(int aq)
{
	int a, v, c;
	term examples = PROC(ARG(0, names));
	term p, *last = &ARG(1, index);

	data_set = NEW_SLICE;

	for (p = examples, example_number = 0; p != NULL; p = NEXT(p), example_number++)
	{
		term tp = HEAD(p);

		c = lookup(CLASS_VALUE(tp), n_attributes);
		class[c].freq++;

		if (aq)
		{
			*last = NEW_RULE;
			EXCEPT(*last) = NULL;
			CLASS(*last) = c;
			ARG(example_number+1, index) = *last;
		}
		else
			ARG(example_number+1, index) = tp;

		for (a = 0; a < n_attributes; a++)
		{
			int v = lookup(ARG(a+1, tp), a);

			if (aq)
				SELECTOR(a, *last) = 1L << v;

			attribute[a].value[v].freq++;
			attribute[a].value[v].class[c].freq++;
			set_add(attribute[a].value[v].instances, example_number);
		}
		set_add(class[c].instances, example_number);
		set_add(data_set, example_number);
		if (aq) last = &NEXT(*last);
	}

	for (c = 0; c < n_classes; c++)
	{
		double p = (class[c].freq + 1)/(float)(n_examples + n_classes);

		class[c].prob = p;

		for (a = 0; a < n_attributes; a++)
			for (v = 0; v < attribute[a].n_values; v++)
			{
				int f = attribute[a].value[v].class[c].freq;
				int t = attribute[a].value[v].freq;
	
				attribute[a].value[v].class[c].prob = ((f + 2*p)/(t + 2))/p;
			}
	}
}


/************************************************************************/
/*		Set up a new data set for induction			*/
/************************************************************************/

data_struct *
new_data_set(int aq, term relation_name)
{
	static data_struct *data = NULL;

	if (data_error)
	{
		if (data != NULL)
			free_data(data);
		data_error = FALSE;
		data = NULL;
	}
	else if (data != NULL)
	{
		if (relation_name == ARG(0, data -> names))
			return(data);
		free_data(data);
		data = NULL;
	}

	names = get_table(relation_name);
	data = malloc(sizeof(data_struct));
	count_examples();
	new_attributes();
	enter_data(aq);

	data -> n_words = n_words;
	data -> n_classes = n_classes;
	data -> n_attributes = n_attributes;
	data -> n_examples = n_examples;
	data -> names = names;
	data -> class = class;
	data -> attribute = attribute;
	data -> index = index;
	data -> data_set = data_set;

	return data;
}


/********************************************************************************/
/*			     Naive Bayes classifier				*/
/********************************************************************************/

static int
classify(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, FN, IN);
	double max_prob = 0.0;
	int c, max_prob_class = 0;

	new_data_set(FALSE, ARG(0, x));

	for (c = 0; c < n_classes; c++)
		if (class[c].freq)
		{
			int a;
			double prob = class[c].prob;
	
			for (a = 0; a < n_attributes; a++)
			{
				int v = lookup(unbind(ARG(a+1, x), frame), a);
	
				prob *= attribute[a].value[v].class[c].prob;
			}
	
			if (prob > max_prob)
			{
				max_prob = prob;
				max_prob_class = c;
			}
		}
	return unify(CLASS_VALUE(x), frame, CLASS_NAME(max_prob_class), NULL);
}


/********************************************************************************/
/*				Initialise module				*/
/********************************************************************************/

void
bayes_init(void)
{
	new_pred(put_table, "table");
	new_pred(classify, "bayes");
}
