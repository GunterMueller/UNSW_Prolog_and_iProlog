/************************************************************************/
/*	Manipulate a robocup BFL file					*/
/* 	176x144 image							*/
/* 	1 row each for Y, U, V and colour class				*/
/************************************************************************/

#include <math.h>
#include "prolog.h"
#include "p_pgm.h"
#include "bfl.h"


static pixel map[MAX_CLASS];


/************************************************************************/
/*			Create a new bfl structure			*/
/************************************************************************/

bfl *
new_bfl(void)
{
	bfl *p;

	p = malloc(sizeof (bfl));
	p -> Y = pgm_allocarray(BFL_WIDTH, BFL_HEIGHT);
	p -> U = pgm_allocarray(BFL_WIDTH, BFL_HEIGHT);
	p -> V = pgm_allocarray(BFL_WIDTH, BFL_HEIGHT);
	p -> C = pgm_allocarray(BFL_WIDTH, BFL_HEIGHT);

	return p;
}


/************************************************************************/
/*			Read a robocup BFL file				*/
/************************************************************************/

static void
read_row(FILE *fp, gray *band)
{
	unsigned char buf[BFL_WIDTH];
	int j;

	fread(buf, BFL_WIDTH, 1, fp);
	for (j = 0; j < BFL_WIDTH; j++)
		band[j] = (gray)(buf[j]);
}


bfl *
read_bfl(char *fname)
{
	FILE *fp;
	bfl *p;
	int i;

	if ((fp = fopen(fname, "r")) == NULL)
		fail("Could not read BFL file");

	p = new_bfl();

	for (i = 0; i < BFL_HEIGHT; i++)
	{
		read_row(fp, p -> Y[i]);
		read_row(fp, p -> U[i]);
		read_row(fp, p -> V[i]);
		read_row(fp, p -> C[i]);
	}

	fclose(fp);
	return p;
}


/************************************************************************/
/*			Convert bfl image to ppm format			*/
/************************************************************************/

ppm *
bfltoppm(bfl *p)
{
	ppm	*rgb = new_ppm(BFL_WIDTH, BFL_HEIGHT, 255);
	gray	**Y = p -> Y, **U = p -> U, **V = p -> V;
	pixel	**I = rgb -> image;
	int	i, j;

	for (i = 0; i < BFL_HEIGHT; i++)
		for (j = 0; j < BFL_WIDTH; j++)
		{
			pixval R, G, B;
			int y = Y[i][j] - 16;
			int u = U[i][j] - 128;
			int v = V[i][j] - 128;

			R = (pixval)((1.164 * y) + (1.596 * u));
			G = (pixval)((1.164 * y) - (0.813 * u) - (0.391 * v));
			B = (pixval)((1.164 * y) + (1.596 * v));

			if (R < 0) R = 0; else if (R > MAX) R = MAX;
			if (G < 0) G = 0; else if (G > MAX) G = MAX;
			if (B < 0) B = 0; else if (B > MAX) B = MAX;

			PPM_ASSIGN(I[i][j], R, G, B);
		}

	return rgb;
}


/************************************************************************/
/*			Map C-plane to an rgb image			*/
/************************************************************************/

ppm *
map_cplane(bfl *p)
{
	gray	**C = p -> C;
	ppm	*cplane = new_ppm(BFL_WIDTH, BFL_HEIGHT, 255);
	pixel	**rgb = cplane -> image;
	int	i, j;

	for (i = 0; i < BFL_HEIGHT; i++)
		for (j = 0; j < BFL_WIDTH; j++)
			rgb[i][j] = map[C[i][j]];
	return cplane;
}


/************************************************************************/
/*		Display a BFL file as four images for each plane	*/
/************************************************************************/

void
display_bfl(bfl *b)
{
	FILE *send, *receive;
	ppm *rgb = bfltoppm(b);
	ppm *cplane = map_cplane(b);

	send_to("xv - ", &send, &receive);
	ppm_writeppm(send, rgb -> image, BFL_WIDTH, BFL_HEIGHT, 255, 0);
	fclose(send);
	fclose(receive);

	send_to("xv - ", &send, &receive);
	pgm_writepgm(send, b -> Y, BFL_WIDTH, BFL_HEIGHT, 255, 0);
	fclose(send);
	fclose(receive);

	send_to("xv - ", &send, &receive);
	pgm_writepgm(send, b -> U, BFL_WIDTH, BFL_HEIGHT, 255, 0);
	fclose(send);
	fclose(receive);

	send_to("xv - ", &send, &receive);
	pgm_writepgm(send, b -> V, BFL_WIDTH, BFL_HEIGHT, 255, 0);
	fclose(send);
	fclose(receive);

	send_to("xv - ", &send, &receive);
	ppm_writeppm(send, cplane -> image, BFL_WIDTH, BFL_HEIGHT, 255, 0);
	fclose(send);
	fclose(receive);

	free_ppm(rgb);
	free_ppm(cplane);
}


/************************************************************************/
/*			    Output a data file				*/
/************************************************************************/

static void
dump_data(bfl *image, char *fname)
{
	gray	**Y = image -> Y, **U = image -> U, **V = image -> V, **C = image -> C;
	FILE *fp = fopen(fname, "w");
	int i, j;

	for (i = 0; i < BFL_HEIGHT; i++)
		for (j = 0; j < BFL_WIDTH; j++)
			fprintf(fp, "%d %d %d %d\n", Y[i][j], U[i][j], V[i][j], C[i][j]);
	fclose(fp);
}


/************************************************************************/
/*			Prolog hook to read BFL file			*/
/************************************************************************/

static int
p_read_bfl(term goal, term *frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);
	bfl *p = read_bfl(NAME(fname));
	display_bfl(p);
//	dump_data(p, "../fred");
	return TRUE;
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
bfl_init(void)
{
	int argc_dummy = 0;
	char *argv_dummy[] = {"prolog", NULL};

	ppm_init(&argc_dummy, argv_dummy);
	new_pred(p_read_bfl, "read_bfl");

	map[ORANGE]		= ppm_parsecolor("orange", MAX);
	map[LIGHT_BLUE]		= ppm_parsecolor("lightblue", MAX);
	map[DARK_GREEN]		= ppm_parsecolor("darkgreen", MAX);
	map[YELLOW]		= ppm_parsecolor("yellow", MAX);
	map[PINK]		= ppm_parsecolor("pink", MAX);
	map[BLUE]		= ppm_parsecolor("blue", MAX);
	map[RED]		= ppm_parsecolor("red", MAX);
	map[GREEN]		= ppm_parsecolor("green", MAX);
	map[BACKGROUND]		= ppm_parsecolor("lightgrey", MAX);	
	map[UNCLASSIFIED]	= ppm_parsecolor("white", MAX);
}

