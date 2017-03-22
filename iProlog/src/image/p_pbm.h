#include "pbm.h"

#define PBM_IMAGE(p) ((pbm *)(CHUNK_DATA(p)))

typedef struct
{
	int width;
	int height;
	bit **image;
} pbm;

pbm *new_pbm(int width, int height);
void free_pbm(pbm *p);
pbm *read_pbm(char *fname);
void write_pbm(char *fname, pbm *p);
void display_pbm(pbm *p);
