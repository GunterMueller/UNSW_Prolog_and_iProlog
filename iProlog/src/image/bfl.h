#include <stdlib.h>
#include <stdio.h>
#include "p_ppm.h"

#define BFL_WIDTH	176
#define BFL_HEIGHT	144

#define MAX		256
#define MAX_CLASS	9

#define ORANGE		0
#define LIGHT_BLUE	1
#define DARK_GREEN	2
#define YELLOW		3
#define PINK		4
#define BLUE		5
#define RED		6
#define GREEN		7
#define BACKGROUND	8
#define UNCLASSIFIED	9

typedef struct
{
	gray **Y;		/* brightness */
	gray **U;		/* colour */
	gray **V;		/* colour */
	gray **C;		/* class */
} bfl;
