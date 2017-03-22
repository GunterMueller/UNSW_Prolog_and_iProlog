/************************************************************************/
/*     		Functions to manipulate an atom's property list		*/
/************************************************************************/

#include <stdarg.h>
#include "prolog.h"

static term self = NULL;


/************************************************************************/
/*			Add a property/value pair			*/
/************************************************************************/

term
putprop(term obj, term prop, term val)
{
	term *p;

	for (p = &PLIST(obj); *p != _nil; p = &CDR(*p))
		if (PROPERTY(CAR(*p)) == prop)
			return NULL;

	*p = hcons(h_fn1(prop, val), _nil);
	return CAR(*p);
}


static int
p_putprop(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term val = check_arg(3, goal, frame, ANY, IN);

	if (putprop(obj, prop, make(val, frame)) == NULL)
		fail("Property already exists");
	return TRUE;
}


/************************************************************************/
/*			Get a property/value pair			*/
/************************************************************************/

term
getprop(term obj, term prop)
{
	term p;

	for (p = PLIST(obj); p != _nil; p = CDR(p))
		if (PROPERTY(CAR(p)) == prop)
			return CAR(p);
	return NULL;
}

static term
p_getprop(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term rval = getprop(obj, prop);

	if (rval == NULL)
		fail("Property does not exists");

	return VALUE(rval);
}


/************************************************************************/
/* 			Remove a property/value pair			*/
/************************************************************************/

int
remprop(term obj, term prop)
{
	term *p;

	for (p = &PLIST(obj); *p != _nil; p = &CDR(*p))
		if (PROPERTY(CAR(*p)) == prop)
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
p_remprop(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);

	return remprop(obj, prop);
}

/************************************************************************/
/*	Public routines for adding a set of property/value pairs     	*/
/************************************************************************/

term
build_plist(char *isa, ...)
{
	va_list ap;
	char *prop;
	term obj = gensym(isa);

	va_start(ap, isa);
	while (prop = va_arg(ap, char *))
	{
		term val = va_arg(ap, term);

		putprop(obj, intern(prop), val);
	}
	va_end(ap);

	return obj;
}


term
build_plist_named(char *isa, ...)
{
	va_list ap;
	char *prop;
	term obj = intern(isa);

	va_start(ap, isa);
	while (prop = va_arg(ap, char *))
	{
		term val = va_arg(ap, term);

		putprop(obj, intern(prop), val);
	}
	va_end(ap);

	return obj;
}


/************************************************************************/
/*	Prolog function to get or set an atom's inheritance list	*/
/************************************************************************/

static int
p_properties(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);

	switch (ARITY(goal))
	{
	case 1:	{
			term p;

			fprintf(output, "\n\t%-10s", "inherits");
			print(INHERITS(obj));

			for (p = PLIST(obj); p != _nil; p = CDR(p))
			{
				term f = CAR(p);

				fprintf(output, "\t%-10s", NAME(CAR(f)));
				print(CDR(f));
			}
		}
		break;
	case 2:	{
			term plist = check_arg(2, goal, frame, LIST, OUT);

			if (TYPE(plist) == LIST)
			{
				if (PLIST(obj) != NULL)
					free_term(PLIST(obj));
				PLIST(obj) = make(plist, frame);
			}
			else if (! unify(plist, frame, PLIST(obj), NULL))
				return FALSE;
		}
		break;
	case 3:	{
			term inherits = check_arg(2, goal, frame, LIST, OUT);
			term plist = check_arg(3, goal, frame, LIST, OUT);

			if (TYPE(inherits) == LIST)
			{
				if (INHERITS(obj) != NULL)
					free_term(INHERITS(obj));
				INHERITS(obj) = make(inherits, frame);
			}
			else if (! unify(inherits, frame, INHERITS(obj), NULL))
				return FALSE;

			if (TYPE(plist) == LIST)
			{
				if (PLIST(obj) != NULL)
					free_term(PLIST(obj));
				PLIST(obj) = make(plist, frame);
			}
			else if (! unify(plist, frame, PLIST(obj), NULL))
				return FALSE;
		}
		break;
	}
	return TRUE;
}


/************************************************************************/
/*	Prolog predicate to get or set a property/value pair		*/
/************************************************************************/

static int
p_property(term goal, term *frame)
{
	term obj = check_arg(1, goal, frame, ATOM, IN);
	term prop = check_arg(2, goal, frame, ATOM, IN);
	term val = check_arg(3, goal, frame, ANY, OUT);

	switch (TYPE(val))
	{
		case ANON:
		case FREE:
		case BOUND:
		case REF:
		{
			term rval = getprop(obj, prop);
			
			if (rval == NULL)
				return FALSE;
			else
				return unify(val, frame, rval, NULL);
			
		}
		default:
			if (putprop(obj, prop, make(val, frame)) == NULL)
				fail("Property already exists");
			return TRUE;
	}
}


/************************************************************************/
/* 		     Get property value with inheritance		*/
/************************************************************************/

term
getpropval(term obj, term prop)
{
	term p;

	for (p = PLIST(obj); p != _nil; p = CDR(p))
		if (PROPERTY(CAR(p)) == prop)
			return VALUE(CAR(p));

	for (p = INHERITS(obj); p != _nil; p = CDR(p))
	{
		term rval;

		if ((rval = getpropval(CAR(p), prop)) != NULL)
			return rval;
	}

	return NULL;
}


static term
p_getpropval(term goal, term *frame)
{
	term prop = check_arg(1, goal, frame, ATOM, IN);
	term obj = check_arg(2, goal, frame, ATOM, IN);
	term rval = getpropval(obj, prop);

	if (rval == NULL)
		fail("Tried to get non-existent property");

	self = obj;
	rval = eval(rval, frame);
	self = NULL;
	return rval;
}


/************************************************************************/
/*			access local slot values   	  	*/
/************************************************************************/

static term
p_my(term goal, term *frame)
{
	term rval, prop = check_arg(1, goal, frame, ATOM, IN);

	if (self == NULL)
		fail("Can't use 'my' outside a frame");

	rval = getpropval(self, prop);

	if (rval == NULL)
		fail("Tried to get non-existent property");
	else
		return rval;
}


/************************************************************************/
/*			Read macro to convert p:v to p(v)   	  	*/
/************************************************************************/

static term
expand_colon(term x)
{
	return g_fn1(ARG(1, x), ARG(2, x));
}


/************************************************************************/
/*			Save all frames to a file			*/
/************************************************************************/

static int
save_properties(term goal, term *frame)
{
	extern int display;
	extern term hashtable[];
	term fname = check_arg(1, goal, frame, ATOM, IN);
	FILE *old_output = output;
	term p;
	int i;

	output = fopen(NAME(fname), "w");
	display = TRUE;
	
	for (i = 0; i < HASHSIZE; i++)
	{
		for (p = hashtable[i]; p != 0; p = LINK(p))
		{
			if ((INHERITS(p) != _nil || PLIST(p) != _nil)
       			&&  (INHERITS(p) != NULL && PLIST(p) != NULL))
			{
				fprintf(output, "properties(");
				prin(p);
				fprintf(output, ", ");
				prin(INHERITS(p));
				fprintf(output, ", ");
				prin(PLIST(p));
				fprintf(output, ")!\n");
			}
		}
	}

	fclose(output);
	output = old_output;
	return TRUE;
}


/************************************************************************/
/*				init					*/
/************************************************************************/

void
plist_init(void)
{
	term _colon;

	PLIST(_nil)		= _nil; /* fudge because initialisation of [] */
	
	new_pred(p_properties,		"properties");
	new_pred(p_property,		"property");
	new_pred(p_putprop,		"putprop");
	new_subr(p_getprop,		"getprop");
	new_pred(p_remprop,		"remprop");
	new_pred(save_properties,	"save_properties");

	defop(998,	XFX,	_colon = new_subr(expand_colon, ":"));
	defop(50,	XFY,	new_subr(p_getpropval, "of"));
	defop(60,	FX,	new_subr(p_my, "my"));

	TERM_EXPAND(_colon) = expand_colon;
}
