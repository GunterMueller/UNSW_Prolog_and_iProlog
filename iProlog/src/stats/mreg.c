/************************************************************************/
/*			Multiple Linear Regression			*/
/* 		WARNING!!! This program is incomplete			*/
/************************************************************************/

#include <ctype.h>
#include "attr_info.h"
#include "array.h"

static int n_vars, n_observations;


/************************************************************************/
/*			Convert a term into a double			*/
/************************************************************************/

static double
to_double(term p)
{
	switch (TYPE(p))
	{
	case REAL:	return RVAL(p);
	case INT:	return (double) IVAL(p);
	default:	print(p);
			fail("mreg's inputs must be numeric");
	}
}


/************************************************************************/
/* on return, crospr contains the covariance matrix for each group	*/
/************************************************************************/


static double **
readin(term data)
{
	term p;
	int i, j;
	double *x;
	double **A;
	double *mean;			/* vector of means		*/
	double *stddev;			/* vector of std deviations	*/

	if ((data = PROC(data)) == NULL)
		fail("No data for regression");

	n_observations = 0;

	for (p = data; p != NULL; p = NEXT(p))
	{
		term t = HEAD(p);

		n_observations++;

		if (TYPE(t) != FN)
		{
			char buf[256];
			sprintf(buf, "Example %d in regression data is not a tuple", n_observations);
			fail(buf);
		}

		if (ARITY(t) != n_vars)
		{
			char buf[256];
			sprintf(buf, "Example %d in regression data does not have %d variables", n_observations, n_vars);
			fail(buf);
		}
	}

	if ( n_vars >= n_observations)
		fail("More variables than observations");

	x = new_dvector(n_vars+1);
	mean = new_dvector(n_vars+1);
	stddev = new_dvector(n_vars+1);
	A = new_2D_array(n_vars+1, n_vars+1);

	for (i = 0; i <= n_vars; i++)
	{
		mean[i] = stddev[i] = 0.0;

		for (j = 0; j <= n_vars; j++)
			A[i][j] = 0.0;
	}

	for (i = 0, p = data; p != NULL; i++, p = NEXT(p))
	{
		term t = HEAD(p);
		
		x[0] = 1.0;
		for (j = 1; j <= n_vars; j++)
			x[j] = to_double(ARG(j, t));

		for (j = 0; j <= n_vars; j++)
		{
			double u, v;
			int k;

			u = x[j] - mean[j];
			mean[j] = (n_observations * mean[j] + x[j]) / n_observations;
			v = x[j] - mean[j];
			stddev[j] += u * v;

			for (k = 0; k <= j; k++)
			{
				A[k][j] += x[j] * x[k];
				A[j][k] = A[k][j];
			}
		}
	}

	printf("Means:\n");
	for (j = 1; j <= n_vars; j++)
		printf("%12.4g", mean[j]);
	printf("\n");

	for (j = 1; j <= n_vars; j++)
		stddev[j] = sqrt(stddev[j]/n_observations);

	printf("\nStandard deviations:\n");
	for (j = 1; j <= n_vars; j++)
		printf("%12.4g", stddev[j]);
	printf("\n");

	return A;
}


static void
sweep(double **A, int n_vars, int k)
{
	int i, j;
	double d = A[k][k];

	for (j = 0; j <= n_vars; j++)
		A[k][j] /= d;

	for (i = 0; i <= n_vars; i++)
		if (i != k)
		{
			double b = A[i][k];

			for (j = 0; j <= n_vars; j++)
				A[i][j] -= b * A[k][j];

			A[i][k] = -b/d;
		}

	A[k][k] = 1.0/d;
}


static void
finish(double **A, int n, int n_vars, double *SSS, double SST, int N)
{
	int i, DF, DFE = N - n;
	double SSE, MSE, S, SSR, RSQ, F;

/*	printf("---------------- STEP %3d ------------------\n\n", n-1);
*/	printf("\n  I          B(I)         S.E.     T(%d DF)       SEQ.SS\n", DFE);

	SSE = A[n_vars][n_vars];
	MSE = SSE/DFE;

	S = sqrt(MSE);

	for (i = 0; i < n; i++)
	{
		double beta = A[i][n_vars];
		double std_err = S * sqrt(A[i][i]);
		double T = beta/std_err;

		printf("%3d  %12.4g %12.4g %12.4g %12.4g\n", i, beta, std_err, T, SSS[i]);
	}

	SSR = SST - SSE;
	RSQ = 1.0 - SSE/SST;
	DF = n - 1;
	F = SSR/DF/MSE;

	printf("R-SQ = %6.3g    F(%2d, %2d, DF) = %12.4g\n", RSQ, DF, DFE, F);
	printf("NOBS = %6d     STD DEV = %8.4g\n", n_observations, S);

	if (n == n_vars)
		return;

	printf("VARS NOT IN MODEL - F TO ENTER (1, %2d) DF):\n", DFE - 1);
	for (i = n; i < n_vars; i++)
	{
		double AID = fabs(A[i][n_vars]);

		F = AID * AID * (DFE-1) / (A[i][i] * SSE - AID * AID);

		printf("%5d  %8.4g\n", i, F);
	}
}


static term _mul, _is;

static term
make_clause(term relation, double **A)
{
	extern term new_clause(int);
	int i, offset = 0;
	term cl, head, eqn;

	cl = new_clause(1);
	HEAD(cl) = head = new_h_fn(ARITY(relation));
	ARG(0, head) = ARG(0, relation);

	for (i = 1; i <= n_vars; i++)
	{
		extern term new_var(int, int, term);
		char *name = NAME(ARG(i, relation));
		char buf[128];

		sprintf(buf, "%c%s", toupper(name[0]), name+1);
		ARG(i, head) = new_var(FREE, offset++, intern(buf));
	}

	eqn = new_h_real(A[0][n_vars]);

	for (i = 1; i < n_vars; i++)
	{
		term op;
		double d = A[i][n_vars];

		if (d < 0)
		{
			d = -d;
			op = _minus;
		}
		else
			op = _plus;

		eqn = h_fn2(op, eqn, h_fn2(_mul, new_h_real(d), ARG(i, head)));
	}

	eqn = h_fn2(_is, ARG(n_vars, head), eqn);
	GOAL(1, cl) = eqn;

	NVARS(cl) = offset;
	return(cl);
}


static int
mreg(term goal, term *frame)
{
	term names = get_table(check_arg(1, goal, frame, ATOM, IN));
	term cl;
	double **A, SST, *SSS;
	int i;

	n_vars = ARITY(names);

	A = readin(ARG(0, names));
	fflush(output);

	sweep(A, n_vars, 0);

	SST = A[n_vars][n_vars];
	SSS = new_dvector(n_vars+1);

	for (i = 1; i < n_vars; i++)
	{
		double SSBEF, SSAFT;

		SSBEF = A[n_vars][n_vars];
		sweep(A, n_vars, i);
		SSAFT = A[n_vars][n_vars];
		SSS[i] = SSBEF - SSAFT;
	}
	finish(A, i, n_vars, SSS, SST, n_observations);
	cl = make_clause(names, A);
	print(GOAL(1, cl));

	return TRUE;
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
mreg_init(void)
{
	_is		= intern("is");
	_mul		= intern("*");
	new_pred(mreg, "mreg");
}
