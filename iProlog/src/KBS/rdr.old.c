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

extern term varlist, *global;

term   _rdr, _if, _then, _else, _except, _because, _and, _or;
static term _gt, _lt, _eq, _ne, _dont_know;
static term _of, _isa, _value, _this, _object;

int  was_true;
term anon_struct;
term last_rule = NULL;
term last_case = NULL;


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

static void
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
/*			Make a new condition for an RDR			*/
/************************************************************************/

static term
mkbound(term free)
{
	term rval;

	if (TYPE(free) != FREE)
		return free;

	rval = new_ref();
	varlist = gcons(gcons(new_var(BOUND, OFFSET(free), PNAME(free)), rval), varlist);
	return rval;
}


static int
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
	
	if (yes_no(rval, "correct?"))
	{
		ARG(1, rval) = mkbound(var);
		return rval;
	}
	
	if (rval1 && old != _dont_know && yes_no(rval1, "correct?"))
	{
		ARG(1, rval1) = mkbound(var);
		return rval1;
	}

	return NULL;
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
	return conj;
}


/************************************************************************/
/*		Perform interactive RDR maintenance on functions	*/
/************************************************************************/

static int
rdr_fn(term goal, term *frame)
{
	term new_case = copy(check_arg(1, goal, frame, FN, IN), frame);
	term result = eval(new_case, frame);
	term conclusion, rule;
	term *old_global = global;

	if (! NONVAR(CASE(last_rule)))
		fail("RDR must contain cases to perform maintenance");

	if (TYPE(last_case) != FN || ARG(0, last_case) != ARG(0, new_case) || ARITY(last_case) != ARITY(new_case))
		fail("New case and stored case are incompatible");

	fprintf(output, "New case: "); print(new_case);
	fprintf(output, "Old case: "); print(last_case);
	if (yes_no(result, "the correct conclusion?"))
		return TRUE;
	varlist = _nil;

	printf("What is the correct conclusion?\n");
	conclusion = get_atom();
	rule = new_rule(
		make(fn_diff(last_case, new_case), frame),
		conclusion,
		make(new_case, frame),
		anon_struct,
		anon_struct
	);
	add_rdr(rule);
	global = old_global;
	return TRUE;
}

/************************************************************************/
/*		   The difference between property lists		*/
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
			if (! yes_no(cond, "correct?"))
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
			if (! yes_no(cond, "correct?"))
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
	return conj;
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
	term rule_slot;
	term *old_global = global;

	rule_slot = ARG(0, new_case);
	new_case = ARG(1, new_case);

	conclusion = result;
	old_case = last_case;
	print(old_case);

	fprintf(output, "New case: "); print(PLIST(new_case));
	fprintf(output, "Old case: "); print(PLIST(old_case));
	if (yes_no(conclusion, "the correct conclusion?"))
		return TRUE;
	varlist = _nil;

	printf("What is the correct conclusion?\n");
	conclusion = get_atom();
	rule = new_rule(
		 make(plist_diff(PLIST(old_case), PLIST(new_case)), frame),
		 conclusion,
		 new_case,
		 _anon,
		 _anon
	);
	print(rule);
	add_rdr(rule);
	global = old_global;
	return TRUE;
}


/************************************************************************/
/*	Perform interactive RDR maintenance on chat rules		*/
/************************************************************************/

#ifdef PROBOT

term read_sentence(void);
static term _answer, _respond, _star, _var;

static term
sub_seq(term input, term response)
{
	term p, q;
	term L = _nil, *last = &L;
	term subst = _nil, *last_subst = &subst;

	for (p = response; p != _nil; p = CDR(p))
		for (q = input; q != _nil; q = CDR(q))
			if (CAR(p) == CAR(q))
			{
				*last = gcons(CAR(p), _nil);
				last = &CDR(*last);
			}
			else if (L != _nil)
			{
				*last_subst = gcons(L, _nil);
				last_subst = &CDR(*last_subst);
				L = _nil;
				last = &L;
			}
	
	if (L != _nil)
	{
		*last_subst = gcons(L, _nil);
		last_subst = &CDR(*last_subst);
	}

	return subst;
}


static term
gen_pattern(term input, term subst, term marker)
{
	term p;
	term sent = input;
	int n_vars = 0;

	for (p = subst; p != _nil; p = CDR(p))
	{
		term L = _nil, *last = &L;
		term subseq = CAR(p);

		repeat
		{
			if (subseq == _nil)
			{
				if (marker == _var)
					*last = gcons(g_fn1(_var, new_int(++n_vars)), _nil);
				else
					*last = gcons(marker, _nil);
				last = &CDR(*last);

				subseq = CAR(p);
			}
			if (sent == _nil)
				break;
			if (CAR(subseq) == CAR(sent))
			{
				subseq = CDR(subseq);
				sent = CDR(sent);
				continue;
			}
			subseq = CAR(p);

			*last = gcons(CAR(sent), _nil);
			last = &CDR(*last);
			sent = CDR(sent);
		}
		sent = L;
	}

	return sent;
}



static int
rdr_chat(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, FN, IN);
	term conclusion, old_case, new_case, rule;
	term *old_global = global;
	term q[2] = {NULL, NULL};
	term subst = _nil, pattern = _nil, response = _nil;

	*q = x;

	cond(q, frame);
	old_case = last_case;
	new_case = ARG(1, x);

	if (yes_no(intern("this the correct response?"), ""))
		return TRUE;

	fprintf(output, "New case: "); print(new_case);
	fprintf(output, "Old case: "); print(old_case);
	varlist = _nil;

	printf("What is the correct response");
	conclusion = read_sentence();

	subst = sub_seq(new_case, conclusion);
	printf("Subst: "); print(subst);
	pattern = gen_pattern(new_case, subst, _star);
	printf("Pattern: "); print(pattern);
	response = gen_pattern(conclusion, subst, _var);
	printf("Response: "); print(response);
	
	rule = new_rule(
		 make(g_fn1(_answer, pattern), frame),
		 make(g_fn1(_respond, response), frame),
		 make(new_case, frame),
		 _anon,
		 _anon
	);
	print(rule);
	if (was_true)
		EXCEPT(last_rule) = rule;
	else
		ALT(last_rule) = rule;
	global = old_global;
	return TRUE;
}

#endif

/************************************************************************/
/*			Perform interactive RDR maintenance		*/
/************************************************************************/

static int
rdr(term goal, term *frame)
{
	term x = copy(check_arg(1, goal, frame, FN, IN), frame);

	if (TYPE(PROC(ARG(0, x))) == SUBR)
		return rdr_plist(goal, frame);
	else
		return rdr_fn(goal, frame);
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

#ifdef PROBOT
	_answer = intern("answer");
	_respond = intern("respond");
	_star = intern("*");
	_var = intern("var");

	new_pred(rdr_chat, "rdr_chat");
#endif
}
