/************************************************************************/
/*	Maintain a heap of sets ordered by distance from the seed	*/
/*	This is used by the Aq algorithm				*/
/************************************************************************/

#include "prolog.h"
#include "aq.h"

#define KEY(x)		NSEL(x)

static int last = 0;
static int heap_size = 0;
static term *H = NULL;


/************************************************************************/
/*		print a queue for debugging purposes			*/
/************************************************************************/

static void
printq(void)
{
	int i;

	printf("#");
	for (i = 0; i < last; i++)
		printf("%10d", KEY(H[i]));
	printf("\n");
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
 		return 0;

	repeat
	{
		parent = (place + 1)/2 - 1;
		if (parent < 0 || KEY(H[parent]) < KEY(newitem))
			break;
		H[place] = H[parent];
		place = parent;
	}
	H[place] = newitem;
	return 1;
}


/************************************************************************/
/* Delete and return minimum item in heap. Adjust tree after deletion	*/
/************************************************************************/

term
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
			if (KEY(H[child + 1]) <= KEY(H[child]))
				child++;
	
		if (KEY(H[child]) > KEY(last_element))
			break;

		H[i] = H[child];
	}
	H[i] = last_element;
	return first_element;
}


/************************************************************************/
/*		   Fill the queue with negative examples		*/
/************************************************************************/

void
fill_q(term pos, term Examples)
{
	term p;

	for (p = Examples; p != NULL; p = NEXT(p))
		if (CLASS(p) != CLASS(pos))
		{
			KEY(p) = distance(p, pos);

			if (! q_insert(p))
				fprintf(stderr, "Queue is full\n");
		}
}


/************************************************************************/
/*			  Create and destroy heap			*/
/************************************************************************/


void
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
