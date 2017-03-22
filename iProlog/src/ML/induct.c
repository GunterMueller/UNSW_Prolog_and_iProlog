/************************************************************************/
/*		     Implementation of Gaines' INDUCT-RDR		*/
/************************************************************************/

#ifdef PROFILE
#include <profile.h>
#endif

#include "attr_info.h"
#include "set.h"


/************************************************************************/
/*				Local Variables				*/
/************************************************************************/

static class_struct *class;
static attribute_struct *attribute;

static int n_words, n_attributes, n_classes;
static term names, index;

static double threshold = 0.5;

/************************************************************************/
/* Cumulative binomial probability:					*/
/* probability of selecting s and getting c or more correct at random.	*/
/* This function combines the computation of choose and power to avoid	*/
/* the overflows that result in a naive implementation.			*/
/* We use the fact that choose(n, s) = choose(n, n-s) to reduce work	*/
/************************************************************************/

static double
binomial_sum(double p, int s, int c, double min)
{
	int i, j;
	double r = 0, q = 1-p, m = p*q;

	for (i = c; i <= s; i++)
	{
		double k, n = s, x = 1;

		if (i < s-i)
		{
			for (k = i; k != 0; k--, n--)
				x *= (n/k)*m;

			for (j = s-i-i; j != 0; j--)
				x *= q;
		}
		else
		{
			for (k = s - i; k != 0; k--, n--)
				x *= (n/k)*m;

			for (j = i+i-s; j != 0; j--)
				x *= p;
		}
		if ((r += x) > min)
			return 1.0;
	}
	return r;
}

/************************************************************************/
/*		Find the majority class in a data set			*/
/************************************************************************/

static int
majority_class(term data)
{
	int c, majority_class = -1, majority = 0;

	for (c = 0; c < n_classes; c++)
	{
		int class_frequency = intersection_size(data, class[c].instances);

		if (class_frequency > majority)
		{
			majority_class = c;
			majority = class_frequency;
		}
	}

	return majority_class;
}

/************************************************************************/
/* Use cumulative binomial probabilities to find attr/vals least likely	*/
/* occur by chance							*/
/*									*/
/* Probability of selecting s and getting c or more correct at random	*/
/* e	number of cases in data set					*/
/* q	number of cases in target class					*/
/* s	number of cases with attribute under test			*/
/* c	number of cases of target class with attribute under test	*/
/* p	q/e (estimated probability of class in given data set)		*/
/************************************************************************/

static term
best_rule(int default_class, int exception_class, term data)
{
	term rule, covered, not_covered, exceptions, tmp;
	int e, q, a, v, best_attr = 0, best_val = 0;
	double p, best_r, min_prob = 1.0;

	if (exception_class == -1)
		return(NULL);

	rule = NEW_RULE;
	EXCEPT(rule) = NEXT(rule) = NULL;
	CLASS(rule) = exception_class;
	covered = copy_set(data);
	tmp = NEW_SLICE;

	e = set_cardinality(data);
	q = intersection_size(data, class[exception_class].instances);
	p = ((double) q) / ((double) e);

	repeat
	{
		best_r = min_prob;
		for (a = 0; a < n_attributes; a++)
		{
			if (attribute[a].used)
				continue;
	
			for (v = 0; v < attribute[a].n_values; v++)
			{
				int s, c;
				double r;

				set_intersection(covered, attribute[a].value[v].instances, tmp);

				s = set_cardinality(tmp);
				c = intersection_size(tmp, class[exception_class].instances);
				r = binomial_sum(p, s, c, best_r);
	
				if (r < best_r)
				{
					best_r = r;
					best_attr = a;
					best_val = v;
				}
/*				fprintf(stderr, "%3d%3d%8d%8d%8d%8d%15g\n", a, v, e, q, s, c, r);
*/			}
		}
		if (best_r == min_prob)
			break;

		min_prob = best_r;
		set_intersection(covered, attribute[best_attr].value[best_val].instances, covered);
		SELECTOR(best_attr, rule) = (1L << best_val);
		attribute[best_attr].used = TRUE;
	}
	for (a = 0; a < n_attributes; a++)
		attribute[a].used = FALSE;

	if (min_prob >= threshold || set_eq(data, covered))
	{
/* 		fprintf(output, "Cannot build rule for ");
		print_complex(rule);
		fprintf(output, "\n%d/%d instances belong to the class\n", q, e);
		fprintf(output, "---------------------------------------------\n");
		print_set_raw(data); putchar('\n');
		fflush(output);
*/
		free_term(rule);
		free_term(covered);
		free_term(tmp);
		return NULL;
	}
/*
	print_complex(rule);
	putc('\n', output);
	fflush(output);
*/
	exceptions = NEW_SLICE;
	set_diff(covered, class[exception_class].instances, exceptions);

	if (! empty_set(exceptions))
		EXCEPT(rule) = best_rule(exception_class, majority_class(exceptions), covered);

	not_covered = NEW_SLICE;
	set_diff(data, covered, not_covered);
	set_diff(not_covered, class[default_class].instances, tmp);

	if (! empty_set(tmp))
		NEXT(rule) = best_rule(default_class, majority_class(tmp), not_covered);

	free_term(covered);
	free_term(not_covered);
	free_term(exceptions);
	free_term(tmp);
	return rule;
}

/************************************************************************/
/*  Builds the default rule with no conditions and starts RDR building	*/
/************************************************************************/

static term
build_rdr(term data)
{
	term default_rule, exceptions;

	if (empty_set(data))
		return(NULL);

	default_rule = NEW_RULE;
	CLASS(default_rule) = majority_class(data);

	exceptions = NEW_SLICE;
	set_diff(data, class[CLASS(default_rule)].instances, exceptions);

	EXCEPT(default_rule) = best_rule(CLASS(default_rule), majority_class(exceptions), data);
	NEXT(default_rule) = NULL;

	free_term(exceptions);
	return default_rule;
}

/************************************************************************/
/*			Call INDUCT from Prolog				*/
/************************************************************************/

static term
induct(term goal, term *frame)
{
	double start_time, finish_time;
	term rule, rval = _true;
	data_struct *data;
	term relation_name = check_arg(1, goal, frame, ATOM, IN);

	if (ARITY(goal) == 2)
		threshold = RVAL(check_arg(2, goal, frame, REAL, IN));

	data = new_data_set(TRUE, relation_name);
	n_words = data -> n_words;
	n_classes = data -> n_classes;
	n_attributes = data -> n_attributes;
	class = data -> class;
	attribute = data -> attribute;
	names = data -> names;
	index = data -> index;

	fprintf(stderr, "No. of examples = %d\n", data -> n_examples);

#ifdef PROFILE
	InitProfile(200, 200);
#endif
	start_time = get_time();
	rule = build_rdr(data -> data_set);
	finish_time = get_time();
#ifdef PROFILE
	DumpProfile();
#endif
/*
	print_cover(rule);
	fprintf(output, "%lg seconds\n", finish_time - start_time);
*/
	rval = build_plist("rdr",
			"creator",	make(goal, frame),
			"date",		intern(date_time()),
			"n_examples",	new_h_int(data -> n_examples),
			"time",		new_h_real(finish_time - start_time),
			NULL
	);
	add_to_world(rval, cover_to_clause(rule));
	free_term(rule);

	return rval;
}

/************************************************************************/
/*			Module initialisation				*/
/************************************************************************/

void
induct_init(void)
{
	new_subr(induct, "induct");
}
