/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





/*			Global definitions for input routines		*/

#define LOOKAHEAD 32

#define readch getc(input)


/*	funny chars for making quoted atoms	*/

#define isfunny(ch) (chtype[ch] != WORDCH)

typedef enum
{
	WORDCH, STRINGCH, SYMBOLCH, PUNCTCH, QUOTECH, DIGIT, WHITESP, 
	ILLEGALCH
} chartype;


typedef enum
{
	WORD_T, STRING_T, SYMBOL_T, PUNCT_T, FUNCT_T, INT_T, ILLEGAL, END
} lextype;

#define	INPUT	1
#define	OUTPUT	2
