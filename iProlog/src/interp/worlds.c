/************************************************************************/
/* Routines implementing simple "worlds" mechanism.			*/
/* A world is associated with an atom.					*/
/* Clauses are added to the atom's proc list.				*/
/* Clauses are inherited through atom's inheritance list.		*/
/************************************************************************/

#include "prolog.h"

/************************************************************************/
/*		Use Prolog to check the conditions in a function	*/
/************************************************************************/

static void
succeed(term *result, term varlist, term *frame)
{
	*result = _true;
}


/************************************************************************/
/*			Apply a given clause to a term			*/
/************************************************************************/

static int
in_world(term goal, term *frame)
{
	term world = check_arg(1, goal, frame, ATOM, IN);
	term t = check_arg(2, goal, frame, ANY, IN);
	term parents = INHERITS(world);

	for (;;)
	{
		term cl;

		for (cl = PROC(world); cl != NULL; cl = NEXT(cl))
		{
			term *old_global = global;
//			term *current_frame = local;
			term current_frame[NVARS(cl)];

//			MAKE_FRAME(NVARS(cl));
			if (unify(t, frame, HEAD(cl), current_frame))
				if (call_prove(BODY(cl), current_frame, _nil, 1, succeed, FALSE) != _nil)
					if (rest_of_clause())
						return FALSE;

			_untrail();
//			local = current_frame;
			global = old_global;
		}
		if (parents == _nil)
			break;
		world = CAR(parents);
		parents = CDR(parents);
	}
	return FALSE;
}


/************************************************************************/
/*			Add a new clause to a world			*/
/************************************************************************/

void
add_to_world(term world, term cl)
{
	if (PROC(world) == NULL)
		PROC(world) = cl;
	else
	{
		term *p;

		for (p = &PROC(world); *p != NULL; p = &NEXT(*p));
		*p = cl;
	}
}


static int
p_add_to_world(term goal, term *frame)
{
	term world = check_arg(1, goal, frame, ATOM, IN);
	term cl = check_arg(2, goal, frame, ANY, IN);
	
	add_to_world(world, mkclause(cl, frame));

	return TRUE;
}


/************************************************************************/
/*			   Module initialisation			*/
/************************************************************************/

void
worlds_init()
{
	defop(800, XFX, new_pred(in_world, "::"));
	new_pred(p_add_to_world, "add_to_world");
}
