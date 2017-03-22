/************************************************************************/
/*			memory management routines			*/
/************************************************************************/

#include <unistd.h>
#include <string.h>
#include "prolog.h"

#ifdef ALIGN
#define ROUND_TO_WORD(bytes)	((((bytes + WORD_LENGTH - 1) / WORD_LENGTH) + 1) & ~1)
#else
#define ROUND_TO_WORD(bytes)	((bytes + WORD_LENGTH - 1) / WORD_LENGTH)
#endif

#define MARKED(x)		(FLAGS(x) & MARK)

#define	K		1024

#define SEG_SIZE	16 * K


/************************************************************************/
/* Heap space is allocated in fixed sized segments and subdivided into	*/
/* cells for storing terms. All terms must have a type tag and enought	*/
/* information to work out their size.					*/
/************************************************************************/

typedef struct segment
{
	struct segment *sg_link;
	void **sg_ptr, **sg_end;
#ifdef ALIGN
	void *dummy;		/* needed for correct alignment on SPARCs */
#endif
	void *sg_space[SEG_SIZE];
} segment;


/************************************************************************/
/* Segments are form a linked list.					*/
/* "free_list" keeps track of free blocks with in segments		*/
/************************************************************************/

static segment *segs = NULL;

static term free_list[FREE_SIZE];

static int nsegs = 0;


/************************************************************************/
/* These globals the start, end and size of the local and global stacks	*/
/* The size of the stack is given in N * K words			*/
/************************************************************************/

//term *local_start, *local, *local_end;
term *global_start, *global, *global_end;

#ifdef THINK_C
//size_t	local_size = 32768,
size_t	global_size = 32768;
#else
//size_t	local_size = 32 * K,
size_t	global_size = 32 * K;
#endif


/************************************************************************/
/*	add a segment to the available memory				*/
/*	each node in segment is added to the freelist			*/
/************************************************************************/

static void
addseg(void)
{
	segment *newseg;
 
 	/* fprintf(stderr, "Allocating new segment %d\n", nsegs); */
	if ((newseg = (segment *) malloc((size_t) sizeof(segment))) == NULL)
	{
		fprintf(stderr, "\nCannot allocate new heap segment\n");
		exit(1);
	}

	newseg -> sg_link = segs;
	newseg -> sg_ptr = newseg -> sg_space;
	newseg -> sg_end = newseg -> sg_space + SEG_SIZE;
	segs = newseg;
 
 	++nsegs;
}


/************************************************************************/
/*	Allocate space from heap, keeping free list up to date		*/
/************************************************************************/

term
halloc(int n)
{
	term rval, *p;

	n = ROUND_TO_WORD(n);			/* compute number of words */
	
	if (n < FREE_SIZE)
	{
		if (free_list[n] != NULL)
		{
			rval = free_list[n];
			free_list[n] = NEXT_FREE(rval);
			return rval;
		}
	}
	else for (p = &free_list[0]; *p != NULL; p = &NEXT_FREE(*p))
		if (SIZE(*p) == n)
		{
			rval = *p;
			*p = NEXT_FREE(*p);
			return rval;
		}

	if (segs == NULL || segs -> sg_ptr + n >= segs -> sg_end)
		addseg();
	rval = (term)(segs -> sg_ptr);
	segs -> sg_ptr += n;
	return rval;
}


/************************************************************************/
/*		Release space in heap, adding it to the free list	*/
/************************************************************************/

static void
hfree(term t, int n)
{
	int i;

	n = (n + WORD_LENGTH - 1) / WORD_LENGTH;  /* compute number of words */
	
	i = n < FREE_SIZE ? n: 0;
	TYPE(t) = AVAIL;
	SIZE(t) = n;
	NEXT_FREE(t) = free_list[i];
	free_list[i] = t;
}


/************************************************************************/
/*	External interface to hfree that knows about object sizes	*/
/*	Doesn't recurse through structure the way free_term does	*/
/************************************************************************/

void
dispose(term t)
{
	int i;

	if (t == NULL) return;

	switch (TYPE(t))
	{
	   case INT:	hfree(t, sizeof(integer));
	   		break;
	   case REAL:	hfree(t, sizeof(real));
	   		break;
	   case FREE:
	   case BOUND:	hfree(t, sizeof(var));
			break;
	   case STREAM:	hfree(t, sizeof(stream));
			break;
	   case FN:
	   case LIST:	hfree(t, sizeof(compterm) + ARITY(t) * WORD_LENGTH);
			break;
	   case CLAUSE: for (i = 1; GOAL(i, t); i++);
			hfree(t, sizeof(clause) + i * WORD_LENGTH);
			break;
	   case SET:	free(t);
			break;
	   case CHUNK:	FREE_CHUNK(t);
			hfree(t, sizeof (chunk));
			break;
	   default:	fprintf(stderr, "\nProlog error: dispose - Unknown type %d\n", TYPE(t));
			exit(1);
	}
}


/************************************************************************/
/*			      Garbage disposal				*/
/************************************************************************/

void
free_term(term t)
{
	int i, limit;

	if (t == NULL) return;

	switch(TYPE(t))
	{
	   case ANON:	/* do nothing */
			break;
	   case AVAIL:	/* do nothing */
	   		break;
	   case ATOM:	/* don't free atoms in heap */
	   		/* hfree(t, sizeof(atom) + strlen(NAME(t))); */
	   		break;
	   case INT:	hfree(t, sizeof(integer));
	   		break;
	   case REAL:	hfree(t, sizeof(real));
	   		break;
	   case FREE:
	   case BOUND:	hfree(t, sizeof(var));
			break;
/*
	   case REF:	hfree(t, sizeof(ref));
	   		break;
*/
	   case STREAM:	hfree(t, sizeof(stream));
			break;
	   case FN:
	   case LIST:	limit = ARITY(t);
			for (i= 0; i <= limit; i++)
				free_term(ARG(i, t));
			hfree(t, sizeof(compterm) + limit * WORD_LENGTH);
			break;
	   case CLAUSE: free_term(HEAD(t));
			for (i = 1; GOAL(i, t); i++)
				free_term(GOAL(i, t));
			hfree(t, sizeof(clause) + i * WORD_LENGTH);
			break;
	   case SET:	free(t);
			break;
	   case CHUNK:	FREE_CHUNK(t);
			hfree(t, sizeof (chunk));
			break;
	   default:	fprintf(stderr, "\nProlog error: FREE - Unknown type %d\n", TYPE(t));
			abort();
	}
}


void
free_proc(term a)
{
	term p = PROC(a);

	PROC(a) = NULL;
	while (p != NULL)
	{
		term tmp = p;
		p = NEXT(p);
		free_term(tmp);
	}
}


/************************************************************************/
/*			Allocate space from global stack		*/
/************************************************************************/

term
galloc(int n)
{
	term rval;

	n = ROUND_TO_WORD(n);			/* compute number of words */

	if (global + n >= global_end)
	{
		fflush(output);
		fprintf(stderr, "GLOBAL STACK OVERFLOW\n");
		exit(1);
//		fail("GLOBAL STACK OVERFLOW");
	}

	rval = (term)(global);
	global += n;
	return rval;
}


/************************************************************************/
/* Allocate a big chunk of memory for a stack				*/
/* Both the Mac and PC have limits on the size malloc can allocate.	*/
/* Get around it on a unix system by using sbrk. This must be done	*/
/* before any mallocs so that malloc doesn't get confused.		*/
/************************************************************************/

static term *
big_chunk(size_t size)
{
	void *rval;
#ifdef USE_SBRK
	if ((rval = sbrk(size * WORD_LENGTH)) == (void *)(-1))
#else
	if ((rval = malloc(size * WORD_LENGTH)) == NULL)
#endif
	{
		fprintf(stderr, "Cannot allocate enough stack space for Prolog\n");
		exit(1);
	}
	return ((term *) rval);
}


/************************************************************************/
/* allocate space for local and global stacks and initialise free list	*/
/************************************************************************/

void
mem_init(void)
{
	int i;

//	local = local_start = big_chunk(local_size);
//	local_end = local_start + local_size;

	global = global_start = big_chunk(global_size);
	global_end = global_start + global_size;
	
	for (i = 0; i < FREE_SIZE; i++)
		free_list[i] = NULL;
}
