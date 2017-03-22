#include "prolog.h"
#include "p_ppm.h"
#include "p_image.h"


/************************************************************************/
/*		Specify public routines for chunk structure		*/
/************************************************************************/

static void free_rgb_image(term b);
static void print_rgb_image(term p);

chunk_spec rgb_image =
{
	"rgb image",
	free_rgb_image,
	print_rgb_image
};


/************************************************************************/
/*		Create ppm structure but not in a chunk			*/
/************************************************************************/

ppm *
new_ppm(int width, int height, pixval maxval)
{
	ppm *p;
	pixel **I;

	p = malloc(sizeof (ppm));
	p -> width = width;
	p -> height = height;
	p -> maxval = maxval;
	p -> image = I = ppm_allocarray(width, height);

	return p;
}

void
free_ppm(ppm *p)
{
	ppm_freearray(p -> image, p -> height);
	free(p);
}

/************************************************************************/
/*			Routines required for chunk spec		*/
/************************************************************************/

static void
free_rgb_image(term b)
{
	free_ppm(PPM_IMAGE(b));
}


static void
print_rgb_image(term p)
{
	fprintf(output,
	 	"|| rgb image (%d x %d) ||",
	 	PPM_IMAGE(p) -> width,
	 	PPM_IMAGE(p) -> height
	 );
}


/************************************************************************/
/* 				Additional utilities			*/
/************************************************************************/

ppm *
read_ppm(char *fname)
{
	ppm *p;
	FILE *fp = fopen(fname, "r");

	if (fp == NULL)
		fail("Could not read PBM file");

	p = malloc(sizeof (ppm));
	p -> image = ppm_readppm(fp, &(p -> width), &(p -> height), &(p -> maxval));
	fclose(fp);
	return p;
}


void
write_ppm(char *fname, ppm *p)
{
	FILE *fp = fopen(fname, "w");

	if (fp == NULL)
		fail("Could not write to PGM file");

	ppm_writeppm(fp, p -> image, p -> width, p -> height, p -> maxval, 0);

	fclose(fp);
}


void
display_ppm(ppm *p)
{
	FILE *send, *receive;

	send_to("xv - ", &send, &receive);
	ppm_writeppm(send, p -> image, p -> width, p -> height, p -> maxval, 0);
	fclose(send);
	fclose(receive);	
}


/************************************************************************/
/* 			Prolog built-ins for utilities			*/
/************************************************************************/

static term
p_read_ppm(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);
	return push_image(new_chunk(&rgb_image, read_ppm(NAME(fname))));
}


static int
p_write_ppm(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);
	term im;

	if (ARITY(goal) == 2)
		im = check_arg(2, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to write");
	
	write_ppm(NAME(fname), PPM_IMAGE(im));

	return TRUE;
}


static int
p_display_ppm(term goal, term *frame)
{
	term im;

	if (ARITY(goal) == 1)
		im = check_arg(1, goal, frame, CHUNK, EVAL);
	else if ((im = current_image()) == NULL)
		fail("No image to display");
	
	display_ppm(PPM_IMAGE(im));

	return TRUE;
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
p_ppm_init(void)
{
	new_subr(p_read_ppm, "read_ppm");
	new_pred(p_write_ppm, "write_ppm");
	new_pred(p_display_ppm, "display_ppm");
}
