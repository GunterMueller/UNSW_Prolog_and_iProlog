/************************************************************************/
/*	Maintain a heap of sets ordered by distance from the seed	*/
/*	This is used by the Aq algorithm				*/
/************************************************************************/

#include "prolog.h"

#define EQ(x, y)	(term_compare(x, var_frame, y, var_frame) == 0)
#define LT(x, y)	(term_compare(x, var_frame, y, var_frame) < 0)
#define LE(x, y)	(term_compare(x, var_frame, y, var_frame) <= 0)
#define GT(x, y)	(term_compare(x, var_frame, y, var_frame) > 0)
#define GE(x, y)	(term_compare(x, var_frame, y, var_frame) >= 0)

static int last = 0;
static int heap_size = 0;
static term *H = NULL;
static term *var_frame;


/************************************************************************/
/*			  Create and destroy heap			*/
/************************************************************************/

static void
create_q(int n)
{
	if (H != NULL)
		free(H);

	if ((H = malloc(n * sizeof (term))) == NULL)
	{
		fprintf(stderr, "Cannot allocate space for heap\n");
		exit(1);
	}
		
	heap_size = n;
	last = 0;
}


/************************************************************************/
/* Insert newitem into priority queue H. The operation fails if the	*/
/* heap is full. Shift items down until position for new item is found.	*/
/************************************************************************/

static int
q_insert(term newitem)
{
	int place, parent;

	if ((place = last++) == heap_size)
 		return(0);

	repeat
	{
		parent = (place + 1)/2 - 1;
		if (parent < 0 || LT(H[parent], newitem))
			break;
		H[place] = H[parent];
		place = parent;
	}
	H[place] = newitem;
	return(1);
}


/************************************************************************/
/* Delete and return minimum item in heap. Adjust tree after deletion	*/
/************************************************************************/

static term
q_remove(void)
{
	int i, child;
	term first_element, last_element;
 
	if (last == 0)
		return(NULL);

	first_element = H[0];
	last_element = H[--last];

	for (i = 0; (child = 2 * i + 1) < last; i = child)
	{
		if (child != last)
			if (LE(H[child + 1], H[child]))
				child++;
	
		if (GT(H[child], last_element))
			break;

		H[i] = H[child];
	}
	H[i] = last_element;
	return(first_element);
}


/************************************************************************/
/*				 Heap sort				*/
/************************************************************************/

static int
heap_sort(term goal, term *frame)
{
	term reln = check_arg(1, goal, frame, ATOM, IN);
	int argn = IVAL(check_arg(2, goal, frame, INT, IN));
	term p, q;
	int n, count, categories;

	for (n = 0, p = PROC(reln); p != NULL; n++, p = NEXT(p));

	if (n == 0)
		fail("1st argument is not the name of a relation");

	var_frame = frame;
	create_q(n);

	for (p = PROC(reln); p != NULL; p = NEXT(p))
		if (! q_insert(ARG(argn, HEAD(p))))
			fail("Queue is full\n");

	q = q_remove();
	count = 1;
	categories = 0;
	while ((p = q_remove()) != NULL)
		if (EQ(p, q))
			count++;
		else
		{
			fprintf(output, "%d # ", count); print(q);
			q = p;
			count = 1;
			categories++;
		}
	if (q != NULL)
	{
		fprintf(output, "%d # ", count); print(q);
		categories++;
	}
	fprintf(output, "#examples = %d, #categories = %d\n", n, categories);

	return(TRUE);
}


/************************************************************************/
/*			   Module Initialisation			*/
/************************************************************************/

void
sort_init(void)
{
	new_pred(heap_sort, "sort");
}
