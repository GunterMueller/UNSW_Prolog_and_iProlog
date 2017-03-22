#include "prolog.h"
#include "p_pbm.h"
#include "p_pgm.h"
#include "p_image.h"
#include "edge.h"


/************************************************************************/
/*		Specify public routines for chunk structure		*/
/************************************************************************/

static void free_grayscale_image(term b);
static void print_grayscale_image(term p);

chunk_spec grayscale_image =
{
	"grayscale image",
	free_grayscale_image,
	print_grayscale_image
};


/************************************************************************/
/*			Create and free pgm structure			*/
/************************************************************************/

pgm *
new_pgm(int width, int height, gray maxval)
{
	pgm *p;
	gray **I;

	p = malloc(sizeof (pgm));
	p -> width = width;
	p -> height = height;
	p -> maxval = maxval;
	p -> histogram = NULL;
	p -> image = I = pgm_allocarray(width, height);

	return p;
}

void
free_pgm(pgm *p)
{
	if (p -> histogram != NULL)
		free(p -> histogram);
	pgm_freearray(p -> image, p -> height);
	free(p);
}

/************************************************************************/
/*			Routines required for chunk spec		*/
/************************************************************************/

static void
free_grayscale_image(term b)
{
	free_pgm(PGM_IMAGE(b));
}


static void
print_grayscale_image(term p)
{
	fprintf(output,
	 	"|| grayscale image (%d x %d) ||",
	 	PGM_IMAGE(p) -> width,
	 	PGM_IMAGE(p) -> height
	 );
}


/************************************************************************/
/* 				I/O utilities				*/
/************************************************************************/

pgm *
read_pgm(char *fname)
{
	pgm *p;
	FILE *fp = fopen(fname, "r");

	if (fp == NULL)
		fail("Could not read PBM file");

	p = malloc(sizeof (pgm));
	p -> image = pgm_readpgm(fp, &(p -> width), &(p -> height), &(p -> maxval));
	fclose(fp);
	return p;
}


void
write_pgm(char *fname, pgm *p)
{
	FILE *fp = fopen(fname, "w");

	if (fp == NULL)
		fail("Could not write to PGM file");

	pgm_writepgm(fp, p -> image, p -> width, p -> height, p-> maxval, 0);

	fclose(fp);
}


void
display_pgm(pgm *p)
{
	FILE *send, *receive;

	send_to("xv - ", &send, &receive);
	pgm_writepgm(send, p -> image, p -> width, p -> height, p-> maxval, 0);
	fclose(send);
	fclose(receive);	
}


/************************************************************************/
/* 		   Prolog built-ins for I/O utilities			*/
/************************************************************************/

static term
p_read_pgm(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);

	return push_image(new_chunk(&grayscale_image, read_pgm(NAME(fname))));
}


static int
p_write_pgm(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);
	term im;

	if (ARITY(goal) == 2)
		im = check_arg(2, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to write");

	write_pgm(NAME(fname), PGM_IMAGE(im));
	return TRUE;
}


static int
p_display_pgm(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to display");

	display_pgm(PGM_IMAGE(im));
	return TRUE;
}


/************************************************************************/
/*		Cut out a sub-window of the given image			*/
/************************************************************************/

pgm *
cut_pgm(pgm *image1, int x, int y, int width, int height)
{
	pgm *image2 = new_pgm(width, height, image1 -> maxval);
	gray **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int i, j;

	if (x < 0)
		x = image1 -> width + x;
	if (y < 0)
		y = image1 -> height + y;

	if (x+width > image1 -> width || y + height > image1 -> height)
	{
		free_pgm(image2);
		fail("Tried to cut too much");
	}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = I1[y + i][x + j];

	return image2;
}


static term
p_cut_pgm(term goal, term *frame)
{
	pgm *p;
	
	switch (ARITY(goal))
	{
		case 5:
		{
			term im = check_arg(1, goal, frame, CHUNK, IN);
			term x = check_arg(2, goal, frame, INT, IN);
			term y = check_arg(3, goal, frame, INT, IN);
			term width = check_arg(4, goal, frame, INT, IN);
			term height = check_arg(5, goal, frame, INT, IN);

			p = cut_pgm(PGM_IMAGE(im), IVAL(x), IVAL(y), IVAL(width), IVAL(height));

			return push_image(new_chunk(&grayscale_image, p));
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

			p = cut_pgm(PGM_IMAGE(im), IVAL(x), IVAL(y), IVAL(width), IVAL(height));

			return push_image(new_chunk(&grayscale_image, p));
		}
		default:
			fail("Incorrect number of arguments to cut_pgm");
	}
}


/************************************************************************/
/*		clip all pixels above 'hi' and below 'lo'		*/
/************************************************************************/

pgm *
clip_pgm(pgm *image1, int lo, int hi)
{
	int width = image1 -> width, height = image1 -> height;
	pgm *image2 = new_pgm(width, height, image1 -> maxval);
	gray **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			if (I1[i][j] > hi)
				I2[i][j] = hi;
			else if (I1[i][j] < lo)
				I2[i][j] = lo;
			else
				I2[i][j] = I1[i][j];

	return image2;
}


static term
p_clip_pgm(term goal, term *frame)
{
	pgm *p;
	
	switch (ARITY(goal))
	{
		case 3:
		{
			term im = check_arg(1, goal, frame, CHUNK, IN);
			term lo = check_arg(2, goal, frame, INT, IN);
			term hi = check_arg(3, goal, frame, INT, IN);

			p = clip_pgm(PGM_IMAGE(im), IVAL(lo), IVAL(hi));

			return push_image(new_chunk(&grayscale_image, p));
		}
		case 2:
		{
			term im = current_image();
			term lo = check_arg(2, goal, frame, INT, IN);
			term hi = check_arg(3, goal, frame, INT, IN);

			if (im == NULL)
				fail("No image to clip");
			
			p = clip_pgm(PGM_IMAGE(im), IVAL(lo), IVAL(hi));

			return push_image(new_chunk(&grayscale_image, p));
		}
		default:
			fail("Incorrect number of arguments to clip_pgm");
	}
}


/************************************************************************/
/*				Band pass filter			*/
/************************************************************************/

pgm *
band_pass(pgm *image1, int lo, int hi)
{
	int width = image1 -> width, height = image1 -> height;
	pgm *image2 = new_pgm(width, height, image1 -> maxval);
	gray **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = (I1[i][j] < lo || I1[i][j] > hi) ? 0 : I1[i][j];

	return image2;
}


static term
p_band_pass(term goal, term *frame)
{
	pgm *p;
	
	switch (ARITY(goal))
	{
		case 3:
		{
			term im = check_arg(1, goal, frame, CHUNK, IN);
			term lo = check_arg(2, goal, frame, INT, IN);
			term hi = check_arg(3, goal, frame, INT, IN);

			p = band_pass(PGM_IMAGE(im), IVAL(lo), IVAL(hi));

			return push_image(new_chunk(&grayscale_image, p));
		}
		case 2:
		{
			term im = current_image();
			term lo = check_arg(2, goal, frame, INT, IN);
			term hi = check_arg(3, goal, frame, INT, IN);

			if (im == NULL)
				fail("No image for band pass");

			p = band_pass(PGM_IMAGE(im), IVAL(lo), IVAL(hi));

			return push_image(new_chunk(&grayscale_image, p));
		}
		default:
			fail("Incorrect number of arguments to band_pass");
	}
}


/************************************************************************/
/*				Band reject filter			*/
/************************************************************************/

pgm *
band_reject(pgm *image1, int lo, int hi)
{
	int width = image1 -> width, height = image1 -> height;
	pgm *image2 = new_pgm(width, height, image1 -> maxval);
	gray **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int i, j;
	
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = (I1[i][j] < lo || I1[i][j] > hi) ? I1[i][j] : 0;

	return image2;
}


static term
p_band_reject(term goal, term *frame)
{
	pgm *p;
	
	switch (ARITY(goal))
	{
		case 3:
		{
			term im = check_arg(1, goal, frame, CHUNK, IN);
			term lo = check_arg(2, goal, frame, INT, IN);
			term hi = check_arg(3, goal, frame, INT, IN);

			p = band_reject(PGM_IMAGE(im), IVAL(lo), IVAL(hi));

			return push_image(new_chunk(&grayscale_image, p));
		}
		case 2:
		{
			term im = current_image();
			term lo = check_arg(2, goal, frame, INT, IN);
			term hi = check_arg(3, goal, frame, INT, IN);

			if (im == NULL)
				fail("No image for band reject");

			p = band_reject(PGM_IMAGE(im), IVAL(lo), IVAL(hi));

			return push_image(new_chunk(&grayscale_image, p));
		}
		default:
			fail("Incorrect number of arguments to band_reject");
	}
}


/************************************************************************/
/*				histogram				*/
/************************************************************************/

static pgm *
histogram(pgm *im)
{
	int width = im -> width, height = im -> height;
	gray maxval = im -> maxval;
	gray **I = im -> image;
	int i, j, *h;

	if (im -> histogram !=  NULL)
		return im;

	h = malloc(maxval+1 * sizeof (int));

	for (i = 0; i <= maxval; i++)
		h[i] = 0;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			h[I[i][j]]++;

	im -> histogram = h;
	return im;
}


static int
print_hist(term goal, term *frame)
{
	term im = check_arg(1, goal, frame, CHUNK, IN);
	pgm *p = PGM_IMAGE(im);
	gray maxval = p -> maxval;
	int i;

	histogram(p);

	for (i = 0; i <= maxval; i++)
		printf("%d %d\n", i, p -> histogram[i]);

	return TRUE;
}


/************************************************************************/
/*	Perform histogram equalisation on grey scale image.		*/
/************************************************************************/

static pgm *
histogram_equalisation(pgm *image1)
{
	int width = image1 -> width, height = image1 -> height;
	gray maxval = image1 -> maxval;
	pgm *image2 = new_pgm(width, height, maxval);
	gray **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int sum = 0, running_sum[maxval+1];
	int i, j;

	histogram(image1);

	for (i = 0; i <= maxval; i++)
	{
		sum += image1 -> histogram[i];
		running_sum[i] = sum;
	}

	for (i = 0; i <= maxval; i++)
		running_sum[i] = maxval * running_sum[i] / sum;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = running_sum[I1[i][j]];

	return image2;
}


static term
p_histogram_equalisation(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image for histogram");

	return push_image(new_chunk(&grayscale_image, histogram_equalisation(PGM_IMAGE(im))));
}


/************************************************************************/
/*	Perform histogram stretching on grey scale image.		*/
/*	Produces new image leaving original untouched.			*/
/************************************************************************/

pgm *
stretch_pgm(pgm *image1)
{
	int width = image1 -> width, height = image1 -> height;
	gray maxval = image1 -> maxval;
	pgm *image2 = new_pgm(width, height, maxval);
	gray **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	gray lo = maxval, hi = 0;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			if (I1[i][j] > hi)
				hi = I1[i][j];
			if (I1[i][j] < lo)
				lo = I1[i][j];
		}

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2 [i][j] = maxval * (I1[i][j] - lo)/(hi - lo);

	return image2;
}


static term
p_stretch_pgm(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to stretch");
	
	return push_image(new_chunk(&grayscale_image, stretch_pgm(PGM_IMAGE(im))));
}


/************************************************************************/
/* Simple thresholding of grey scale image, producing binary image	*/
/* Produces new image leaving original untouched.			*/
/************************************************************************/

pbm *
threshold(pgm *image1, int value)
{
	int width = image1 -> width, height = image1 -> height;
	pbm *image2 = new_pbm(width, height);
	gray **I1 = image1 -> image;
	bit **I2 = image2 -> image;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = I1[i][j] < value ? 0 : 1;

	return image2;
}


static term
p_threshold(term goal, term *frame)
{
	term im = check_arg(1, goal, frame, CHUNK, IN);
	term t = check_arg(2, goal, frame, INT, IN);

	return push_image(new_chunk(&grayscale_image, threshold(PGM_IMAGE(im), IVAL(t))));
}


/************************************************************************/
/*		Prolog hook for routine defined in edge.c 		*/
/************************************************************************/

static term
p_sobel(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to display");

	return push_image(new_chunk(&grayscale_image, sobel(PGM_IMAGE(im))));
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
p_pgm_init(void)
{
	new_subr(p_read_pgm,			"read_pgm");
	new_pred(p_write_pgm,			"write_pgm");
	new_pred(p_display_pgm,			"display_pgm");

	new_subr(p_cut_pgm,			"cut_pgm");
	new_subr(p_clip_pgm,			"clip_pgm");
	new_subr(p_band_pass,			"band_pass");
	new_subr(p_band_reject,			"band_reject");
	new_pred(print_hist,			"histogram");
	new_subr(p_histogram_equalisation,	"histogram_equalisation");
	new_subr(p_histogram_equalisation,	"histogram_equalization");
	new_subr(p_stretch_pgm,			"stretch_pgm");
	new_subr(p_threshold,			"threshold");
	new_subr(p_sobel,			"sobel");
}
