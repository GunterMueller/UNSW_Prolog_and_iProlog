/************************************************************************/
/*			    Script input routines			*/
/************************************************************************/

#include <ctype.h>
#include "prolog.h"

#define CONTEXT(x)	ARG(0, x)
#define TIME_STAMP(x)	ARG(1, x)
#define PATTERN(x)	ARG(2, x)
#define RESPONSE(x)	ARG(3, x)

static term _end, _rule, _semi, _double_colon, _double_arrow, _lt, _gt;
static term _question_mark, _exclamation, _full_stop, _star;
static term _non_terminal, _nextof, _anyof, _var, _once;
static term _left_expr, _right_expr;


/************************************************************************/
/*			get the next non-space character		*/
/************************************************************************/

static int
nextch(void)
{
	int c;

	while (isspace(c = getc(input)))
		if (c == '\n') prompt();

	return c;
}


/************************************************************************/
/*				Skip white space			*/
/************************************************************************/

static int
skip_space(void)
{
	int c = nextch();

	ungetc(c, input);
	return c;
}


/************************************************************************/
/*				Skip line comment			*/
/************************************************************************/

static int
check_comment(void)
{
	int c;

	if ((c = getc(input)) != '%')
	{
		ungetc(c, input);
		return 0;
	}
	
	repeat
	{
		switch (c = getc(input))
		{
		   case EOF:
		   		ungetc(c, input);
		   case '\n':
		   		break;
		}
	}
	return 1;
}


/************************************************************************/
/*				Read a word				*/
/************************************************************************/

term get_ungot_atom(void);

static term
word(void)
{
	char c, buf[TOKEN_LENGTH+1];
	term rval = NULL;
	int i = 0, punct = 0, num = 0;

	if ((rval = get_ungot_atom()) != NULL)
		return rval;

	skip_space();

	while (! isspace(c = getc(input)))
		switch (c)
		{
		case EOF:
				if (i == 0)
					return _end_of_file;
				else
					goto L;
		case '[':
		case ']':
		case '{':
		case '}':
		case '|':
		case '#':
		case '\\':
				ungetc(c, input);
				goto L;	
		case '(':
		case ')':
		case '*':
		case '?':
		case '!':
		case ';':
		case ',':
		case '.':
				if (i == 0 && ! punct)
					buf[i++] = c;
				else
					ungetc(c, input);
				goto L;
		case '<':
		case '>':
		case '=':
				if (i == 0 || punct)
				{
					buf[i++] = c;
					punct = 1;
				}
				else
				{
					ungetc(c, input);
					goto L;
				}
				break;
		default:
				if (isdigit(c))
					num = (num || i == 0) ? 1 : 0;
				else if (punct)
				{
					ungetc(c, input);
					goto L;
				}
				if (i < TOKEN_LENGTH)
					buf[i++] = c;
				else
					fail("Word is too long");
				break;
		}

L:	buf[i] = '\0';
	if (i == 0)
		return NULL;
	else if (num)
		return new_int(atol(buf));
	else
		return intern(buf);
}


/************************************************************************/
/*		   Read user's input for probot processing		*/
/************************************************************************/

term
read_sentence(void)
{
	FILE *old_input = input;
	FILE *old_output = output;
	term L = _nil, *last = &L;

	input = dialog_in;
	output = dialog_out;

	repeat
	{
		term expr = word();

		if (expr == NULL)
		{
			nextch();
			fprintf(stderr, "You typed some strange characters. Please try again\n");
			continue;
		}
		
		if (expr == _question_mark)		break;
		if (expr == _exclamation)	break;
		if (expr == _full_stop)		break;
		if (expr == _end_of_file)	break;

		*last = gcons(expr, _nil);
		last = &CDR(*last);
	}

	input = old_input;
	output = old_output;

	return L;
}


static int
p_read_sentence(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, OUT);

	return unify(x, frame, read_sentence(), frame);
}


/************************************************************************/
/*			       script parser				*/
/************************************************************************/

static term word_expr(int *);
static term storage_context;

static term
alternatives(char end)
{
	term L = _nil, *last = &L;
	int cond;

	do
	{
		term expr = word_expr(&cond);

		if (cond)
			expr = g_fn2(_double_arrow, expr, word_expr(&cond));

		*last = gcons(expr, _nil);
		last = &CDR(*last);
	}
	while (nextch() != end);

	return L;
}


static term
word_expr(int *cond)
{
	term L = _nil, *last = &L;
	*cond = 0;

	repeat
	{
		term expr;
		char c = nextch();

		switch (c)
		{
		case EOF:
				fail("Unexpected end of file");
		case ']':
		case '}':
		case '|':
				ungetc(c, input);
				return L;
		case '_':
				expr = _nil;
				break;
		case '[':
				expr = g_fn1(_nextof, alternatives(']'));
				break;
		case '{':
				expr = g_fn1(_anyof, alternatives('}'));
				break;
		case '<':
				expr = g_fn1(_non_terminal, word());
				if (word() != _gt)
					fail("Non-terminal must end with '>'");
				break;
		case '^':
				expr = g_fn1(_var, read_term());
				break;
		case '#':
				expr = read_term();
				if (TYPE(expr) == ATOM)
					expr = g_fn1(_once, expr);
				break;
		case '-':
				if ((c = getc(input)) == '>')
				{
					*cond = 1;
					return L;
				}
				else
					c = '-';
		default:
				ungetc(c, input);
				expr = word();
				break;
		}

		*last = gcons(expr, _nil);
		last = &CDR(*last);
	}
}


static term
response(void)
{
	char c;
	term x;

	if ((x = get_ungot_atom()) != NULL)
		if (x == _lt)
			c = '<';
		else if (x == _anon)
			c = '_';
		else
			return x;
	else
		c = nextch();
		
	switch (c)
	{
		case '[':
				return g_fn1(_nextof, alternatives(']'));
		case '{':
				return g_fn1(_anyof, alternatives('}'));
		case '<':
			{
				term rval = g_fn1(_non_terminal, word());
				if (word() != _gt)
					fail("Non-terminal must end with '>'");
				return rval;
			}
		case '^':
				return g_fn1(_var, read_term());
		case '#':
				x = read_term();
				if (TYPE(x) == ATOM)
					x = g_fn1(_once, x);
				return x;
		case '_':
				return _nil;
		case ';':
				return _semi;
		default:
				ungetc(c, input);
				return word();
	}
}


static term
expr_macro(void)
{
	term next, L = _nil, *last = &L;

	while ((next = response()) != _right_expr)
	{
		*last = gcons(next, _nil);
		last = &CDR(*last);
	}

	return L;
}


static term
pattern(void)
{
	term L = _nil, *last = &L;

	repeat
	{
		term next = response();

		if (next == _semi)
		{
			ungetc(';', input);
			break;
		}
		if (next == _double_arrow || next == _end_of_file)
			break;

		*last = gcons(next, _nil);
		last = &CDR(*last);
	}

	return L;
}


static void
rule(void)
{
	term pat, res, rval;

	pat = pattern();
	if ((res = response()) == _semi)
		res = _nil;

	rval = new_g_fn(3);
	CONTEXT(rval) = storage_context;
	TIME_STAMP(rval) = new_int(0);
	PATTERN(rval) = pat;
	RESPONSE(rval) = res;
	rval = add_clause(mkclause(rval, NULL), FALSE);
}


static void
rule_set(void)
{
	extern term varlist;
	term name, sep;

	varlist = _nil;
	storage_context = _rule;

	while (skip_space() != EOF)
	{
		if (check_comment())
			continue;
		name = word();
		if (name == _end)
			break;
		if (name == _semi)
		{
			storage_context = _rule;
			continue;
		}
		if ((sep = word()) == _double_colon)
			storage_context = name;
		else
		{
			if (sep != NULL)
				ungetatom(sep);
			ungetatom(name);
		}
		rule();
	}
}


static term
script_macro(term x)
{
	rule_set();
	return x;
}


int
read_script(term file_name)
{
	extern int linen, syntax_error;
	term old_input = current_input;
	term p = get_stream(file_name, intern("r"));

	if (p == _nil)
		return 1;

	input = FPTR(p);
	current_input = p;
	syntax_error = FALSE;
	linen = 1;

	rule_set();

	close_stream(current_input);
	current_input = old_input;
	input = FPTR(current_input);

	if (syntax_error)
		return 2;

	return 0;
}


static int
script(term goal, term *frame)
{
	term file_name = check_arg(1, goal, frame, ATOM, IN);

	switch (read_script(file_name))
	{
		case 0:	return TRUE;
		case 1: fail("Couldn't open file");
		case 2: return FALSE;
	}
}


/************************************************************************/
/*			   Module Initialisation			*/
/************************************************************************/

void
script_init(void)
{
	term _dialog;

	_end		= intern("end");
	_rule		= intern("$probot");
	_double_colon	= intern("::");
	_semi		= intern(";");
	_double_arrow	= intern("==>");
	_non_terminal	= intern("\\");
	_lt		= intern("<");
	_gt		= intern(">");

	_nextof		= intern("nextof");
	_anyof		= intern("anyof");
	_var		= intern("var");
	_once		= intern("once");

	_question_mark	= intern("?");
	_exclamation	= intern("!");
	_full_stop	= intern(".");
	_star		= intern("*");

	defop(200, FX, _non_terminal);

	_dialog = intern("dialog");
	MACRO(_dialog) = script_macro;

	new_pred(script, "script");
	new_pred(p_read_sentence, "read_sentence");

	_left_expr = intern("<<");
	_right_expr = intern(">>");
	MACRO(_left_expr) = expr_macro;
}
