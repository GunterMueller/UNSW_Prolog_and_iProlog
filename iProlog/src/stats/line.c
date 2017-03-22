/************************************************************************/
/* 			   Find the line of best fit			*/
/* 			   Written by Sarel Aiber			*/
/************************************************************************/

#include "prolog.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>

#define PI M_PI


/************************************************************************/
/* The following functions are to support the manoeuver experiments	*/
/************************************************************************/

static double
sum(double v[], int n)
{
	int i;
	double sum = 0;
	
	for (i = 0; i < n; i++)
		sum += v[i];
	
	return sum;
}

static double
sum_squares(double x[], double y[], int n)
{
	int i;
	double sum = 0;
	
	for (i = 0; i < n; i++)
		sum += x[i] * y[i];
	
	return sum;
}

static int
list_length(term list)
{
	term temp;
	int count = 0;
	
	for (temp = list; temp != _nil; temp = CDR(temp))
		count++;

	return count;
}


static int
line_best_fit(double x[], double y[], double z[], int n, double *b0, double *b1, double *b2)
{
	double matrix[3][4];
	int i, j, k;
	double value;

	matrix[0][0] = n;
	matrix[0][1] = sum(x, n);
	matrix[0][2] = sum(y, n);
	matrix[0][3] = sum(z, n);
	
	matrix[1][0] = sum(x, n);
	matrix[1][1] = sum_squares(x, x, n);
	matrix[1][2] = sum_squares(x, y, n);
	matrix[1][3] = sum_squares(x, z, n);

	matrix[2][0] = sum(y, n);
	matrix[2][1] = sum_squares(y, x, n);
	matrix[2][2] = sum_squares(y, y, n);
	matrix[2][3] = sum_squares(y, z, n);

	for (i = 0; i <= 1; i++)
	{
		for (j = i+1; j <= 2; j++)
		{
			value = matrix[j][i] / matrix[i][i];
         
			for (k = i; k <= 3; k++)
				matrix[j][k] -= matrix[i][k]*value;
		}
	}
	   
	for (i = 2; i >= 1; i--)
	{
		for (j = i-1; j >= 0; j--)
		{
			value = matrix[j][i] / matrix[i][i];
         
			for (k = i; k <= 3; k++)
				matrix[j][k] -= matrix[i][k]*value;
		}
	}
   
	for (i = 0; i <= 2; i++)
		matrix[i][3] /= matrix[i][i];
   
	*b0 = matrix[0][3];
	*b1 = matrix[1][3];
	*b2 = matrix[2][3];
}


static int
p_line_best_fit(term goal, term *frame)
{
	term x_list = check_arg(1, goal, frame, LIST, IN);
	term y_list = check_arg(2, goal, frame, LIST, IN);
	term z_list = check_arg(3, goal, frame, LIST, LIST);
	term v_A = check_arg(4, goal, frame, REAL, OUT);
	term v_B = check_arg(5, goal, frame, REAL, OUT);
	term v_C = check_arg(6, goal, frame, REAL, OUT);
	term v_D = check_arg(7, goal, frame, REAL, OUT);
	double *x, *y, *z;
	term temp1, temp2, temp3;
	int n = list_length(x_list);
	int i;
	double b0, b1, b2;
	
	if (n < 2)
		fail("not enough data points");
	
	if (list_length(y_list) != n || list_length(z_list) != n)
		fail("all data lists must be of the same length");
	
	x = malloc(n * sizeof (double));
	y = malloc(n * sizeof (double));
	z = malloc(n * sizeof (double));
	
	for (temp1 = x_list, temp2 = y_list, temp3 = z_list, i = 0; temp1 != _nil;
	     temp1 = CDR(temp1), temp2 = CDR(temp2), temp3 = CDR(temp3), i++)
	{
		x[i] = (TYPE(CAR(temp1)) == REAL ? RVAL(CAR(temp1)) : IVAL(CAR(temp1)));
		y[i] = (TYPE(CAR(temp2)) == REAL ? RVAL(CAR(temp2)) : IVAL(CAR(temp2)));
		z[i] = (TYPE(CAR(temp3)) == REAL ? RVAL(CAR(temp3)) : IVAL(CAR(temp3)));
	}	
	
	line_best_fit(x, y, z, n, &b0, &b1, &b2);

	free(x);
	free(y);
	free(z);
	
	return	unify(v_A, frame, new_real(b1), frame)
	&&	unify(v_B, frame, new_real(b2), frame)
	&&	unify(v_C, frame, new_real(-1.0), frame)
	&&	unify(v_D, frame, new_real(b0), frame);
}


static double
euclid_distance(double x0, double y0, double z0, double A, double B, double C,
                double D)
{
	double dx, dy, dz;
	
	dx = (-B*y0-C*z0-D)/A - x0;
	dy = (-A*x0-C*z0-D)/B - y0;
	dz = (-A*x0-B*y0-D)/C - z0;

	return sqrt(dx*dx + dy*dy + dz*dz);	
}


static void
avg_distance(double x[], double y[], double z[], int n, double a, double b,
             double c, double d, double *sd)
{
	int i;
	double sum = 0.0;
	
	for (i = 0; i < n; i++)
		sum += euclid_distance(x[i], y[i], z[i], a, b, c, d);
	
	*sd = sum/n;
}


/*
double
stddev(double x[], double y[], double z[], int n, double *sd)
{
	double xm, ym, zm;
	int i;
	double sum = 0.0;
	
	mean(x, y, z, n, &xm, &ym, &zm);
	
	for (i = 0; i < n; i++)
	{
		sum += pow(euclid_distance(x[i], y[i], z[i], xm, ym, zm), 2);
	}
	
	return sqrt(sum / (n-1));
}
*/


static int
p_avg_distance(term goal, term *frame)
{
	term x_list = check_arg(1, goal, frame, LIST, IN);
	term y_list = check_arg(2, goal, frame, LIST, IN);
	term z_list = check_arg(3, goal, frame, LIST, IN);
	double a = RVAL(check_arg(4, goal, frame, REAL, IN));
	double b = RVAL(check_arg(5, goal, frame, REAL, IN));
	double c = RVAL(check_arg(6, goal, frame, REAL, IN));
	double d = RVAL(check_arg(7, goal, frame, REAL, IN));
	term v_sd = check_arg(8, goal, frame, REAL, OUT);
	double *x, *y, *z;
	term temp1, temp2, temp3;
	int n = list_length(x_list);
	int i;
	double sd;
	
	if (n < 2)
		fail("not enough data points");
	
	if (list_length(y_list) != n || list_length(z_list) != n)
		fail("all data lists must be of the same length");
	
	x = malloc(n * sizeof (double));
	y = malloc(n * sizeof (double));
	z = malloc(n * sizeof (double));
	
	for (temp1 = x_list, temp2 = y_list, temp3 = z_list, i = 0; temp1 != _nil;
	     temp1 = CDR(temp1), temp2 = CDR(temp2), temp3 = CDR(temp3), i++)
	{
		x[i] = (TYPE(CAR(temp1)) == REAL ? RVAL(CAR(temp1)) : IVAL(CAR(temp1)));
		y[i] = (TYPE(CAR(temp2)) == REAL ? RVAL(CAR(temp2)) : IVAL(CAR(temp2)));
		z[i] = (TYPE(CAR(temp3)) == REAL ? RVAL(CAR(temp3)) : IVAL(CAR(temp3)));
	}	
	
	avg_distance(x, y, z, n, a, b, c, d, &sd);

	free(x);
	free(y);
	free(z);
	
	return unify(v_sd, frame, new_real(sd), frame);
}


static void
mean(double x[], double y[], double z[], int n, double *xm, double *ym, double *zm)
{
	double x_mean = 0, y_mean = 0, z_mean = 0;
	int i;
	
	for (i = 0; i < n; i++)
	{
		x_mean += x[i];
		y_mean += y[i];
		z_mean += z[i];
	}
	
	*xm = x_mean / n;
	*ym = y_mean / n;
	*zm = z_mean / n;
}


static double
vector_size(double v1, double v2, double v3)
{
	return sqrt(v1*v1 + v2*v2 + v3*v3);
}


static double
vertical_slope(double A, double B, double C, double D)
{
	double z;
	
	z = (-A-B-D)/C;
	
	return 180 * (PI/2 - (acos(z / sqrt(2 + z*z))))/PI;
}


static int
p_vertical_slope(term goal, term *frame)
{
	term a0 = check_arg(1, goal, frame, REAL, IN);
	term a1 = check_arg(2, goal, frame, REAL, IN);
	term a2 = check_arg(3, goal, frame, REAL, IN);
	term a3 = check_arg(4, goal, frame, REAL, IN);
	term v_s = check_arg(5, goal, frame, REAL, OUT);
	double s;
	
	s = vertical_slope(RVAL(a0), RVAL(a1), RVAL(a2), RVAL(a3));
	
	return unify(v_s, frame, new_real(s), frame);
}


static int
p_stddev(term goal, term *frame)
{
	term list = check_arg(1, goal, frame, LIST, IN);
	term v_s = check_arg(2, goal, frame, REAL, OUT);
	int n = list_length(list);
	double x[n];
	double mean, sum;
	int i;
	term temp;
	
	for (temp = list, i = 0; temp != _nil; temp = CDR(temp), i++)
	{
		x[i] = (TYPE(CAR(temp)) == REAL ? RVAL(CAR(temp)) : IVAL(CAR(temp)));
	}

	sum = 0.0;
	for (i = 0; i < n; i++)
		sum += x[i];
	
	mean = sum/n;
	
	sum = 0.0;
	for (i = 0; i < n; i++)
		sum += pow(x[i] - mean, 2);
		
	return unify(v_s, frame, new_real(sqrt(sum/(n-1))), frame);
}


/************************************************************************/
/*			     Initialise Module				*/
/************************************************************************/

void
line_init(void)
{
	new_pred(p_line_best_fit, "line_best_fit");
	new_pred(p_avg_distance, "avg_distance");
	new_pred(p_vertical_slope, "vertical_slope");
	new_pred(p_stddev, "stddev");
}
