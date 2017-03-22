/************************************************************************/
/*	 Regression tree algorithm reconstructed from CART book		*/
/************************************************************************/

#include <ctype.h>
#include "attr_info.h"

#define MAX_ATTR	50

typedef float *tuple;

typedef struct list
{
	tuple first;
	struct list *next;
} list;

typedef struct value_list
{
	float value;
	list *tuples;
	struct value_list *rest;
} value_list;

typedef struct
{
	char *name;
	value_list *value_table;
} rt_attribute_struct;

typedef struct node
{
	int attr, N;
	list *data;
	float R, average, threshold;
	struct node *left, *right; 
} node;	

extern void *malloc(), *calloc();


/************************************************************************/
/*			Local data structures for rt			*/
/************************************************************************/

static rt_attribute_struct attribute[MAX_ATTR];
static float M = 0.01;
static int N_min = 30;
static int dependent_variable = -1;
static int n_attributes = 0;
static int n_examples;


/************************************************************************/
/*	Routines for outputting decision tree node for debugging	*/
/************************************************************************/

static void
print_tuple(tuple tp)
{
	int i;

	for (i = 0; i < n_attributes-1; i++)
		printf("%g,", tp[i]);
	printf("%g\n", tp[n_attributes-1]);
}


static void
print_data(list *p)
{
	for (; p != NULL; p = p -> next)
		print_tuple(p -> first);
}


static void
print_node(node *n)
{
	printf("--------------------\n");
	if (n -> left == NULL)
		printf("leaf node: %g\n", n -> average);
	else
		printf("intermediate node: %s\n", attribute[n -> attr].name);

/*
	print_data(n -> data);
*/
}


/************************************************************************/
/*	Routines for outputting decision tree as if-statements		*/
/************************************************************************/

static void
tab(int n)
{
	int i;

	putchar('\n');
	for (i = 0; i < n; i++)
		printf("        ");
}


static void
print_tree(node *p, int depth)
{
	tab(depth);
	if (p -> left == NULL || p -> right == NULL)
	{
		printf(" %s = %1.2f", attribute[dependent_variable].name, p -> average);
		return;
	}

	printf("(%s <= %g ->", attribute[p -> attr].name, p -> threshold);
	print_tree(p -> left, depth + 1);
	printf(";");
	print_tree(p -> right, depth + 1);
	printf(")");
}


static void
print_result(node *tree, term relation)
{
	int i;
	char *sep = "(";

	printf("%s", NAME(ARG(0, relation)));
	for (i = 1; i <= ARITY(relation); i++)
	{
		printf("%s%s", sep, NAME(ARG(1, ARG(i, relation))));
		sep = ", ";
	}
	printf(") :-");
		
	print_tree(tree, 1);
	printf(".\n");
}


/************************************************************************/
/*		Routines for turning tree into Prolog clauses		*/
/************************************************************************/

static term head, _le;
static term *bound_var;

static term
make_tree(node *p)
{
	if (p -> left == NULL || p -> right == NULL)
		return(h_fn2(_equal, bound_var[dependent_variable], new_h_real(p -> average)));

	return h_fn2(_semi_colon,
			h_fn2(_arrow,
				h_fn2(_le, bound_var[p->attr], new_h_real(p -> threshold)),
				make_tree(p -> left)),
			make_tree(p -> right));

}


static term
make_clause(node *tree, term relation)
{
	extern term new_var(int, int, term), new_clause(int);
	int i;
	term cl;

	head = new_h_fn(ARITY(relation));
	ARG(0, head) = ARG(0, relation);
	bound_var = malloc(ARITY(relation) * sizeof(term));

	for (i = 1; i <= ARITY(relation); i++)
	{
		char *name = NAME(ARG(1, ARG(i, relation)));
		char buf[128];

		sprintf(buf, "%c%s", toupper(name[0]), name+1);
		ARG(i, head) = new_var(FREE, i-1, intern(buf));
		bound_var[i-1] = new_var(BOUND, i-1, intern(buf));
	}

	cl = new_clause(1);
	NVARS(cl) = ARITY(relation);
	HEAD(cl) = head;
	GOAL(1, cl) = make_tree(tree);
	free(bound_var);
	return cl;
}

/************************************************************************/
/*			Read all attribute declarations			*/
/************************************************************************/

static void
create_attributes(term relation)
{
	int i;

	n_attributes = ARITY(relation);
	dependent_variable = n_attributes - 1;

	for (i = 1; i <= n_attributes; i++)
	{
		term p = ARG(i, relation);

		switch (TYPE(p))
		{
		    case ATOM:	break;
		    case FN:	if (ARG(0, p) == _minus)
				{
					dependent_variable = i-1;
					p = ARG(1, p);
					break;
				}
				if (ARG(0, p) == _plus)
				{
					p = ARG(1, p);
					break;
				}
		    default:	fail("Attribute names must be atoms");
		}

		attribute[i - 1].name = NAME(p);
		attribute[i - 1].value_table = NULL;
	}

	/* fprintf(stderr, "No. of attributes: %d\n", n_attributes); */
}


/************************************************************************/
/*		Allocate and initialise space for a list cell  		*/
/************************************************************************/

static list *
cons(tuple car, list *cdr)
{
	list *p;

	if ((p = (list *) malloc((size_t) sizeof (list))) == NULL)
		fail("Could not allocate space for new rt list cell");

	p -> first = car;
	p -> next = cdr;
	return p;
}


/************************************************************************/
/*			Allocate space for a tuple  			*/
/************************************************************************/

static tuple
new_tuple(term example)
{
	tuple t;
	int i;

	if ((t = (tuple) calloc((size_t)(n_attributes), (size_t) sizeof (float))) == NULL)
		fail("Could not allocate space for new tuple");

	for (i = 0; i < n_attributes; i++)
	{
		value_list *p, **v;
		term x = ARG(i+1, example);

L:		switch (TYPE(x))
		{
		   case REAL:	t[i] = (float) RVAL(x);
				break;
		   case INT:	t[i] = (float) IVAL(x);
				break;
		   case REF:	if ((x = POINTER(x)) != NULL)
					goto L;
		   default:	print(x);
				fail("Only numeric data are allowed for regression trees");
		}

		v = &(attribute[i].value_table);
		for (;;)
		{
			if (*v == NULL || t[i] < (*v) -> value)
			{
				if ((p = (value_list *) malloc((size_t) sizeof (value_list))) == NULL)
					fail("Could not allocate memory of value list");

				p -> value = t[i];
				p -> tuples = cons(t, p -> tuples);
				p -> rest = *v;
				*v = p;
				break;
			}
			if (t[i] == (*v) -> value)
			{
				(*v) -> tuples = cons(t, (*v) -> tuples);
				break;
			}
			v = &((*v) -> rest);
		}
	}
	return t;
}


/************************************************************************/
/*			Read all examples from datafile		   	*/
/************************************************************************/

#ifdef EXPLICIT

static list *
create_data(term rel)
{
	list *data = NULL, **p = &data;

	n_examples = 0;

	for (rel = PROC(ARG(0, rel)); rel != NULL; rel = NEXT(rel))
	{
		*p = cons(new_tuple(HEAD(rel)), NULL);
		p = &((*p) -> next);
		n_examples++;
	}

	/* fprintf(stderr, "No. examples: %d\n", n_examples); */
	return data;
}

#else

static list *data, **dp;

static term
build_query(term rel)
{
	int i = ARITY(rel);
	term p = new_g_fn(i);

	ARG(0, p) = ARG(0, rel);

	for (; i != 0; --i)
		ARG(i, p) = new_ref();

	return p;
}

static void
add1(term *result, term x, term *frame)
{
	*dp = cons(new_tuple(x), NULL);
	dp = &((*dp) -> next);
	n_examples++;
}


static list *
create_data(term rel)
{
	term q[2] = {NULL, NULL};

	data = NULL;
	dp = &data;
	n_examples = 0;

	*q = build_query(rel);
	call_prove(q, NULL, *q, -1, add1, TRUE);

	/* fprintf(stderr, "No. examples: %d\n", n_examples); */
	return data;
}

#endif

/************************************************************************/
/*		Allocate a new node in the decision tree 		*/
/************************************************************************/

node *new_node()
{
	node *rval;

	if ((rval = (node *) malloc(sizeof (node))) == NULL)
		fail("Cannot allocate space for a node");

	rval -> data = NULL;
	rval -> left = NULL;
	rval -> right = NULL;

	return rval;
}


/************************************************************************/
/*				Calculate R				*/
/************************************************************************/

static void
R(node *nd)
{
	int N;
	list *p;
	double tmp, avg, sum;

	for (p = nd -> data, N = 0, sum = 0.0; p != NULL; p = p -> next, N++)	
		sum += p -> first[dependent_variable];

	nd -> average = avg = sum/N;
	nd -> N = N;

	for (p = nd -> data, sum = 0.0; p != NULL; p = p -> next)
	{
		tmp = (p -> first[dependent_variable]) - avg;
		sum += tmp * tmp;
	}
	nd -> R = sum/N;
}


/************************************************************************/
/*		Calculate the sum of squared error			*/
/************************************************************************/

static float
sum_of_squares(list *data, int attr, float R, float threshold)
{
	list *p;
	int N, N_left, N_right;
	float R_left, R_right;
	double tmp, left_av, right_av, left_sum, right_sum;


	N_left = N_right = 0;
	left_sum = right_sum = 0.0;

	for (p = data; p != NULL; p = p -> next)
		if (p -> first[attr] <= threshold)
		{
			left_sum += p -> first[dependent_variable];
			N_left++;
		}
		else
		{
			right_sum += p -> first[dependent_variable];
			N_right++;
		}

	N = N_left + N_right;
	left_av = N_left ? left_sum/N_left : 0.0;
	right_av = N_right ? right_sum/N_right : 0.0;

	left_sum = right_sum = 0.0;

	for (p = data; p != NULL; p = p -> next)
		if (p -> first[attr] <= threshold)
		{
			tmp = (p -> first[dependent_variable]) - left_av;
			left_sum += tmp * tmp;
		}
		else
		{
			tmp = (p -> first[dependent_variable]) - right_av;
			right_sum += tmp * tmp;
		}

	R_left = left_sum/N;
	R_right = right_sum/N;

	return (R - R_left - R_right);
}


/************************************************************************/
/*			Split the data on attribute "a"			*/
/************************************************************************/

static node *build_tree(list *);

static void
split(node *n)
{
	list *p, *left = NULL, *right = NULL;
	int attr = n -> attr;

	for (p = n -> data; p != NULL; p = p -> next)
		if (p -> first[attr] <= n -> threshold)
			left = cons(p -> first, left);
		else
			right = cons(p -> first, right);

	n -> left = build_tree(left);
	n -> right = build_tree(right);
}


/************************************************************************/
/*		Build a regression tree for the given data		*/
/************************************************************************/

node *
build_tree(list *data)
{
	node *rval;
	value_list *p;
	int attr, best_attr;
	float threshold, best_threshold = 0.0, value, best_value = 0.0;

	if (data == NULL)
		return NULL;

	rval = new_node();
	rval -> data = data;
	R(rval);
	if (rval -> N < N_min)
		return(rval);

	for (attr = 0; attr < n_attributes; attr++)
	{
		if (attr == dependent_variable)
			continue;

		for (p = attribute[attr].value_table; p -> rest != NULL; p = p -> rest)
		{
			threshold = (p -> rest -> value - p -> value)/2 + p -> value;
			if ((value = sum_of_squares(data, attr, rval -> R, threshold)) > best_value)
			{
				best_value = value;
				best_attr = attr;
				best_threshold = threshold;
			}
		}
	}

	rval -> threshold = best_threshold;
	rval -> attr = best_attr;

	if (best_value >= M)
		split(rval);
	return rval;
}


/************************************************************************/
/* Get attributes and data from Prolog data base and encode it		*/
/* Call build tree to create regression tree and output it all		*/
/************************************************************************/

static term
rt(term goal, term *frame)
{
	extern char *date_time(void);
	double start_time, finish_time;
	term relation = get_table(check_arg(1, goal, frame, ATOM, IN));
	term rval;
	node *tree;

	if (ARITY(goal) == 2)
		N_min = IVAL(check_arg(2, goal, frame, INT, IN));
	if (ARITY(goal) == 3)
		M = RVAL(check_arg(3, goal, frame, REAL, IN));

	create_attributes(relation);

	start_time = get_time();
	tree = build_tree(create_data(relation));
	finish_time = get_time();

	rval = build_plist("rt",
			"creator",	make(goal, frame),
			"date",		intern(date_time()),
			"n_examples",	new_h_int(n_examples),
			"errors",	new_h_int(0),
			"time",		new_h_real(finish_time - start_time),
			NULL
	);
	add_to_world(rval, make_clause(tree, relation));

	/* TODO: free all data structures!!! */
	
	return rval;
}


/************************************************************************/
/*			Clustering and Discretisation			*/
/************************************************************************/

typedef struct tree
{
	float data;
	int count;
	struct tree *left, *right;
} tree;


static void
insert(float n, tree **p)
{
	if (*p == NULL)
	{
		tree *q = malloc(sizeof(tree));
		q -> data = n;
		q -> count = 1;
		q -> left = q -> right = NULL;
		*p = q;
	}
	else if (n == (*p) -> data)
		((*p) -> count)++;
	else if (n < (*p) -> data)
		insert(n, &((*p) -> left));
	else
		insert(n, &((*p) -> right));
}


static void
traverse(tree *p)
{
	if (p == NULL)
		return;

	traverse(p -> left);
	printf("%10.3f %d\n", p -> data, p -> count);
	traverse(p -> right);
}


static int
discretise(term goal, term *frame)
{
	term relation = get_table(check_arg(1, goal, frame, FN, IN));
	int arg = IVAL(check_arg(2, goal, frame, INT, IN));
	tree *root = NULL;
	list *p;	

	create_attributes(relation);

	for (p = create_data(relation); p != NULL; p = p -> next)
		insert(p -> first[arg-1], &root);

	traverse(root);
	return TRUE;
}


/************************************************************************/
/*			Initialise this module				*/
/************************************************************************/

void
rt_init()
{
	_le = intern("<=");

	new_fsubr(rt, "rt");
	new_pred(discretise, "discretise");
}
