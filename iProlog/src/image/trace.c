#include "prolog.h"
#include "p_pbm.h"
#include "trace.h"

#define ADJUST_CODE(x)	(x < 0 ? 8 + x : x % 8)

typedef enum
{
	N	= 0,
	NW	= 1,
	W	= 2,
	SW	= 3,
	S	= 4,
	SE	= 5,
	E	= 6,
	NE	= 7
} freeman_code;


typedef struct
{
	int i, j;
} coord;


typedef struct flink
{
	freeman_code dirn;
	struct flink *next;
} flink;


typedef struct segment
{
	coord start, end;
	flink *chain;
	struct segment *branch[8];
} segment;


/************************************************************************/
/*				Module globals				*/
/************************************************************************/

static int incr[8][2] =
{
	{-1,  0},
	{-1, -1},
	{ 0, -1},
	{+1, -1},
	{+1,  0},
	{+1, +1},
	{ 0, +1},
	{-1, +1}
};


static char *code_name[] = {"n", "nw", "w", "sw", "s", "se", "e", "ne"};

static pbm *visited;

static int segment_count;


/************************************************************************/
/*		Allocate space and initialise new chain link		*/
/************************************************************************/

static flink *
new_link(freeman_code dirn)
{
	flink *L = malloc(sizeof (flink));

	L -> dirn = dirn;
	L -> next = NULL;

	return L;
}


/************************************************************************/
/*		Allocate space and initialise new segment		*/
/************************************************************************/

static segment *
new_segment(int start_i, int start_j, freeman_code start_dirn)
{
	freeman_code n;
	segment *s = malloc(sizeof(segment));

	s -> start.i = start_i;
	s -> start.j = start_j;
	s -> end.i = start_i+incr[start_dirn][0];
	s -> end.j = start_j+incr[start_dirn][1];
	s -> chain = new_link(start_dirn);

	for (n = 0; n < 8; n++)
		s -> branch[n] = NULL;

	return s;
}


/************************************************************************/
/*				Debugging				*/
/************************************************************************/

static void
print_neighbours(int neighbour[])
{
	int i;
	char *separator = "{";

	for (i = 0; i < 8; i++)
	{
		fprintf(stderr, "%s%d", separator, neighbour[i]);
		separator = ", ";
	}
	fprintf(stderr, "}\n");
}


static void
print_chain(flink *L)
{
	flink *p;
	char *separator = "[";

	for (p = L; p != NULL; p = p -> next)
	{
		printf("%s%s", separator, code_name[p -> dirn]);
		separator = ", ";
		fflush(stdout);
	}
	printf("]\n");
}


static void
tab(int n)
{
	while (n--)
		putchar('\t');
}

static void
print_segment(segment *s, int t)
{
	freeman_code n;

	tab(t); printf("-----------\n");
	tab(t); printf("Start: (%d %d)\n", s -> start.i, s -> start.j);
	tab(t); printf("End:   (%d %d)\n", s -> end.i, s -> end.j);
	tab(t); print_chain(s -> chain);

	for (n = 0; n < 8; n++)
		if (s -> branch[n] != NULL)
			print_segment(s -> branch[n], t+1);
}


/************************************************************************/
/*		Deallocate space for the data structures		*/
/************************************************************************/

static void
free_chain(flink *L)
{
	if (L == NULL)
		return;

	free_chain(L -> next);
	free(L);
}


static void
free_segment(segment *s)
{
	int n;

	if (s == NULL)
		return;

	for (n = 0; n < 8; n++)
		free_segment(s -> branch[n]);

	free(s);
}


/************************************************************************/
/*		Assert segment data into Prolog's database		*/
/************************************************************************/

static term
construct_chain(flink *L)
{
	flink *p;
	term t = _nil, *q = &t;

	for (p = L; p != NULL; p = p -> next)
	{
		*q = hcons(intern(code_name[p -> dirn]), _nil);
		q = &CDR(*q);
	}

	return t;
}


static void
build_segment(segment *s)
{
	freeman_code n;
	term seg = new_h_fn(5);
	term neighbours = _nil, *p = &neighbours;

	ARG(0, seg) = intern("segment");
	ARG(1, seg) = new_h_int(segment_count);
	ARG(2, seg) = h_fn2(_comma, new_h_int(s -> start.i), new_h_int(s -> start.j));
	ARG(3, seg) = h_fn2(_comma, new_h_int(s -> end.i), new_h_int(s -> end.j));
	ARG(4, seg) = construct_chain(s -> chain);

	add_clause(new_unit(seg), FALSE);

	for (n = 0; n < 8; n++)
		if (s -> branch[n] != NULL)
		{
			*p = hcons(new_h_int(++segment_count), _nil);
			p = &CDR(*p);

			build_segment(s -> branch[n]);
		}

	ARG(5, seg) = neighbours;
}


/************************************************************************/
/*				Tracing					*/
/************************************************************************/


static segment *
find_main(pbm *im)
{
	int i, j;
	int height = im -> height;
	int width = im -> width;
	bit **I = im -> image;
	bit **P = visited -> image;

	for (i = height - 1; i > 0; i--)
		for (j = 0; j < width; j++)
			if (I[i][j] == 1 && I[i-1][j])
			{
				P[i][j] = 1;
				return new_segment(i, j, N);
			}

	return NULL;
}


static segment *
trace_segment(pbm *im, segment *s)
{
	int neighbour[8];
	int i = s -> end.i, j = s -> end.j;
	flink **last_link = &(s -> chain -> next);
	int previous_direction = s -> chain -> dirn;
	bit **I = im -> image;
	bit **P = visited -> image;

	for (;;)
	{
		int d, n_neighbours = 0;
		freeman_code n, which_neighbour;

		if (P[i][j])
			return s;
		else
			P[i][j] = 1;

		for (n = 0; n < 8; n++)
			neighbour[n] = 0;

		for (d = previous_direction - 2; d <= previous_direction + 2; d++)
		{
			n = ADJUST_CODE(d);

			if(neighbour[n] = I[i+incr[n][0]][j+incr[n][1]])
			{
				n_neighbours++;
				which_neighbour = n;
			}
		}

		if (n_neighbours == 0)
			return s;

		if (n_neighbours > 1)
		{
			int neigh = ADJUST_CODE((int)(which_neighbour)-1);

			if (n_neighbours == 2 && neighbour[neigh])
			{
				if ((int)(which_neighbour) % 2)
				{
					neighbour[which_neighbour] = 0;
					which_neighbour--;
				}
				else
					neighbour[neigh] = 0;
			}
			else
			{
				s -> end.i = i;
				s -> end.j = j;

				for (n = 0; n < 8; n++)
					if (neighbour[n])
					{
						s -> branch[n] = new_segment(i, j, n);
						trace_segment(im, s -> branch[n]);
					}

				return s;
			}
		}

		*last_link = new_link(which_neighbour);
		last_link = &((*last_link) -> next);
		previous_direction = which_neighbour;

		i = i+incr[which_neighbour][0];
		j = j+incr[which_neighbour][1];
	}
}


void
trace_skeleton(pbm *I)
{
	segment *s;

	visited = new_pbm(I -> width, I -> height);

	s = find_main(I);
	trace_segment(I, s);
	display_pbm(visited);

	segment_count = 0;
	build_segment(s);
/*
	print_segment(s, 0);
*/
	free_pbm(visited);
	free_segment(s);
}
