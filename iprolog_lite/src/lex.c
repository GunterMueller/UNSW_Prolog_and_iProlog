#include <math.h>
#include <unistd.h>
#include "prolog.h"

#define NEXTCH		getc(input)

#define ACCEPT(x)	{if (token_length <= TOKEN_LENGTH)\
				token[token_length++] = x;\
			else fprintf(stderr, "token length exceeded on line: %d\n", linen);}

#define REPLACE(x)	ungetc(x, input)

#define RETURN(x)	token[token_length] = '\0'; return(x)

extern chartype chtype[];

void syn_err(char *);

int linen = 0;


/************************************************************************/
/*		prompt is a procedure so that it can be exported	*/
/************************************************************************/

void
prompt(void)
{
	if (isatty(fileno(input)))
	{
		fflush(output);
		if (_prompt)
			fprintf(stderr, "%s", NAME(_prompt));
		fflush(stderr);
	}
	else
		linen++;
}


/************************************************************************/
/*	skip line after syntax error from interactive input		*/
/************************************************************************/

void
skip_line(void)
{
	while (NEXTCH != '\n');
	prompt();
}


/************************************************************************/
/*		Finite state machine for recognising tokens		*/
/************************************************************************/


static lextype
lex(char *token)
{
	int c, token_length = 0;

START:
	if ((c = NEXTCH) == EOF)
		return(END);
	switch (chtype[c])
	{
	   case WORDCH:		goto IDENT;
	   case STRINGCH:	goto STRING;
	   case SYMBOLCH: 	goto SYMBOL;
	   case PUNCTCH:	goto PUNCT;
	   case QUOTECH:	goto QATOM;
	   case DIGIT:		goto READ_INT;
	   case WHITESP:	if (c == '\n') prompt();
				goto START;
	   default:		goto ERROR;
	}

IDENT:
	ACCEPT(c);
	if ((c = NEXTCH) != EOF)
		switch (chtype[c])
		{
		   case WORDCH:		goto IDENT;
		   case DIGIT:		goto IDENT;
		}
	REPLACE(c);
	RETURN(WORD_T);

SYMBOL:
	switch (c)
	{
	   case '%':		goto LINE_COMMENT;
	   case '/':		if ((c = NEXTCH) == '*')
					goto COMMENT;
				else
					ACCEPT('/');
	   default:		goto REST_OF_SYMBOL;
	}

REST_OF_SYMBOL:
	switch (chtype[c])
	{
	   case SYMBOLCH:	ACCEPT(c);
				if ((c = NEXTCH) != EOF)
					goto REST_OF_SYMBOL;
	   default:		REPLACE(c);
				RETURN(SYMBOL_T);
	}

LINE_COMMENT:
	switch (NEXTCH)
	{
	   case '\n':		prompt();
	   case EOF:		goto START;
	   default:		goto LINE_COMMENT;
	}

COMMENT:
	switch (NEXTCH)
	{
	   case EOF:		goto START;
	   case '*':		if (NEXTCH == '/') goto START;
	   default:		goto COMMENT;
	}

PUNCT:
	ACCEPT(c);
	RETURN(PUNCT_T);

QATOM:
	if ((c = NEXTCH) == EOF) goto ERROR;
	switch (chtype[c])
	{
	   case QUOTECH:	if ((c = NEXTCH) != '\'')
				{
					REPLACE(c);
	   				RETURN(QUOTED_T);
				}
	   default:		ACCEPT(c);
				goto QATOM;
	}

STRING:
	if ((c = NEXTCH) == EOF) goto ERROR;
	switch (chtype[c])
	{
	   case STRINGCH:	RETURN(STRING_T);
	   case SYMBOLCH:	if (c == '\\')
					goto STR_ESCAPE;
	   default:		ACCEPT(c);
				goto STRING;
	}

STR_ESCAPE:
	switch(c = NEXTCH)
	{
	   case 'b':	ACCEPT((char) 07);		/* bell */
			break;
	   case 'n':	ACCEPT('\n');
			break;
	   case 't':	ACCEPT('\t');
			break;
	   default:	ACCEPT(c);
			break;
	}
	goto STRING;

READ_INT:
	ACCEPT(c);
	if ((c = NEXTCH) != EOF)
		switch (chtype[c])
		{
		   case DIGIT:		goto READ_INT;
		   case SYMBOLCH:	if (c == '.')
						goto FRACTION;
		   case WORDCH:		if (c == 'e' || c == 'E')
						goto E;	
		}
	REPLACE(c);
	RETURN(INT_T);

FRACTION:
	if ((c = NEXTCH) != EOF && chtype[c] == DIGIT)
	{
	 	ACCEPT('.');
		goto READ_REAL;
	}
	REPLACE(c);
	REPLACE('.');
	RETURN(INT_T);

READ_REAL:
	ACCEPT(c);
	if ((c = NEXTCH) != EOF)
		switch (chtype[c])
		{
		   case DIGIT:	goto READ_REAL;
		   case WORDCH:	if (c == 'e' || c == 'E')
					goto E;	
		}
	REPLACE(c);
	RETURN(REAL_T);

E:
	if ((c = NEXTCH) == EOF) goto ERROR;
	switch (chtype[c])
	{
	   case DIGIT:		ACCEPT('e');
				goto EXPRST;
	   case SYMBOLCH:	if (c == '+' || c == '-')
				{
					ACCEPT('e');
					goto EXP;
				}
	   default:		REPLACE(c);
				REPLACE('e');
				RETURN(REAL_T);
	}


EXP:
	ACCEPT(c);
	if ((c = NEXTCH) == EOF) goto ERROR;
	switch (chtype[c])
	{
	   case DIGIT:		goto EXPRST;
	   default:		goto ERROR;
	}

EXPRST:
	ACCEPT(c);
	if ((c = NEXTCH) == EOF) goto ERROR;
	switch (chtype[c])
	{
	   case DIGIT:		goto EXPRST;
	   default:		REPLACE(c);
				RETURN(REAL_T);
	}

ERROR:
	token[token_length] = '\0';
	fprintf(stderr, "Illegal symbol in input stream");
	return(ILLEGAL);
}


/************************************************************************/
/*	Routines to scan input for an token.				*/
/*	Includes the ability to push a token back onto the input stream	*/
/************************************************************************/

#define LOOKAHEAD	16

int lex_type;

static int pbottom = -1;
static int pushed_back = -1;
static term ptoken[LOOKAHEAD];


static int
isvar(char *s)
{
	char c = s[0];

	return(('A' <= c && c <= 'Z') || c == '_');
}


term
get_atom(void)
{
	term rval, lookup_var(term);
	char token[TOKEN_LENGTH];

	if (pushed_back > pbottom)
		return(ptoken[pushed_back--]);

	switch (lex_type = lex(token))
	{
	   case QUOTED_T:
	   case SYMBOL_T:
	   case PUNCT_T:
	   case STRING_T:
			rval = intern(token);
			break;
	   case WORD_T:
			rval = intern(token);
			if (isvar(token))
				rval = lookup_var(rval);
			break;
	   case INT_T:
			rval= new_int(atol(token));
			break;
	   case REAL_T:
			rval = new_real(atof(token));
			break;
	   case END:
			rval = _end_of_file;
			break;
	   default:
			syn_err("Illegal Symbol");
			break;
	}
	return(rval);
}


void
ungetatom(term p)
{
	if (pushed_back == LOOKAHEAD - 1)
		syn_err("Too many symbols pushed back");
	ptoken[++pushed_back] = p;
}


term
get_ungot_atom(void)
{
	if (pushed_back > pbottom)
		return ptoken[pushed_back--];
	else
		return NULL;
}


void
clear_input(void)
{
	pushed_back = pbottom;
}


term
read_atom(void)
{
	char token[TOKEN_LENGTH];

	if (pushed_back > pbottom)
		return ptoken[pushed_back--];

	switch (lex_type = lex(token))
	{
	   case QUOTED_T:
	   case SYMBOL_T:
	   case PUNCT_T:
	   case STRING_T:
	   case WORD_T:
			return intern(token);
	   case INT_T:
			return new_int(atol(token));
	   case REAL_T:
			return new_real(atof(token));
	   case END:
			return _end_of_file;
	   default:
			syn_err("Illegal Symbol");
			break;
	}
}


static int
ratom(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term rval = read_atom();

	return(unify(x, frame, rval, frame));
}

/*
static int
backtrackable_ratom(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ANY, OUT);
	term rval = get_atom();

	if (unify(x, frame, rval, frame))
		if (rest_of_clause())
			return(TRUE);
	ungetatom(rval);
	return(FALSE);
}
*/
static int
backtrackable_ratom(term goal, term *frame)
{
	term x, rval;

	if (TYPE(goal) != LIST)
		fail("tokeniser must have a single argument");

	x = check_arg(0, goal, frame, ANY, OUT);
	rval = get_atom();

	if (unify(x, frame, rval, frame))
		if (rest_of_clause())
			return(FALSE);

	ungetatom(rval);
	return(FALSE);
}


static int
p_getc(term goal, term *frame)
{
	term x;
	char rval[2] = {'\0', '\0'};

	if (ARITY(goal) == 2)
	{
		term old_input = current_input;

		x = check_arg(2, goal, frame, ATOM, OUT);
		current_input = check_arg(1, goal, frame, STREAM, IN);
		input = FPTR(current_input);

		rval[0] = fgetc(input);

		current_input = old_input;
		input = FPTR(current_input);
	}
	else
	{
		x = check_arg(1, goal, frame, ATOM, OUT);
		rval[0] = fgetc(input);
	}

	if (rval[0] == EOF)
		return(FALSE);
	else
		return(unify(x, frame, intern(rval), frame));
}


static void
skip(char c)
{
	char x;

	do {
		x = fgetc(input);
	} while (x != EOF && x != c);
}


static int
p_skip(term goal, term *frame)
{
	term x;

	if (ARITY(goal) == 2)
	{
		term old_input = current_input;

		x = check_arg(2, goal, frame, ATOM, IN);
		current_input = check_arg(1, goal, frame, STREAM, IN);
		input = FPTR(current_input);

		skip(NAME(x)[0]);

		current_input = old_input;
		input = FPTR(current_input);
	}
	else
	{
		x = check_arg(1, goal, frame, ATOM, IN);
		skip(NAME(x)[0]);
	}

	return(TRUE);
}


static int
p_eof(term goal, term *frame)
{
	int rval;

	if (ARITY(goal) == 1)
	{
		term old_input = current_input;

		current_input = check_arg(1, goal, frame, STREAM, IN);
		input = FPTR(current_input);

		rval = feof(input);

		current_input = old_input;
		input = FPTR(current_input);
	}
	else
		rval = feof(input);

	return(rval);
}



void
lex_init()
{
	extern term _list_proc;

	new_pred(ratom, "ratom");
/*	defop(100, FX, new_pred(backtrackable_ratom, "#"));
*/
	_list_proc = new_pred(backtrackable_ratom, "$.$");
	new_pred(p_getc, "getc");
	new_pred(p_skip, "skip");
	new_pred(p_eof, "eof");
}
	
