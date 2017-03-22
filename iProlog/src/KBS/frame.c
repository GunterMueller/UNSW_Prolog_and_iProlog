/************************************************************************/
/*     	Implemantation of Frame System with procedural attachments	*/
/************************************************************************/

#include <stdarg.h>
#include "prolog.h"
#include "frame.h"
#include "print_rule.h"

static term
	_if_new, _if_added, _if_needed, _if_removed, _if_replaced,
	_default, _cache, _range, _help, _multivalued,
	_fget_macro, fget_goal, _with;


/************************************************************************/
/* 	Apply a daemon, taking care of "new value" and "old value"	*/
/************************************************************************/

static term current_object = NULL;
static term current_slot = NULL;
static term new_value = NULL;
static term old_value = NULL;

static term
apply(term daemon, term obj, term slot, term new_val, term old_val)
{
	term store_current_object = current_object;
	term store_current_slot = current_slot;
	term store_new_value = new_value;
	term store_old_value = old_value;
	term rval;

	current_object = obj;
	current_slot = slot;
	new_value = new_val;
	old_value = old_val;

	rval = progn(daemon, NULL);

	current_object = store_current_object;
	current_slot = store_current_slot;
	new_value = store_new_value;
	old_value = store_old_value;

	return rval;
}


/************************************************************************/
/* 			Get daemons that are inherited			*/
/************************************************************************/

static term
get_daemon(term obj, term slot, term facet)
{
	term p, q, r, rval;

	if (PLIST(slot) != _nil)
		return getpropval(slot, facet);

	for (p = INHERITS(obj); p != _nil; p = CDR(p))
	{
		for (q = PLIST(CAR(p)); q != _nil; q = CDR(q))
			if (SLOT_NAME(CAR(q)) == slot)
				for (r = VALUE(CAR(q)); r != _nil; r = CDR(r))
					if (FACET(CAR(r)) == facet)
						return DAEMON(CAR(r));

		if ((rval = get_daemon(CAR(p), slot, facet)) != NULL)
			return rval;
	}

	return NULL;
}


/************************************************************************/
/*	Public routine to get a facet of a slot from a frame		*/
/*	Used by GUI							*/
/************************************************************************/

term
get_facet(term object, term slot, term facet)
{
	term p, prop = getprop(object, slot);

	if (prop == NULL)
		return NULL;

	for (p = VALUE(prop); p != _nil; p = CDR(p))
		if (facet == FACET(CAR(p)))
			return DAEMON(CAR(p));

	return NULL;
}


/************************************************************************/
/*	Public routine to add a facet to a slot in a frame		*/
/*	Used by GUI							*/
/************************************************************************/

void
put_facet(term obj, term prop, term facet, term daemon)
{
	term *p;
	term slot = getprop(obj, prop);

	if (slot == NULL)
		slot = putprop(obj, prop, _nil);
	
	for (p = &VALUE(slot); *p != _nil; p = &CDR(*p))
	{
		term q = CAR(*p);

		if (FACET(q) == facet)
		{
			free_term(DAEMON(q));
			DAEMON(q) = daemon;
			return;
		}
	}

	*p = hcons(h_fn1(facet, daemon), _nil);
}


/************************************************************************/
/*		Prolog routine to manipulate daemons			*/
/************************************************************************/

static int
p_facet(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term facet = check_arg(3, goal, frame, ATOM, IN);
	term val = check_arg(4, goal, frame, ANY, OUT);

	switch (TYPE(val))
	{
		case ANON:
		case FREE:
		case BOUND:
		case REF:
		{
			term rval = get_facet(obj, prop, facet);

			if (rval == NULL)
				return FALSE;
			else
				return unify(val, frame, rval, NULL);

		}
		default:
			put_facet(obj, prop, facet, make(val, frame));
			return TRUE;
	}
	
}

/************************************************************************/
/* when a new value is put into a slot a range check is performed	*/
/* if the value is out of range a help daemon is invoked (if present)	*/
/************************************************************************/

static int
interval(term goal, term *frame)
{
	term lo_term = check_arg(1, goal, frame, NUMBER, IN);
	term hi_term = check_arg(2, goal, frame, NUMBER, IN);

	if (TYPE(new_value) == INT || TYPE(new_value) == REAL)
	{
		double lo = TYPE(lo_term) == REAL ? RVAL(lo_term) : ((double) IVAL(lo_term));
		double hi = TYPE(hi_term) == REAL ? RVAL(hi_term) : ((double) IVAL(hi_term));
		double x = TYPE(new_value) == REAL ? RVAL(new_value) : ((double) IVAL(new_value));

		return lo <= x && x <= hi;
	}
	return FALSE;
}


static int
in_range(term obj, term slot, term val)
{
	term help, range = get_daemon(obj, slot, _range);

	if (range == NULL)
		return TRUE;

	switch (TYPE(range))
	{
		case LIST:
			if (member(val, range, NULL))
				return TRUE;
			break;
		case ATOM:
		{
			static term temp;

			if (temp == NULL)
				temp = new_h_fn(1);

			ARG(0, temp) = range;
			ARG(1, temp) = val;

			if (apply(temp, NULL, NULL, NULL, NULL) == _true)
				return TRUE;
			break;
		}
		default:
			if (apply(range, obj, slot, val, NULL) == _true)
				return TRUE;
			break;
	}

	if ((help = get_daemon(obj, slot, _help)) != NULL)
		apply(help, obj, slot, val, NULL);

	return FALSE;
}


/************************************************************************/
/*		Routine for external calls to "in_range"		*/
/************************************************************************/

int
check_range(term val)
{
	if (current_object == NULL || current_slot == NULL)
		fail("Can't check range outside an object");

	return in_range(current_object, current_slot, val);
}


/************************************************************************/
/* Put a new value in a slot.						*/
/* First call the range check,						*/
/* then check if there is a value already and the slot is multivalued	*/
/* Check to see if the same value already exists			*/
/* Once added, the if_added daemon is invoked.				*/
/* Modified to create variable stack frame				*/
/************************************************************************/

static int
put_value_f(term obj, term prop, term val, term *frame)
{
	term if_added, old_slot = getprop(current_object, prop);

	if (! in_range(obj, prop, val))
		return FALSE;

	if (get_daemon(obj, prop, _multivalued) == _true)
	{
		if (old_slot == NULL)
			putprop(current_object, prop, hcons(make(val, frame), _nil));
		else if (member(val, VALUE(old_slot), frame))
				return TRUE;
		else
			VALUE(old_slot) = hcons(make(val, frame), VALUE(old_slot));
	}
	else if (old_slot != NULL)
	{
		if (unify(val, frame, VALUE(old_slot), frame))
			return TRUE;
		fail("Tried to add a new value to a slot that is not multivalued");
	}
	else
		putprop(current_object, prop, make(val, frame));

	if ((if_added = get_daemon(obj, prop, _if_added)) != NULL)
		apply(if_added, obj, prop, val, NULL);

	return TRUE;
}

static int
put_value(term obj, term prop, term val)
{
    return put_value_f(obj, prop, val, NULL);
}

static int
p_fput(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term val = check_arg(3, goal, frame, ANY, IN);
	term store_current_object = current_object;
	int rval;

	current_object = obj;
	rval = put_value_f(obj, prop, val, frame);
	current_object = store_current_object;
	return rval; 
}


/************************************************************************/
/* 			infix operator version of fput			*/
/************************************************************************/

static int
p_has_value(term goal, term *frame)
{
	term x   = check_arg(1, goal, frame, ANY, OUT);
	term val = check_arg(2, goal, frame, ANY, EVAL);
	term store_current_object = current_object;
	term obj, prop;
	int rval;

	if (val == NULL)
		return(FALSE);

	switch (TYPE(x))
	{
	case ATOM:
		if (current_object == NULL)
			fail("slot is not defined");

		obj = current_object;
		prop = x;
		break;
	case FN:
		obj = eval(ARG(1, x), frame);
		prop = ARG(0, x);
		break;
	default:
		fail("left hand side is not a valid slot");
	}

	current_object = obj;
	rval = put_value_f(obj, prop, val, frame);
	current_object = store_current_object;

	return rval;
}


/************************************************************************/
/* 			Get a slot's default value			*/
/************************************************************************/

static term
get_default(term obj, term prop)
{
	term val;

	if ((val = get_daemon(obj, prop, _default)) != NULL)
	{
		term store_current_object = current_object;
		current_object = obj;

		val = apply(val, obj, prop, NULL, NULL);

		current_object = store_current_object;
		return val;
	}

	return NULL;
}


/************************************************************************/
/* 	Get a slot value, invoking "if_needed", if needed :-)		*/
/************************************************************************/

term
fget(term obj, term prop)
{
	term val;

	if ((val = getprop(obj, prop)) != NULL)
		return VALUE(val);
	if ((val = get_daemon(obj, prop, _if_needed)) != NULL)
	{
		term store_current_object = current_object;
		current_object = obj;

		val = apply(val, obj, prop, NULL, NULL);

		if (get_daemon(obj, prop, _cache) == _true)
			if (! put_value(obj, prop, val))
				val = NULL;

		current_object = store_current_object;
		return val; 
	}
	return NULL;
}


static term
p_fget(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);

	return fget(obj, prop);
}

/************************************************************************/
/* 		Fudge to allow slot names to be used as functions	*/
/************************************************************************/

static term
fget_macro(term goal, term *frame)
{
	if (TYPE(goal) == ATOM)
	{
		ARG(2, fget_goal) = goal;

		if (current_object != NULL)
			ARG(1, fget_goal) = current_object;
		else
			fail("Slot is undefined");
	}
	else
	{
		ARG(2, fget_goal) = check_arg(0, goal, frame, ATOM, IN);
		ARG(1, fget_goal) = check_arg(1, goal, frame, ATOM, IN);
	}

	return p_fget(fget_goal, frame);
}

/************************************************************************/
/* 				Remove a slot				*/
/************************************************************************/

int
fremove(term obj, term prop)
{
	term *p;

	for (p = &PLIST(obj); *p != _nil; p = &CDR(*p))
		if (SLOT_NAME(CAR(*p)) == prop)
		{
			term tmp = *p;
			term if_removed = get_daemon(obj, prop, _if_removed);

			if (if_removed != NULL)
				apply(if_removed, obj, prop, NULL, VALUE(CAR(tmp)));

			*p = CDR(*p);
			CDR(tmp) = _nil; /*protect rest of list from being freed */
			free_term(tmp);
			return TRUE;
		}

	return FALSE;
}


static int
p_fremove(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);

	return fremove(obj, prop);
}


static int
p_remove(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, IN);

	switch (TYPE(x))
	{
	case ATOM:
		if (current_object != NULL)
			return fremove(current_object, x);
		else
			fail("slot is not defined");
	case FN:
		return fremove(eval(ARG(1, x), frame), ARG(0, x));
	default:
		fail("left hand side is not a valid slot");
	}
}


/************************************************************************/
/* 				Replace a slot				*/
/************************************************************************/

int
freplace(term obj, term prop, term val, term *frame)
{
	term slot  = getprop(obj, prop);
	term if_replaced, old_val;

	if (! in_range(obj, prop, val))
		return FALSE;

	if (slot != NULL)
	{
		old_val = VALUE(slot);
		VALUE(slot) = make(val, frame);
	}
	else
	{
		old_val = NULL;
		putprop(obj, prop, make(val, frame));
	}
	if ((if_replaced = get_daemon(obj, prop, _if_replaced)) != NULL)
		apply(if_replaced, obj, prop, val, old_val);

	free_term(old_val);

	return TRUE;
}

static int
p_freplace(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term val = check_arg(3, goal, frame, ANY, IN);

	return freplace(obj, prop, val, frame);
}


static int
p_is_replaced_by(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term y = check_arg(2, goal, frame, ANY, EVAL);

	if (y == NULL)
		return(FALSE);
	switch (TYPE(x))
	{
	case ATOM:
		if (current_object != NULL)
			return freplace(current_object, x, y, frame);
		else
			fail("slot is not defined");
	case FN:
		return freplace(eval(ARG(1, x), frame), ARG(0, x), y, frame);
	default:
		fail("left hand side is not a valid slot");
	}
}


/************************************************************************/
/*			Process "if_new" daemons			*/
/************************************************************************/

static void
send_new(term inherits)
{
	for (; inherits != _nil; inherits = CDR(inherits))
	{
		term obj = CAR(inherits);
		term p;

		send_new(INHERITS(obj));
	
		for (p = PLIST(obj); p != _nil; p = CDR(p))
		{
			term slot = SLOT_NAME(CAR(p));
			term q, if_new = NULL;
			
			for (q = VALUE(CAR(p)); q != _nil; q = CDR(q))
				if (FACET(CAR(q)) == _if_new)
				{
					if_new = DAEMON(CAR(q));
					break;
				}
			
			if (if_new != NULL && getprop(current_object, slot) == NULL)
			{
				term val = apply(if_new, current_object, slot, NULL, NULL);

				if (get_daemon(current_object, slot, _cache) == _true)
					put_value(current_object, slot, val);
			}
		}
	}
}


/************************************************************************/
/* 	Public routines for creating and inspecting propery list	*/
/************************************************************************/

static int
p_generic(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term inherits = check_arg(2, goal, frame, LIST, IN);
	term slots = check_arg(3, goal, frame, LIST, IN);
	term store_current_object = current_object;
	term p;

	if (INHERITS(obj) != _nil)
		free_term(INHERITS(obj));

	if (PLIST(obj) != _nil)
		free_term(PLIST(obj));

	INHERITS(obj) = make(inherits, frame);
	PLIST(obj) = make(slots, frame);
	FLAGS(obj) |= GENERIC;

	for (p = PLIST(obj); p != _nil; p = CDR(p))
	{
		term slot_name = ARG(0, CAR(p));

		if (PROC(slot_name) == NULL)
			PROC(slot_name) = PROC(_fget_macro);
	}
	current_object = obj;
	current_object = store_current_object;

	return TRUE;
}


static void
index(term obj, term inherits)
{
	term p;

	if (inherits == _nil)
		return;

	for (p = inherits; p != _nil; p = CDR(p))
	{
		term parent = CAR(p);

		add_clause(mkclause(g_fn1(parent, obj), NULL), 0);
		index(obj, INHERITS(parent));
	}
}


void
make_instance(term obj, term inherits, term slots, term *frame)
{
	term store_current_object = current_object;

	current_object = obj;

	if (INHERITS(obj) != _nil)
	{
		free_term(INHERITS(obj));
		INHERITS(obj) = _nil;
	}

	if (PLIST(obj) != _nil)
	{
		free_term(PLIST(obj));
		PLIST(obj) = _nil;
	}

	INHERITS(obj) = make(inherits, frame);
	index(obj, INHERITS(obj));

	while (slots != _nil)
	{
		term slot_name = SLOT_NAME(CAR(slots));

		if (PROC(slot_name) == NULL)
			PROC(slot_name) = PROC(_fget_macro);

 		put_value_f(obj, slot_name, VALUE(CAR(slots)), frame);

		slots = CDR(slots);
		DEREF(slots);
	}

	send_new(INHERITS(obj));
	current_object = store_current_object;
}


static int
p_frame(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term inherits = check_arg(2, goal, frame, LIST, IN);
	term slots = check_arg(3, goal, frame, LIST, IN);
	
	make_instance(obj, inherits, slots, frame);

	return TRUE;
}


static term
p_new_instance(term goal, term *frame)
{
	term inherits = check_arg(1, goal, frame, LIST, IN);
	term obj = intern(date_time());
	
	make_instance(obj, inherits, _nil, frame);

	return obj;
}


static int
p_slot(term goal, term *frame)
{
	term slot_name = check_arg(1, goal, frame, ATOM, IN);

	if (PROC(slot_name) != NULL)
		fail("Tried to redefine a built-in as a slot");

	if (PLIST(slot_name) != _nil)
		fail("Tried to redefine a slot");

	PROC(slot_name) = PROC(_fget_macro);

	if (ARITY(goal) > 1)
		return p_frame(goal, frame);
	else
		return TRUE;
}


static int
p_instance(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term inherits = check_arg(2, goal, frame, LIST, IN);
	term plist = check_arg(3, goal, frame, LIST, IN);

	INHERITS(obj) = make(inherits, frame);
	PLIST(obj) = make(plist, frame);

	index(obj, INHERITS(obj));

	return TRUE;
}


/************************************************************************/
/* 			     Destroy frames				*/
/************************************************************************/

static void
remove_from_index(term obj, term inherits)
{
	term p;

	if (inherits == _nil)
		return;

	for (p = inherits; p != _nil; p = CDR(p))
	{
		term parent = CAR(p);
		term *proc = &PROC(parent);

		while (*proc != NULL)
			if (ARG(1, HEAD(*proc)) == obj)
			{
				term q = *proc;

				*proc = NEXT(*proc);
				free_term(q);
				break;
			}
			else
				proc = &NEXT(*proc);

		remove_from_index(obj, INHERITS(parent));
	}
}


int
delete_frame(term obj)
{
	if (FLAGS(obj) & GENERIC)
	{
		term p = PROC(obj);

		while (p != NULL)
		{
			term next = NEXT(p);

			print(ARG(1, HEAD(p)));
			delete_frame(ARG(1, HEAD(p)));
			p = next;
		}

		FLAGS(obj) &= ~GENERIC;
	}
	else
		remove_from_index(obj, INHERITS(obj));

	free_term(INHERITS(obj));
	INHERITS(obj) = _nil;
	free_term(PLIST(obj));
	PLIST(obj) = _nil;

	return TRUE;
}


static int
p_delete_frame(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);

	delete_frame(obj);

	return TRUE;
}


/************************************************************************/
/* 			     Delete slots and facets			*/
/************************************************************************/

int
delete_slot(term obj, term prop)
{
	term *p;

	if (! (FLAGS(obj) & GENERIC))
		return FALSE;

	for (p = &PLIST(obj); *p != _nil; p = &CDR(*p))
		if (SLOT_NAME(CAR(*p)) == prop)
		{
			term tmp = *p;

			*p = CDR(*p);
			CDR(tmp) = _nil; /*protect rest of list from being freed */
			free_term(tmp);
			return TRUE;
		}

	return FALSE;
}

static int
p_delete_slot(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);

	return delete_slot(obj, prop);
}


int
delete_facet(term obj, term prop, term facet)
{
	term p;

	if (! (FLAGS(obj) & GENERIC))
		return FALSE;

	for (p = PLIST(obj); p != _nil; p = CDR(p))
		if (SLOT_NAME(CAR(p)) == prop)
		{
			term *q;

			for (q = &VALUE(CAR(p)); *q != _nil; q = &CDR(*q))
				if (FACET(CAR(*q)) == facet)
				{
					term tmp = *q;

					*q = CDR(*q);
					CDR(tmp) = _nil; /*protect rest of list from being freed */
					free_term(tmp);
					return TRUE;
				}

			return FALSE;
		}

	return FALSE;
}


static int
p_delete_facet(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term facet = check_arg(3, goal, frame, ATOM, IN);

	return delete_facet(obj, prop, facet);
}


/************************************************************************/
/* 			     Ask user for input				*/
/************************************************************************/
/* 		Variable qa allows xforms gui to replace q_and_a	*/
/************************************************************************/

term (*qa)(term, term, term *);

static term
q_and_a(term question, term def, term *frame)
{
	FILE *old_input = input;
	FILE *old_output = output;
	term rval;

	input = dialog_in;
	output = dialog_out;

	if (ARITY(question) > 0)
		p_prin(question, frame);
	else
		prin(question);

	if (def != NULL)
	{
		fputs(" [", output);
		prin(def);
		fputs("]", output);
	}

	fputc('\n', output);
	fflush(output);

	if ((rval = get_atom()) == _dot)
		rval = def;

	input = old_input;
	output = old_output;

	return rval;
}

static term
ask_user(term goal, term *frame)
{
	term def = get_default(current_object, current_slot);
	term question, rval;

	do
	{
		if (getprop(current_object, current_slot))
			break;

		question = ARITY(goal) > 0 ? goal : current_slot;

		if (qa == NULL)
			rval = q_and_a(question, def, frame);
		else
			rval = (*qa)(question, def, frame);


	}
	while (rval == NULL || ! in_range(current_object, current_slot, rval));

	return rval;
}


/************************************************************************/
/* 			Hooks for new and old values			*/
/************************************************************************/

static term
p_cur_obj(term goal, term *frame)
{
	return current_object;
}

static term
p_cur_slot(term goal, term *frame)
{
	return current_slot;
}

static term
p_new_value(term goal, term *frame)
{
	return new_value;
}


static term
p_old_value(term goal, term *frame)
{
	return old_value;
}


/************************************************************************/
/*  Look through hash table for user defined predicates and print them	*/
/************************************************************************/

int
list_frames(void)
{
	extern term hashtable[];
	extern int display;
	int i;
	term p;

	display = TRUE;

	for (i = 0; i < HASHSIZE; i++)
		for (p = hashtable[i]; p != NULL; p = LINK(p))
		{
			if (p == _nil || PLIST(p) == _nil)
				continue;

			if (FLAGS(p) & GENERIC)
				fputs("generic(", output);
			else
				fputs("instance(", output);

			prin(p);
			fputs(", ", output);
			prin(INHERITS(p));
			fputs(", ", output);
			prin(PLIST(p));
			fputs(")!\n", output);
		}

	return TRUE;
}


/************************************************************************/
/*			Pretty Printing Routines			*/
/************************************************************************/

void
print_frame(term x)
{
	term p;
	char *end_line = " with\n";

	if (PLIST(x) == NULL)
		fail("No such frame exists");

	fprintf(output, "%s isa ", NAME(x));
	prin(INHERITS(x));

	for (p = PLIST(x); p != _nil; p = CDR(p))
	{
		term f = CAR(p);

		fputs(end_line, output);
		end_line = ";\n";
		fprintf(output, "\t%s: ", NAME(SLOT_NAME(f)));
		if (TYPE(VALUE(f)) == LIST)
		{
			term q;

			for (q = VALUE(f); q != _nil; q = CDR(q))
			{
				fputs("\n\t\t", output);
				print_rule(CAR(q));
			}
		}
		else
			print_rule(VALUE(f));
	}
	fputs("!\n", output);
}


static int
pf(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);

	print_frame(x);
	return TRUE;
}


/************************************************************************/
/*				Input Macros				*/
/************************************************************************/

static term
expand_ako(term x)
{
	term rval;

	if (TYPE(ARG(2, x)) != FN || ARG(0, ARG(2, x)) != _with)
		fail("Missing 'with'");

	rval = new_g_fn(3);
	ARG(0, rval) = intern("generic");
	ARG(1, rval) = ARG(1, x);
	ARG(2, rval) = ARG(1, ARG(2, x));
	if (TYPE(ARG(2, rval)) == ATOM && ARG(2, rval) != _nil)
		ARG(2, rval) = gcons(ARG(2, rval), _nil);
	ARG(3, rval) = ARG(2, ARG(2, x));
	return rval;
}

static term
expand_isa(term x)
{
	term rval;

	if (TYPE(ARG(2, x)) != FN || ARG(0, ARG(2, x)) != _with)
		fail("Missing 'with'");

	rval = new_g_fn(3);
	ARG(0, rval) = intern("frame");
	ARG(1, rval) = ARG(1, x);
	ARG(2, rval) = ARG(1, ARG(2, x));
	if (TYPE(ARG(2, rval)) == ATOM && ARG(2, rval) != _nil)
		ARG(2, rval) = gcons(ARG(2, rval), _nil);
	ARG(3, rval) = ARG(2, ARG(2, x));
	return rval;
}


/************************************************************************/
/*				init					*/
/************************************************************************/

void
frame_init(void)
{
	extern int rdr_frame_init();
	term _isa, _ako;

	rdr_frame_init();

	defop(997, FX, _if_new		= intern("if_new"));
	defop(997, FX, _if_added	= intern("if_added"));
	defop(997, FX, _if_needed	= intern("if_needed"));
	defop(997, FX, _if_removed	= intern("if_removed"));
	defop(997, FX, _if_replaced	= intern("if_replaced"));
	defop(997, FX, _default		= intern("default"));
	defop(997, FX, _cache		= intern("cache"));
	defop(997, FX, _range		= intern("range"));
	defop(997, FX, _help		= intern("help"));
	defop(997, FX, _multivalued	= intern("multivalued"));

	PLIST(_nil)			= _nil; /* fudge because initialisation of [] */
	
	_fget_macro			= intern("fget_macro");
	fget_goal 			= h_fn2(_fget_macro, NULL, NULL);
	new_subr(fget_macro, 		"fget_macro");

	new_pred(p_facet,		"facet");
	new_pred(p_fput,		"fput");
	new_subr(p_fget,		"fget");	
	new_pred(p_fremove,		"fremove");
	new_pred(p_freplace,		"freplace");

	new_subr(p_cur_obj,		"current_object");	
	new_subr(p_cur_slot,		"current_slot");	
	new_subr(p_new_value,		"new_value");	
	new_subr(p_old_value,		"old_value");	
	new_pred(p_generic, 		"generic");
	new_pred(p_instance, 		"instance");
	new_pred(p_frame, 		"frame");
	new_pred(p_delete_frame, 	"delete_frame");
	new_pred(p_delete_slot, 	"delete_slot");
	new_pred(p_delete_facet, 	"delete_facet");
	new_pred(p_slot, 		"slot");
	new_subr(ask_user, 		"ask_user");

	defop(40, FX, new_subr(p_new_instance,	 	"new"));
	defop(700, XFX, new_fpred(p_has_value,		"has_value"));
	defop(700, XFX, new_fpred(p_is_replaced_by,	"is_replaced_by"));
	defop(700,  FX, new_fpred(p_remove,		"remove"));

	defop(700, XFX, new_pred(interval, ".."));
	defop(700, FX, new_pred(pf, "pf"));

	defop(1120, XFX, _ako = intern("ako"));
	TERM_EXPAND(_ako) = expand_ako;
	defop(1120, XFX, _isa = intern("isa"));
	TERM_EXPAND(_isa) = expand_isa;
	defop(1110, XFX, _with = intern("with"));
}
