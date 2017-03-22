/************************************************************************/
/*			Frame system embedded in Prolog			*/
/************************************************************************/

#include <stdarg.h>
#include "prolog.h"

#define FACET(x)		ARG(0, x)
#define FNAME(x)		ARG(1, x)
#define ATTR(x)			ARG(2, x)
#define VALUE(x)		ARG(3, x)

#define SAVE(x)			frame_desc old_frame = this; this.var_frame = x
#define RETURN(type, val)	{type rval = val; this = old_frame; return(rval);}

typedef struct
{
	term object;
	term slot;
	term new_value;
	term old_value;
	term *var_frame;
} frame_desc;

static frame_desc this = {NULL};

static term
	_of, _from, _to, _by, _replace, _remove, _add, _dot_dot, _slot,
	_if_new, _if_added, _if_needed, _if_removed, _if_replaced,
	_default, _cache, _range, _help, _multivalued, _colon, _with,
	_generic, _object, _value, _isa, _yes,
	_new, _old;


/************************************************************************/
/*		Public routing for creating a new slot      		*/
/************************************************************************/

void
make_slot(term name, char *slot, term value)
{
	term cl = new_h_fn(3);
	ARG(0, cl) = _value;
 	ARG(1, cl) = name;
	ARG(2, cl) = intern(slot);
	ARG(3, cl) = value;
	add_clause(new_unit(cl), FALSE);
}


/************************************************************************/
/*		Public routine for creating a new frame      		*/
/************************************************************************/

term
build_frame(char *isa, ...)
{
	va_list ap;
	char *slot_name;
	term frame_name = gensym(isa);

	make_slot(frame_name, "isa", hcons(intern(isa), _nil));

	va_start(ap, isa);
	while (slot_name = va_arg(ap, char *))
	{
		term slot_value = va_arg(ap, term);

		make_slot(frame_name, slot_name, slot_value);
	}
	va_end(ap);

	return(frame_name);
}


/************************************************************************/
/*		Create a new clause for frame/slot/facet/value		*/
/************************************************************************/

static void
new_slot(term name, term slot, term facet, term value)
{
	term cl = new_h_fn(3);
	ARG(0, cl) = facet;
	ARG(1, cl) = name;
	ARG(2, cl) = slot;
	ARG(3, cl) = make(value, this.var_frame);
	add_clause(new_unit(cl), FALSE);
}


/************************************************************************/
/*			Remove a clause given frame/slot		*/
/************************************************************************/

static term
remove_slot(term name, term slot)
{
	term *p;

	for (p = &PROC(_value); *p != NULL; p = &NEXT(*p))
	{
		term q = HEAD(*p);

		if (FNAME(q) == name && ATTR(q) == slot)
		{
			term tmp = *p;
			*p = NEXT(*p);
			free_term(tmp);
			return(copy(VALUE(q), this.var_frame));
		}
	}
	return(NULL);
}


/************************************************************************/
/*	Remove all the clauses associated with an instance frame	*/
/************************************************************************/

void
remove_frame(term name)
{
	term *p = &PROC(_value);

	while (*p != NULL)
	{
		term q = HEAD(*p);

		if (FNAME(q) == name)
		{
			term tmp = *p;
			*p = NEXT(*p);
			free_term(tmp);
		}
		else
			p = &NEXT(*p);
	}
}


/************************************************************************/
/*		Find a clause given frame/slot/facet			*/
/************************************************************************/

static term
find_clause(term name, term slot, term facet)
{
	term p, q;

	for (p = PROC(facet); p != NULL; p = NEXT(p))
	{
		q = HEAD(p);
		if (FNAME(q) == name && ATTR(q) == slot)
			return(q);
	}
	return(NULL);
}


/************************************************************************/
/*		Lookup a value given frame/slot/facet			*/
/************************************************************************/

static term
lookup(term name, term slot, term facet)
{
	term rval = find_clause(name, slot, facet);

	if (rval != NULL)
		rval = VALUE(rval);
	return(rval);
}


/************************************************************************/
/*	Return the value of a slot in an value in an instance frame	*/
/************************************************************************/

term
slot_value(term name, term slot)
{
	term rval = find_clause(name, slot, _value);

	if (rval != NULL)
		rval = VALUE(rval);
	return(rval);
}


/************************************************************************/
/*			Get a daemon from a frame			*/
/************************************************************************/

static term
find_daemon(term obj, term attr, term facet)
{
	term p, rval = NULL;

	if ((rval = lookup(obj, attr, facet)) == NULL)
		for (p = lookup(obj, _isa, _value); p != NULL; p = CDR(p))
			if ((rval = find_daemon(CAR(p), attr, facet)) != NULL)
				break;
	return(rval);
}


/************************************************************************/
/* when a new value is put into a slot a range check is performed	*/
/* if the value is out of range a help demon is invoked (if present)	*/
/************************************************************************/

static int
in_range(term obj)
{
	term p, range = find_daemon(obj, this.slot, _range);

	if (range == NULL)
		return(TRUE);
	if (progn(range, this.var_frame) == _true)
		return(TRUE);
	if ((p = find_daemon(obj, this.slot, _help)) != NULL)
		progn(p, this.var_frame);
	return(FALSE);
}


/************************************************************************/
/*			Put a value into a frame			*/
/************************************************************************/

int
put_value()
{
	term old_clause	= find_clause(this.object, this.slot, _value);
	term if_added	= find_daemon(this.object, this.slot, _if_added);

	if (! in_range(this.object))
		return(FALSE);

	if (find_daemon(this.object, this.slot, _multivalued) == _yes)
		if (old_clause == NULL)
			new_slot(this.object, this.slot, _value, gcons(this.new_value, _nil));
		else
			VALUE(old_clause) = hcons(make(this.new_value, this.var_frame), VALUE(old_clause));
	else if (old_clause != NULL)
		fail("Tried to add a second value to a slot that is not multivalued");
	else
		new_slot(this.object, this.slot, _value, this.new_value);

	if (if_added != NULL)
		progn(if_added, this.var_frame);
	return(TRUE);
}


int
fput_is(term slot, term val, term *var_frame)
{
	SAVE(var_frame);

	this.object = eval(ARG(2, slot), var_frame);
	this.slot = eval(ARG(1, slot), var_frame);
	this.new_value = val;
	DEREF(this.new_value);

	RETURN(int, put_value());
}


static int
fput(term goal, term *var_frame)
{
	term x = check_arg(1, goal, var_frame, FN, IN);
	SAVE(var_frame);

	if (ARG(0, x) != _to || TYPE(this.slot = ARG(2, x)) != FN || ARG(0, this.slot) != _of)
		fail("Usage: add <value> to <slot> of <object>");

	this.object = eval(ARG(2, this.slot), var_frame);
	this.slot = eval(ARG(1, this.slot), var_frame);
	this.new_value = eval(ARG(1, x), var_frame);
	DEREF(this.new_value);

	RETURN(int, put_value());
}


/************************************************************************/
/*			Replace a value in a frame			*/
/************************************************************************/

int
replace_value()
{
	term if_replaced = find_daemon(this.object, this.slot, _if_replaced);
	term old_clause  = find_clause(this.object, this.slot, _value);

	if (!in_range(this.object))
		return(FALSE);

	if (old_clause != NULL)
	{
		this.old_value = VALUE(old_clause);
		VALUE(old_clause) = make(this.new_value, this.var_frame);
	}
	else
	{
		this.old_value = NULL;
		new_slot(this.object, this.slot, _value, this.new_value);
	}
	if (if_replaced != NULL)
		progn(if_replaced, this.var_frame);
	free_term(this.old_value);
	return(TRUE);
}


static int
freplace(term goal, term *var_frame)
{
	term x = check_arg(1, goal, var_frame, FN, IN);
	SAVE(var_frame);

	if (ARG(0, x) != _by || TYPE(this.slot = ARG(1, x)) != FN || ARG(0, this.slot) != _of)
		fail("Usage: replace <slot> of <object> by <value>");

	this.object = eval(ARG(2, this.slot), var_frame);
	this.slot = eval(ARG(1, this.slot), var_frame);
	this.new_value = eval(ARG(2, x), var_frame);
	DEREF(this.new_value);
	
	RETURN(int, replace_value());
}


/************************************************************************/
/*			Remove a slot from a frame			*/
/************************************************************************/

static int
fremove(term goal, term *var_frame)
{
	term rm, x = check_arg(1, goal, var_frame, FN, IN);
	SAVE(var_frame);

	if (ARG(0, x) != _from || TYPE(this.slot = eval(ARG(1, x), var_frame)) != ATOM)
		fail("Usage: remove <slot> from <object>");
	
	this.object = eval(ARG(2, x), var_frame);

	this.old_value = remove_slot(this.object, this.slot);
	if ((rm = find_daemon(this.object, this.slot, _if_removed)) != NULL)
		progn(rm, this.var_frame);

	RETURN(int, this.old_value != NULL);
}


/************************************************************************/
/*			Get a value from a frame			*/
/************************************************************************/

static term
get_value(term obj)
{
	term rval, p;

	if ((rval = lookup(obj, this.slot, _value)) != NULL)
		return(rval);
	if ((rval = lookup(obj, this.slot, _default)) != NULL)
		return(rval); 
	if ((rval = lookup(obj, this.slot, _if_needed)) != NULL)
	{
		term cache = lookup(obj, this.slot, _cache);
		this.new_value = progn(rval, this.var_frame);

		if (cache == _yes)
			put_value();
		return(this.new_value); 
	}
	for (p = lookup(obj, _isa, _value); p != NULL; p = CDR(p))
		if ((rval = get_value(CAR(p))) != NULL)
			return(rval);
	return(NULL);
}


static term
fget(term goal, term *var_frame)
{
	SAVE(var_frame);

	this.slot   = check_arg(1, goal, var_frame, ATOM, IN);
	this.object = check_arg(2, goal, var_frame, ATOM, IN);

	RETURN(term, get_value(this.object));
}


static term
have_property(term expr, term *var_frame)
{
	return (ARG(1, expr) != NULL) ? _true : _false;
}


/************************************************************************/
/*			Process "if_new" daemons			*/
/************************************************************************/

static void
send_new(term obj)
{
	term p, q, r;

	if (obj == _object)
		return;

	if ((p = lookup(obj, _isa, _value)) == NULL)
		fail("A parent is missing from the inheritance hierarchy");

	for (; p != _nil; p = CDR(p))
		send_new(CAR(p));

	for (q = PROC(_if_new); q != NULL; q = NEXT(q))
		if (FNAME(r = HEAD(q)) == obj)
		{
			this.slot = ATTR(r);

			/* disable if_new if a value is already present */
			if (lookup(this.object, this.slot, _value) != NULL)
				continue;

			this.new_value = progn(VALUE(r), this.var_frame);

			if (find_daemon(this.object, this.slot, _cache) == _yes)
				put_value();
		}
}


/************************************************************************/
/*		Create new generic or instance frames			*/
/************************************************************************/

static void
create_daemons(term obj, term attr, term val)
{
	if (TYPE(val) != FN)
		fail("Incorrect daemon");

	if (ARITY(val) == 1)
		new_slot(obj, attr, ARG(0, val), ARG(1, val));
	else
	{
		create_daemons(obj, attr, ARG(1, val));
		new_slot(obj, attr, ARG(0, val), ARG(2, val));
	}
}


static void
create_values(term obj, term attr, term val)
{
	this.object = obj;
	this.slot = attr;
	this.new_value = val;
	put_value();
}


static void
traverse(term obj, term plist,  void (*what_to_do)())
{
	if (plist == NULL)
		return;
	if (TYPE(plist) != FN)
		fail("Missing semicolon in frame definition");
	if (ARG(0, plist) == _semi_colon)
	{
		traverse(obj, ARG(1, plist), what_to_do);
		traverse(obj, ARG(2, plist), what_to_do);
		return;
	}
	if (ARG(0, plist) == _colon)
		(* what_to_do)(obj, ARG(1, plist), ARG(2, plist));
}


static term
new_frame(term name, term body, void (*what_to_do)())
{
	term parents = NULL, attributes = NULL;
	term rval, fr;

	if (TYPE(body) == FN && ARG(0, body) == _with)
	{
		parents = ARG(1, body);
		attributes = ARG(2, body);
	}
	else
	{
		parents = body;
		attributes = NULL;
	}

	switch (TYPE(parents))
	{
	  case ATOM:	parents = hcons(parents, _nil);
	  case LIST:	break;
	  default:	fail("Inheritance list is incorrect");
	}

	new_slot(name, _isa, _value, parents);
	traverse(name, attributes, what_to_do);
	return(name);
}


static int
new_generic(term goal, term *var_frame)
{
	term name = check_arg(1, goal, var_frame, ATOM, IN);
	term body = check_arg(2, goal, var_frame, ANY, IN);
	new_frame(name, body, create_daemons);
	return(TRUE);
}


static int
new_instance(term goal, term *var_frame)
{
	term name = check_arg(1, goal, var_frame, ATOM, IN);
	term body = check_arg(2, goal, var_frame, ANY, IN);
	SAVE(var_frame);
	this.object = new_frame(name, body, create_values);
	send_new(this.object);
	RETURN(int, TRUE);
}


static term
make_name(void)
{
	static int i = 0;
	char buf[16];

	sprintf(buf, "frame%d", i++);
	return(intern(buf));
}


static term
make_frame(term goal, term *var_frame)
{
	term name = make_name();
	term body = check_arg(1, goal, var_frame, ANY, IN);
	SAVE(var_frame);
	this.object = new_frame(name, body, create_values);
	send_new(this.object);
	RETURN(term, name);
}


/************************************************************************/
/*			User callable utilities				*/
/************************************************************************/


void
print_frame(term x)
{
	term y = find_clause(x, _isa, _value);
	term p;
	char *end_line = " with\n";

	if (y == NULL)
		fail("No such frame exists");

	fprintf(output, "%s isa ", NAME(x));
	prin(VALUE(y));

	for (p = PROC(_value); p != NULL; p = NEXT(p))
	{
		term f = HEAD(p);

		if (FNAME(f) == x && ATTR(f) != _isa)
		{
			fputs(end_line, output);
			end_line = ";\n";
			fprintf(output, "\t%s: ", NAME(ATTR(f)));
			/* prin(VALUE(f)); */
			print_rule(VALUE(f));
		}
	}
	fputs("!\n", output);
}


static int
pf(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);

	print_frame(x);
	return(TRUE);
}


static int
print_daemon(term goal, term *frame)
{
	term generic_name = check_arg(1, goal, frame, ATOM, IN);
	term slot_name    = check_arg(2, goal, frame, ATOM, IN);
	term facet_name   = check_arg(3, goal, frame, ATOM, IN);
	term p = lookup(generic_name, slot_name, facet_name);

	fputs("\n\t", output);
	print_goal(p, 0);
	fputs("\n\n", output);
	return(TRUE);
}


static term
ask(term goal, term *frame)
{
	int i;
	FILE *old_input = input;
	FILE *old_output = output;

#ifdef THINK_C
	{
		extern FILE *open_dialog();
		input = output = open_dialog();
	}
#else
	input = stdin;
	output = stdout;
#endif
	do
	{
		if (this.object != NULL && TYPE(goal) == ATOM)
			fputs(NAME(this.slot), output);
		else
			for (i = 1; i <= ARITY(goal); i++)
				rprin(ARG(i, goal), frame);

		fflush(output);
		this.new_value = get_atom();
	}
	while (this.object != NULL && ! in_range(this.object));

#ifdef THINK_C
	fclose(input);
#endif

	input = old_input;
	output = old_output;

	return(this.new_value);
}


static term
p_this(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);

	if (this.object == NULL)
		fail("\"this ...\" can only be used inside a generic frame");

	if (x == _slot)
		return(this.slot);
	return(this.object);
}


static term
new(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);

	if (this.object == NULL)
		fail("\"new value\" can only be used inside a generic frame");

	return(this.new_value);
}


static term
old(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);

	if (this.object == NULL)
		fail("\"old value\" can only be used inside a generic frame");

	if (this.old_value == NULL)
		fail("\"old value\" is undefined");

	return(this.old_value);
}


static int
must_be_help(term g, term parents)
{
	term p;

	for (p = parents; p != NULL; p = CDR(p))
		if (g == CAR(p))
			return(TRUE);
	for (p = parents; p != NULL; p = CDR(p))
		if (must_be_help(g, lookup(CAR(p), _isa, _value)))
			return(TRUE);
	return(FALSE);
}


static int
must_be_a(term goal, term *frame)
{
	term f = check_arg(1, goal, frame, ATOM, IN);
	term g = check_arg(2, goal, frame, ATOM, IN);

	return(must_be_help(g, lookup(f, _isa, _value)));
}


static int
interval(term goal, term *frame)
{
	term lo_term = check_arg(1, goal, frame, NUMBER, IN);
	term hi_term = check_arg(2, goal, frame, NUMBER, IN);

	if (TYPE(this.new_value) == INT || TYPE(this.new_value) == REAL)
	{
		double lo = TYPE(lo_term) == REAL ? RVAL(lo_term) : ((double) IVAL(lo_term));
		double hi = TYPE(hi_term) == REAL ? RVAL(hi_term) : ((double) IVAL(hi_term));
		double x = TYPE(this.new_value) == REAL ? RVAL(this.new_value) : ((double) IVAL(this.new_value));

		return(lo <= x && x <= hi);
	}
	return(FALSE);
}


/************************************************************************/
/*				Initialise module			*/
/************************************************************************/

void
frame_init(void)
{
	_generic	= intern("generic");
	_object		= intern("object");
	_value		= intern("value");
	_slot		= intern("slot");
	_isa		= intern("isa");
	_yes		= intern("yes");
	_new		= intern("new");
	_old		= intern("old");

	_of		= intern("of");			defop(50,   XFY, _of);

	_from		= intern("from");		defop(600,  XFX, _from);
	_to		= intern("to");			defop(600,  XFX, _to);
	_by		= intern("by");			defop(600,  XFX, _by);

	_replace	= intern("replace");		defop(650,  FX,  _replace);
	_remove		= intern("remove");		defop(650,  FX,  _remove);
	_add		= intern("add");		defop(650,  FX,  _add);

	_dot_dot	= intern("..");			defop(700,  XFX, _dot_dot);

	_if_new		= intern("if_new");		defop(1040, FX,  _if_new);
							defop(1050, YFX, _if_new);
	_if_added	= intern("if_added");		defop(1040, FX,	 _if_added);
							defop(1050, YFX, _if_added);
	_if_needed	= intern("if_needed");		defop(1040, FX,  _if_needed);
							defop(1050, YFX, _if_needed);
	_if_removed	= intern("if_removed");		defop(1040, FX,  _if_removed);
							defop(1050, YFX, _if_removed);
	_if_replaced	= intern("if_replaced");	defop(1040, FX,  _if_replaced);
							defop(1050, YFX, _if_replaced);
	_default	= intern("default");		defop(1040, FX,  _default);
							defop(1050, YFX, _default);
	_cache		= intern("cache");		defop(1040, FX,  _cache);
							defop(1050, YFX, _cache);
	_range		= intern("range");		defop(1040, FX,  _range);
							defop(1050, YFX, _range);
	_help		= intern("help");		defop(1040, FX,	 _help);
							defop(1050, YFX, _help);
	_multivalued	= intern("multivalued");	defop(1040, FX,  _multivalued);
							defop(1050, YFX, _multivalued);

	_colon		= intern(":");			defop(1090, XFX, _colon);
	_with		= intern("with");		defop(1110, XFX, _with);

	defop(1120, XFX, new_fpred(new_generic, "ako"));
	defop(1120, XFX, new_fpred(new_instance, "isa"));
	defop(50, FX, new_subr(make_frame, "make"));
	defop(700, FX, new_subr(have_property, "have_property"));

	new_subr(fget, "of");
	new_fpred(fput, "add");
	new_fpred(freplace, "replace");
	new_fpred(fremove, "remove");

	defop(700, FX, new_pred(pf, "pf"));
	new_pred(print_daemon, "print_daemon");
	new_pred(print_daemon, "print_demon");
	new_subr(ask, "ask");
	defop(50, XFX, new_pred(must_be_a, "must_be_a"));
	defop(50, XFX, new_pred(must_be_a, "instance_of"));
//	new_pred(interval, "..");
	defop(40, FX, new_subr(p_this, "this"));
	defop(40, FX, new_subr(new, "new"));
	defop(40, FX, new_subr(old, "old"));
}
