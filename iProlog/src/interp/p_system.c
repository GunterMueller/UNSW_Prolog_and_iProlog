#include <time.h>
#include <string.h>
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
	return ((double)(t.tms_utime)/HZ);
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
	sprintf(buf, "%04d/%02d/%02d - %02d:%02d:%02d",
		t -> tm_year + 1900,
		t -> tm_mon + 1,
		t -> tm_mday,
		t -> tm_hour,
		t -> tm_min,
		t -> tm_sec
	);
	return buf;
}

term
today(void)
{
	struct tm *t;
	time_t clock;
	static char buf[64];
	term year, month, day, rval;

	time(&clock);
	t = localtime(&clock);

	sprintf(buf, "%d", t -> tm_year + 1900);
	year = new_int(atol(buf));

	sprintf(buf, "%d", t -> tm_mon + 1);
	month = new_int(atol(buf));

	sprintf(buf, "%d", t -> tm_mday);
	day = new_int(atol(buf));

	rval = new_g_fn(3);
	ARG(0, rval) = intern("date");
	ARG(1, rval) = year;
	ARG(2, rval) = month;
	ARG(3, rval) = day;

	return rval;
}


static int
is_date(term goal, term *frame)
{
	term d = check_arg(1, goal, frame, FN, IN);
	long year, month, day;
	long ndays; 

	if (ARG(0, d) != intern("date"))
		return FALSE;

	year = IVAL(ARG(1, d));
	if (year < 1)
		return FALSE;

	month = IVAL(ARG(2, d));
	if (month < 1 || month > 12)
		return FALSE;

	if (month == 2)
		ndays = year % 4 ? 28 : 29;
	else
		ndays = month % 2 ? 31 : 30;

	day = IVAL(ARG(3, d));
	if (day < 1 || day > ndays)
		return FALSE;

	return TRUE;
}


term
time_now(void)
{
	struct tm *t;
	time_t clock;
	static char buf[64];
	term hour, min, sec, rval;

	time(&clock);
	t = localtime(&clock);

	sprintf(buf, "%d", t -> tm_hour);
	hour = new_int(atol(buf));

	sprintf(buf, "%d", t -> tm_min);
	min = new_int(atol(buf));

	sprintf(buf, "%d", t -> tm_sec);
	sec = new_int(atol(buf));

	rval = new_g_fn(3);
	ARG(0, rval) = intern("time");
	ARG(1, rval) = hour;
	ARG(2, rval) = min;
	ARG(3, rval) = sec;

	return rval;
}


static int
is_time(term goal, term *frame)
{
	term t = check_arg(1, goal, frame, FN, IN);

	if (ARG(0, t) != intern("time"))
		return FALSE;

	if (IVAL(ARG(1, t)) < 0 || IVAL(ARG(1, t)) >= 24)
		return FALSE;

	if (IVAL(ARG(2, t)) < 0 || IVAL(ARG(2, t)) >= 60)
		return FALSE;

	if (IVAL(ARG(3, t)) < 0 || IVAL(ARG(3, t)) >= 60)
		return FALSE;

	return TRUE;
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
	return TRUE;
}


static int
atopt(char *buff)
{
	static char *ops[] = {"fx", "fy", "xfx", "xfy", "yfx", "xf", "yf"};
	int i;
 
	for (i = FX; i <= YF; i++)
		if (strcmp(ops[i], buff) == 0)
			return(i);
	return -1;
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
	if (TERM_EXPAND(sym) != NULL)
	{
		char buf[128];

		sprintf(buf, "\"%s\" is already defined by Prolog", NAME(sym));
		warning(buf);
		TERM_EXPAND(sym) = NULL;
		PORTRAY(sym) = NULL;
	}
	defop(p, op, sym);
	return TRUE;
}


int
undefop(term goal, term *goal_frame)
{
	term sym = check_arg(1, goal, goal_frame, ATOM, IN);

	FLAGS(sym) &= ~((unsigned char) OP);
	PREFIX(sym) = INFIX(sym) = POSTFIX(sym) = 0;
	return TRUE;
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
	return TRUE;
}


void
system_init(void)
{
	_slash = intern("/");

	new_fsubr(cputime, "cputime");
	new_fsubr(today, "today");
	new_pred(is_date, "is_date");
	new_fsubr(time_now, "time_now");
	new_pred(is_time, "is_time");
	new_pred(halt, "halt");
	new_fpred(trace, "trace");
	new_fpred(notrace, "notrace");
	defop(400, FX, new_fpred(spy, "spy"));
	defop(400, FX, new_fpred(nospy, "nospy"));
	new_fpred(op, "op");
	new_fpred(undefop, "undefop");
	new_fpred(dynamic, "dynamic");
}
