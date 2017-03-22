/************************************************************************/
/*	  Implementation of William Cohen's refinement rules		*/
/************************************************************************/

#include "prolog.h"

#define USED_FLAG	128

#define SET_USED(x)	FLAGS(x) |= USED_FLAG
#define UNSET_USED(x)	FLAGS(x) &= ~USED_FLAG
#define USED(x)		(FLAGS(x) & USED_FLAG)

int match_head(term, term *, term, term *);

extern term *global, trail;

static term _where, _asserting, _check;
term temporary_clauses;


/************************************************************************/
/*		Debugging - print clauses in temporary list		*/
/************************************************************************/

void
print_temp()
{
	term p;

	fprintf(output, "TEMPORARY CLAUSES\n");
	for (p = temporary_clauses; p != _nil; p = CDR(p))
		print(CAR(p));
}


/************************************************************************/
/* There should only be one head rule. Look for it in list of all rules	*/
/************************************************************************/

static term get_head_rule(term rules)
{
	term p;

	for (p = rules; p != NULL; p = NEXT(p))
	{
		term lhs;

		if (ARITY(HEAD(p)) != 2)
			fail("Refinement rule needs left and right hand sides");

		lhs = ARG(1, HEAD(p));

		if (TYPE(lhs) == FN && ARG(0, lhs) == _neck)
			continue;
		else
			return p;
	}
	fail("Missing head rule");
}


/************************************************************************/
/*	Get the clauses of positive examples specified by head rule	*/
/************************************************************************/

static term
get_positive_examples(term head_rule)
{
	term head_template = ARG(1, head_rule);
	term pos;

	switch (TYPE(head_template))
	{
	  case ATOM:
		pos  = PROC(head_template);
		break;
	  case FN:
		pos = PROC(ARG(0, head_template));
		break;
	  default:
		fail("Left hand side of head rule is not a valid term");
	}

	if (pos == NULL)
		fail("No positive examples have been given");

	return pos;
}


/************************************************************************/
/* Hook to call Prolog interpreter when needed to check preconditions	*/
/************************************************************************/

static int
call_prolog(term x, term y, term *f)
{
	term q[3] = {NULL, NULL, NULL};
	term *old_global = global;
	term old_trail = trail;

	q[0] = x;
	q[1] = y;

	if (cond(q, f))
		return TRUE;

	global = old_global;
	untrail(old_trail);
	return FALSE;
}


/************************************************************************/
/* Called when postcondition is being asserted. Puts postconditions in	*/
/* a "temporary" list so that they can be retracted after refinments.	*/
/************************************************************************/

static term
add_to_clause_list(term cl)
{
	temporary_clauses = hcons(cl, temporary_clauses);
	return cl;
}


/************************************************************************/
/*	Retract a postcondition asserted by a refinement rule		*/
/************************************************************************/

static void
retract_temp(term cl)
{
	term *p, reln = HEAD(cl);

	if (TYPE(reln) == FN)
		reln = ARG(0, reln);

	for (p = &PROC(reln); *p != NULL && TYPE(*p) == CLAUSE; p = &NEXT(*p))
		if (*p == cl)
		{
			*p = NEXT(*p);
			free_term(cl);
			return;
		}
}


/************************************************************************/
/*		Called to clean up all asserted postconditions		*/
/************************************************************************/

void
retract_temporary_clauses(void)
{
	term p = temporary_clauses;

	while (p != _nil)
	{
		term q = p;
		p = CDR(p);
		retract_temp(CAR(q));
		dispose(q);
	}
	temporary_clauses = _nil;
}


/************************************************************************/
/* Assert all the postconditions of a satisfied refinement rule.	*/
/* Calls add_to_clause_list to keep list of temporary assertions.	*/
/************************************************************************/

static void
assert_postconditions(term post_conditions, term *frame)
{
	term p;

	if (post_conditions == NULL)
		return;

	for (p = post_conditions; TYPE(p) == FN && ARG(0, p) == _comma; p = ARG(2, p))
	{
/* 		fprintf(output, "Asserting "); rprint(ARG(1, p), frame);
 */		if (! call_prolog(ARG(1, p),  NULL, frame))
			add_clause(add_to_clause_list(mkclause(ARG(1, p), frame)), FALSE);
	}

/* 	fprintf(output, "Asserting "); rprint(p, frame);
 */	if (! call_prolog(p,  NULL, frame))
		add_clause(add_to_clause_list(mkclause(p, frame)), FALSE);
/* 	print_temp();
 */
}


/************************************************************************/
/* Apply refinement rule to create head literal. Create clause struct	*/
/************************************************************************/

static term
create_clause(term head_rule, term *f1, term example, term *f2)
{
	term head_template = ARG(1, head_rule);
	term pre_conditions = ARG(2, head_rule);
	term post_conditions = NULL;

	if (TYPE(pre_conditions) == FN
	&&  ARITY(pre_conditions) == 2
	&&  ARG(0, pre_conditions) == _asserting)
	{
		post_conditions = ARG(2, pre_conditions);
		pre_conditions = ARG(1, pre_conditions);
	}

	if (! match_head(head_template, f1, example, f2))
	{
		print(example);
		fail("Head rule and example are incompatible");
	}

	call_prolog(pre_conditions, NULL, f1);
	assert_postconditions(post_conditions, f1);

	return g_fn2(_neck, head_template, _true);
}


/************************************************************************/
/* Loop to end of comma separated list of literals. Add new one at end	*/
/************************************************************************/

static int
add_literal(term lit, term cl)
{
	term *p = &ARG(2, cl);

/* 	fprintf(output, "Add literal "); print(lit);
	fprintf(output, "cl = "); print(cl);
	fprintf(output, "*p = "); print(*p);
 */
	while (TYPE(*p) == FN && ARITY(*p) == 2 && ARG(0, *p) == _comma)
		if (term_compare(lit, NULL, ARG(1, *p), NULL) == 0)
			return FALSE;
		else
			p = &ARG(2, *p);

	if (*p == _true)
		*p = lit;
	else if (term_compare(lit, NULL, *p, NULL) == 0)
		return FALSE;
	else
		*p = h_fn2(_comma, *p, lit);

/* 	fprintf(output, "Add: ");
	print(lit);
 */	return TRUE;
}

/************************************************************************/
/* When preconditions are satisfied, this is called to add the new	*/
/* literal to the clause and to call the routine to assert the		*/
/* postconditions							*/
/************************************************************************/

static term g_body_template, g_post_conditions, g_cl;

static void
activate_rule(term *result, term varlist, term *frame)
{
	if (add_literal(make(g_body_template, frame), g_cl))
	{
		assert_postconditions(g_post_conditions, frame);
		*result = _true;
	}
	else
		*result = _nil;
}


/************************************************************************/
/*	Applies a refinement rule to create a new body literal		*/
/************************************************************************/

static int
try_body_rule(term rule, term cl, term *f)
{
	term body_template = ARG(1, ARG(1, rule));
	term pre_conditions = ARG(2, rule);
	term post_conditions = NULL;
	term q[3] = {NULL, NULL, NULL};

/* 	fprintf(output, "Rule: "); print(rule);
 */
	if (TYPE(pre_conditions) == FN
	&&  ARITY(pre_conditions) == 2
	&&  ARG(0, pre_conditions) == _asserting)
	{
		post_conditions = ARG(2, pre_conditions);
		pre_conditions = ARG(1, pre_conditions);
	}

	q[0] = pre_conditions;
	q[1] = body_template;

	g_body_template = body_template;
	g_post_conditions = post_conditions;
	g_cl = cl;

	if (call_prove(q, f, _nil, -1, activate_rule, TRUE) != _nil)
		return TRUE;

	return FALSE;
}


/************************************************************************/
/* The real top-level routine for saturation. Keep looping through	*/
/* refinement rules until no rule can create a new literal.		*/
/************************************************************************/

static void
create_body(term rules, term cl, term *f)
{
	term p;
	int still_looping;

	do
	{
		still_looping = FALSE;
		for (p = rules; p != NULL; p = NEXT(p))
			if (ARG(0, ARG(1, HEAD(p))) == _neck)
				if (try_body_rule(copy(HEAD(p), f), cl, f))
					still_looping = TRUE;
	}
	while (still_looping);
}


/************************************************************************/
/* REFINE built-in loops through each positive example creating a	*/
/* saturated clause. The new clauses are collected into a frame.	*/
/************************************************************************/

static term
refine(term goal, term *frame)
{
	term head_rule, pos;
	term rules = PROC(_where);
	term rule_list, *p = &rule_list;

	if (rules == NULL)
		fail("No refinement rules have been given");

	temporary_clauses = _nil;
	head_rule = get_head_rule(rules);

	{
		term f1[NVARS(head_rule)];
		head_rule = copy(HEAD(head_rule), f1);

		for (pos = get_positive_examples(head_rule); pos != NULL; pos = NEXT(pos))
		{
			term *old_global = global;
			term old_trail = trail;
			term f2[NVARS(pos)];
			term example = copy(HEAD(pos), f2);
			term cl = create_clause(head_rule, f1, example, f2);

			create_body(rules, cl, f1);

			*p = mkclause(cl, f1);
			p = &NEXT(*p);

			retract_temporary_clauses();
			untrail(old_trail);
			global = old_global;
		}
	}

	return build_plist("refine",
		    "creator",	make(goal, frame),
		    "date",	intern(date_time()),
		    "rule",	rule_list,
		    NULL
		);
}


/************************************************************************/
/*			Saturate a single example			*/
/************************************************************************/

term
saturate(term example)
{
	term head_rule;
	term cl, rval;
	term old_trail = trail;
	term *old_global = global;
	term rules = PROC(_where);

	if (rules == NULL)
		fail("No refinement rules have been given");

	temporary_clauses = _nil;
	head_rule = get_head_rule(rules);

	{
		term f1[NVARS(head_rule)];
		term f2[NVARS(example)];

		head_rule = copy(HEAD(head_rule), f1);
		example = copy(HEAD(example), f2);

		cl = create_clause(head_rule, f1, example, f2);
		create_body(rules, cl, f1);
		rval = mkclause(cl, f1);
	}

	retract_temporary_clauses();
	untrail(old_trail);
	global = old_global;
	
	return rval;
}


/************************************************************************/
/* SATURATE built-in loops through each positive example creating a	*/
/* saturated clause. The new clauses are collected into a frame.	*/
/************************************************************************/

static term
saturate_reln(term list)
{
	term e, rule_list, *p = &rule_list;

	while (list != _nil)
	{
		if (TYPE(list) != LIST && TYPE(CAR(list)) != ATOM)
			fail("Argument must be a list of relation names");

		for (e = PROC(CAR(list)); e != NULL; e = NEXT(e))
		{
			fprintf(output, "-----------------\n");
			print(HEAD(e));
			fflush(output);
			*p = saturate(e);
			list_proc(*p);
			fflush(output);
			p = &NEXT(*p);
		}

		list = CDR(list);
		DEREF(list);
	}

	return rule_list;
}

static term
saturate_all_examples(term goal, term *frame)
{
	term pos = check_arg(1, goal, frame, LIST, IN);
	term neg = check_arg(2, goal, frame, LIST, IN);

	term f =  build_plist("saturate",
			"creator",	make(goal, frame),
			"date",		intern(date_time()),
			"pos",		saturate_reln(pos),
			"neg",		saturate_reln(neg),
			NULL
	);

	return f;
}


/************************************************************************/
/*	Called from LGG to check if a new lgg obeys refinement rules	*/
/************************************************************************/

int
refine_lgg(term literal)
{
	term p;
	term old_trail = trail;
	term *old_global = global;
	int found_template = FALSE;

	for (p = PROC(_where); p != NULL; p = NEXT(p))
	{
		term template, pre_conditions, post_conditions;
		term frame[NVARS(p)];

		if (ARG(0, ARG(1, HEAD(p))) == _neck)
			template = ARG(1, ARG(1, HEAD(p)));
		else
			template = ARG(1, HEAD(p));

		pre_conditions = ARG(2, HEAD(p));
		if (TYPE(pre_conditions) == FN
		&&  ARITY(pre_conditions) == 2
		&&  ARG(0, pre_conditions) == _asserting)
		{
			post_conditions = ARG(2, pre_conditions);
			pre_conditions = ARG(1, pre_conditions);
		}
		else
			post_conditions = NULL;

		if (unify(template, frame, literal, NULL))
		{
			term q[4] = {NULL, NULL, NULL, NULL};

			found_template = TRUE;
			if (TYPE(pre_conditions) == FN
			&&  ARG(0, pre_conditions) == _check)
			{
				q[1] = copy(ARG(1, pre_conditions), frame);
				q[0] = copy(ARG(2, pre_conditions), frame);
				q[2] = _bang;
			}
			else
			{
				q[0] = pre_conditions;
				q[1] = _bang;
			}
			if (trap_cond(q, frame))
			{
				cache_predicate(add_to_clause_list(mkclause(literal, NULL)));
				assert_postconditions(post_conditions, frame);

				global = old_global;
				untrail(old_trail);
				return TRUE;
			}
		}
		global = old_global;
		untrail(old_trail);
	}
	return (! found_template);
}


/************************************************************************/
/*			   Initialise Module				*/
/************************************************************************/

void
refine_init(void)
{
	_check = intern("checking");

	defop(1160, XFX, _where = intern("where"));
	defop(1150, XFX, _asserting = intern("asserting"));

	new_subr(refine, "refine");
	new_subr(saturate_all_examples, "saturate");
}
