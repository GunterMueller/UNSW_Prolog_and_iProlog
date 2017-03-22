#include "prolog.h"
#include "p_image.h"

static term _image_stack;

/************************************************************************/
/* The results of image operations are stored in the "image stack".	*/
/* These routines are for basic stack manipulation.			*/
/************************************************************************/

term
push_image(term image)
{
	add_clause(new_unit(h_fn1(_image_stack, image)), TRUE);
	return image;
}


int
pop_image(void)
{
	if (PROC(_image_stack) == NULL)
		return FALSE;
	else
	{
		term p = PROC(_image_stack);
		PROC(_image_stack) = NEXT(p);
		free_term(p);
		return TRUE;
	}
}


term
current_image(void)
{
	if (PROC(_image_stack) == NULL)
		return NULL;
	return  ARG(1, HEAD(PROC(_image_stack)));
}


/************************************************************************/
/* The results of image operations are stored in the "image stack".	*/
/* These routines are for basic stack manipulation.			*/
/************************************************************************/

static int
p_push_image(term goal, term *frame)
{
	push_image(check_arg(1, goal, frame, CHUNK, EVAL));
	return TRUE;
}


static int
p_pop_image(term goal, term *frame)
{
	return pop_image();
}


static term
p_current_image(term goal, term *frame)
{
	term rval = current_image();

	if (rval == NULL)
		fail("No current images");
	return  rval;
}


/************************************************************************/
/*			Module Initialisation				*/
/************************************************************************/

void
image_init(void)
{
	void p_pbm_init(), p_pgm_init(), p_ppm_init(), bfl_init();

	p_pbm_init();
	p_pgm_init();
	p_ppm_init();
	bfl_init();
	
	_image_stack = intern("image_stack");

	new_pred(p_push_image,		"push_image");
	new_pred(p_pop_image,		"pop_image");
	new_subr(p_current_image,	"current_image");
}
