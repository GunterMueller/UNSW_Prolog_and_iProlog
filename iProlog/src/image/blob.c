#include "prolog.h"
#include "p_pbm.h"
#include "p_pgm.h"
#include "blob.h"

#define MAX_BLOBS	2000

#define DEBUG

typedef struct equiv
{
	int		done;
	int		label;
	int		area;
	int		perimeter;
	int		min_x, min_y, max_x, max_y;
	struct equiv	*next;
} equiv;


static void
insert(int label, equiv *L)
{
	equiv **p;

	for (p = &L; *p != NULL; p = &((*p) -> next))
		if (label == (*p) -> label)
			return;

	*p = malloc(sizeof (equiv));
	(*p) -> label = label;
	(*p) -> next = NULL;
}


static void
adjust_label(equiv L[], equiv *p, int label)
{
	if (p -> done)
		return;
	p -> label = label;
	p -> done = 1;
	for (p = p -> next; p != NULL; p = p -> next)
		adjust_label(L, &L[p -> label], label);
}


pgm *
blob(pbm *image1)
{
	int width = image1 -> width, height = image1 -> height;
	pgm *image2 = new_pgm(width, height, MAX_BLOBS);
	bit **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int i, j, blob_count = 0;
	equiv L[MAX_BLOBS];
#ifdef DEBUG
	FILE *fp = fopen("../blob.out", "w");
#endif
	for (i = 1; i < MAX_BLOBS; i++)
	{
		L[i].done = 0;
		L[i].label = i;
		L[i].area = 0;
		L[i].perimeter = 0;
		L[i].min_x = width-1;
		L[i].max_x = 0;
		L[i].min_y = height-1;
		L[i].max_y = 0;
		L[i].next = NULL;
	}

	// Process top row
	
	for (j = 1; j < width; j++)
	{
		if (I1[0][j] == 0)
			continue;
		else if (I1[0][j-1] == 0)
			I2[0][j] = ++blob_count;
		else
			I2[0][j] = I2[0][j-1];
	}
	
	for (i = 1; i < height; i ++)
	{
		// Check first column
		
		if (I1[i][0] != 0)
		{
			if (I1[i-1][0] == 0)
				I2[i][0] = ++blob_count;
			else
				I2[i][0] = I2[i-1][0];
		}
		
		for (j = 1; j < width; j++)
		{
			gray above, left;
			
			if (I1[i][j] == 0)
				continue;

			above = I1[i-1][j];
			left = I1[i][j-1];

			if (above == 0 && left == 0)
				I2[i][j] = ++blob_count;
			else if (above == 1 && left == 0)
				I2[i][j] = I2[i-1][j];
			else if (above == 0 && left == 1)
				I2[i][j] = I2[i][j-1];
			else if (I2[i-1][j] == I2[i][j-1])
				I2[i][j] = I2[i-1][j];
			else
			{
				int label1 = I2[i-1][j], label2 = I2[i][j-1];

				I2[i][j] = I2[i-1][j];

				insert(label2, &L[label1]);
				insert(label1, &L[label2]);
			}
		}
	}

#ifdef DEBUG
	for (i = 1; i <= blob_count; i++)
	{
		equiv *p;

		if (L[i].next == NULL)
			continue;
		fprintf(fp, "%4d: ", i);
		for (p = &L[i]; p != NULL; p = p -> next)
			fprintf(fp, "% d", p -> label);
		fprintf(fp, "\n");
	}
	fprintf(fp, "-------------------\n");
#endif

	for (i = 1; i < MAX_BLOBS; i++)
		if (L[i].label == i)
			adjust_label(L, &L[i], L[i].label);

#ifdef DEBUG
	for (i = 1; i <= blob_count; i++)
	{
		equiv *p;

		if (L[i].next == NULL)
			continue;
		fprintf(fp, "%4d: ", i);
		for (p = &L[i]; p != NULL; p = p -> next)
			fprintf(fp, "% d", p -> label);
		fprintf(fp, "\n");
	}
#endif
	
	for (i = 0; i < height; i ++)
	{
		for (j = 0; j < width; j++)
		{
			equiv *p = &L[L[I2[i][j]].label];
			I2[i][j] = p -> label;
			p -> area++;

			if (i == 0 || I2[i-1][j] != I2[i][j])
				p -> perimeter++;
			else if (i == height-1 || I2[i+1][j] != I2[i][j])
				p -> perimeter++;
			else if (j == 0 || I2[i][j-1] != I2[i][j])
				p -> perimeter++;
			else if (j == width-1 || I2[i][j+1] != I2[i][j])
				p -> perimeter++;
			
			if (j < p -> min_x)
				p -> min_x = j;
			if (j > p -> max_x)
				p -> max_x = j;
			if (i < p -> min_y)
				p -> min_y = i;
			if (i > p -> max_y)
				p -> max_y = i;
#ifdef DEBUG
			fprintf(fp, "%4d", I2[i][j]);
#endif
		}
#ifdef DEBUG
		putc('\n', fp);
#endif
	}

	for (i = 1; i < MAX_BLOBS; i++)
	{
		equiv **p = &(L[i].next);

		while (*p != NULL)
		{
			equiv *q = *p;
			*p = (*p) -> next;
			free(q);
		}
	}

#ifdef DEBUG
	for (i = 1; i <= blob_count; i++)
		if (L[i].area != 0)
			fprintf(fp, "%4d: %d %d (%d, %d)-(%d, %d)\n", i, L[i].area, L[i].perimeter, L[i].min_x, L[i].min_y, L[i].max_x, L[i].max_y);
	fclose(fp);
#endif
	
	return image2;
}
