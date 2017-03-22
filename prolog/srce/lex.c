/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/



#include "g.h"
#include "in.h"

#ifdef BSD
#include "ungetc.i"
#endif

#define ID_SIZE 511


int linen;
char token_buff[ID_SIZE+1];
int token_size;

chartype chtype[128] =
{WHITESP,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	PUNCTCH,
/* nul		soh		stx		etx		eot    */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* enq		ack		bel		bs		ht     */
 WHITESP,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* nl		vt		np		cr		so     */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* si		dle		dc1		dc2		dc3    */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* dc4		nak		syn		etb		can    */
 ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,	ILLEGALCH,
/* em		sub		esc		fs		gs     */
 ILLEGALCH,	ILLEGALCH,	WHITESP,	PUNCTCH,	STRINGCH,
/* rs 		us		sp		!		"      */
 SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	QUOTECH,
/* #		$		%		&		'       */
PUNCTCH,        PUNCTCH,	SYMBOLCH,	SYMBOLCH,	PUNCTCH,
/* (		)		*		+		,       */
 SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	DIGIT,		DIGIT,
/* -		.		/		0		1       */
 DIGIT,		DIGIT,		DIGIT,		DIGIT,		DIGIT,
/* 2		3		4		5		6       */
 DIGIT,		DIGIT,          DIGIT,		SYMBOLCH,	PUNCTCH,
/* 7		8		9		:		;       */
 SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	SYMBOLCH,	SYMBOLCH,
/* <		=		>		?		@      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* A		B		C		D		E      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* F		G		H		I		J      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* K		L		M		N		O      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* P		Q		R 		S		T      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* U		V		W		X		Y      */
 WORDCH,	PUNCTCH,	SYMBOLCH,	PUNCTCH,	SYMBOLCH,
/* Z		[		\	\	]		^      */
 WORDCH,	SYMBOLCH,	WORDCH,		WORDCH,		WORDCH,
/* _		`		a		b		c      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* d		e		f		g		h      */
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* i		j		k		l		m	*/
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* n		o		p		q		r	*/
 WORDCH,	WORDCH,		WORDCH,		WORDCH,		WORDCH,
/* s		t		u		v		w	*/
 WORDCH,	WORDCH,		WORDCH,		PUNCTCH,	SYMBOLCH,
/* x		y		z		{		|	*/
 PUNCTCH,	SYMBOLCH,	PUNCTCH};
/* }		~		del					*/


/*		find out if an atom should be quoted			*/

atype(s)
register char *s;
{
	register chartype c;

	if (*s < 'a' || *s > 'z') return(QATOM);
	while (*++s)
	{
		c = chtype[*s];
		if (c != WORDCH && c != DIGIT)
			return(QATOM);
	}
	return(NONOP);
}


/*			Get nonop from atom list			*/



pval nonop(type, a)
optype type;
register atom *a;
{
	register atom *p;

	for (p = a; p && strcmp(NAME(a), NAME(p)) == 0; p = p -> link)
		if (p -> op_t == type)
			return((pval) p);
		else a = p;
	p = (atom *) new(ATOM);
	p -> link = a -> link;
	a -> link = p;
	p -> name = a -> name;
	p -> op_t = type;
	p -> pred = 0;
	return((pval) p);
}



/*  			low level input routines			*/



/*	peekc looks at the next character and then replaces it.		*/


#define peekc ungetc(getc(input), input)


/*	prompt is a procedure so that it can be exported		*/

prompt()
{
	extern atom *prompt_string;

	fprintf(stderr, "%s", NAME(prompt_string));
}


/* skipd skips blanks, tabs, newlines and comments which begin with '%' */


skipd()
{
	register char c;

	repeat
		switch(c = readch)
		{
		   case '%'	: while (readch != '\n');
		   case '\n'    : if (isatty(fileno(input))) prompt();
				  else linen++;
		   case ' '     :  
		   case '\t'    : break;
		   default	: ungetc(c, input);
				  return;
		}
}


/*	number read an integer and returns it	  */

static
int number()
{
	register int sum = 0;
	register char c;

	while (chtype[c = readch] == DIGIT)
		sum = sum * 10 + c - '0';
	ungetc(c, input);
	return(sum);
}



/*	inword reads an atom and puts it in the token buffer     */


static
inword()
{
	register char *p = token_buff;
	register chartype nexttype;
	register char c;

	token_size = 0;
	repeat
	{
		nexttype = chtype[c = readch];
		if (nexttype == WORDCH || nexttype == DIGIT)
		{
			if (token_size < ID_SIZE)
			{
				token_size++;
				*p++ = c;
			}
		}
		else break;
	}
	ungetc(c, input);
	*p = 0; token_size++;
}


/*		read strings into token buffer			*/

static
instring(ctype)
chartype ctype;
{
	register char *p = token_buff;
	register char c;

	token_size = 0;
	readch;
	while (chtype[c = readch] != ctype)
	{
		if (c == '\\')
			switch (c = readch)
			{
			   case 'b':	c = 07;		/* bell */
					break;
			   case 'n':	c = '\n';
					break;
			   case 't':	c = '\t';
					break;
			   default:	if (chtype[c] == DIGIT)
					{
						ungetc(c, input);
						c = number();
						if (c > 127)
							error("invalid character in string", TRUE);
					}
			}
		if (token_size < ID_SIZE)
		{
			token_size++;
			*p++ = c;
		}
	}
	*p = 0; token_size++;
}



/*		read symbolic atoms into token buffer			*/


static
insymbol()
{
	register char *p = token_buff;
	register char c;

	token_size = 0;
	while (chtype[c = readch] == SYMBOLCH)
		if (token_size < ID_SIZE)
		{
			token_size++;
			*p++ = c;
		}
	ungetc(c, input);
	*p = 0; token_size++;
}


static pval ptoken[LOOKAHEAD];

int pushed_back = -1;

int readatom = FALSE;

pval getatom()
{
	int quotes = FALSE;
	chartype ctype;
	pval rval;

	if (pushed_back >= 0) return(ptoken[pushed_back--]);

	skipd();
	if (feof(input)) {readch; return((pval) _eof);}

	switch(ctype = chtype[peekc])
	{
	   case DIGIT:	{
			char *c;
			char in_type;
			char number[20];

			in_type = INT; c = number;
			while (chtype[*c = readch] == DIGIT) ++c;
			if (*c == '.' && chtype[peekc] == DIGIT)
			{
				in_type = REAL; ++c;
				while (chtype[*c = readch] == DIGIT) ++c;
			}
			if (*c == 'e' || *c == 'E')
			{
				++c;
				if (chtype[*c = readch] != DIGIT) ++c;
				while (chtype[*c = readch] == DIGIT) ++c;
			}
			ungetc(*c, input);
			*c = 0;
			if (in_type == REAL)
			{
				rval = new(REAL);
				sscanf(number,"%lf",&rval->r.real_val);
			}
			else {
				rval = new(INT);
				sscanf(number,"%d",&rval->i.int_val);
			}
			return(rval);
		}
	   case WORDCH:		inword(); 
				break;
	   case QUOTECH:
	   case STRINGCH:	instring(ctype);
				quotes = TRUE;
				break;
	   case SYMBOLCH:	insymbol();
				break;
	   case PUNCTCH:	token_buff[0] = readch;
				token_buff[1] = '\0';
				break;
	   default:		error("Illegal symbol", TRUE);
	} 
	rval = intern(token_buff, token_size-1);
	if (readatom || quotes)
		rval = nonop(atype(token_buff), rval);
	return(rval);
}


ungetatom(a)
pval a;
{
	if (pushed_back < LOOKAHEAD - 1)
		ptoken[++pushed_back] = a;
	else error("Too many symbols pushed back", TRUE);
}
