/************************************************************************/
/*    		    Muggleton and Feng's Golem algorithm		*/
/************************************************************************/

#include <math.h>
#include <time.h>
#include "prolog.h"
#include "refine.h"
#include "lgg.h"

#define COVERED_FLAG		128

#define MARK_COVERED(x)		FLAGS(x) |= COVERED_FLAG
#define UNMARK_COVERED(x)	FLAGS(x) &= COVERED_FLAG
#define COVERED(x)		(FLAGS(x) & COVERED_FLAG)

#define DEFAULT_SAMPLE_SIZE	5

extern int trace_on;

static int sample_size = DEFAULT_SAMPLE_SIZE;
static double p_pos, p_neg;


/************************************************************************/
/* Deal m samples from a range of n. Returns an array of ints.		*/
/************************************************************************/

static int *
deal(int m, int n)
{
	double f;
	int *p, datum, i, size = m;

	if (m < 0 || m > n)
		fail("Internal error - bad call to deal");

	p = calloc(m, sizeof(int));

	datum = 0;
	for (i = 0; n != 0; n--)
	{
		f = m;
		f /= n;
		if (random()/(double)INT_MAX < f)
		{
			p[i++] = datum;
			m--;
		}
		datum++;
	}
	m = size;
	while (m > 0)
	{
		f = random()/(double)INT_MAX;
		n = m * f;
		m--;
		if (n != m)
		{
			int d1 = p[n];
			int d2 = p[m];
			p[n] = d2;
			p[m] = d1;
		}
	}
	return p;
}


/************************************************************************/
/*	Return the nth uncovered clause in a clause list		*/
/************************************************************************/

static term
nth_clause(term clause_list, int n)
{
	term p;

	for (p = clause_list; p != NULL; p = NEXT(p))
		if (! COVERED(p) && n-- == 0)
			return p;
}


/************************************************************************/
/* Randomly select clauses from a list clause_list whose length is	*/
/*  n_clauses								*/
/************************************************************************/

static term *
sample(term clause_list, int n_clauses)
{
	int i, *d = deal(sample_size, n_clauses);
	term *p = calloc(sample_size, sizeof (term));

	fprintf(output, "\nSample examples: ");
	for (i = 0; i < sample_size; i++)
		fprintf(output, "%4d", d[i]);
	fprintf(output, "\n");
	fflush(output);

	for (i = 0; i < sample_size; i++)
		p[i] = nth_clause(clause_list, d[i]);

	free(d);
	return p;
}


/************************************************************************/
/*	Count the number of negative examples in training set		*/
/************************************************************************/

static int
count_neg(term neglist)
{
	term p = neglist;
	int sum = 0;

	while (p != _nil)
	{
		term q = CAR(p);

		if (TYPE(q) != ATOM)
			fail("List of negative predicates must only contain atoms");
		if ((q = PROC(q)) == NULL)
			fail("Undefined negative relation");

		for (; q != NULL; q = NEXT(q))
			sum++;

		p = CDR(p);
		DEREF(p);
	}
	return sum;
}


/************************************************************************/
/*	Likelihood ratio for determining significance of a clause	*/
/************************************************************************/

static double
LikelihoodRatio(double pos_cover, double neg_cover)
{
	double n = pos_cover + neg_cover;
	double q_pos = pos_cover/n;
	double q_neg = 1 - q_pos;
	double e1 = (q_pos == 0 ? 0 : q_pos * log(q_pos/p_pos));
	double e2 = (q_neg == 0 ? 0 : q_neg * log(q_neg/p_neg));

	return 2 * n * (e1 + e2);
}


/************************************************************************/
/* Main routine of Golem. Find the LGG of random pairs of clauses,	*/
/* looking for LGG that gives the best cover.				*/
/************************************************************************/

static term
best_lgg(int n_pos, term pos, term neglist)
{
	double L, minimum_accuracy = 0;
	term *sample1, best_clause = NULL, rval = NULL;

	fprintf(output, "\n%d examples remain to be covered\n", n_pos);
	fflush(output);

	if (n_pos == 0)
		return NULL;

	if (n_pos < sample_size)
		sample_size = n_pos;

	sample1 = sample(pos, n_pos);

	/****************************************************************/
	/* Keep finding LGG's of pairs of clauses until you can't find	*/
	/* an LGG that has better expected accuracy.			*/
	/****************************************************************/

	repeat
	{
		int i;
		double best_accuracy = 0;
		term *sample2 = sample(pos, n_pos);

		for (i = 0; i < sample_size; i++)
		{
			double pos_cover, neg_cover, expected_accuracy;
			term C1 = (sample1 == NULL ? best_clause : saturate(sample1[i]));
			term C2 = saturate(sample2[i]);
			term lgg = lgg_clause(C1, C2);

			fprintf(output, "\nLGG of\n\t");
			if (sample1 != NULL)
				print(sample1[i]);
			else
				fprintf(output, "best LGG from last pass\n");
			fprintf(output, "and\n\t");
			print(sample2[i]);
			list_proc(lgg);
			fflush(output);

			pos_cover = count_cover(lgg, pos);
			neg_cover = count_neg_cover(lgg, neglist);
			expected_accuracy = (pos_cover + 1)/(pos_cover + neg_cover + 2);

			fprintf(output, "Covers %0.0f positive and %0.0f negative examples\n",
				pos_cover,
				neg_cover
			);
			fprintf(output, "Expected Accuracy = %f\n", expected_accuracy);
			fflush(output);

			if (expected_accuracy > best_accuracy)
			{
//				free_term(best_clause);
				best_clause = lgg;
				best_accuracy = expected_accuracy;
				L = LikelihoodRatio(pos_cover, neg_cover);

				fprintf(output, "*** Best clause so far\n");
				fflush(output);
			}
//			else
//				free_term(lgg);


		}

		free(sample1); sample1 = NULL;
		free(sample2); sample2 = NULL;

		if (best_accuracy <= minimum_accuracy)
		{
//			free_term(best_clause);
			break;
		}
		minimum_accuracy = best_accuracy;
		rval = best_clause;
	}

	if (L < 6.64)
	{
//		free_term(rval);
		fprintf(output, "\nNo significant clause found\n");
		fflush(output);
		return NULL;
	}
	else
	{
		fprintf(output, "\n--------------\nFound Clause:\n");
		list_proc(rval);
		fflush(output);
	}
	return rval;
}


/************************************************************************/
/* Unmark all positive examples.					*/
/* Call best_lgg repeatedly until can't expand cover any further.	*/
/* Once finished, mark all positive examples covered.			*/
/************************************************************************/

static term
golem(term pos, term neglist)
{
	term p, clause_list = NULL;
	term *last = &clause_list;
	double n_pos = 0, n_neg = 0;
	int remaining_pos;

	n_neg = count_neg(neglist);

	for (p = pos; p != NULL; p = NEXT(p))
	{
		n_pos++;
		UNMARK_COVERED(p);
	}

	p_pos = n_pos/(n_pos+n_neg);
	p_neg = 1 - p_pos;

	remaining_pos = n_pos;

	while ((p = best_lgg(remaining_pos, pos, neglist)) != NULL)
	{

		*last = p;
		last = &NEXT(p);

		remaining_pos -= mark_covered(p, pos);
	}

	fprintf(output, "\n--------------\nFINAL CONCEPT:\n");
	list_proc(clause_list);

	return clause_list;
}


/************************************************************************/
/*	Prolog hook for call to golem on a relation			*/
/*	Clauses are stored as usual in Prolog's database		*/
/************************************************************************/

static int
p_golem_reln(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, ATOM, IN);
	term neglist = check_arg(2, goal, frame, LIST, IN);
	term rval;

	if (PROC(pos) == NULL)
		fail("Undefined relation");

	if (ARITY(goal) == 3)
		sample_size = IVAL(check_arg(3, goal, frame, INT, IN));
	else
		sample_size = DEFAULT_SAMPLE_SIZE;

	rval = golem(PROC(pos), neglist);

	return TRUE;
}


/************************************************************************/
/*	Prolog hook for call to golem					*/
/*	Clauses are stored in a frame as the result of a refinement	*/
/************************************************************************/

static int
p_golem_frame(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, ATOM, IN);
	term neglist = check_arg(2, goal, frame, LIST, IN);
	term rval;

	if ((pos = getprop(pos, intern("rule"))) == NULL)
		fail("Undefined relation");

	if (ARITY(goal) == 3)
		sample_size = IVAL(check_arg(3, goal, frame, INT, IN));
	else
		sample_size = DEFAULT_SAMPLE_SIZE;

	rval = golem(pos, neglist);

	list_proc(rval);
	return TRUE;
}


/************************************************************************/
/*	Prolog hook for call to golem					*/
/*	1st argument is clauses of +ve examples				*/
/*	2nd argument is clauses of -ve examples				*/
/************************************************************************/

static int
p_golem_clause(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, CLAUSE, IN);
	term neg = check_arg(2, goal, frame, CLAUSE, IN);
	term rval;

	if (ARITY(goal) == 3)
		sample_size = IVAL(check_arg(3, goal, frame, INT, IN));
	else
		sample_size = DEFAULT_SAMPLE_SIZE;

	rval = golem(pos, neg);

	list_proc(rval);
	return TRUE;
}


/************************************************************************/
/*			   Initialise Module				*/
/************************************************************************/

void
golem_init(void)
{
	srandom((unsigned int)(time(NULL)));

	new_pred(p_golem_reln, "golem_reln");
	new_pred(p_golem_frame, "golem_frame");
	new_pred(p_golem_clause, "golem_clause");
}
