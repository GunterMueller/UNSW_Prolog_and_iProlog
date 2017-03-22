#include "prolog.h"
#include "p_pbm.h"
#include "p_pgm.h"
#include "p_image.h"
#include "skeleton.h"
#include "trace.h"
#include "blob.h"


/************************************************************************/
/*		Specify public routines for chunk structure		*/
/************************************************************************/

static void free_binary_image(term b);
static void print_binary_image(term p);

chunk_spec binary_image =
{
	"binary image",
	free_binary_image,
	print_binary_image
};


/************************************************************************/
/*		Create pbm structure but not in a chunk			*/
/************************************************************************/

pbm *
new_pbm(int width, int height)
{
	pbm *p;
	int i, j;
	bit **I;

	p = malloc(sizeof (pbm));
	p -> width = width;
	p -> height = height;
	p -> image = I = pbm_allocarray(width, height);

	for (i = 0; i < height; i ++)
		for (j = 0; j < width; j++)
			I[i][j] = 0;

	return p;
}

void
free_pbm(pbm *p)
{
	pbm_freearray(p -> image, p -> height);
	free(p);
}


/************************************************************************/
/*			Routines required for chunk spec		*/
/************************************************************************/

static void
free_binary_image(term b)
{
	free_pbm(PBM_IMAGE(b));
}


static void
print_binary_image(term p)
{
	fprintf(output,
	 	"|| binary image (%d x %d) ||",
	 	PBM_IMAGE(p) -> width,
	 	PBM_IMAGE(p) -> height
	 );
}


/************************************************************************/
/* 				I/O utilities				*/
/************************************************************************/

pbm *
read_pbm(char *fname)
{
	pbm *p;
	FILE *fp = fopen(fname, "r");

	if (fp == NULL)
		fail("Could not read PBM file");

	p = malloc(sizeof (pbm));
	p -> image = pbm_readpbm(fp, &(p -> width), &(p -> height));
	fclose(fp);
	return p;
}


void
write_pbm(char *fname, pbm *p)
{
	FILE *fp = fopen(fname, "w");

	if (fp == NULL)
		fail("Could not write to PGM file");

	pbm_writepbm(fp, p -> image, p -> width, p -> height, 0);

	fclose(fp);
}


void
display_pbm(pbm *p)
{
	FILE *send, *receive;

	send_to("xv - ", &send, &receive);
	pbm_writepbm(send, p -> image, p -> width, p -> height, 0);
	fclose(send);
	fclose(receive);	
}


/************************************************************************/
/* 		     Prolog built-ins for I/O utilities			*/
/************************************************************************/

static term
p_read_pbm(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);

	return push_image(new_chunk(&binary_image, read_pbm(NAME(fname))));
}


static int
p_write_pbm(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);
	term im;

	if (ARITY(goal) == 2)
		im = check_arg(2, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to write");
	
	write_pbm(NAME(fname), PBM_IMAGE(im));

	return TRUE;
}


static int
p_display_pbm(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to display");
	
	display_pbm(PBM_IMAGE(im));

	return TRUE;
}


/************************************************************************/
/*		Cut out a sub-window of the given image			*/
/************************************************************************/

pbm *
cut_pbm(pbm *image1, int x, int y, int width, int height)
{
	pbm *image2 = new_pbm(width, height);
	bit **I1 = image1 -> image;
	bit **I2 = image2 -> image;
	int i, j;

	if (x < 0)
		x = image1 -> width + x;
	if (y < 0)
		y = image1 -> height + y;

	if (x+width > image1 -> width || y + height > image1 -> height)
	{
		free_pbm(image2);
		fail("Tried to cut too much");
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = I1[y + i][x + j];

	return image2;
}


static term
p_cut_pbm(term goal, term *frame)
{
	pbm *p;

	switch (ARITY(goal))
	{
		case 5:
		{
			term im = check_arg(1, goal, frame, CHUNK, IN);
			term x = check_arg(2, goal, frame, INT, IN);
			term y = check_arg(3, goal, frame, INT, IN);
			term width = check_arg(4, goal, frame, INT, IN);
			term height = check_arg(5, goal, frame, INT, IN);

			p = cut_pbm(PBM_IMAGE(im), IVAL(x), IVAL(y), IVAL(width), IVAL(height));

			return push_image(new_chunk(&binary_image, p));
		}
		case 4:
		{
			term im = current_image();
			term x = check_arg(1, goal, frame, INT, IN);
			term y = check_arg(2, goal, frame, INT, IN);
			term width = check_arg(3, goal, frame, INT, IN);
			term height = check_arg(4, goal, frame, INT, IN);

			if (im == NULL)
				fail("No image to cut");

			p = cut_pbm(PBM_IMAGE(im), IVAL(x), IVAL(y), IVAL(width), IVAL(height));

			return push_image(new_chunk(&binary_image, p));
		}
		default:
			fail("Incorrect number of arguments to cut_pgm");
	}
}


/************************************************************************/
/*		Prolog hook for routine defined in skeleton.c 		*/
/************************************************************************/

static term
p_skeletonise(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to skeletonise");
	
	return push_image(new_chunk(&binary_image, skeletonise(PBM_IMAGE(im))));
}

static int
p_trace_skeleton(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to trace");
	
	trace_skeleton(PBM_IMAGE(im));

	return TRUE;
}


/************************************************************************/
/*		Prolog hook for routine defined in blob.c 		*/
/************************************************************************/

static term
p_blob(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to blob");
	
	return push_image(new_chunk(&binary_image, blob(PBM_IMAGE(im))));
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
p_pbm_init(void)
{
	new_subr(p_read_pbm,		"read_pbm");
	new_pred(p_write_pbm,		"write_pbm");
	new_pred(p_display_pbm,		"display_pbm");

	new_subr(p_cut_pbm,		"cut_pbm");
	new_subr(p_skeletonise,		"skeletonise");
	new_subr(p_skeletonise,		"skeletonize");
	new_pred(p_trace_skeleton,	"trace_skeleton");
	new_subr(p_blob,		"blob");
}
