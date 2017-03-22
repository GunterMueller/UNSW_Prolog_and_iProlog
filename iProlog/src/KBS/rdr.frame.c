/************************************************************************/
/*		An RDR interpreter and maintenance system		*/
/************************************************************************/

#include "prolog.h"

#define	COND(x)		ARG(1, x)
#define CONCL(x)	ARG(2, x)
#define	CASE(x)		ARG(3, x)
#define EXCEPT(x)	ARG(4, x)
#define ALT(x)		ARG(5, x)

#define NONVAR(x) (x != _anon && TYPE(x) != REF && TYPE(x) != ANON)

extern term varlist, new_var();
extern term *global;

term   _rdr, _if, _then, _else, _except, _because, _and, _or;
static term _gt, _lt, _eq, _ne, _dont_know;
static term _of, _isa, _value, _this, _object;

static int  was_true;
static term anon_struct;
static term last_rule = NULL;
static term last_case = NULL;

/************************************************************************/
/*				Make a new RDR				*/
/************************************************************************/

static term
new_rule(term cond, term concl, term because, term except, term alt)
{
	term rval = new_h_fn(5);

	ARG(0, rval) = _if;
	COND(rval) = cond;
	CONCL(rval) = concl;
	CASE(rval) = because;
	EXCEPT(rval) = except;
	ALT(rval) = alt;

	return rval;
}


/************************************************************************/
/*		Evaluate a conjunction of expressions			*/
/************************************************************************/

static term
and(term expr, term *frame)
{
	term orig = expr;

	while (TYPE(expr) == FN && ARG(0, expr) == _and)
	{
		if (eval(ARG(1, expr), frame) != _true)
			return(_false);
		expr = ARG(2, expr);
	}
	return(orig == expr ? orig : eval(expr, frame));
}


/************************************************************************/
/*		Evaluate a disjunction of expressions			*/
/************************************************************************/

static term
or(term expr, term *frame)
{
	term orig = expr;

	while (TYPE(expr) == FN && ARG(0, expr) == _or)
	{
		if (eval(ARG(1, expr), frame) == _true)
			return(_true);
		expr = ARG(2, expr);
	}
	return(orig == expr ? orig : eval(expr, frame));
}


/************************************************************************/
/*		Evaluate an "if-then-else" expression			*/
/************************************************************************/

static term
if_expr(term expr, term *frame)
{
	last_rule = expr;

	if (was_true = (eval(COND(expr), frame) == _true))
	{
		if (NONVAR(EXCEPT(expr)))
		{
			term rval = eval(EXCEPT(expr), frame);

			if (rval != NULL)
				return rval;
		}

		last_case = CASE(expr);
		return progn(CONCL(expr), frame);
	}

	if (NONVAR(ALT(expr)))
		return progn(ALT(expr), frame);

	return NULL;
}


/************************************************************************/
/*			  Add a new rule to an RDR			*/
/************************************************************************/

static void
add_rdr(term rule, term *frame)
{
	if (was_true)
		EXCEPT(last_rule) = make(rule, frame);
	else
		ALT(last_rule) = make(rule, frame);
}


static int
p_add_rdr(term goal, term *frame)
{
	add_rdr(ARG(1, goal), frame);

	return(TRUE);
}


/************************************************************************/
/*	Find the corner stone case for the last use of an RDR		*/
/************************************************************************/

static int
corner_stone_case(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, FN, OUT);

	if (last_case == NULL)
		fail("No rules have been executed yet");

	return unify(x, frame, last_case, frame);
}


/************************************************************************/
/*			Make a new condition for an RDR			*/
/************************************************************************/

static term
mkbound(term free)
{
	term rval;

	if (TYPE(free) != FREE)
		return(free);

	rval = new_ref();
	varlist = gcons(gcons(new_var(BOUND, OFFSET(free), PNAME(free)), rval), varlist);
	return(rval);
}


static int
yes_no(term x, char *question)
{
	term ans;

	fputs("Is ", output);
	prin(x);
	fputc(' ', output);
	fputs(question, output);
	ans = get_atom();
	return(NAME(ans)[0] != 'n');
}


static term
make_cond(term var, term old, term new)
{
	term rval, rval1 = NULL, mid_point;

	switch (TYPE(old))
	{
	case INT:
		switch (TYPE(new))
		{
		case INT:
			mid_point = new_real((IVAL(old) + IVAL(new))/2.0);

			if (IVAL(old) < IVAL(new))
				rval =  g_fn2(_gt, var, mid_point);
			else if (IVAL(old) > IVAL(new))
				rval = g_fn2(_lt, var, mid_point);
			break;
		case REAL:
			mid_point = new_real((IVAL(old) + RVAL(new))/2.0);

			if (IVAL(old) < RVAL(new))
				rval =  g_fn2(_gt, var, mid_point);
			else if (IVAL(old) > RVAL(new))
				rval =  g_fn2(_lt, var, mid_point);
			break;
		default:
			rval = g_fn2(_ne, var, old);
			break;
		}
		break;
	case REAL:
		switch (TYPE(new))
		{
		case INT:
			mid_point = new_real((RVAL(old) + IVAL(new))/2.0);

			if (RVAL(old) < IVAL(new))
				rval = g_fn2(_gt, var, mid_point);
			else if (RVAL(old) > IVAL(new))
				rval =  g_fn2(_lt, var, mid_point);
			break;
		case REAL:
			mid_point = new_real((RVAL(old) + RVAL(new))/2.0);

			if (RVAL(old) < RVAL(new))
				rval =  g_fn2(_gt, var, mid_point);
			else if (RVAL(old) > RVAL(new))
				rval =  g_fn2(_lt, var, mid_point);
			break;
		default:
			rval = g_fn2(_ne, var, old);
			break;
		}
		break;
	default:
		rval = g_fn2(_eq, var, new);
		rval1 = g_fn2(_ne, var, old);
	}
	
	if (yes_no(rval, "correct"))
	{
		ARG(1, rval) = mkbound(var);
		return(rval);
	}
	
	if (rval1 && old != _dont_know && yes_no(rval1, "correct"))
	{
		ARG(1, rval1) = mkbound(var);
		return(rval1);
	}

	return(NULL);
}


/************************************************************************/
/* Compare two cases, represented as functional terms and return the	*/
/* difference expressed as a conjunction of conditions.			*/
/************************************************************************/

static term
fn_diff(term old_case, term new_case)
{
	int i;
	term cond, conj = NULL, *p = &conj;
	term template = ARG(1, HEAD(PROC(ARG(0, old_case))));

	for (i = 1; i <= ARITY(old_case); i++)
	{
		if (unify(ARG(i, old_case), NULL, ARG(i, new_case), NULL))
			continue;

		cond = make_cond(ARG(i, template), ARG(i, old_case), ARG(i, new_case));

		if (cond != NULL)
			if (*p == NULL)
				*p = cond;
			else
			{
				*p = g_fn2(_and, *p, cond);
				p = &ARG(2, *p);
			}
	}

	if (conj == NULL)
		fail("Conclusion was incorrect, but no distinguishing features were found");
	return(conj);
}


/************************************************************************/
/*		Perform interactive RDR maintenance on functions	*/
/************************************************************************/

static int
rdr_fn(term goal, term *frame)
{
	term new_case = copy(check_arg(1, goal, frame, FN, IN), frame);
	term result = eval(new_case, frame);
	term conclusion;
	term *old_global = global;

	if (! NONVAR(CASE(last_rule)))
		fail("RDR must contain cases to perform maintenance");

	if (TYPE(last_case) != FN || ARG(0, last_case) != ARG(0, new_case) || ARITY(last_case) != ARITY(new_case))
		fail("New case and stored case are incompatible");

	fprintf(output, "New case: "); print(new_case);
	fprintf(output, "Old case: "); print(last_case);
	if (yes_no(result, "the correct conclusion"))
		return TRUE;
	varlist = _nil;

	printf("What is the correct conclusion");
	conclusion = get_atom();
	add_rdr(new_rule(fn_diff(last_case, new_case), conclusion, new_case, anon_struct, anon_struct), frame);
	global = old_global;
	return TRUE;
}

/************************************************************************/
/*			    RDR's for property lists			*/
/************************************************************************/

static term
plist_diff(term old_case, term new_case)
{
	term p, q;
	term conj = NULL, *x = &conj;

	for (p = new_case; p != _nil; p = CDR(p))
	{
		term cond = NULL;

		for (q = old_case; q != _nil; q = CDR(q))
			if (CAR(CAR(p)) == CAR(CAR(q)))
			{
				if (! unify(CDR(CAR(q)), NULL, CDR(CAR(p)), NULL))
					cond = make_cond(CAR(CAR(p)), CDR(CAR(q)), CDR(CAR(p)));
				break;
			}
		if (q == _nil)
		{
			cond = g_fn2(_eq, CAR(CAR(p)), CDR(CAR(p)));
			if (! yes_no(cond, "correct"))
				cond = NULL;
		}
		if (cond != NULL)
			if (*x == NULL)
				*x = cond;
			else
			{
				*x = g_fn2(_and, *x, cond);
				x = &ARG(2, *x);
			}
	}
	for (q = old_case; q != _nil; q = CDR(q))
	{
		term cond = NULL;

		for (p = new_case; p != _nil; p = CDR(p))
			if (CAR(CAR(p)) == CAR(CAR(q)))
				break;
		if (p == _nil)
		{
			cond = g_fn2(_ne, CAR(CAR(q)), CDR(CAR(q)));
			if (! yes_no(cond, "correct"))
				cond = NULL;
		}
		if (cond != NULL)
			if (*x == NULL)
				*x = cond;
			else
			{
				*x = g_fn2(_and, *x, cond);
				x = &ARG(2, *x);
			}
	}

	if (conj == NULL)
		fail("Conclusion was incorrect, but no distinguishing features were found");
	return(conj);
}
	

/************************************************************************/
/*	Perform interactive RDR maintenance on property lists		*/
/************************************************************************/

static int
rdr_plist(term goal, term *frame)
{
	term new_case = copy(check_arg(1, goal, frame, FN, IN), frame);
	term result = eval(new_case, frame);
	term conclusion, old_case, rule;
	term generic, rule_slot;
	term *old_global = global;

	rule_slot = ARG(0, new_case);
	new_case = ARG(1, new_case);

	conclusion = result;
	old_case = last_case;
	print(old_case);

	fprintf(output, "New case: "); print(PLIST(new_case));
	fprintf(output, "Old case: "); print(PLIST(old_case));
	if (yes_no(conclusion, "the correct conclusion"))
		return(TRUE);
	varlist = _nil;

	printf("What is the correct conclusion");
	conclusion = get_atom();
	rule = new_rule(
		make(plist_diff(PLIST(old_case), PLIST(new_case)), frame),
		conclusion,
		new_case,
		_anon,
		_anon
	);
	print(rule);
	if (was_true)
		EXCEPT(last_rule) = rule;
	else
		ALT(last_rule) = rule;
	global = old_global;
	return(TRUE);
}


/************************************************************************/
/*				RDR's for frames			*/
/************************************************************************/

#define TABLE_SIZE 100

static struct
{
	term slot;
	term old_value;
	term new_value;
} table[TABLE_SIZE];

static int next_hole;


/************************************************************************/
/* Collect correspoding slot values into a table for later comparison	*/
/************************************************************************/

static void
insert_in_table(term slot, term old_value, term new_value)
{
	int i;

	for (i = 0; i < next_hole; i++)
		if (slot == table[i].slot)
			break;

	if (i == next_hole)
		if (next_hole == TABLE_SIZE)
			fail("Too many slots while finding frame differences");
		else
			next_hole++;

	table[i].slot = slot;
	if (old_value != NULL)
		table[i].old_value = old_value;
	if (new_value != NULL)
		table[i].new_value = new_value;
}	

static term
collect_slots(term old_case, term new_case)
{
	term p;

	for (p = PROC(_value); p != NULL; p = NEXT(p))
	{
		term x = HEAD(p);

		if (ARG(1, x) == old_case)
			insert_in_table(ARG(2, x), ARG(3, x), NULL);
		else if (ARG(1, x) == new_case)
			insert_in_table(ARG(2, x), NULL, ARG(3, x));
	}
}


static term
frame_diff(term old_case, term new_case)
{
	int i;
	term cond, conj = NULL, *p = &conj;

	next_hole = 0;
	collect_slots(old_case, new_case);

	for (i = 0; i < next_hole; i++)
	{
		term var = g_fn2(_of, table[i].slot, g_fn1(_this, _object));

		if (table[i].old_value == NULL && table[i].new_value != NULL)
		{
			cond = g_fn2(_eq, var, table[i].new_value);
			if (! yes_no(cond, "correct"))
				cond = NULL;
		}
		else if (table[i].old_value != NULL && table[i].new_value == NULL)
		{
			cond = g_fn2(_ne, var, table[i].old_value);
			if (! yes_no(cond, "correct"))
				cond = NULL;
		}
		else if (unify(table[i].old_value, NULL, table[i].new_value, NULL))
			continue;
		else
			cond = make_cond(var, table[i].old_value, table[i].new_value);

		if (cond != NULL)
			if (*p == NULL)
				*p = cond;
			else
			{
				*p = g_fn2(_and, *p, cond);
				p = &ARG(2, *p);
			}
	}

	if (conj == NULL)
		fail("Conclusion was incorrect, but no distinguishing features were found");
	return(conj);
}
	

/************************************************************************/
/*		Perform interactive RDR maintenance on frames		*/
/************************************************************************/

static int
rdr_frame(term goal, term *frame)
{
	term new_case = copy(check_arg(1, goal, frame, FN, IN), frame);
	term result = eval(new_case, frame);
	term conclusion, old_case, rule;
	term generic, rule_slot;
	term *old_global = global;

	if (ARG(0, new_case) != _of)
		fail("Argument should look like '<slot> of <frame>'");

	rule_slot = ARG(1, new_case);
	new_case = ARG(2, new_case);

	if (TYPE(result) != FN || ARG(0, result) != _because)
		fail("RDR must contain cases to perform maintenance");

	conclusion = ARG(1, result);
	old_case = ARG(2, result);

	fprintf(output, "New case: "); print_frame(new_case);
	fprintf(output, "Old case: "); print_frame(old_case);
	if (yes_no(conclusion, "the correct conclusion"))
		return(TRUE);
	varlist = _nil;

	printf("What is the correct conclusion");
	conclusion = get_atom();
	rule = h_fn2(_because, conclusion, new_case);
	rule = h_fn2(_then, frame_diff(old_case, new_case), rule);
	rule = h_fn1(_if, rule);
	add_rdr(rule, frame);
	global = old_global;
	return(TRUE);
}


/************************************************************************/
/*			Perform interactive RDR maintenance		*/
/************************************************************************/

static int
rdr(term goal, term *frame)
{
	term x = copy(check_arg(1, goal, frame, FN, IN), frame);

	if (ARG(0, x) == _of)
		return(rdr_frame(goal, frame));
	else if (TYPE(PROC(ARG(0, x))) == SUBR)
		return(rdr_plist(goal, frame));
	else
		return(rdr_fn(goal, frame));
}
	

/************************************************************************/
/*				Read RDR				*/
/************************************************************************/

static term
if_macro(term x)
{
	extern term read_expr();
	term a, cond, concl;
	term because = anon_struct, except = anon_struct, alt = anon_struct;
	
	cond = read_expr(999);
	if ((a = get_atom()) != _then)
		syn_err("missing \"then\" in if statement");
	concl = read_expr(999);
	if ((a = get_atom()) == _because)
		because = read_expr(999);
	else
		ungetatom(a);
	if ((a = get_atom()) == _except)
		except = read_expr(999);
	else
		ungetatom(a);
	if ((a = get_atom()) == _else)
		alt = read_expr(999);
	else
		ungetatom(a);
	
	return new_rule(cond, concl, because, except, alt);
}


/************************************************************************/
/*				Print RDR				*/
/************************************************************************/

static int tabs = 1;

static void
tab(int n)
{
	while (n--)
		fputc('\t', output);
}

static int
print_rdr(term goal, term *frame)
{
	term rdr = check_arg(1, goal, frame, FN, IN);
	term x;

	fprintf(output, "if ");
	rprin(COND(rdr), frame);

	x = CONCL(rdr);
	DEREF(x);
	if (NONVAR(x))
	{
		fprintf(output, " then ");
		if (TYPE(x) == FN && ARG(0, x) == _rdr)
		{
			fputc('\n', output);
			tab(++tabs);
			rprin(x, frame);
			--tabs;
		}		
		else
			rprin(x, frame);
	}

	x = CASE(rdr);
	DEREF(x);
	if (NONVAR(x))
	{
		fprintf(output, " because ");
		rprin(x, frame);
	}

	x = EXCEPT(rdr);
	DEREF(x);
	if (NONVAR(x))
	{
		fputc('\n', output);
		tab(++tabs);
		fprintf(output, "except ");
		rprin(x, frame);
		--tabs;
	}		

	x = ALT(rdr);
	DEREF(x);
	if (NONVAR(x))
	{
		fputc('\n', output);
		tab(tabs);
		fprintf(output, "else ");
		rprin(x, frame);
	}

	return TRUE;
}
	

/************************************************************************/
/*				Initialisation				*/
/************************************************************************/

void
rdr_init(void)
{
	_gt = intern(">");
	_lt = intern("<");
	_eq = intern("=");
	_ne = intern("\\=");
	_dont_know = intern("$");
	_of = intern("of");
	_isa = intern("isa");
	_value = intern("value");
	_this = intern("this");
	_object = intern("object");
	
	defop(999,	XFX,	_else		= intern("else"));
	defop(999,	XFX,	_except		= intern("except"));
	defop(999,	XFX,	_then		= intern("then"));
	defop(999,	XFX,	_because	= intern("because"));

	defop(750,	XFY,	_and		= new_fsubr(and, "and"));
	defop(800,	XFY,	_or		= new_fsubr(or, "or"));
	
	_rdr		= new_pred(rdr, "rdr");
	_if		= new_fsubr(if_expr, "if");
	anon_struct	= new_var(ANON, 0, _anon);

	MACRO(_if)	= if_macro;
	PORTRAY(_if)	= new_pred(print_rdr, "print_rdr");

	new_pred(p_add_rdr, "add_rdr");
	new_pred(corner_stone_case, "corner_stone_case");
}
