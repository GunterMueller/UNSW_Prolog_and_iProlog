/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/





		/* SETUP FUNCTIONS AND ATOMS IN SYMBOL TABLE */


#include "pred.h"


atom_table pred =
{
	SET_ATOM(NONOP, 0, "nil"),
	SET_ATOM(NONOP, 0, "n"),
 	SET_ATOM(XFY, 1000, ","), 
	SET_ATOM(XF, 1200, "."),
	SET_ATOM(NONOP, 0, ".."),
	SET_ATOM(FX, 1200, "("),
	SET_ATOM(XF, 1201, ")"),
	SET_ATOM(FX, 1200, "["),
	SET_ATOM(XF, 1201, "]"),
	SET_ATOM(FX, 1200, "?-"),
	SET_ATOM(XF, 1200, "?"),
	SET_ATOM(NONOP, 0, "!"),
	SET_ATOM(XF, 1200, "!"),
	SET_ATOM(NONOP, 0, "_"),
	SET_ATOM(NONOP, 0, "end_of_file"),
	SET_ATOM(NONOP, 0, "user_input"),
	SET_ATOM(NONOP, 0, "user_output"),
	SET_ATOM(XFX, 1200, ":-"),
	SET_ATOM(FX, 1200, ":-"),
	SET_ATOM(NONOP, 0, "true"),
	SET_ATOM(NONOP, 0, "repeat"),
	SET_ATOM(NONOP, 0, "file"),
	SET_ATOM(FX, 100, "prefix"),
	SET_ATOM(FX, 100, "infix"),
	SET_ATOM(FX, 100, "postfix"),
	SET_PRED(FX, 320, 1, "+", UPLUS),	/* atoms with same name */
	SET_PRED(YFX, 500, 2, "+", PLUS),	/* must appear together */
	SET_PRED(FX, 300, 1, "-", UMINUS),
	SET_PRED(YFX, 500, 2, "-", MINUS),
	SET_ATOM(XFY, 1100, "|"),
	SET_ATOM(NONOP, 0, ": "),
	SET_ATOM(NONOP, 0, "> "),
	SET_ATOM(FX, 1200, "{"),
	SET_ATOM(XF, 1201, "}"),
	SET_ATOM(NONOP, 0, "append"),
	SET_ATOM(NONOP, 0, "user_error"),
	SET_ATOM(NONOP, 0, "current_stream"),
	SET_ATOM(NONOP, 0, "fail"),
	SET_ATOM(XFY, 1100, ";"),
	SET_ATOM(XFY, 1050, "->"),
	SET_ATOM(XFY, 800, "&"),
	SET_ATOM(XFX, 700, "="), 
	SET_ATOM(XFX, 700, "/="),
	SET_PRED(YFX, 400, 2, "*", TIMES),
	SET_PRED(YFX, 400, 2, "/", DIV),
	SET_PRED(YFX, 400, 2, "mod", MOD),
	SET_PRED(XFY, 350, 2, "^", POW),
	SET_PRED(YFX, 500, 2, "/\\", LAND),
	SET_PRED(YFX, 500, 2, "\\/", LOR),
	SET_PRED(YFX, 500, 2, "xor", XOR),
	SET_PRED(YFX, 400, 2, "<<", LSHIFT),
	SET_PRED(YFX, 400, 2, ">>", RSHIFT),
	SET_PRED(FX, 350, 1, "\\", NEG),
	END_MARK
};



	/* atoms which C must refer to should be together at start of table */

atom	*nil		= &(pred[0]),
	*no		= &(pred[1]),
	*_comma		= &(pred[2]),
	*_dot		= &(pred[3]),
	*_dot_dot	= &(pred[4]),
	*_lpren		= &(pred[5]),
	*_rpren		= &(pred[6]),
	*_lbrac		= &(pred[7]),
	*_rbrac		= &(pred[8]),
	*_query		= &(pred[9]),
	*_question	= &(pred[10]),
	*_cut		= &(pred[11]),
	*_bang		= &(pred[12]),
	*anon		= &(pred[13]),
	*_eof		= &(pred[14]),
	*_user_input	= &(pred[15]),
	*_user_output	= &(pred[16]),
	*_neck		= &(pred[17]),
	*_command	= &(pred[18]),
	*_true		= &(pred[19]),
	*_repeat	= &(pred[20]),
	*_file		= &(pred[21]),
	*_prefix	= &(pred[22]),
	*_infix		= &(pred[23]),
	*_postfix	= &(pred[24]),
	*_uplus		= &(pred[25]),
	*_plus		= &(pred[26]),
	*_uminus	= &(pred[27]),
	*_minus		= &(pred[28]),
	*_bar		= &(pred[29]),
	*init_prompt	= &(pred[30]),
	*prompt_string	= &(pred[30]),	/* same as init_prompt */
	*read_prompt	= &(pred[31]),
	*_lbrace	= &(pred[32]),
	*_rbrace	= &(pred[33]),
	*_append	= &(pred[34]),
	*_user_error	= &(pred[35]),
	*_current_stream= &(pred[36]),
	*_fail		= &(pred[37]);

atom *_read, *_write;

pval	stack_int;

var _1 = {VAR, 0, &pred[6]};
var _2 = {VAR, 1, &pred[6]};

static clause *rep_cl;

extern atom *hashtable[];

extern atom_table
	p_IO,
	p_basic,
	p_behave,
	p_clause,
	p_streams,
	p_meta,
	p_unix;

struct table_index
{
	atom *tab_addr;
	atom *end_addr;
	char *tab_name;
}
tab_info[] =
{
	{p_IO,		0,	"p_IO"},
	{p_basic,	0,	"p_basic"},
	{p_behave,	0,	"p_behave"},
	{p_clause,	0,	"p_clause"},
	{p_streams,	0,	"p_streams"},
	{p_meta,	0,	"p_meta"},
	{p_unix,	0,	"p_unix"},
	{pred,		0,	"pred"}
};


static
add_atoms()
{
	register atom *p;
	register int i, h;

	i = 0;
	do {
		for (p = tab_info[i].tab_addr; p -> name != 0; p++)
		{
			h = hash(p -> name);
			p -> link = hashtable[h];
			hashtable[h] = p;
		}
		tab_info[i].end_addr = p;
	} while (tab_info[i++].tab_addr != (atom *) pred);
}


static
hash_init()
{
	register int i;

	for(i = 0; i < HASHSIZE ; i++)
		hashtable[i] = 0;
}



setup()
{
	hash_init();
	add_atoms();

	stack_int = new(INT);
	rep_cl = create(0, 0);
	_repeat -> val = rep_cl;
	_repeat -> lib = TRUE;
	rep_cl -> goal[0] = (pval)(_repeat);
	rep_cl -> rest = rep_cl;

	_read  = &p_IO[0];
	_write = &p_IO[1];
}



arith_op(expr)
pval expr;
{
	register pval a;

	if (iscompound(expr)) a = expr -> c.term[0];
	else if (isatom(expr)) a = expr;
	else return(FALSE);

	switch ((int)VAL(a))
	{
	   case PLUS	:
	   case MINUS	:
	   case TIMES	:
	   case DIV	:
	   case MOD	:
	   case POW	:
	   case UPLUS	:
	   case UMINUS	:
	   case LAND	:
	   case LOR	:
	   case XOR	:
	   case NEG	:
	   case LSHIFT	:
	   case RSHIFT	: return(TRUE);
	   default	: return(FALSE);
	}
}
