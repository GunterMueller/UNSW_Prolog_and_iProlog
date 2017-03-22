/************************************************************************/
/*		An RDR interpreter and maintenance system		*/
/************************************************************************/

#include "prolog.h"
#include "rdr.h"

#define MAX_COND 20

#define	COND(x)		ARG(1, x)
#define CONCL(x)	ARG(2, x)
#define	CASE(x)		ARG(3, x)
#define EXCEPT(x)	ARG(4, x)
#define ALT(x)		ARG(5, x)

#define NONVAR(x) (x != _anon && TYPE(x) != REF && TYPE(x) != ANON)

term _rdr, _if, _then, _else, _except, _because, _and, _or;
term _gt, _lt, _eq, _ne, _dont_know;

int  was_true;
term anon_struct;
term last_rule = NULL;
term last_case = NULL;
term whole_rule = NULL;

term condition_list[MAX_COND];
term selected_conds[MAX_COND];
int n_conditions;


/************************************************************************/
/*				Make a new RDR				*/
/************************************************************************/

term
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
/*			  Add a new rule to an RDR			*/
/************************************************************************/

void
add_rdr(term rule)
{
	if (was_true)
		EXCEPT(last_rule) = rule;
	else
		ALT(last_rule) = rule;
}


static int
p_add_rdr(term goal, term *frame)
{
	add_rdr(make(ARG(1, goal), frame));

	return TRUE;
}


/************************************************************************/
/*	Return the corner stone case for the last use of an RDR		*/
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
/*		Evaluate a conjunction of expressions			*/
/************************************************************************/

static term
and(term expr, term *frame)
{
	term orig = expr;

	while (TYPE(expr) == FN && ARG(0, expr) == _and)
	{
		if (eval(ARG(1, expr), frame) != _true)
			return _false;
		expr = ARG(2, expr);
	}
	return (orig == expr ? orig : eval(expr, frame));
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
			return _true;
		expr = ARG(2, expr);
	}
	return (orig == expr ? orig : eval(expr, frame));
}


/************************************************************************/
/*			Evaluate an RDR expression			*/
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
/*			Add a condition to condition list		*/
/************************************************************************/

void
add_condition(term cond)
{
	if (n_conditions >= MAX_COND)
		fail("Too many conditions in RDR");

	condition_list[n_conditions++] = cond;
}


/************************************************************************/
/*			Make a new condition for an RDR			*/
/************************************************************************/

void
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
	
	add_condition(rval);
	
	if (rval1 && old != _dont_know)
		add_condition(rval1);
}


/************************************************************************/
/*		Make conjunction from selected conditions		*/
/************************************************************************/

term
make_conj(int n_selected)
{
	int i;
	term conj = selected_conds[n_selected - 1];

	for (i = n_selected - 2; i >= 0; i--)
		conj = g_fn2(intern("and"), selected_conds[i], conj);

	return conj;
}


/************************************************************************/
/*			Ask a simple "yes" or "no" question		*/
/************************************************************************/

int
yes_no(term x, char *question)
{
	term ans;

	fputs("Is ", output);
	prin(x);
	fputc(' ', output);
	fputs(question, output);
	fputc('\n', output);
	ans = get_atom();
	return (NAME(ans)[0] != 'n');
}


/************************************************************************/
/*	Make conjunction from selected conditions.			*/
/*	If the condition is of the form Attr = val, and Attr is a free	*/
/*		variable, it has to be turned into a bound variable	*/
/************************************************************************/

extern term varlist;


static void
mkbound(term cond)
{
	term free, ref;

	if (TYPE(cond) != FN || ARG(0, cond) != _eq)
		return;

	free = ARG(1, cond);

	if (TYPE(free) != FREE)
		return;

	ARG(1, cond) = ref = new_ref();

	varlist = gcons(gcons(new_var(BOUND, OFFSET(free), PNAME(free)), ref), varlist);
}


static int
select_condition(void)
{
	int i, n;

	varlist = _nil;

	for (i = 0, n = 0; i < n_conditions; i++)
		if (yes_no(condition_list[i], "correct?"))
			mkbound(selected_conds[n++] = condition_list[i]);

	return n;
}


/************************************************************************/
/*			Perform interactive RDR maintenance		*/
/************************************************************************/

void
fix_rdr(int (*rdr_interaction)(), term new_case, term *frame)
{
	extern term *global;
	term *old_global = global;
	term conclusion, rule, whole_rule;
	term result = eval(new_case, frame);
	int n;

	n_conditions = 0;

	if ((*rdr_interaction)(last_case, &new_case, &whole_rule, result, &conclusion))
		return;

	if ((n = select_condition()) == 0)
		fail("Conclusion was incorrect, but no distinguishing features were found");

	rule = new_rule(
		 make(make_conj(n), frame),
		 make(conclusion, frame),
		 make(new_case, frame),
		 _anon,
		 _anon
	);

	add_rdr(rule);
	print(whole_rule);
	global = old_global;
}
	

/************************************************************************/
/*				Read RDR				*/
/************************************************************************/

static term
if_macro(term x)
{
	term a, cond, concl;
	term because = anon_struct, except = anon_struct, alt = anon_struct;

	ungetatom(a = get_atom());
	if (INFIX(a)) return x;

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
	extern void rdr_fn_init();

	rdr_fn_init();

	_gt = intern(">");
	_lt = intern("<");
	_eq = intern("=");
	_ne = intern("\\=");
	_dont_know = intern("?");
	_rdr = intern("rdr");
	
	defop(999,	XFX,	_else		= intern("else"));
	defop(999,	XFX,	_except		= intern("except"));
	defop(999,	XFX,	_then		= intern("then"));
	defop(999,	XFX,	_because	= intern("because"));

	defop(750,	XFY,	_and		= new_fsubr(and, "and"));
	defop(800,	XFY,	_or		= new_fsubr(or, "or"));
	
	_if		= new_fsubr(if_expr, "if");
	anon_struct	= new_var(ANON, 0, _anon);

	MACRO(_if)	= if_macro;
	PORTRAY(_if)	= new_pred(print_rdr, "print_rdr");

	new_pred(p_add_rdr, "add_rdr");
	new_pred(corner_stone_case, "corner_stone_case");
}
