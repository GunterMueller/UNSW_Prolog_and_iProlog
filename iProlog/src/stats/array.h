/* array.c */

int *new_ivector(int);
int **new_2D_iarray(int, int);
double *new_dvector(int);
double **new_2D_array(int, int);
double ***new_3D_array(int, int, int);
void matinv(double **, double **, int);
void matmul(double **, double **, double **, int , int, int);
void inner(double **, double **, double *, int, int);
void free_2D_iarray(int **, int, int);
void free_2D_array(double **, int, int);
void free_3D_array(double ***, int, int, int);
void print_matrix(char *, double **, int, int);
void print_ivector(char *, int *, int);
void print_coeffs(term, double **);
