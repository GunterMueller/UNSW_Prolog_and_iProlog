/******************************************************************************

			UNSW Prolog (version 4.2)

		   Written by Tony Grech/Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/
#include "g.h"


#define	K	1024

#define INIT_HEAP_SIZE	(4 * K * WORD_LENGTH * MEM_AVAIL)
#define HEAP_CHUNK	(1 * K * WORD_LENGTH * MEM_AVAIL)
#define TBL_SIZE	20

 
typedef struct free_tag
{
	char *free_space;
	card size;
	struct free_tag *next_free;
} free_tag;


#define	FREE_SPACE(f)	((f)->free_space)
#define	NEXT_FREE(f)	((f)->next_free)
#define	FSIZE(f)	((f)->size)


static	char		**heap_start, **heap, **HEAP_END;
static	free_tag	*free_tags = NULL;
static	free_tag	*free_list[TBL_SIZE];

extern	char		*sbrk();


static
char *
new_space(words)
{
	char *ret;
	if (heap + words >= HEAP_END)
	{
		heap = (char **) sbrk(HEAP_CHUNK);
		HEAP_END = heap + HEAP_CHUNK/WORD_LENGTH;

		if ((int)heap == -1)
		{
			fprintf(stderr, "\nOUT OF MEMORY\n");
			exit(1);
		}
	}

	ret = (char *)heap;
	heap += words;
	return(ret);
}



static
enqueue_free_tag(f)
free_tag *f;
{
	f->next_free = free_tags;
	free_tags = f;
}



static
free_tag *
dequeue_free_tag()
{
	if (free_tags)
	{
		free_tag *f = free_tags;
		free_tags = NEXT_FREE(free_tags);
		return(f);
	}
	else	return((free_tag *)new_space(WORDS(sizeof(free_tag))));
}



/*	Allocate space from heap, keeping free list up to date	*/



char *_halloc(words)
int words;
{
	register free_tag **p, *f;

	if (words < TBL_SIZE)
	{
		if (free_list[words])
		{
			f = free_list[words];
			free_list[words] = NEXT_FREE(f);
			enqueue_free_tag(f);
			return(FREE_SPACE(f));
		}
		else if (words == WORDS(sizeof(free_tag)))
			return((char *)dequeue_free_tag());
	}
	else
		for (p = &free_list[0]; *p != 0; p = &NEXT_FREE(*p))
			if (FSIZE(*p) == words)
			{
				f = *p;
				*p = NEXT_FREE(f);
				enqueue_free_tag(f);
				return(FREE_SPACE(f));
			}

	return(new_space(words));
}



/*	Release space in heap, adding it to the free list	*/


_hfree(t, words)
char *t;
{
	int i = (words < TBL_SIZE ? words : 0);
	free_tag *f = dequeue_free_tag();

	FREE_SPACE(f)	= t;
	FSIZE(f)	= words;
	NEXT_FREE(f)	= free_list[i];

	free_list[i]	= f;
}


static
free_init()
{
	register int i;

	for (i = 0; i < TBL_SIZE; i++)
		free_list[i] = 0;
}


init_heap()
{
	heap = (char **) sbrk(INIT_HEAP_SIZE);

	if ((int)heap == -1) return(0);

	heap_start = heap;
	HEAP_END   = heap + INIT_HEAP_SIZE/WORD_LENGTH;

	free_init();
	return(1);
}
