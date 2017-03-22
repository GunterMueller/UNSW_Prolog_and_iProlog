#include "p_pbm.h"
#include "skeleton.h"

/************************************************************************/
/*		Thinning algorithm of Chin et al (1987) 		*/
/************************************************************************/

#define MASK_SIZE	3
#define N_MASKS		8
#define X		-1

static int changed;

static int thinning_mask[N_MASKS][MASK_SIZE][MASK_SIZE] =
{
	{{0, 0, 0},
	{1, 1, 1},
	{X, 1, X}},
	
	{{0, 1, X,},
	{0, 1, 1,},
	{0, 1, X,}},
	
	{{X, 1, X},
	{1, 1, 1},
	{0, 0, 0}},
	
	{{X, 1, 0,},
	{1, 1, 0,},
	{X, 1, 0,}},
	
	{{X, 0, 0},
	{1, 1, 0},
	{X, 1, X}},
	
	{{0, 0, X},
	{0, 1, 1},
	{X, 1, X}},
	
	{{X, 1, X},
	{0, 1, 1},
	{0, 0, X}},
	
	{{X, 1, X},
	{1, 1, 0},
	{X, 0, 0}}
};


static int
restore(bit **I, int i, int j)
{
	return
	I[i-1][j] == 0 &&
	I[i][j]   == 1 &&
	I[i+1][j] == 1 &&
	I[i+2][j] == 0 ||
	I[i][j-1] == 0 &&
	I[i][j]   == 1 &&
	I[i][j+1] == 1 &&
	I[i][j+2] == 0;
}


static int
apply_thinning_mask(int mask[MASK_SIZE][MASK_SIZE], bit **I, int i, int j)
{
	int m, n;

	for (m = 0; m < MASK_SIZE; m++)
		for (n = 0; n < MASK_SIZE; n++)
			if (mask[m][n] != X && mask[m][n] != I[i-1+m][j-1+n])
				return 0;
	return 1;
}


static int
apply_all_masks(pbm *p, int i, int j)
{
	int m;

	if (i < p -> height-2 && j < p -> width-2 && restore(p -> image, i, j))
		return 1;

	for (m = 0; m < N_MASKS; m++)
		if (apply_thinning_mask(thinning_mask[m], p -> image, i, j))
		{
			changed = 1;
			return 0;
		}

			return 1;
}


static pbm *
erode(pbm *image1)
{
	int width = image1 -> width, height = image1 -> height;
	pbm *image2 = new_pbm(width, height);
	bit **I1 = image1 -> image;
	bit **I2 = image2 -> image;
	int i, j;

	for (i = 1; i < height-1; i++)
		for (j = 1; j < width-1; j++)
			if (I1[i][j])
				I2[i][j] = apply_all_masks(image1, i, j);
			else
				I2[i][j] = I1[i][j];
	return image2;
}


pbm *
skeletonise(pbm *I1)
{
	int n = 0;
	pbm *I2;

	do
	{
		changed = 0;
		I2 = erode(I1);
		if (n++ > 0)
			free_pbm(I1);
		I1 = I2;
	} while (changed);

	return I2;
}
