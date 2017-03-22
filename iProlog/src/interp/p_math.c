#include <math.h>
#include <time.h>
#include "prolog.h"

#ifndef RAND_MAX
#define RAND_MAX MAXLONG
#endif

#ifndef PI
#define PI	3.141592653589793
#endif


term
new_int(long i)
{
	term rval =  galloc(sizeof(integer));

	TYPE(rval) = INT;
	FLAGS(rval) = COPY;
	IVAL(rval) = i;
	return rval;
}


term
new_h_int(long i)
{
	term rval =  halloc(sizeof(integer));

	TYPE(rval) = INT;
	FLAGS(rval) = 0;
	IVAL(rval) = i;
	return rval;
}


term
new_real(double d)
{
	term rval =  galloc(sizeof(real));

	TYPE(rval) = REAL;
	FLAGS(rval) = COPY;
	RVAL(rval) = d;
	return rval;
}


term
new_h_real(double d)
{
	term rval =  halloc(sizeof(real));

	TYPE(rval) = REAL;
	FLAGS(rval) = 0;
	RVAL(rval) = d;
	return rval;
}


static term
plus(term goal, term *frame)
{
	term x, y;

	switch (ARITY(goal))
	{
	case 1:
			x = check_arg(1, goal, frame, NUMBER, IN);
			return x;
	case 2:
			x = check_arg(1, goal, frame, NUMBER, IN);
			y = check_arg(2, goal, frame, NUMBER, IN);

			if (TYPE(x) == INT)
				if (TYPE(y) == INT)
					return new_int(IVAL(x) + IVAL(y));
				else
					return new_real(((double) IVAL(x)) + RVAL(y));
			else
				if (TYPE(y) == INT)
					return new_real(RVAL(x) + ((double) IVAL(y)));
				else
					return new_real(RVAL(x) + RVAL(y));
	default:
			return goal;
	}
}


static term
minus(term goal, term *frame)
{
	term x, y;

	switch (ARITY(goal))
	{
	case 1:
			x = check_arg(1, goal, frame, NUMBER, IN);

			if (TYPE(x) == INT)
				return new_int(- IVAL(x));
			else
				return new_real(- RVAL(x));
	case 2:
			x = check_arg(1, goal, frame, NUMBER, IN);
			y = check_arg(2, goal, frame, NUMBER, IN);

			if (TYPE(x) == INT)
				if (TYPE(y) == INT)
					return new_int(IVAL(x) - IVAL(y));
				else
					return new_real(((double) IVAL(x)) - RVAL(y));
			else
				if (TYPE(y) == INT)
					return new_real(RVAL(x) - ((double) IVAL(y)));
				else
					return new_real(RVAL(x) - RVAL(y));
	default:
			return goal;
	}
}


static term
times(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	term y = check_arg(2, goal, frame, NUMBER, IN);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return new_int(IVAL(x) * IVAL(y ));
		else
			return new_real(((double) IVAL(x)) * RVAL(y));
	else
		if (TYPE(y) == INT)
			return new_real(RVAL(x) * ((double) IVAL(y)));
		else
			return new_real(RVAL(x) * RVAL(y));
}


static term
f_div(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	term y = check_arg(2, goal, frame, NUMBER, IN);

	if (TYPE(y) == INT)
	{
		if (IVAL(y) == 0)
			fail("Division by zero");

		if (TYPE(x) == INT)
			if (IVAL(x) % IVAL(y) != 0)
				return new_real((double) IVAL(x) / IVAL(y));
			else
				return new_int(IVAL(x) / IVAL(y));
		else
			return new_real(RVAL(x) / ((double) IVAL(y)));
	}
	else
	{
		if (RVAL(y) == 0)
			fail("Division by zero");

		if (TYPE(x) == INT)
			return new_real(((double) IVAL(x)) / RVAL(y));
		else
			return new_real(RVAL(x) / RVAL(y));
	}
}


static term
i_div(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, INT, IN);
	term y = check_arg(2, goal, frame, INT, IN);

	if (IVAL(y) == 0)
		fail("Division by zero");

	return new_int(IVAL(x) / IVAL(y));
}


static term
i_mod(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, INT, IN);
	term y = check_arg(2, goal, frame, INT, IN);

	if (IVAL(y) == 0)
		fail("Division by zero");

	return new_int(IVAL(x) % IVAL(y));
}


static term
p_pow(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	term y = check_arg(2, goal, frame, NUMBER, IN);

	if (TYPE(y) == INT)
		if (TYPE(x) == INT)
		{
			int i;
			long n = IVAL(x), rval = 1;

			for (i = IVAL(y); i != 0; i--)
				rval *= n;
	
			return new_int(rval);
		}
		else
		{
			int i;
			double n = RVAL(x), rval = 1;

			for (i = IVAL(y); i != 0; i--)
				rval *= n;
	
			return new_real(rval);
		}
	else
		if (TYPE(x) == INT)
			return new_real(pow(((double) IVAL(x)), RVAL(y)));
		else
			return new_real(pow(RVAL(x), RVAL(y)));
}


static term
p_abs(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);

	if (TYPE(x) == INT)
		if (IVAL(x) < 0)
			return new_int(- IVAL(x));
		else
			return(x);
	else
		if (RVAL(x) < 0)
			return new_real(- RVAL(x));
		else
			return x;
}


static term
p_sin(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(sin(r));
}


static term
p_cos(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(cos(r));
}


static term
p_tan(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(tan(r));
}



static term
p_asin(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, REAL, IN);
	double r = RVAL(x);

	if (r < -1 || r > 1)
		fail("Argument must be in the range -1 to +1");
	return new_real(asin(r));
}


static term
p_acos(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, REAL, IN);
	double r = RVAL(x);

	if (r < -1 || r > 1)
		fail("Argument must be in the range -1 to +1");
	return new_real(acos(r));
}


static term
p_atan(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, REAL, IN);
	double r = RVAL(x);

	if (r < -1 || r > 1)
		fail("Argument must be in the range -1 to +1");
	return new_real(atan(r));
}


static term
p_atan2(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	term y = check_arg(1, goal, frame, NUMBER, IN);
	double rx = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);
	double ry = (TYPE(y) == INT) ? ((double) IVAL(y)) : RVAL(y);

	return new_real(atan2(ry, rx));
}


static term
p_log(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	if (r <= 0)
		fail("Argument must be a positive number");
	return new_real(log(r));
}


static term
p_log10(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	if (r <= 0)
		fail("Argument must be a positive number");
	return new_real(log10(r));
}


static term
p_exp(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(exp(r));
}


static term
p_logistic(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(1.0 / (1.0 + exp(-r)));
}


static term
p_sqrt(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(sqrt(r));
}


static term
p_float(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);

	if (TYPE(x) == INT)
		return new_real((double) IVAL(x));
	return x;
}


static term
p_round(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);

	if (TYPE(x) == REAL)
		return(new_int((long) (RVAL(x) + 0.5)));
	return x;
}


static term
p_floor(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, REAL, IN);

	return new_real(floor(RVAL(x)));
}


static term
p_ceil(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, REAL, IN);

	return new_real(ceil(RVAL(x)));
}


static term
p_trunc(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);

	if (TYPE(x) == REAL)
		return new_int((long) RVAL(x));
	return x;
}


static term
p_sign(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);

	if (TYPE(x) == REAL)
	{
		if (RVAL(x) < 0) return(new_int(-1L));
		if (RVAL(x) > 0) return(new_int(1L));
		return new_int(0);
	}
	if (TYPE(x) == INT)
	{
		if (IVAL(x) < 0) return(new_int(-1L));
		if (IVAL(x) > 0) return(new_int(1L));
		return new_int(0);
	}
}


static int
eq(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, EVAL);
	term y = check_arg(2, goal, frame, NUMBER, EVAL);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return (IVAL(x) == IVAL(y));
		else
			return (((double) IVAL(x)) == RVAL(y));
	else
		if (TYPE(y) == INT)
			return(RVAL(x) == ((double) IVAL(y)));
		else
			return (RVAL(x) == RVAL(y));
}


static int
neq(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, EVAL);
	term y = check_arg(2, goal, frame, NUMBER, EVAL);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return (IVAL(x) != IVAL(y));
		else
			return (((double) IVAL(x)) != RVAL(y));
	else
		if (TYPE(y) == INT)
			return (RVAL(x) != ((double) IVAL(y)));
		else
			return (RVAL(x) != RVAL(y));
}


static int
lt(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, EVAL);
	term y = check_arg(2, goal, frame, NUMBER, EVAL);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return (IVAL(x) < IVAL(y));
		else
			return (((double) IVAL(x)) < RVAL(y));
	else
		if (TYPE(y) == INT)
			return (RVAL(x) < ((double) IVAL(y)));
		else
			return (RVAL(x) < RVAL(y));
}


static int
le(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, EVAL);
	term y = check_arg(2, goal, frame, NUMBER, EVAL);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return (IVAL(x) <= IVAL(y));
		else
			return(((double) IVAL(x)) <= RVAL(y));
	else
		if (TYPE(y) == INT)
			return (RVAL(x) <= ((double) IVAL(y)));
		else
			return (RVAL(x) <= RVAL(y));
}


static int
gt(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, EVAL);
	term y = check_arg(2, goal, frame, NUMBER, EVAL);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return (IVAL(x) > IVAL(y));
		else
			return (((double) IVAL(x)) > RVAL(y));
	else
		if (TYPE(y) == INT)
			return (RVAL(x) > ((double) IVAL(y)));
		else
			return (RVAL(x) > RVAL(y));
}


static int
ge(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, EVAL);
	term y = check_arg(2, goal, frame, NUMBER, EVAL);

	if (TYPE(x) == INT)
		if (TYPE(y) == INT)
			return (IVAL(x) >= IVAL(y));
		else
			return (((double) IVAL(x)) >= RVAL(y));
	else
		if (TYPE(y) == INT)
			return (RVAL(x) >= ((double) IVAL(y)));
		else
			return (RVAL(x) >= RVAL(y));
}


static term
pi(term goal, term *frame)
{
	static real pi_struct = {REAL, 0, PI};

	return (term)(&pi_struct);
}


static term
degrees(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(r * PI/180);
}


static term
radians(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, IN);
	double r = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);

	return new_real(r * 180/PI);
}


static term
p_random(term goal, term *frame)
{
	term x;
	double from = 0.0, to = 1.0;

	if (TYPE(goal) == FN)
		switch (ARITY(goal))
		{
		case 1:
			x = check_arg(1, goal, frame, NUMBER, IN);
			to = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);
			break;
		case 2:
			x = check_arg(1, goal, frame, NUMBER, IN);
			from = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);
			x = check_arg(2, goal, frame, NUMBER, IN);
			to = (TYPE(x) == INT) ? ((double) IVAL(x)) : RVAL(x);
			break;
		default:
			fail("Too many arguments to \"random\"");
		}

	return new_real((double)(rand())/(double)(RAND_MAX) * (to - from) + from);
}


static int
seed(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, INT, EVAL);

	srand(IVAL(x));
	return TRUE;
}



void
math_init(void)
{
	extern term _plus, _minus;
	time_t clock;

	time(&clock);
	srand((unsigned int) clock);

	defop(500, YFX, (_plus = new_subr(plus, "+")));
	defop(100, FX,  _plus);
	defop(500, YFX, (_minus = new_subr(minus, "-")));
	defop(100, FX,  _minus);
	defop(400, YFX, new_subr(times, "*"));
	defop(400, YFX, new_subr(f_div, "/"));
	defop(400, YFX, new_subr(i_div, "div"));
	defop(400, YFX, new_subr(i_div, "//"));
	defop(400, YFX, new_subr(i_mod, "mod"));
	defop(200, XFY, new_subr(p_pow, "**"));
	defop(200, XFY, new_subr(p_pow, "^"));

	new_subr(p_abs,		"abs");
	new_subr(p_sin,		"sin");
	new_subr(p_cos,		"cos");
	new_subr(p_tan,		"tan");
	new_subr(p_asin,	"asin");
	new_subr(p_acos,	"acos");
	new_subr(p_atan,	"atan");
	new_subr(p_atan2,	"atan2");
	new_subr(p_log,		"log");
	new_subr(p_log10,	"log10");
	new_subr(p_exp,		"exp");
	new_subr(p_logistic,	"logistic");
	new_subr(p_sqrt,	"sqrt");

	new_subr(p_float,	"float");
	new_subr(p_round,	"round");
	new_subr(p_trunc,	"truncate");
	new_subr(p_floor,	"floor");
	new_subr(p_ceil,	"ceiling");
	new_subr(p_sign,	"sign");

	defop(700, XFX, new_fpred(eq, "=:="));
	defop(700, XFX, new_fpred(neq, "=\\="));
	defop(700, XFX, new_fpred(lt, "<"));
	defop(700, XFX, new_fpred(le, "<="));
	defop(700, XFX, new_fpred(le, "=<"));
	defop(700, XFX, new_fpred(gt, ">"));
	defop(700, XFX, new_fpred(ge, ">="));

	defop(600, XF,  new_subr(degrees, "degrees"));
	defop(600, XF,  new_subr(degrees, "degree"));
	defop(600, XF,  new_subr(radians, "radians"));
	defop(600, XF,  new_subr(radians, "radian"));

	new_fsubr(pi, "pi");
	new_subr(p_random, "random");
	new_pred(seed, "seed");
}
