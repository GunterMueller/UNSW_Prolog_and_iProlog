/************************************************************************/
/* Implementation of Quinlan's ID3 with Bratko & Niblett style pruning	*/
/************************************************************************/

#include <ctype.h>
#include "attr_info.h"
#include "set.h"

#define PROBABILITY(i)	((double)(child[i] -> n_examples) / (double)(p -> n_examples))

typedef struct node
{
	int label;
	int majority_class;
	int majority;
	int n_examples;
	double node_error;
	double backed_up_error;
	struct node **child; 
} node;	


/************************************************************************/
/*				Local Variables				*/
/************************************************************************/

static term names;
static class_struct *class;
static attribute_struct *attribute;

static int n_attributes, n_classes;


/************************************************************************/
/*	     Pre-compute a log table to save calculation time		*/
/************************************************************************/

#define LOG2(x)		(log((double) x) / log(2.0))
#define P_LOG_P(x)	((x) * LOG2(x))

static double *log_table;


static void
init_log_table(int n)
{
	int i;

	if ((log_table = (double *) calloc((size_t) n, sizeof (double))) == NULL)
		fail("Cannot allocate space for log table");

	for (i = 0; i < n; i++)
		log_table[i] = P_LOG_P(i + 1); 
}


static double
p_log_p(int n)
{
	if (n == 0)
		fail("Tried to find p_log_p of 0");
	return log_table[n - 1];
}


/************************************************************************/
/*		  Routines for freeing tree data structures		*/
/************************************************************************/

static void free_node(node *);

static void
free_children(node *p)
{
	int i;
	node **child;

	if ((child = p -> child) != NULL)
		for (i = attribute[p -> label].n_values - 1; i >= 0; i--)
			free_node(child[i]);

	p -> child = NULL;
}


static void
free_node(node *p)
{
	if (p == NULL)
		return;

	free_children(p);
	free(p);
}


/************************************************************************/
/*	Calculate the expected information content for the data		*/
/************************************************************************/

static void
information_content(term data, double *info, int *n_examples, int *majority_class, int* majority)
{
	double sum = 0.0;
	int c;

	*majority = 0;
	*n_examples = set_cardinality(data);

	for (c = 0; c < n_classes; c++)
	{
		int class_frequency = intersection_size(data, class[c].instances);

		if (class_frequency)
		{
			if (class_frequency > *majority)
			{
				*majority_class = c;
				*majority = class_frequency;
			}
			sum += p_log_p(class_frequency);
		}
	}
	*info = (p_log_p(*n_examples) - sum)/(*n_examples);
}


/************************************************************************/
/*   Calculate the expected information for a split on attribute "a"	*/
/************************************************************************/

static double
expected_information(int a, term data, int n_examples)
{
	int v;
	term s = new_set(SET_SIZE(data), NULL);
	double total = 0.0;

	for (v = 0; v < attribute[a].n_values; v++)
	{
		int c, n_values = 0;
		double sum = 0.0;

		set_intersection(attribute[a].value[v].instances, data, s);
		for (c = 0; c < n_classes; c++)
		{
			int f = intersection_size(class[c].instances, s);

			if (f != 0)
			{
				sum += p_log_p(f);
				n_values += f;
			}
		}
		if (n_values != 0)
			total += p_log_p(n_values) - sum;
	}
	free_term(s);
	return (total/n_examples);
}


/************************************************************************/
/*		Find best attribute based on information gain		*/
/************************************************************************/

static int
best_attribute(term data, int n_examples, double base)
{
	int attr, best_attr = -1;
	double gain, best_gain = 0;

	for (attr = 0; attr < n_attributes; attr++)
	{
		if (attribute[attr].used)
			continue;

		gain = base - expected_information(attr, data, n_examples);
		if (gain > best_gain)
		{
			best_gain = gain;
			best_attr = attr;
		}
	}
	return best_attr;
}


/************************************************************************/
/*		Construct a new node for the decision tree		*/
/************************************************************************/

static node *
build_tree(term data)
{
	term s;
	node *rval;
	double information;
	int a, v;

	if (empty_set(data))
		return NULL;

	/****************************************************************/
	/*	  Create a new node and calculate information content	*/
	/****************************************************************/

	if ((rval = (node *) malloc(sizeof (node))) == NULL)
		fail("Cannot allocate space for a node");

	information_content(
		data,
		&information,
		&(rval -> n_examples),
		&(rval -> majority_class),
		&(rval -> majority)
	);

	/****************************************************************/
	/* If all examples have same class make node a leaf and return	*/
	/****************************************************************/

	if (information == 0)
	{
		rval -> label = rval -> majority_class;
		rval -> child = NULL;
		return rval;
	}

	/****************************************************************/
	/*	Split the node and build sub-trees recursively		*/
	/****************************************************************/

	rval -> label = a = best_attribute(data, rval -> n_examples, information);

	if ((rval -> child = (node **) calloc((size_t) attribute[a].n_values, sizeof(node *))) == NULL)
		fail("Cannot allocate space for children");

	s = new_set(SET_SIZE(data), NULL);
	attribute[a].used = 1;

	for (v = 0; v < attribute[a].n_values; v++)
	{
		set_intersection(data, attribute[a].value[v].instances, s);
		rval -> child[v] = build_tree(s);
	}
	
	attribute[a].used = 0;
	free(s);

	return rval;
}


/************************************************************************/
/*			Error estimate for a single node		*/
/************************************************************************/

static double
node_error(node *p)
{
	return	(double)(p->n_examples - p->majority + n_classes - 1)
		/
		(double)(p->n_examples + n_classes);
}


/************************************************************************/
/*			   Backed up error of children			*/
/************************************************************************/

static double prune(node *);

static double
backed_up_error(node *p)
{
	node **child = p -> child;
	double sum = 0;
	int i;

	for (i = attribute[p -> label].n_values - 1; i >= 0; i--)
		if (child[i] != NULL)
			sum += PROBABILITY(i) * prune(child[i]);

	return sum;
}


/************************************************************************/
/*	Prune subtree if node error is less than backed up error	*/
/************************************************************************/

static double
prune(node *p)
{
	double node_err, child_err;

	p -> node_error = node_err = node_error(p);

	if (p -> child == NULL)
		return node_err;

	p -> backed_up_error = child_err = backed_up_error(p);

	if (node_err < child_err)
	{
		free_children(p);
		p -> label = p -> majority_class;
		return node_err;
	}

	return child_err;
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
	int i, first;
	char *name;

	if (p -> child == NULL)
	{
		name = NAME(ARG(0, CLASS_NAMES));
		printf("%c%s = %s", toupper(name[0]), name+1, NAME(ARG(p -> label + 1, CLASS_NAMES)));
		return;
	}

	for (i = 0, first = 1; i < attribute[p -> label].n_values; i++)
		if (p -> child[i] != NULL)
		{
			tab(depth);
			name = NAME(ARG(0, ARG(p -> label + 1, names)));
			printf("%s%c%s = %s -> ",
				(first ? "(" : "|"),
				toupper(name[0]),
				name+1,
				NAME(ARG(i+1, ARG(p -> label + 1, names)))
			);
			first = 0;
			print_tree(p -> child[i], depth + 1);
		}
	putchar(')');
}


static void
print_result(node *tree)
{
	int i;
	char *name, *sep = "(";

	printf("%s", NAME(ARG(0, names)));
	for (i = 1; i <= ARITY(names); i++)
	{
		name = NAME(ARG(0, ARG(i, names)));
		printf("%s%c%s", sep, toupper(name[0]), name+1);
		sep = ", ";
	}
	printf(") :-");
	print_tree(tree, 1);
	printf(".\n");
}


/************************************************************************/
/*	    Routines for turning decision tree into a clause		*/
/************************************************************************/

static term head, *bound_var;

static term 
make_node(int attr, int val)
{
	return h_fn2(_equal, bound_var[attr], ARG(val+1, ARG(attr+1, names)));
}


static term
make_tree(node *p)
{
	int i;
	term rval = NULL;

	if (p -> child == NULL)
		return(make_node(N_ATTR(names)-1, p -> label));

	for (i = attribute[p -> label].n_values - 1; i >= 0; i--)
		if (p -> child[i] != NULL)
		{
			term rule = h_fn2(
				_arrow,
				make_node(p -> label, i),
				make_tree(p -> child[i]));

			if (rval == NULL)
				rval = rule;
			else
				rval = h_fn2(_bar, rule, rval);
		}
	return rval;
}


static term
make_clause(node *tree)
{
	extern term new_var(int, int, term), new_clause(int);
	int i;
	term cl;

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
	GOAL(1, cl) = make_tree(tree);
	free(bound_var);
	return cl;
}


/************************************************************************/
/*			Call ID3 from Prolog				*/
/************************************************************************/

static term
id(term goal, term *frame)
{
	double start_time, finish_time;
	term relation_name = check_arg(1, goal, frame, ATOM, IN);
	data_struct *data;
	term rval;
	node *tree;

	data = new_data_set(TRUE, relation_name);
	n_classes = data -> n_classes;
	n_attributes = data -> n_attributes;
	class = data -> class;
	attribute = data -> attribute;
	names = data -> names;

	init_log_table(data -> n_examples);

	start_time = get_time();
	prune(tree = build_tree(data -> data_set));
	finish_time = get_time();

	rval = build_plist("id",
			"creator",	make(goal, frame),
			"date",		intern(date_time()),
			"n_examples",	new_h_int(data -> n_examples),
			"errors",	new_h_int(0),
			"time",		new_h_real(finish_time - start_time),
			NULL
	);
	add_to_world(rval, make_clause(tree));

	free(log_table);
	free_node(tree);
	return rval;
}


/************************************************************************/
/*			 	Initialise module			*/
/************************************************************************/

void
id_init(void)
{
	new_subr(id, "id");
}
