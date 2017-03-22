/******************************************************************************

			UNSW Prolog (version 4.2)

			Written by Claude Sammut
		     Department of Computer Science
		     University of New South Wales

		   Copyright (c)  1983 - Claude Sammut

******************************************************************************/

#include <stdio.h>

/* 	global declarations		*/


/*---------------------- user definable globals -------------------- */


#define BSD		/* define if BSD Unix */
			/* remove otherwise */

#define	PROFILING	/* define if you wish to have profiling ability */
			/* the cost is one extra word per clause */

#define LIB		"/usr/local/lib/prolog/prolog.lib"
#define DFLT_ED		"/usr/ucb/vi"
#define	DFLT_SHELL	"/bin/csh"

/*	PRINC_VAR is defined if princial terms are to be unified	*/

#define PRINC_VAR

#define	MEM_AVAIL	2	/* 1 if main memory is limited ( < 400 K )
				 * 2 otherwise */

#define	HZ		60	/* line frequency */

/*------------------- end of user definable globals ---------------- */


#define FALSE		0
#define TRUE		1
#define WORD_LENGTH 	sizeof(char *)
#define NPRED		-1
#define HASHSIZE	128
#define MAXVAR		100
#define repeat		for(;;)


#define VAR	0	/* keep this 0 so TYPE tests on var will be fastest */
#define FN	1
#define ATOM	2
#define PREDEF	3
#define INT	4
#define CLAUSE	5
#define LIST	6
#define FREE	7
#define	STREAM	8
#define REAL	9

#define XFX	0
#define XFY	1
#define YFX	2
#define FX	3
#define FY	4
#define XF	5
#define YF	6
#define	NONOP	7
#define QATOM	8

typedef char card;
typedef char itemtype;
typedef char optype;


/*	record structures in free space		*/

typedef union pobj *pval;

typedef struct atom
{
	itemtype type;
	optype op_t;
	short pred;
	card tflags;	/* trace flags		*/
	char lib;
	char *name;
	struct clause *val;
	struct atom *link;
} atom;

/* atom tflag values */

#define TRACED		1
#define PROFILED	2

typedef struct predef
{
	itemtype type;
	optype op_t;
	short pred;
	card tflags;
	card nargs;
	char *name;
	int (*fn)();
	struct atom *link;
} predef;

typedef struct integer {itemtype type; int int_val;} integer;

typedef struct real {itemtype type; double real_val;} real;

typedef struct compterm
{
	itemtype type;
	card size;
	pval term[1];
} compterm;		/* KLUDGE */


typedef struct clause
{
	itemtype type;
	card nvars;
	struct clause *rest;
#ifdef PROFILING
	char *prof_data;
#endif
	pval goal[1];
} clause;

typedef struct var
{
	itemtype type;
	card offset;
	struct atom *pname;
} var;

typedef struct stream
{
	itemtype type;
	char *sname;
	FILE *file;
	char mode;
	short ref_cnt;
} stream;

typedef union pobj 
{
	struct atom	a;
	struct compterm	c;
	struct predef	p;
	struct integer	i;
	struct var	v;
	struct clause	g;
	struct stream	s;
	struct real	r;
} pobj;

/*	stack records used by interpreter	*/


typedef struct binding
{
	pval termv;
	struct binding *framev;
} binding;

typedef struct environment
{
	pval *cl;
	short tracing;
	binding *sp;
	clause *clist;
	struct environment *parent;
	binding **tp;
} environment;


/* stream modes */

#define	CLOSED	0
#define	R_MODE	1
#define	W_MODE	2
#define	A_MODE	3

/*		type checking macros		*/


#define isinteger(x) (((integer *) x) -> type == INT)

#define isreal(x) (((real *) x) -> type == REAL)

#define isvariable(x) (((var *) x) -> type == VAR)

#define iscompound(x) (((compterm *) x) -> type == FN)

#define islist(x) (((compterm *) x) -> type == LIST)

#define isstream(x) (((stream *) x) -> type == STREAM)

#define isatom(x)\
(\
	   (((atom *) x) -> type == ATOM)\
	|| (((atom *) x) -> type == PREDEF)\
)

#define TYPE(x)	(((compterm *) x) -> type)
#define SIZE(x) (((compterm *) x) -> size)
#define NAME(x) (((atom *) x) -> name)
#define VAL(x) (((atom *) x) -> val)
#define FVAL(x) (* (x -> p.fn))
#define OFFSET(x) (((var *) x) -> offset)

#define CALL	0
#define EXIT	1
#define FAILG	2
#define REDO	3


#define	WORDS(n)	(((n) + WORD_LENGTH - 1) / WORD_LENGTH)

#define	halloc(n)	_halloc(WORDS(n))

#define hfree(t, n)	_hfree((char *)t, WORDS(n))

#define unify(t1, f1, t2, f2)\
		_unify((pval)t1, (binding *)f1, (pval)t2, (binding *)f2)

#define bind(v, f1, term, f2)\
		_bind((var *)v, (binding *)f1, (pval)term, (binding *)f2)



extern char	*_halloc();
extern pval	intern(), new(), mkclause(), nonop();
extern compterm	*record();
extern clause	*create();
extern var	*variable();

extern atom
	*_comma, *_dot, *_lpren, *_cut, *nil, *_prefix, *_infix, *_postfix,
	*_rpren, *_lbrac, *_rbrac, *_query, *_uplus, *_uminus, *_eof, *_neck,
	*_dot_dot, *_question, *_command, *_bang, *_bar, *_lbrace, *_rbrace,
	*_true, *no, *_read, *_write, *anon;

extern pval	stack_int;

extern pval	termb;
extern binding	*frameb;

extern var **varcell;
extern FILE *output, *poport, *input, *piport, *prog_file, *trace_output;

extern binding *sp, *stack;
extern binding **trail, **tp;
extern environment *env_stack, *parent, *envp;


#ifdef PROFILING
extern char profiling;
#endif
