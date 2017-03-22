/************************************************************************/
/*   Backpropgation algorithm following Matjaz Kukar and the PDP book	*/
/************************************************************************/

#include <ctype.h>
#include "attr_info.h"

#define F(x)	(1.0 / (1.0 + exp(-x)))		/* squashing function	*/
#define d(x)	(x * (1 - x))			/* derivative of F	*/


/************************************************************************/
/* BP netwwork is assumed to be strcitly layered with completely	*/
/* connected adjacent layers. Therefore, number of weights attached	*/
/* to each unit is the same as the number of units in the layer below.	*/
/************************************************************************/

typedef struct
{
	term name;
	float output, delta;
	float bias, delta_bias;
	float *weight, *delta_weight;
} bp_unit;


typedef struct
{
	int n_units;
	bp_unit *unit;
} bp_layer;


/************************************************************************/
/*		   Global parameters for a training run			*/
/************************************************************************/

static term _epochs, _momentum, _learning_rate, _error;

static int max_epochs = 1000;

static float
	momentum	= 0.9,
	learning_rate	= 1,
	max_error	= 0.01;

/************************************************************************/
/*				Global variables			*/
/************************************************************************/

static int n_inputs, n_outputs, n_layers, n_units;

static bp_layer *layer;

static float *target;


/************************************************************************/
/*				Display routines			*/
/************************************************************************/


static void
show_net(void)
{
	int i, j, k;

	for (i = 1; i < n_layers; i++)
		for (j = 0; j < layer[i].n_units; j++)
		{
			fprintf(output, "X%d%d is %g", i, j, layer[i].unit[j].bias);

			for (k = 0; k < layer[i-1].n_units; k++)
				fprintf(output, " + %g * X%d%d", layer[i].unit[j].weight[k], i-1, k);

			fputc('\n', output);
		}
}


static void
show_output(void)
{
	int i, n;
	bp_unit *u;

	u = layer[n_layers - 1].unit;
	n = layer[n_layers - 1].n_units;

	for (i = 0; i < n; i++)
		fprintf(output, "%10g", u[i].output);

	fputc('\n', output);
}


/************************************************************************/
/*			Turn a network into a clause			*/
/************************************************************************/

static term head, _mul, _is, _logistic;
static int offset;

static term
unit_name(int i, int j)
{
	extern term new_var(int, int, term);

	if (layer[i].unit[j].name != NULL)
		return layer[i].unit[j].name;

	if (i == 0)
	{
		layer[i].unit[j].name = new_var(BOUND, OFFSET(ARG(j+1, head)), PNAME(ARG(j+1, head)));
		return layer[i].unit[j].name;
	}
	else if (i == n_layers-1)
	{
		layer[i].unit[j].name = new_var(BOUND, OFFSET(ARG(n_inputs+j+1, head)), PNAME(ARG(n_inputs+j+1, head)));
		return layer[i].unit[j].name;
	}
	else
	{
		char buf[32];

		sprintf(buf, "X%d%d", i, j);
		layer[i].unit[j].name = new_var(BOUND, offset, intern(buf));
		return new_var(FREE, offset++, intern(buf));
	}
}


static term
make_clause(term relation)
{
	extern term new_clause(int);
	int i, j, k, n = 1;
	term cl;

	cl = new_clause(n_units - n_inputs);
	HEAD(cl) = head = new_h_fn(ARITY(relation));
	ARG(0, head) = ARG(0, relation);

	offset = 0;
	for (i = 1; i <= ARITY(relation); i++)
	{
		extern term new_var(int, int, term);
		char *name = NAME(ARG(1, ARG(i, relation)));
		char buf[128];

		sprintf(buf, "%c%s", toupper(name[0]), name+1);
		ARG(i, head) = new_var(FREE, offset++, intern(buf));
	}

	for (i = 1; i < n_layers; i++)
		for (j = 0; j < layer[i].n_units; j++)
		{
			term eqn = new_h_real(layer[i].unit[j].bias);

			for (k = 0; k < layer[i-1].n_units; k++)
				eqn = h_fn2(_plus, eqn, h_fn2(_mul, new_h_real(layer[i].unit[j].weight[k]), unit_name(i-1, k)));

			eqn = h_fn2(_is, unit_name(i, j), h_fn1(_logistic, eqn));

			GOAL(n++, cl) = eqn;
		}

	NVARS(cl) = offset;
	return cl;
}


/************************************************************************/
/* Scan mode declaration and work out number of inputs and outputs	*/
/* Allocate array to store target values. Size = n_outputs.		*/
/************************************************************************/

static void
count_inputs_outputs(term relation)
{
	int i;
	int n_attributes = ARITY(relation);

	n_inputs = 0;
	n_outputs = 0;

	for (i = 1; i <= n_attributes; i++)
	{
		term p = ARG(i, relation);

		switch (TYPE(p))
		{
		    case ATOM:	break;
		    case FN:	if (ARG(0, p) == _plus)
				{
					n_inputs++;
					p = ARG(1, p);
					break;
				}
				if (ARG(0, p) == _minus)
				{
					n_outputs++;
					p = ARG(1, p);
					break;
				}
		    default:	fail("Attribute names must be atoms");
		}
	}

	if (n_inputs == 0 && n_outputs == 0)
	{
		n_inputs = ARITY(relation) - 1;
		n_outputs = 1;
	}

	target = malloc(sizeof(float) * n_outputs);
}


/************************************************************************/
/* Initialise a unit, including malloc  weight and delta_weight arrays	*/
/* Biases and initial weights are randomly initialise between -1 and +1	*/
/************************************************************************/

static float
rand_init()
{
	return (2.0 * rand()/RAND_MAX - 1.0);
}


static void
init_unit(bp_unit *u, int n_weights)
{
	int i;

	n_units++;
	u -> name = NULL;
	u -> output = 0.0;
	u -> delta = 0.0;
	u -> bias = rand_init();
	u -> delta_bias = 0.0;

	if (n_weights == 0)
	{
		u -> weight = NULL;
		u -> delta_weight = NULL;
		return;
	}

	u -> weight = malloc(sizeof(float) * n_weights);
	u -> delta_weight = malloc(sizeof(float) * n_weights);

	for (i = 0; i < n_weights; i++)
	{
		u -> weight[i] = rand_init();
		u -> delta_weight[i] = 0.0;
	}
}


/************************************************************************/
/* Allocate space for a new layer of n_units, each has n_weights	*/
/* Call routine to initialise units.					*/
/************************************************************************/

static bp_unit*
new_layer(int n_units, int n_weights)
{
	int i;
	bp_unit *unit = malloc(sizeof(bp_unit) * n_units);

	for (i = 0; i < n_units; i++)
		init_unit(&unit[i], n_weights);

	return unit;
}


/************************************************************************/
/* Scan list describing hidden layers and allocate space for them,	*/
/* as well as input and output layers.					*/
/************************************************************************/

static void
create_network(term layer_spec)
{
	int i, n_weights;
	term p;

	n_units = 0;
	n_layers = length(layer_spec, NULL) + 2;
	layer = malloc(sizeof(bp_layer) * n_layers);
	layer[0].unit = new_layer(n_inputs, 0);
	layer[0].n_units = n_inputs;

	n_weights = n_inputs;
	for (i = 1, p = layer_spec; p != _nil; p = CDR(p), i++)
	{
		int n;

		if (TYPE(CAR(layer_spec)) != INT)
			fail("layer spec must only contain integers");
			
		layer[i].unit = new_layer(n = IVAL(CAR(p)), n_weights);
		layer[i].n_units = n;
		n_weights = n;
	}

	layer[n_layers - 1].unit = new_layer(n_outputs, n_weights);
	layer[n_layers - 1].n_units = n_outputs;
}


/************************************************************************/
/*	  Do feed forward computatation to get output of network	*/
/************************************************************************/

static void
compute_output(void)
{
	int i, j, k;
	float sum;

	for (i = 1; i < n_layers; i++)
	{
		int n_weights = layer[i-1].n_units;
	
		for (j = layer[i].n_units - 1; j >= 0; j--)
		{
			bp_unit *u = &layer[i].unit[j];

			sum = u -> bias;
			for (k = n_weights - 1; k >= 0; k--)
				sum += layer[i-1].unit[k].output * u -> weight[k];
			u -> output = F(sum);
		}
	}
}


/************************************************************************/
/* The backprop algorithm based on the PDP book.			*/
/* Includes weight elimination as implemented by Matjaz Kukar		*/
/************************************************************************/

static void
backprop(void)
{
	int i, j, k, l;

	/* Calculate delta for output units */
	for (j = layer[n_layers - 1].n_units - 1; j >= 0; j--)
	{
		bp_unit *u = &layer[n_layers - 1].unit[j];
		u -> delta = (target[j] - u -> output) * d(u -> output);
	}

	/* Calculate delta for hidden units */
	for (l = n_layers-2; l > 0; l--)
	{
		for (j = layer[l].n_units - 1; j >= 0; j--)
		{
			bp_unit *u = &layer[l].unit[j];
			float sum = 0.0;

			for (k = layer[l+1].n_units - 1; k >= 0; k--)
				sum += layer[l+1].unit[k].delta * layer[l+1].unit[k].weight[j];
			u -> delta = sum * d(u -> output);
		}
	}
 
	for (l = n_layers-1; l > 0; l--)
	{
		for (j = layer[l].n_units - 1; j >= 0; j--)
		{
			bp_unit *u = &layer[l].unit[j];

			/* Change bias */
			u -> delta_bias =  (learning_rate * u -> delta) + (momentum * u -> delta_bias);
			u -> bias += u -> delta_bias;
 
			/* Change weights */
			for (i = layer[l-1].n_units - 1; i >= 0; i--)
			{
				u -> delta_weight[i] = (learning_rate * u -> delta * layer[l-1].unit[i].output) + (momentum * u -> delta_weight[i]);
				u -> weight[i] += u -> delta_weight[i];
			}
		 }
	}
}


/************************************************************************/
/* The sum of squared differences between the target and network	*/
/* output is taken as the error of the network.				*/
/************************************************************************/

static float
error(void)
{
	int i, n;
	bp_unit *u;
	float error = 0.0;

	u = layer[n_layers - 1].unit;
	n = layer[n_layers - 1].n_units;

	for (i = 0; i < n; i++)
	{
		float x = target[i] - u[i].output;
		error += x*x;
	}

	return error;
}


/************************************************************************/
/* Run backprop for each example in the training relation		*/
/* Repeat until mean error below threshold or number of epochs exceeded	*/
/************************************************************************/

static int
cycle(term relation)
{
	int m;

	for (m = 0; m < max_epochs; m++)
	{
		term rel;
		int n = 0;
		float err = 0.0;

		for (rel = PROC(ARG(0, relation)); rel != NULL; rel = NEXT(rel))
		{
			int i, j;
			term p = HEAD(rel);

			for (i = 1, j = 0; i <= ARITY(p); i++)
			{
				float x;

				switch (TYPE(ARG(i, p)))
				{
				case INT:	x = (float)(IVAL(ARG(i, p)));
						break;
				case REAL:	x = (float)(RVAL(ARG(i, p)));
						break;
				default:	fail("Non-numeric value in backprop data");
				}

				if (i <= n_inputs)
					layer[0].unit[i-1].output = x;
				else
					target[j++] = x;
			}

			n++;
			compute_output();
			err += error();
			backprop();
		}

		if (err/n < max_error)
			break;
	}
	return m;
}


/************************************************************************/
/*		Free all the space allocated for the network		*/
/************************************************************************/

static void
clean_up(void)
{
	int i, j;

	free(target);
	free(layer[0].unit);
	for (i = 1; i < n_layers; i++)
	{
		for (j = 0; j < layer[i].n_units; j++)
		{
			free(layer[i].unit[j].weight);
			free(layer[i].unit[j].delta_weight);
		}
		free(layer[i].unit);
	}
	free(layer);
}


/************************************************************************/
/*				Process options				*/
/************************************************************************/

static void
process_options(term goal, term *frame)
{
	term options = check_arg(3, goal, frame, LIST, IN);

	for (; options != _nil; options = CDR(options))
	{
		term x = CAR(options);

		if (TYPE(x) != FN && ARG(0, x) != _equal)
			fail("bp options: parameter = value");

		if (ARG(1, x) == _epochs)
		{
			if (TYPE(ARG(2, x)) != INT)
				fail("epoch must be an integer value");
			max_epochs = IVAL(ARG(2, x));
		}
		if (ARG(1, x) == _error)
		{
			if (TYPE(ARG(2, x)) != REAL)
				fail("error must be real valued");
			max_error = RVAL(ARG(2, x));
			if (max_error < 0.0 || max_error > 1.0)
				fail("error must between 0 and 1");
		}
		if (ARG(1, x) == _momentum)
		{
			if (TYPE(ARG(2, x)) != REAL)
				fail("momentum must be real valued");
			learning_rate = RVAL(ARG(2, x));
			if (momentum < 0.0 || momentum > 1.0)
				fail("momentum must between 0 and 1");
		}
		if (ARG(1, x) == _learning_rate)
		{
			if (TYPE(ARG(2, x)) != REAL)
				fail("learning_rate must be real valued");
			learning_rate = RVAL(ARG(2, x));
			if (learning_rate < 0.0 || learning_rate > 1.0)
				fail("learning_rate must between 0 and 1");
		}
	}
}


/************************************************************************/
/* Call backprop algorithm:						*/
/*	bp(RelationName/Arity, [UnitsInHiddenLayers], [Options])			*/
/* Eg.									*/
/*	bp(r/3, [4, 6, 4], [epochs = 500, error = 0.1])						*/
/************************************************************************/

static term
bp(term goal, term *frame)
{
	double start_time, finish_time;
	term relation = get_table(check_arg(1, goal, frame, ATOM, IN));
	term layer_spec = check_arg(2, goal, frame, LIST, IN);
	term rval;
	int epochs;

	if (ARITY(goal) == 3)
		process_options(goal, frame);

	count_inputs_outputs(relation);
	create_network(layer_spec);

	start_time = get_time();
	if ((epochs = cycle(relation)) == max_epochs)
	{
		clean_up();
		return FALSE;
	}
	finish_time = get_time();

	rval = build_plist("bp",
			"creator",	make(goal, frame),
			"date",		intern(date_time()),
			"epochs",	new_h_int(epochs),
			"time",		new_h_real(finish_time - start_time),
			NULL
	);
	add_to_world(rval, make_clause(relation));

	clean_up();
	return rval;
}


/************************************************************************/
/*			Initialise this module				*/
/************************************************************************/

void
bp_init()
{
	_is		= intern("is");
	_mul		= intern("*");
	_logistic	= intern("logistic");

	_epochs		= intern("epochs");
	_error		= intern("error");
	_momentum	= intern("momentum");
	_learning_rate	= intern("learning_rate");
	
	new_fsubr(bp, "bp");
}

