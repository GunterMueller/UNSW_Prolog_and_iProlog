#include <string.h>
#include "prolog.h"

term	_nil, _true, _false, _anon, _prompt, _prolog_prompt, _op,
	_end_of_file, _file, _rbrace, _rpren, _rbrac, _lpren, _lbrac,
	_lbrace, _neck, _dot, _bang, _question, _table, _export, _import, _bar,
	_comma, _equal, _plus, _minus, _user_prompt, _arrow, _semi_colon,
	_double_arrow;


/************************************************************************/
/*		Hash table used to uniquely store atoms			*/
/************************************************************************/


term hashtable[HASHSIZE];


static
hash_init(void)
{
	int i;

	for (i = 0; i < HASHSIZE ; i++)
		hashtable[i] = 0;
}


/*
static int
hash(char *string)
{
	int h = 0;

	while (*string)
		h += *string++;
	return(h & 0377);
}
*/

static unsigned long
hash(char *string)
{
	static int m[] = {16, 8, 0};
	unsigned long int h = 0;
	int k = 0;

	while (*string)
	{
		h += (unsigned long)(*string++) << m[k];
		k = ++k % 3;
	}
	return(h % HASHSIZE);
}


static term
new_atom(char *string)
{
	term rval = halloc(sizeof(atom) + strlen(string));

	TYPE(rval) = ATOM;
	FLAGS(rval) = 0;
	MACRO(rval) = NULL;
	PORTRAY(rval) = NULL;
	INHERITS(rval) = _nil;
	PLIST(rval) = _nil;
	PROC(rval) = NULL;
	PREFIX(rval) = INFIX(rval) = POSTFIX(rval) = 0;
	LINK(rval) = NULL;
	strcpy(NAME(rval), string);
	return rval;
}
	

term
intern(char *string)
{
	term p;
	unsigned long h = hash(string);

	for (p = hashtable[h]; p != 0; p = LINK(p))
		if (strcmp(string, NAME(p)) == 0)
			return(p);

	p = new_atom(string);
	LINK(p) = hashtable[h];
	hashtable[h] = p;
	return(p);
}


/************************************************************************/
/*	     	Create a new atom with an arbitrary name		*/
/************************************************************************/

term
gensym(char *base)
{
	static int n = 0;
	char buf[32];

	sprintf(buf, "%s%d", base, n++);
	return(intern(buf));
}


void
defop(int prec, int op_t, term a)
{
	FLAGS(a) |= (unsigned char) OP;
	switch (op_t)
	{
	   case FX:	PREFIX(a) = NONASS | (short) prec;
			break;
	   case FY:	PREFIX(a) = (short)  prec;
			break;
	   case XFX:	INFIX(a) = NONASS | (short) prec;
			break;
	   case XFY:	INFIX(a) = RIGHT | (short) prec;
			break;
	   case YFX:	INFIX(a) = (short) prec;
			break;
	   case XF:	POSTFIX(a) = NONASS | (short) prec;
			break;
	   case YF:	POSTFIX(a) = (short) prec;
			break;
	}
}


/************************************************************************/
/*			User callable predicates			*/
/************************************************************************/


static int
atom_length(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);
	term y = check_arg(2, goal, frame, INT, OUT);

	return(unify(y, frame, new_int(strlen(NAME(x))), frame));
}


static int
atom_concat(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);
	term y = check_arg(2, goal, frame, ATOM, IN);
	term z = check_arg(3, goal, frame, ATOM, OUT);
	char buf[TOKEN_LENGTH];

	sprintf(buf, "%s%s", NAME(x), NAME(y));

	return unify(z, frame, intern(buf), frame);
}


static term
get_sub_atom(term goal, term *frame)
{
	term Atom = check_arg(1, goal, frame, ATOM, IN);
	term N    = check_arg(2, goal, frame, INT, IN);
	term L    = check_arg(3, goal, frame, INT, IN);
	term Sub  = check_arg(4, goal, frame, ATOM, OUT);
	int n = IVAL(N), len = IVAL(L);
	char buf[TOKEN_LENGTH];

	if (n <= 0 || len <= 0 || strlen(NAME(Atom)) < n + len - 1)
		fail("substring index is out of range");

	strncpy(buf, NAME(Atom) + n - 1, len);
	buf[len] = '\0';
	return(intern(buf));
}


static int
find_sub_atom(term goal, term *frame)
{
	term A = check_arg(1, goal, frame, ATOM, IN);
	term N = check_arg(2, goal, frame, INT, OUT);
	term L = check_arg(3, goal, frame, INT, OUT);
	term S = check_arg(4, goal, frame, ATOM, IN);
	char *p = NAME(A), *q = NAME(S);
	int i, l1 = strlen(p), l2 = strlen(q);

	if (TYPE(L) != INT)
		unify(L, frame, new_int(l2), frame);
	else if (IVAL(L) != l2)
			return(FALSE);		

	for (i = 0; i <= l1 - l2; i++)
		if (strncmp(p++, q, l2) == 0)
		{
			if (unify(N, frame, new_int(i + 1), frame))
				if (rest_of_clause())
					break;

			if (TYPE(N) == REF)
				POINTER(N) = NULL;
		}
	return(FALSE);
}


static int
sub_atom(term goal, term *frame)
{
	term x = check_arg(4, goal, frame, ATOM, OUT);

	if (TYPE(x) == ATOM)
		return(find_sub_atom(goal, frame));
	else
		return(unify(x, frame, get_sub_atom(goal, frame), frame));
}


static int
char_code(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, OUT);
	term y = check_arg(2, goal, frame, INT, OUT);

	if (TYPE(x) == ATOM)
		return(unify(y, frame, new_int(NAME(x)[0]), frame));
	if (TYPE(y) == INT)
	{
		char buf[2] = " ";

		buf[0] = (char) IVAL(y);
		return(unify(x, frame, intern(buf), frame));
	}
}


static int
concat(term x, term y, term *frame)
{
	if (TYPE(x) == ATOM)
	{
		term p = _nil, *q = &p;
		char *s = NAME(x);

		while (*s)
		{
			char buf[2] = {'\0', '\0'};

			buf[0] = *s++;
			*q = gcons(intern(buf), _nil);
			q = &CDR(*q);
		}
		return(unify(y, frame, p, frame));
	}
	
	if (TYPE(y) == LIST)
	{
		char buf[256] = "", *p = buf;

		while (y != _nil)
		{ 
			term z;

			if (TYPE(y) != LIST)
				fail("malformed list of atoms");
			z = unbind(CAR(y), frame);

			switch (TYPE(z))
			{
			case ATOM:	sprintf(p, "%s", NAME(z));
					break;
			case INT:	sprintf(p, "%ld", IVAL(z));
					break;
			case REAL:	sprintf(p, "%g", RVAL(z));
					break;
			default:	fail("atom_chars and concat only work on atomic terms");
			}
			p = buf + strlen(buf);
			y = CDR(y);
			DEREF(y);
		}
		*p = '\0';
		return unify(x, frame, intern(buf), frame);
	}

	fail("Either 1st or 2nd argument or both must be instantiated");
}


static int
p_concat(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, OUT);
	term y = check_arg(2, goal, frame, ATOM, OUT);

	return(concat(y, x, frame));
}


static int
atom_chars(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, OUT);
	term y = check_arg(2, goal, frame, LIST, OUT);

	return(concat(x, y, frame));
}


static int
number_chars(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, NUMBER, OUT);
	term y = check_arg(2, goal, frame, LIST, OUT);
	char buf[64], *s = buf;

	*s = '\0';

	if (TYPE(x) == INT)
		sprintf(buf, "%d", IVAL(x));
	else if (TYPE(x) == REAL)
		sprintf(buf, "%g", RVAL(x));

	if (*s)
	{
		term p = _nil, *q = &p;

		while (*s)
		{
			char buf[2] = {'\0', '\0'};

			buf[0] = *s++;
			*q = gcons(intern(buf), _nil);
			q = &CDR(*q);
		}
		return(unify(y, frame, p, frame));
	}
	
	if (TYPE(y) == LIST)
	{
		char *p;
		long i;
		double d;

		while (y != _nil)
		{ 
			term z;

			if (TYPE(y) != LIST)
				fail("malformed list in first argument");
			z = unbind(CAR(y), frame);

			if (TYPE(z) != ATOM || strlen(NAME(z)) != 1)
				fail("list elements must be single character atoms");

			*s++ = NAME(z)[0];
			y = CDR(y);
			DEREF(y);
		}
		*s = '\0';
		i = strtol(buf, &p, 10);
		if (*p == '\0')
			return unify(x, frame, new_int(i), frame);
		d = strtod(buf, &p);
		if (*p == '\0')
			return unify(x, frame, new_real(d), frame);
		fail("list elements do not form a valid number");
	}

	fail("Either 1st or 2nd argument or both must be instantiated");
}


/************************************************************************/
/*		Useful for looking at density of hash table		*/
/************************************************************************/

static int
dump_hash(term goal, term *frame)
{
	term p;
	int i;

	for (i = 0; i < HASHSIZE; i++)
	{
		printf("%3d:", i);
		for (p = hashtable[i]; p != 0; p = LINK(p))
		{
			fputc(' ', output);
			prin(p);
		}
		fputc('\n', output);
	}
	return(TRUE);
}


/************************************************************************/
/*				init					*/
/************************************************************************/


void
atom_init(void)
{
	hash_init();

	_nil		= intern("[]");
	_true		= intern("true");
	_false		= intern("false");	FLAGS(_false) |= DYNAMIC;
	_anon		= intern("_");
	_op		= intern("op");
	_end_of_file	= intern("end_of_file");
	_file		= intern("file");
	_prolog_prompt	= intern(": ");
	_user_prompt	= intern("? ");
	_prompt		= _prolog_prompt;

	_rbrace		= intern("}");		defop(1201, YF,  _rbrace);
	_rpren		= intern(")");		defop(1201, YF,  _rpren);
	_rbrac		= intern("]");		defop(1201, YF,  _rbrac);
	_lpren		= intern("(");		//defop(1200, FY,  _lpren);
	_lbrac		= intern("[");		//defop(1200, FY,  _lbrac);
	_lbrace		= intern("{");		//defop(1200, FY,  _lbrace);
	_double_arrow	= intern("-->");	defop(1200, XFX, _double_arrow);
	_neck		= intern(":-");		defop(1200, XFX, _neck);
						defop(1200, FX,  _neck);
	_dot		= intern(".");		defop(1200, XF,  _dot);
	_bang		= intern("!");		defop(1200, XF,  _bang);
	_question	= intern("?");		defop(1200, XF,  _question);
	_table		= intern("table");	defop(1150, FX,  _table);
	_export		= intern("export");	defop(1150, FX,  _export);
	_import		= intern("import");	defop(1150, FX,  _import);
	_bar		= intern("|");		defop(1100, XFY, _bar);
	_semi_colon	= intern(";");		defop(1100, XFY, _semi_colon);
	_arrow		= intern("->");		defop(1050, XFY, _arrow);
	_comma		= intern(",");		defop(1000, XFY, _comma);
	_equal		= intern("=");		defop(700,  XFY, _equal);

	defop(1200, XFX,  intern("where"));
	defop(1150, XFX,  intern("asserting"));

	new_pred(atom_length,	"atom_length");
	new_pred(atom_concat,	"atom_concat");
	new_pred(sub_atom,	"sub_atom");
	new_pred(char_code,	"char_code");
	new_pred(p_concat,	"concat");
	new_pred(atom_chars,	"atom_chars");
	new_pred(number_chars,	"number_chars");
	new_pred(dump_hash,	"dump_hash");
}
