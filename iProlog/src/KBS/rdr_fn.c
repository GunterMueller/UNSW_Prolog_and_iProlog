/************************************************************************/
/*	Perform interactive RDR maintenance on functional terms		*/
/************************************************************************/

#include "prolog.h"
#include "rdr.h"


/************************************************************************/
/* Compare two cases, represented as functional terms and return the	*/
/* difference expressed as a conjunction of conditions.			*/
/************************************************************************/

static void
fn_diff(term old_case, term new_case)
{
	int i;
	term template = ARG(1, HEAD(PROC(ARG(0, old_case))));

	for (i = 1; i <= ARITY(old_case); i++)
	{
		if (unify(ARG(i, old_case), NULL, ARG(i, new_case), NULL))
			continue;

		make_cond(ARG(i, template), ARG(i, old_case), ARG(i, new_case));
	}
}


/************************************************************************/
/*		Perform interactive RDR maintenance on functions	*/
/************************************************************************/

static int
rdr_fn(term old_case, term *new_case, term *whole_rule, term result, term *conclusion)
{
	fprintf(output, "New case: "); print(*new_case);
	fprintf(output, "Old case: "); print(old_case);

	if (TYPE(old_case) != FN || ARG(0, old_case) != ARG(0, *new_case) || ARITY(old_case) != ARITY(*new_case))
		fail("New case and stored case are incompatible");

	if (yes_no(result, "the correct conclusion?"))
		return 1;

	printf("What is the correct conclusion?\n");
	*conclusion = get_atom();

	*whole_rule = ARG(2, GOAL(0, PROC(ARG(0, old_case))));

	fn_diff(old_case, *new_case);

	return 0;
}


static int
p_rdr_fn(term goal, term *frame)
{
	term x = copy(check_arg(1, goal, frame, FN, IN), frame);

	fix_rdr(rdr_fn, x, frame);
	return TRUE;
}
	

/************************************************************************/
/*				Initialisation				*/
/************************************************************************/

void
rdr_fn_init(void)
{
	new_pred(p_rdr_fn, "rdr_fn");
}
