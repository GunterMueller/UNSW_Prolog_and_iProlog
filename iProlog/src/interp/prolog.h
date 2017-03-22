/************************************************************************/
/*			global definitions				*/
/************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#define DDD

#define TRUE		-1
#define FALSE		0

#define EXTENSIBLE	1
#define ALL_BITS	0xffffffff

#define BITSPERBYTE	8
#define WORD_LENGTH	sizeof (void *)
#define BITS_IN_WORD	(BITSPERBYTE * WORD_LENGTH)

#define repeat		for(;;)

#define NPRED		-1
#define MAXVAR		100
#define HASHSIZE	997				/* used to be 256 */
#define FREE_SIZE	20
#define TOKEN_LENGTH	1024

#define MARK		1				/* Flags for atom */
#define COPY		2
#define SPY		4
#define PREDEF		8
#define LOCK		16
#define OP		32
#define DYNAMIC		64
#define GENERIC		128

#define PRECEDENCE	07777				/* masks for prec fields */
#define NONASS		010000
#define RIGHT		020000

enum {IN, OUT, EVAL};

enum {FX, FY, XFX, XFY, YFX, XF, YF};

enum {WORDCH, STRINGCH, SYMBOLCH, PUNCTCH, QUOTECH, DIGIT, WHITESP, ILLEGALCH};

enum
{
	WORD_T, QUOTED_T, STRING_T, SYMBOL_T, PUNCT_T, FUNCT_T,
	INT_T, REAL_T, ILLEGAL, END
};


/* NOTE: The order of the types below is important for the compare predicate */

enum
{
	AVAIL, ANON, FREE, BOUND, REF, INT, REAL, ATOM, FN, LIST, CLAUSE, SET,
	PRED, FPRED, SUBR, FSUBR, STREAM, NUMBER, ANY, CHUNK
};

typedef unsigned char bits;
typedef unsigned short card;
typedef unsigned char optype;
typedef unsigned char itemtype;
typedef unsigned char lextype;
typedef unsigned char chartype;


/************************************************************************/
/*	    "term" is a pointer to all Prolog data structures		*/
/************************************************************************/

typedef union pobj *term;


/************************************************************************/
/*		Record structures for standard Prolog types		*/
/************************************************************************/

typedef struct atom
{
	itemtype type;
	bits flags;
	unsigned short prefix, infix, postfix;
	term (*macro)(), portray;
	term inherits, plist;		/* propery list extension */
	term proc;
	term link;
	char name[EXTENSIBLE];		/* kludge for allocated variable length structures */
} atom;

typedef struct integer
{
	itemtype type;
	bits flags;
	long int_val;
} integer;

typedef struct real
{
	itemtype type;
	bits flags;
	double real_val;
} real;

typedef struct compterm
{
	itemtype type;
	bits flags;
	card arity;
	term arg[EXTENSIBLE];		/* kludge for allocated variable length structures */
} compterm;

typedef struct clause
{
	itemtype type;
	bits flags;
	card nvars;
	term next;
	term label;			/* label extension */
	term goal[EXTENSIBLE];		/* kludge for allocated variable length structures */
} clause;


/************************************************************************/
/*	Built-in predicates are references C functions			*/
/*	They return TRUE or FALSE					*/
/************************************************************************/

typedef struct pred
{
	itemtype type;
	bits flags;
	term id;
	int (*c_code)();
} pred;


/************************************************************************/
/*	Built-in functions are references C functions			*/
/*	They return a term						*/
/************************************************************************/

typedef struct subr
{
	itemtype type;
	bits flags;
	term id;
	term (*c_code)();
} subr;


/************************************************************************/
/*	A stream is a Unix FILE pointer.				*/
/*	The buffer is allocated within the stream structure		*/
/************************************************************************/

typedef struct stream
{
	itemtype type;
	bits flags;
	FILE *fptr;
	term mode;
	term fname;
	term next_stream;
	char iobuf[BUFSIZ];
} stream;


/************************************************************************/
/*	A variable is either a stack offset or a reference structure	*/
/************************************************************************/

typedef struct var
{
	itemtype type;
	bits flags;
	card offset;
	term pname;
} var;

typedef struct ref
{
	itemtype type;
	bits flags;
	term pointer;
	term trail;
} ref;


/************************************************************************/
/*	When a structure is deallocated, it turns into a free_cell	*/
/************************************************************************/

typedef struct free_cell
{
	itemtype type;
	bits flags;
	card size;
	term next_free;
} free_cell;


/************************************************************************/
/*		Non-standard set type to support ML libraries		*/
/************************************************************************/

typedef struct set
{
	itemtype type;
	bits flags;
	card set_size;
	term next;
	term contents;
	term subsumes;
	short class;		/* no. of class to which this example belongs	*/
	short nsel;		/* no. of selectors in use			*/
	short npos;		/* no. of positive examples covered		*/
	short nneg;		/* no. of negative examples covered		*/
	unsigned long sel[EXTENSIBLE];
} set;


/************************************************************************/
/*	Non-standard type to support arbitrary binary data		*/
/************************************************************************/

typedef struct
{
	char	*name;			/* the chunk name			*/
	void	(*free)(term);		/* free external data			*/
	void	(*print)(term);		/* print an object of this type		*/
} chunk_spec;


typedef struct chunk
{
	itemtype type;
	bits flags;
	chunk_spec *spec;	/* pointer to structure containing access routines */
	void *data;		/* pointer to malloc'ed data			   */
} chunk;


/************************************************************************/
/* Dummy redefinitions of EXTENSIBLE types helps examining internals	*/
/* using a debugger							*/
/************************************************************************/

#ifdef DDD

typedef struct dummy_atom			/**** DUMMY ****/
{
	itemtype type;
	bits flags;
	unsigned short prefix, infix, postfix;
	term proc;
	term (*macro)(), portray;
	term inherits, plist;			/* propery list extension */
	term link;
	char name[8];
} dummy_atom;

typedef struct dummy_compterm			/**** DUMMY ****/
{
	itemtype type;
	bits flags;
	card arity;
	term arg[6];
} dummy_compterm;

typedef struct dummy_clause			/**** DUMMY ****/
{
	itemtype type;
	bits flags;
	card nvars;
	term next;
	term label;				/* label extension */
	term goal[6];
} dummy_clause;

#endif


/************************************************************************/
/*	Define a union for uniform access to all Prolog data types	*/
/************************************************************************/

union pobj
{
	struct atom		a;
	struct compterm		c;
	struct integer		i;
	struct real		r;
	struct var		v;
	struct clause		g;
	struct pred		b;
	struct subr		u;
	struct ref		p;
	struct free_cell	f;
	struct stream		s;
	struct set		z;
	struct chunk		d;
#ifdef DDD
	struct dummy_atom	d_a;
	struct dummy_compterm	d_c;
	struct dummy_clause	d_g;
#endif
};


/************************************************************************/
/*			Structure Acess Macros				*/
/************************************************************************/

#define TYPE(x)		((x) -> c.type)
#define FLAGS(x)	((x) -> c.flags)
#define ARITY(x)	((x) -> c.arity)
#define ARG(n, x)	((x) -> c.arg[n])
#define CAR(x)		((x) -> c.arg[0])
#define CDR(x)		((x) -> c.arg[1])
#define NAME(x)		((x) -> a.name)
#define PROC(x)		((x) -> a.proc)
#define MACRO(x)	((x) -> a.macro)
#define TERM_EXPAND(x)	((x) -> a.macro)
#define PORTRAY(x)	((x) -> a.portray)
#define INHERITS(x)	((x) -> a.inherits)
#define PLIST(x)	((x) -> a.plist)
#define LINK(x)		((x) -> a.link)
#define PREFIX(x)	((x) -> a.prefix)
#define INFIX(x)	((x) -> a.infix)
#define POSTFIX(x)	((x) -> a.postfix)
#define PRE_PREC(x)	(PREFIX(x) & PRECEDENCE)
#define IN_PREC(x)	(INFIX(x) & PRECEDENCE)
#define POST_PREC(x)	(POSTFIX(x) & PRECEDENCE)
#define IS_OP(x)	(TYPE(x) == ATOM && ((x) -> a.flags & OP))
#define IS_GENERIC(x)	(TYPE(x) == ATOM && ((x) -> a.flags & GENERIC))
#define SPIED(x)	(FLAGS(x) & SPY)
#define OFFSET(x)	((x) -> v.offset)
#define PNAME(x)	((x) -> v.pname)
#define IVAL(x)		((x) -> i.int_val)
#define RVAL(x)		((x) -> r.real_val)
#define NVARS(x)	((x) -> g.nvars)
#define LABEL(x)	((x) -> g.label)
#define HEAD(x)		((x) -> g.goal[0])
#define BODY(x)		&((x) -> g.goal[1])
#define GOAL(n, x)	((x) -> g.goal[n])
#define NEXT(x)		((x) -> g.next)
#define ID(x)		((x) -> b.id)
#define C_CODE(x)	((x) -> b.c_code)
#define S_CODE(x)	((x) -> u.c_code)
#define POINTER(x)	((x) -> p.pointer)
#define TRAIL(x)	((x) -> p.trail)
#define NEXT_FREE(x)	((x) -> f.next_free)
#define SIZE(x)		((x) -> f.size)
#define FPTR(x)		((x) -> s.fptr)
#define MODE(x)		((x) -> s.mode)
#define FILE_NAME(x)	((x) -> s.fname)
#define NEXT_STREAM(x)	((x) -> s.next_stream)
#define IOBUF(x)	((x) -> s.iobuf)

#define SET_SIZE(x)	((x) -> z.set_size)
#define CONTENTS(x)	((x) -> z.contents)
#define SELECTOR(n, x)	((x) -> z.sel[n])
#define CLASS(x)	((x) -> z.class)

#define NSEL(x)		((x) -> z.nsel)
#define NPOS(x)		((x) -> z.npos)
#define NNEG(x)		((x) -> z.nneg)

#define SUBSUMES(x)	((x) -> z.subsumes)
#define OPERATOR(x)	((x) -> z.npos)
#define COMPRESSION(x)	((x) -> z.nneg)

#define CHUNK_SPEC(x)	((x) -> d.spec)
#define CHUNK_DATA(x)	((x) -> d.data)

#define CHUNK_NAME(p)	CHUNK_SPEC(p) -> name
#define FREE_CHUNK(p)	CHUNK_SPEC(p) -> free(p)
#define PRINT_CHUNK(p)	CHUNK_SPEC(p) -> print(p)

/************************************************************************/
/*			type checking macros				*/
/************************************************************************/

#define isinteger(x)	(TYPE(x) == INTEGER)
#define isvariable(x)	(TYPE(x) == FREE || TYPE(x) == BOUND)
#define iscompound(x)	(TYPE(x) == FN)
#define islist(x)	(TYPE(x) == LIST)
#define isatom(x)	(TYPE(x) == ATOM)


/************************************************************************/
/*			Derefernce a reference				*/
/************************************************************************/

#define DEREF(p)	while (TYPE(p) == REF && POINTER(p) != NULL) p = POINTER(p);


/************************************************************************/
/*			Common external declarations			*/
/************************************************************************/

extern FILE *input, *output;
extern FILE *dialog_in, *dialog_out, *alert;
extern term current_input, current_output;

extern term
	_nil, _true, _false, _anon, _prompt, _prolog_prompt, _op,
	_end_of_file, _file, _rbrace, _rpren, _rbrac, _lpren, _lbrac,
	_lbrace, _neck, _dot, _bang, _question, _table, _export, _import, _bar,
	_comma, _equal, _plus, _minus, _user_prompt, _arrow, _semi_colon;


/************************************************************************/
/*			    Common Prototypes				*/
/************************************************************************/

#include "mem.h"
#include "atom.h"
#include "make.h"
#include "pred.h"
#include "unify.h"
#include "prove.h"
#include "eval.h"
#include "evloop.h"
#include "out.h"
#include "files.h"
#include "lex.h"
#include "read.h"
#include "dcg.h"

#include "plist.h"
#include "worlds.h"

#include "p_math.h"
#include "p_lists.h"
#include "p_compare.h"
#include "p_db.h"
#include "p_system.h"
#include "p_sockets.h"
#include "p_unix.h"
