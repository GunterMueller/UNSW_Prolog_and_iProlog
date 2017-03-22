#include "prolog.h"

typedef struct
{
	char	am_pm[2],
		long_day[10],
		user[16],
		comment[256];
	int	day,
		month,
		year,
		hour,
		minute,
		length_hour,
		length_minute;
} plan_data;

static void
get_plan(char *day, plan_data *pdata)
{
	FILE *plan;
	char buf[256];
	int n;

	sprintf(buf, "plan -i -t %s", day);
	plan = popen(buf, "r");

	n = fscanf(plan, "%*d;%[^,], %d.%d.%d; %d:%d%[ap];%d:%d;%[^;];%[^\n]\n",
		pdata -> long_day,
		&(pdata -> day),
		&(pdata -> month),
		&(pdata -> year),
		&(pdata -> hour),
		&(pdata -> minute),
		pdata -> am_pm,
		&(pdata -> length_hour),
		&(pdata -> length_minute),
		pdata -> user,
		pdata -> comment
	);

	pclose(plan);

	if (pdata -> am_pm[0] == 'p')
		pdata -> hour += 12;
}

static int
p_ask_plan(term goal, term *frame)
{
	plan_data pdata;
	term	temp, rval;
	term	day_spec  = check_arg(1, goal, frame, ATOM, IN),
		appt = check_arg(2, goal, frame, FN, OUT);

	get_plan(NAME(day_spec), &pdata);

	temp = new_g_fn(3);
	ARG(0, temp) = intern("date");
	ARG(1, temp) = new_int(2000+pdata.year);
	ARG(2, temp) = new_int(pdata.month);
	ARG(3, temp) = new_int(pdata.day);

	rval = new_g_fn(5);
	ARG(0, rval) = intern("appt");
	ARG(1, rval) = intern(pdata.long_day);
	ARG(2, rval) = temp;
	ARG(3, rval) = g_fn2(intern("time"), new_int(pdata.hour), new_int(pdata.minute));
	ARG(4, rval) = g_fn2(intern("length"), new_int(pdata.length_hour), new_int(pdata.length_minute));
	ARG(5, rval) = intern(pdata.comment);

	return unify(appt, frame, rval, frame);
}

static term
plan_to_frame(term goal, term *frame)
{
	plan_data pdata;
	term	rval;
	term	day_spec  = check_arg(1, goal, frame, ATOM, IN);

	get_plan(NAME(day_spec), &pdata);

	rval = build_plist_named("appt",
			"year",		new_h_int(2000+pdata.year),
			"month",	new_h_int(pdata.month),
			"day",		new_h_int(pdata.day),
			"hour",		new_h_int(pdata.hour),
			"minute",	new_h_int(pdata.minute),
			"am_pm",	intern(pdata.am_pm),
			"length_hour",	new_h_int(pdata.length_hour),
			"length_minute",new_int(pdata.length_minute),
			"comment",	intern(pdata.comment),
			NULL
	);

	return rval;
}

static int
p_tell_plan(term goal, term *frame)
{
	term	date = check_arg(1, goal, frame, ATOM, IN);
	term	length = check_arg(1, goal, frame, ATOM, IN);
	term	comment = check_arg(1, goal, frame, ATOM, IN);
	char buf[256];
	FILE	*plan;

	sprintf(buf, "plan %s -l %s %s", NAME(date), NAME(length), NAME(comment));
	plan = popen(buf, "w");
	pclose(plan);

	return TRUE;
}

void
calendar_init(void)
{
	new_pred(p_ask_plan, "plan_appointment");
	new_pred(p_tell_plan, "make_plan_appointment");
	new_subr(plan_to_frame, "plan_appointment_frame");
}
