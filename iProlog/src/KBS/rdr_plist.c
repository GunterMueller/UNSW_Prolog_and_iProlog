/************************************************************************/
/*	Perform interactive RDR maintenance on frame rules		*/
/************************************************************************/

#include "prolog.h"
#include "rdr.h"
#include "rdr_plist.h"


/************************************************************************/
/*		   The difference between property lists		*/
/************************************************************************/

void
plist_diff(term rule_slot, term old_case, term new_case)
{
	term p, q;

	for (p = new_case; p != _nil; p = CDR(p))
	{
		if (CAR(CAR(p)) == rule_slot)
			continue;

		for (q = old_case; q != _nil; q = CDR(q))
			if (CAR(CAR(p)) == CAR(CAR(q)))
			{
				if (! unify(CDR(CAR(q)), NULL, CDR(CAR(p)), NULL))
					make_cond(CAR(CAR(p)), CDR(CAR(q)), CDR(CAR(p)));
				break;
			}

		if (q == _nil)
			add_condition(g_fn2(_eq, CAR(CAR(p)), CDR(CAR(p))));
	}
	for (q = old_case; q != _nil; q = CDR(q))
	{
		if (CAR(CAR(q)) == rule_slot)
			continue;

		for (p = new_case; p != _nil; p = CDR(p))
			if (CAR(CAR(p)) == CAR(CAR(q)))
				break;
		if (p == _nil)
			add_condition(g_fn2(_ne, CAR(CAR(q)), CDR(CAR(q))));
	}
}


/************************************************************************/
/*	Perform interactive RDR maintenance on property lists		*/
/************************************************************************/

static int
rdr_frame(term old_case, term *new_case, term *whole_rule, term result, term *conclusion)
{
	extern term get_facet();
	term rule_slot = ARG(0, *new_case);
	*new_case = ARG(1, *new_case);

	fprintf(output, "New case: "); print(PLIST(*new_case));
	fprintf(output, "Old case: "); print(PLIST(old_case));

	if (yes_no(result, "the correct conclusion?"))
		return 1;

	printf("What is the correct conclusion?\n");
	*conclusion = get_atom();

	*whole_rule = get_facet(CAR(INHERITS(old_case)), rule_slot, intern("if_needed"));

	plist_diff(rule_slot, PLIST(old_case), PLIST(*new_case));

	return 0;
}


static int
p_rdr_frame(term goal, term *frame)
{
	term x = copy(check_arg(1, goal, frame, FN, IN), frame);

	fix_rdr(rdr_frame, x, frame);
	return TRUE;
}

	

/************************************************************************/
/*				Initialisation				*/
/************************************************************************/

void
rdr_frame_init(void)
{
	new_pred(p_rdr_frame, "rdr_frame");
}
