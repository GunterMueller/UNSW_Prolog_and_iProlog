/************************************************************************/
/*	Linear discriminant program after R.J. Henery (Strathclyde)	*/
/************************************************************************/


#include "attr_info.h"
#include "array.h"

#define BIGNUM		1.0e40

static double	**mean, *alpha, **beta, **cost, **coefficient = NULL;

static int	*freq, n_classes, n_attributes, n_examples;
static int	verbose = 0;


/************************************************************************/
/*		Convert a term into an array of double			*/
/************************************************************************/

static double
to_double(term p)
{
	switch (TYPE(p))
	{
	case REAL:	return(RVAL(p));
	case INT:	return((double) IVAL(p));
	default:	print(p);
			fail("discrim's inputs must be numeric");
	}
}


static void
convert_data(term p, double *x, int *class)
{
	int i;

	for (i = 0; i < n_attributes; i++)
		x[i] = to_double(ARG(i+1, p));

	*class = to_double(ARG(n_attributes+1, p));
	*class = (*class == 0) ? n_classes : (*class) - 1;
}


/************************************************************************/
/* on return, crospr contains the covariance matrix for each group	*/
/************************************************************************/


static void
readin(term names, double **pooled)
{
	int nc;
	term p;
	double *x = new_dvector(n_attributes);
	double ***crospr = new_3D_array(n_attributes, n_attributes, n_classes);

	n_examples = 0;

	for (p = PROC(ARG(0, names)); p != NULL; p = NEXT(p))
	{
		int j, k, class;

		convert_data(HEAD(p), x, &class);

		freq[class]++;

		for (j = 0; j < n_attributes; j++)
		{
			mean[j][class] += x[j];
			for (k = j; k < n_attributes; k++)
				crospr[j][k][class] += x[j]*x[k];
		}

		n_examples++;
	}

	free(x);

        for (nc = 0; nc < n_classes; nc++)
	{
		int j, k;
		double f = (double) freq[nc];

		if (f <= 1)
			continue;

		for (j = 0; j < n_attributes; j++)
			mean[j][nc] /= f;

		for (j = 0; j < n_attributes; j++)
			for (k = j; k < n_attributes; k++)
			{
				double temp = crospr[j][k][nc] - f * mean[j][nc] * mean[k][nc];

				pooled[j][k] += temp/(n_examples-n_classes);
				crospr[j][k][nc] = temp/(f-1);

				if (k > j)
				{
					crospr[k][j][nc] = crospr[j][k][nc];
					pooled[k][j] = pooled[j][k];
				}
			}
	}
	free_3D_array(crospr, n_attributes, n_attributes, n_classes);
}
    

/************************************************************************/
/*	    Compute coefficients for the linear discriminant		*/
/************************************************************************/

static void
compute_coefficients(double **covinv)
{
	int j, m;

	matmul(covinv,mean,beta,n_attributes,n_attributes,n_classes);
	inner(mean,beta,alpha,n_attributes,n_classes);

	for (j = 0; j < n_classes; j++)
		alpha[j] = log((double) freq[j]) - 0.5 * alpha[j];

	/* now set coefficients for the last class to zero */

	for (j = 0; j < n_classes; j++)
		for (m = 0; m <= n_attributes; m++)
			coefficient[m][j] -= coefficient[m][n_classes-1];
}


/************************************************************************/
/*	  Compute the mean distribution over all the classes		*/
/************************************************************************/

static void
meandis(double **beta, double **mean, double **dmeans)
{
	int k;

	for (k = 0; k < n_classes-1; k++)
	{
		int m;

		for (m = 0; m < n_classes; m++)
		{
			int j;

			dmeans[k][m] = beta[0][k];
			for (j = 0; j < n_attributes; j++)
				 dmeans[k][m] += beta[j+1][k]*mean[j][m];
		}
		for (m = 0; m < n_classes; m++)
			dmeans[k][m] -= dmeans[k][n_classes-1];
	}
}      


/************************************************************************/
/*			   Covariance matrix?				*/
/************************************************************************/

static void
betcov(double **betinv, double ckk, double **dmeans, double **betax)
{
	double n = n_examples;
	double q = freq[n_classes-1];
	int l, m;

	for (l = 0; l < n_classes-1; l++)
		for (m = 0; m < n_classes-1; m++)
		{
			double temp = dmeans[l][m]/n + 1.0/q;

			if (l == m)
 				temp += 1.0/freq[m]; 
 
			 betinv[l][m] = betax[l][0]*betax[m][0]/n + ckk*temp;
		}
}
     

/************************************************************************/
/* Standard errors of individual coefficients and overall chi-square	*/
/* statistic for							*/
/*	H0:  attribute k does not improve discrimination		*/
/* Formulae based on Kendall, Stuart and Ord section 44.18 (1983)	*/
/************************************************************************/

static void
zbetax(double **beta, double **inverk)
{
	double **betinv	= new_2D_array(n_classes-1, n_classes-1);
	double **betvar	= new_2D_array(n_classes-1, n_classes-1);
	double **dmeans	= new_2D_array(n_classes, n_classes);
	double **betax	= new_2D_array(n_classes-1, 1);
	double **prod	= new_2D_array(n_classes-1, 1);
	double chisq;
	int k, kl, m;

	/* mean discriminant(1..q) for every class (1..q)			*/
	/* (symmetric)								*/
	/* (hope that the largest term in any row/column is the diagonal term)	*/

	if (verbose)
	{
		meandis(beta,mean,dmeans);
		print_matrix("Mean discriminants", dmeans, n_classes, n_classes);
	}

	/* treat all beta's for a given x together				*/   
	/* attribute Xk associated with beta(k+1,...)				*/

	if (verbose)
	{
		printf("\n  k ");
		for (kl = 0; kl < n_classes-1; kl++)
			printf("  beta[%d]  ", kl);
		printf("   chisq    %d degrees of freedom\n", n_classes -1);
	}

	for (k = 0; k < n_attributes; k++)
	{
		chisq = 0.0;

		/* find chisq statistic for this attribute over all classes	*/

		for (kl = 0; kl < n_classes-1; kl++)
			betax[kl][0] = beta[k+1][kl];

		/* betinv is variance-covariance matrix for beta's		*/
		/* next call is to set up betinv				*/

		betcov(betinv,inverk[k][k],dmeans,betax);

		matinv(betinv,betvar,n_classes-1);
		matmul(betvar,betax,prod,n_classes-1,n_classes-1,1);
		inner(betax,prod,&chisq,n_classes-1,1);

		for (m = 0; m < n_classes-1; m++)
			 betax[m][0] /= sqrt(betinv[m][m]);

		if (verbose)
		{
			printf("%3d", k);
			for (kl = 0; kl < n_classes-1; kl++)
				printf("%10.3lf", betax[kl][0]);
			printf("%10.2lf\n", chisq);
		}
	}

	free_2D_array(betinv, n_classes-1, n_classes-1);
	free_2D_array(betvar, n_classes-1, n_classes-1);
	free_2D_array(dmeans, n_classes, n_classes);
	free_2D_array(betax, n_classes-1, 1);
	free_2D_array(prod, n_classes-1, 1);
}
     

/************************************************************************/
/*			Find Linear Discriminant			*/
/************************************************************************/


static int
discrim(term goal, term *frame)
{
	double **pooled, **inverk;
	term names = get_table(check_arg(1, goal, frame, ATOM, IN));

	verbose = (ARITY(goal) > 1);

	n_attributes = ARITY(names) - 1;
	n_classes = ARITY(ARG(n_attributes+1, names));

	if (coefficient != NULL)
		free_2D_array(coefficient, n_attributes+1, n_classes);

	freq = new_ivector(n_classes);
	mean = new_2D_array(n_attributes, n_classes);
	coefficient = new_2D_array(n_attributes+1, n_classes);
	pooled = new_2D_array(n_attributes, n_attributes);
	inverk = new_2D_array(n_attributes, n_attributes);
	alpha = coefficient[0],
	beta = coefficient + 1;

	readin(names, pooled);
	matinv(pooled,inverk,n_attributes);
	compute_coefficients(inverk);

	if (verbose)
	{
		printf("\n%d examples read\n", n_examples);
		print_ivector("Class frequencies", freq, n_classes);
		print_matrix("Means", mean, n_attributes, n_classes);
		print_matrix("Pooled covariance matrix", pooled, n_attributes, n_attributes);
		print_matrix("Inverse of pooled covariance matrix", inverk, n_attributes, n_attributes);
 	}
 	print_coeffs(names, coefficient);
 
 	zbetax(coefficient,inverk);

	free(freq);
	free_2D_array(mean, n_attributes, n_classes);
	free_2D_array(pooled, n_attributes, n_attributes);
	free_2D_array(inverk, n_attributes, n_attributes);

	return(TRUE);
}


/************************************************************************/
/*		   Test accuracy of linear discriminant			*/
/************************************************************************/

static void
discpr(double *x, double *prob)
{
	int k, n;
	double prmax = -BIGNUM, sumpr = 0.0;

	for (k = 0; k < n_classes-1; k++)
	{
		prob[k] = 0.0;

		for (n = 0; n <= n_attributes; n++)
			prob[k] += x[n] * coefficient[n][k];

		if (prob[k] > prmax)
			prmax = prob[k];
	}

	if (prmax < (prob[k] = 0.0))
		prmax = 0.0;

	/* ensure that the maximum is zero and min = -25
	 * (so that, when exponentiating, max = 1, min = .00000001)
	 */
 
	for (k = 0; k < n_classes; k++)
	{
		prob[k] -= prmax;
		if (prob[k] < -25.0)
			prob[k] = -25.0;

		prob[k] = exp(prob[k]);
		sumpr += prob[k];	 
 	}	

	for (k = 0; k < n_classes; k++)
		prob[k] /= sumpr;

	/* probabilities now sum to one
	 * conditional probabilities of class given x
	 */
}


/************************************************************************/
/*	    Class with the least cost is the decision class		*/		  
/************************************************************************/

static int
classify(double *x)
{
	int j, k, decision_class = 0;
	double min_cost = BIGNUM;
	double *prob = new_dvector(n_classes);

	discpr(x, prob);

	for (k = 0; k < n_classes; k++)
	{
		double excost = 0.0;

		for (j = 0; j < n_classes; j++)
			excost += prob[j] * cost[k][j];

		if (excost < min_cost)
		{
			min_cost = excost;
			decision_class = k;
		}
	}

	free(prob);
	return(decision_class);
}


static double **
init_cost(void)
{
	int i, j;
	double **cost;
	term p, cm = PROC(intern("cost"));

	cost = new_2D_array(n_classes, n_classes);

	if (cm != NULL)
		for (i = 0, cm = ARG(2, HEAD(cm)); i < n_classes; i++, cm = CDR(cm))
			for (j = 0, p = CAR(cm); j < n_classes; j++, p = CDR(p))
				cost[i][j] = to_double(CAR(p));
	else
		for (i = 0; i < n_classes; i++)
			for (j = 0; j < n_classes; j++)
				cost[i][j] = (i != j);

	return(cost);
}


static int
lintest(term goal, term *frame)
{
	int i;
	double total_cost = 0.0;
	double *x = new_dvector(n_attributes+1);
	int **conf = new_2D_iarray(n_classes, n_classes);
	term p, relation_name = check_arg(1, goal, frame, ATOM, IN);

	cost = init_cost();

	n_examples = 0;
	x[0] = 1.0;

	for (p = PROC(relation_name); p != NULL; p = NEXT(p))
	{
		int decision_class, true_class;

		convert_data(HEAD(p), x+1, &true_class);

		n_examples++;

		decision_class = classify(x);
		conf[decision_class][true_class]++;
		total_cost += cost[decision_class][true_class];
	}

	free(x);

	printf("Number of data = %d\n", n_examples);
	printf("Number of classes = %d\n", n_classes);
	printf("Number of attributes = %d\n", n_attributes);

	printf("\nConfusion matrix:\n");
	for (i = 0; i < n_classes; i++)
	{
		int j;

		for (j = 0; j < n_classes; j++)
			printf("%7d", conf[i][j]);
		printf("\n");
	}

	printf("\nTotal cost (number of errors) = %lg\n", total_cost);
	printf("Average cost (error rate)     = %lg\n", total_cost / n_examples);

 	free_2D_iarray(conf, n_classes, n_classes);
	free_2D_array(coefficient, n_attributes+1, n_classes-1);
	free_2D_array(cost, n_classes, n_classes);

	return(TRUE);
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
discrim_init(void)
{
	new_pred(discrim, "discrim");
	new_pred(lintest, "lintest");
}
