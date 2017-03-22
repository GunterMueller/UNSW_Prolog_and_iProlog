/************************************************************************/
/*	This module contains attribute/value processing routines.	*/
/*									*/
/* Attr/val pairs are encoded as bit sets. One attribute per word	*/
/* Bit slices are used to store instances of attr/value pairs		*/
/************************************************************************/

#include <math.h>
#include "prolog.h"

#define CLASS_VALUE(x)	ARG(ARITY(x), x)
#define CLASS_NAMES	ARG(ARITY(names), names)
#define CLASS_NAME(n)	ARG(n+1, CLASS_NAMES)

#define N_ATTR(a)	ARITY(a)
#define A_NAME(i, a)	ARG(0, ARG(i+1, a))
#define NVALS(i, a)	ARITY(ARG(i+1, a))
#define VALUES(i, a)	(&ARG(1, ARG(i+1, a)))

#define EXCEPT(x)	SUBSUMES(x)

#define NEW_SLICE	new_set(n_words, index)
#define NEW_RULE	new_set(n_attributes, names)


typedef struct
{
	int freq;		/* frequency of instances with this class */
	double prob;		/* probability of instances with the class */
	term instances;		/* set of instances with this class */
} class_struct;

typedef struct
{
	int freq;		/* frequency of instances with this value */
	term instances;		/* set of instances with this value */
	class_struct *class;	/* classes associated with this value */
} value_struct;

typedef struct
{
	int used;		/* Has attribute already been tested? */
	int n_values;		/* how many values does attribute have? */
	value_struct *value;	/* Values associated with attribute */
} attribute_struct;

typedef struct
{
	int n_classes;		/* number of classes in data set */
	int n_attributes;	/* number of attributes */
	int n_examples;		/* number of examples in data set */
	int n_words;		/* number of words needed to store sets */
	term names;		/* where to find domain description */
	term index;		/* bit N in set maps to Nth term in index */
	term data_set;		/* clauses making up data set */
	class_struct *class;	/* class sets */
	attribute_struct *attribute;	/* attribute data */
} data_struct;


/************************************************************************/
/*		     Prototypes for exported functions			*/
/************************************************************************/

data_struct *new_data_set(int, term);
term cover_to_clause(term);
void print_complex(term);
void print_cover(term);
term get_table(term name);
