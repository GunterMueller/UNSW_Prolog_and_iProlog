/************************************************************************/
/*		     Array processing support routines			*/
/************************************************************************/


#include <math.h>
#include "prolog.h"


/************************************************************************/
/* Create dynamic arrays that can have any size and can be passed to fn	*/
/************************************************************************/

int *
new_ivector(int n)
{
	int i, *rval;

	if ((rval = (int *) calloc((size_t) n, sizeof(int))) == NULL)
	{
		fprintf(stderr, "Couldn't allocate space for integer vector\n");
		exit(1);
	}

	for (i = 0; i < n; i++)
		rval[i] = 0;

	return(rval);
}


int **
new_2D_iarray(int m, int n)
{
	int i;
	int **rval;

	if ((rval = (int **) calloc((size_t) m, sizeof(int *))) == NULL)
	{
		fprintf(stderr, "Couldn't allocate space for rows of 2D array\n");
		exit(1);
	}

	for (i = 0; i < m; i++)
		rval[i] = new_ivector(n);

	return(rval);
}


double *
new_dvector(int n)
{
	int i;
	double *rval;

	if ((rval = (double *) calloc((size_t) n, sizeof(double))) == NULL)
	{
		fprintf(stderr, "Couldn't allocate space for double vector\n");
		exit(1);
	}

	for (i = 0; i < n; i++)
		rval[i] = 0.0;

	return(rval);
}


double **
new_2D_array(int m, int n)
{
	int i;
	double **rval;

	if ((rval = (double **) calloc((size_t) m, sizeof(double *))) == NULL)
	{
		fprintf(stderr, "Couldn't allocate space for rows of 2D array\n");
		exit(1);
	}

	for (i = 0; i < m; i++)
		rval[i] = new_dvector(n);

	return(rval);
}


double ***
new_3D_array(int m, int n, int r)
{
	int i;
	double ***rval;

	if ((rval = (double ***) calloc((size_t) m, sizeof(double **))) == NULL)
	{
		fprintf(stderr, "Couldn't allocate space for plane of 3D array\n");
		exit(1);
	}

	for (i = 0; i < m; i++)
		rval[i] = new_2D_array(n, r);

	return(rval);
}


/************************************************************************/
/*		Routines to free a multi-dimensional array		*/
/************************************************************************/

void
free_2D_iarray(int **M, int m, int n)
{
	int i;

	for (i = 0; i < m; i++)
		free(M[i]);
	free(M);
}


void
free_2D_array(double **M, int m, int n)
{
	int i;

	for (i = 0; i < m; i++)
		free(M[i]);
	free(M);
}


void
free_3D_array(double ***M, int m, int n, int r)
{
	int i;

	for (i = 0; i < m; i++)
		free_2D_array(M[i], n, r);
	free(M);
}

	
/************************************************************************/
/* Matrix inversion routine						*/
/*	A must be SYMMETRIC						*/
/*	T is upper triangular matrix					*/
/*	Choleski decomposition   A = T'.T				*/
/*	S = INV(T)							*/
/*	 AINV =  S.S' =  INV(A)						*/
/************************************************************************/

#define EPSILON		1.0e-60
#define BIGNUM		1.0e+99

void
matinv(double **a, double **ainv, int n)
{
	int i, j, k, imin;
	double detlog, avmin = BIGNUM;

	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			ainv[i][j] = 0.0;

	if (a[0][0] < EPSILON)
	{
		fprintf(stderr, "Tried to find log of 0\n");
		exit(1);
	}
	detlog = log(a[0][0]);
	ainv[0][0] = sqrt(a[0][0]);

	for (j = 1; j < n; j++)
		ainv[0][j] = a[0][j]/ainv[0][0];

	for (i = 1; i < n; i++)
	{
		ainv[i][i] = a[i][i];
		for (k = 0; k < i; k++)
			ainv[i][i] -= ainv[k][i]*ainv[k][i];

		if (ainv[i][i] < EPSILON)
		{
			fprintf(stderr, "Linearly related\n");
			exit(1);
		}
		if (ainv[i][i] < avmin)
		{
			avmin = ainv[i][i];
			imin = i;
		}
		detlog += log(ainv[i][i]);
		ainv[i][i] = sqrt(ainv[i][i]);

		for (j = i+1; j < n; j++)
		{
			ainv[i][j] = a[i][j];
			for (k = 0; k < i; k++)
				ainv[i][j] -= ainv[k][i]*ainv[k][j];
			ainv[i][j] /= ainv[i][i];
		}
	}


	/****************************************************************/
	/*  AINV  is now upper diagonal factor T of A = T'.T	       	*/
	/*  detlog is now the logarithm of determinant of A	       	*/
	/*  now find inverse S = INV(T)				       	*/
	/****************************************************************/

	for (i = 0; i < n; i++)
	{
		ainv[i][i] = 1.0/ainv[i][i];

		for (j = i+1; j < n; j++)
		{
			double temp = 0.0;

			for (k = 0; k < j; k++)
				temp -= ainv[i][k]*ainv[k][j];

			ainv[i][j] = temp/ainv[j][j];
		}
	}

	/****************************************************************/
	/*		  AINV is now the inverse of T			*/
	/****************************************************************/

	for (i = 0; i < n; i++)
		for (j = i; j < n; j++)
		{
			double temp = 0.0;

			for (k = j; k < n; k++)
				temp += ainv[i][k]*ainv[j][k];

			ainv[i][j] = ainv[j][i] = temp;
		}
}
    

/************************************************************************/
/*    Matrix multiplication - args must be double ** and dimensions	*/
/************************************************************************/

void
matmul(double **a, double **b, double **prod, int n1, int n2, int n3)
{
	int k1, k2, k3;

	for (k1 = 0; k1 < n1; k1++)
		for (k3 = 0; k3 < n3; k3++)
		{
			double sum = 0.0;

			for (k2 = 0; k2 < n2; k2++)
				sum += a[k1][k2]*b[k2][k3];

			prod[k1][k3] = sum;
		}
}
      

/************************************************************************/
/* Find the inner product of two matrices - dimensions must be supplied	*/
/************************************************************************/

void
inner(double **a, double **b, double *prod, int n1, int n2)
{
	int k1, k2;
      
	for (k2 = 0; k2 < n2; k2++)
	{
		double sum = 0.0;

		for (k1 = 0; k1 < n1; k1++)
			sum += a[k1][k2] * b[k1][k2];

		prod[k2] = sum;
	}
		
}


/************************************************************************/
/*				Matrix output routines			*/
/************************************************************************/

void
print_ivector(char *msg, int *vec, int n)
{
	int i;

	printf("%s: ", msg);
	for (i = 0; i < n; i++)
		printf("%6d", vec[i]);
	printf("\n");
}

void
print_dvector(char *msg, double *vec, int n)
{
	int i;

	printf("%s: ", msg);
	for (i = 0; i < n; i++)
		printf("%8.3lf", vec[i]);
	printf("\n");
}


void
print_matrix(char *msg, double **M, int m, int n)
{
	int i, j;

	printf("\n%s:\n", msg);
	for (i = 0; i < m; i++)
	{
		for (j = 0; j < n; j++)
			printf("%8.3f", M[i][j]);
		printf("\n");
	}
}


void
print_coeffs(term names, double **coeff)
{
	int i, j;
	term class = ARG(ARITY(names), names);

	printf("Coefficients:\n");
	printf("------------\n");

	for (i = 0; i < ARITY(class)-1; i++)
	{
		printf("%1.3lf\n", coeff[0][i]);

		for (j = 1; j < ARITY(names); j++)
			if (coeff[j][i] < 0)
				printf("    - %1.3lf * %s\n", - coeff[j][i], NAME(ARG(j, names)));
			else
				printf("    + %1.3lf * %s\n", coeff[j][i], NAME(ARG(j, names)));
	}
	printf("\n");
}
