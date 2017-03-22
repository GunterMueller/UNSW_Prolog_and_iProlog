#include "math.h"
#include "p_pgm.h"
#include "edge.h"


typedef struct
{
	int width;
	int height;
	float *buf;
	float **image;		/* initialised to give 2D access to buf */
} float_image;


static float_image *
new_float_image(int width, int height)
{
	float_image *p;
	int i;
	float *buf, **I;

	p = malloc(sizeof (float_image));
	p -> width = width;
	p -> height = height;

	p -> buf = buf = calloc((size_t)(width * height), sizeof(float));
	p -> image = I = calloc((size_t)(height), sizeof (float *));

	for (i = 0; i < height; i++)
		I[i] = &(buf[i * width]);

	return p;
}


/************************************************************************/
/* 			     Sobel edge detector			*/
/************************************************************************/

#define MASK_SIZE	3

static int sobel_row_mask[MASK_SIZE][MASK_SIZE] =
{
	{-1, -2, -1},
	{ 0,  0,  0},
	{ 1,  2,  1}
};

static int sobel_col_mask[MASK_SIZE][MASK_SIZE] =
{
	{-1,  0,  1}, 
	{-2,  0,  2},
	{-1,  0,  1}
};


static int
apply_sobel_mask(int mask[MASK_SIZE][MASK_SIZE], pgm *im, int i, int j)
{
	gray **I = im -> image;
	int m, n, sum = 0;

	for (m = 0; m < MASK_SIZE; m++)
		for (n = 0; n < MASK_SIZE; n++)
			sum += mask[m][n] * I[i-1+m][j-1+n];
	return sum;
}


static float
sobel_edge_magnitude(pgm *im, int i, int j)
{
	int s1 = apply_sobel_mask(sobel_row_mask, im, i, j);
	int s2 = apply_sobel_mask(sobel_col_mask, im, i, j);

	return sqrt(s1*s1 + s2*s2);
}


static pgm *
remap(float_image *image1, gray gray_max, float float_max)
{
	int width = image1 -> width, height = image1 -> height;
	pgm *image2 = new_pgm(width, height, gray_max);
	float scale_factor = gray_max/float_max;
	float **I1 = image1 -> image;
	gray **I2 = image2 -> image;
	int i, j;

	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
			I2[i][j] = (gray)(I1[i][j] * scale_factor);

	return image2;
}


pgm *
sobel(pgm *image1)
{
	int width = image1 -> width, height = image1 -> height;
	float_image *image2 = new_float_image(width, height);
	float **I2 = image2 -> image;
	gray gray_max = image1 -> maxval;
	float float_max = 0;
	int i, j;

	for (i = 1; i < height - 1; i++)
		for (j = 1; j < width - 1; j++)
		{
			float mag = I2[i][j] = sobel_edge_magnitude(image1, i, j);

			if (mag > float_max)
				float_max = mag;
		}

	return remap(image2, gray_max, float_max);
}
