#include <time.h>
#include "prolog.h"

#define HZ	60.0

#ifdef THINK_C

double
get_time(void)
{
	clock_t x = clock();
	double d = x/HZ;
	return(d);
}

#else

#include <sys/types.h>
#include <sys/times.h>

double
get_time(void)
{
	struct tms t;

	times(&t);
	return((double)(t.tms_utime)/HZ);
}

#endif

char *
date_time(void)
{
	struct tm *t;
	time_t clock;
	static char buf[64];

	time(&clock);
	t = localtime(&clock);
	sprintf(buf, "%02d/%02d/%02d - %02d:%02d:%02d",
		t -> tm_year,
		t -> tm_mon + 1,
		t -> tm_mday,
		t -> tm_hour,
		t -> tm_min,
		t -> tm_sec
	);
	return(buf);
}


static term
cputime(term goal, term *frame)
{
	return(new_real(get_time()));
}


static int
halt(term goal, term *frame)
{
	int exit_code = 0;

	if (ARITY(goal) == 1)
		exit_code = IVAL(check_arg(1, goal, frame, INT, IN));

	exit(exit_code);
}


static int
trace(term goal, term *frame)
{
	extern int trace_on;

	trace_on = TRUE;
	return(TRUE);
}


static int
notrace(term goal, term *frame)
{
	extern int trace_on;

	trace_on = FALSE;
	return(TRUE);
}


static int
spy(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, IN);

	while (x != _nil)
	{
		FLAGS(CAR(x)) |= SPY;
		x = CDR(x);
		DEREF(x);
	}
	return(TRUE);
}


static int
nospy(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, IN);

	while (x != _nil)
	{
		FLAGS(CAR(x)) &= ~SPY;
		x = CDR(x);
		DEREF(x);
	}
	return(TRUE);
}


static int
atopt(char *buff)
{
	static char *ops[] = {"fx", "fy", "xfx", "xfy", "yfx", "xf", "yf"};
	int i;
 
	for (i = FX; i <= YF; i++)
		if (strcmp(ops[i], buff) == 0)
			return(i);
	return(-1);
}


int
op(term goal, term *goal_frame)
{
	int p, op;
	term prec    = check_arg(1, goal, goal_frame, INT, IN);
	term op_type = check_arg(2, goal, goal_frame, ATOM, IN);
	term sym     = check_arg(3, goal, goal_frame, ATOM, IN);

	check_arity(goal, 3);

	p = IVAL(prec);

	if (p < 0 || p >= 1200)
		fail("Operator precedence out of range");
	if ((op = atopt(NAME(op_type))) == -1)
		fail("Incorrect opertor type in second argument");
	defop(p, op, sym);
	return(TRUE);
}


int
undefop(term goal, term *goal_frame)
{
	term sym = check_arg(1, goal, goal_frame, ATOM, IN);

	FLAGS(sym) &= ~((unsigned char) OP);
	PREFIX(sym) = INFIX(sym) = POSTFIX(sym) = 0;
	return(TRUE);
}


static term _slash;

static int
dynamic(term goal, term *frame)
{
	int i, a = ARITY(goal);

	for (i = 1; i <= a; i++)
	{ 
		term x = check_arg(i, goal, frame, FN, IN);

		if (TYPE(x) != FN
		||  ARG(0, x) != _slash
		||  ARITY(x) != 2
		||  TYPE(ARG(1, x)) != ATOM
		||  TYPE(ARG(2, x)) != INT
		   )
			fail("arguments should have the form \"atom/arity\"");

		if (PROC(ARG(1, x)) != NULL && TYPE(PROC(ARG(1, x))) == PRED)
			fail("Can't make a built-in predicate dynamic");

		FLAGS(ARG(1, x)) |= DYNAMIC;
	}
	return(TRUE);
}

#ifdef THINK_C

static int
dosc(term goal, term *frame)
{
	if (ARITY(goal) == 1)
	{
		term msg = check_arg(1, goal, frame, ATOM, IN);
	
		return(ae_send(NAME(msg), NULL));
	}
	if (ARITY(goal) == 2)
	{
		term msg   = check_arg(1, goal, frame, ATOM, IN);
		term reply = check_arg(2, goal, frame, ATOM, OUT);
		char buf[BUFSIZ];
	
		if (ae_send(NAME(msg), buf))
			return(unify(reply, frame, intern(buf), frame));
		return(FALSE);
	}
	fail("dosc can take only one or two arguments");
}

#endif

void
system_init(void)
{
	_slash = intern("/");

	new_fsubr(cputime, "cputime");
	new_pred(halt, "halt");
	new_fpred(trace, "trace");
	new_fpred(notrace, "notrace");
	defop(400, FX, new_fpred(spy, "spy"));
	defop(400, FX, new_fpred(nospy, "nospy"));
	new_fpred(op, "op");
	new_fpred(undefop, "undefop");
	new_fpred(dynamic, "dynamic");
#ifdef THINK_C
	new_pred(dosc, "dosc");
#endif
}
