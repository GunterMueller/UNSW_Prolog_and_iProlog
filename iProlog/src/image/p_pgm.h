#include "pgm.h"

#define PGM_IMAGE(p) ((pgm *)(CHUNK_DATA(p)))

typedef struct
{
	int width;
	int height;
	gray maxval;
	int *histogram;
	gray **image;
} pgm;


pgm *new_pgm(int width, int height, gray maxval);
void free_pgm(pgm *p);
pgm *read_pgm(char *fname);
void write_pgm(char *fname, pgm *p);
void display_pgm(pgm *p);
