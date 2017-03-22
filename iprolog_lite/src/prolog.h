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
	PRED, FPRED, SUBR, FSUBR, STREAM, NUMBER, ANY
};

typedef unsigned char bits;
typedef unsigned short card;
typedef unsigned char optype;
typedef unsigned char itemtype;
typedef unsigned char lextype;
typedef unsigned char chartype;



/************************************************************************/
/*			record structures in free space			*/
/************************************************************************/

typedef union pobj *term;

typedef struct atom
{
	itemtype type;
	bits flags;
	unsigned short prefix, infix, postfix;
	term (*macro)(), portray;
	term inherits, plist;			/* propery list extension */
	term proc;
	term link;
	char name[EXTENSIBLE];
} atom;

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

#endif

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
	term arg[EXTENSIBLE];
} compterm;

#ifdef DDD

typedef struct dummy_compterm			/**** DUMMY ****/
{
	itemtype type;
	bits flags;
	card arity;
	term arg[6];
} dummy_compterm;

#endif

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

typedef struct clause
{
	itemtype type;
	bits flags;
	card nvars;
	term next;
	term label;				/* label extension */
	term goal[EXTENSIBLE];
} clause;

#ifdef DDD

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

typedef struct pred
{
	itemtype type;
	bits flags;
	term id;
	int (*c_code)();
} pred;

typedef struct subr
{
	itemtype type;
	bits flags;
	term id;
	term (*c_code)();
} subr;

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

typedef struct free_cell
{
	itemtype type;
	bits flags;
	card size;
	term next_free;
} free_cell;

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
extern term current_input, current_output;

extern term
	_nil, _true, _false, _anon, _prompt, _prolog_prompt, _op,
	_end_of_file, _file, _rbrace, _rpren, _rbrac, _lpren, _lbrac,
	_lbrace, _neck, _dot, _bang, _question, _table, _export, _import, _bar,
	_comma, _equal, _plus, _minus, _user_prompt, _arrow, _semi_colon;


/************************************************************************/
/*				Prototypes				*/
/************************************************************************/

term intern(char *);
term gensym(char *);
term new_int(long);
term new_h_int(long);
term new_real(double);
term new_h_real(double);
term new_h_fn(int);
term new_g_fn(int);
term new_ref(void);
term new_unit(term);
term new_pred(int (*)(), char *);
term new_fpred(int (*)(), char *);
term new_subr(term (*)(), char *);
term new_fsubr(term (*)(), char *);

term add_stream(term, term, FILE *);

term gcons(term, term);
term hcons(term, term);
term h_fn1(term, term);
term g_fn1(term, term);
term h_fn2(term, term, term);
term g_fn2(term, term, term);
term mkclause(term, term *);
term add_clause(term, int);
term check_arg();
term galloc(int);
term halloc(int);
term make(term, term *);
term copy(term, term *);
term eval(term, term *);
term progn(term, term *);
term get_atom(void);
term p_read(void);

int length(term, term *);
int unify(term, term *, term, term *);
term unbind(term, term *);
term *new_local_frame(int);
int cond(term *, term *);

term call_prove(term *, term *, term, int, void (*)(), int);

/* set.c */

term new_set(int, term);
term copy_set(term);
void clear_set(term);
void set_add(term, short);
int set_intersect(term, term, term);
int set_union(term, term, term);
int set_diff(term, term, term);
int null_set(term);
int set_eq(term, term);
int set_contains(term, term);
int disj_contains(term, term);
int set_cardinality(term);
int intersection_size(term, term);
int distance(term, term);
void print_set(term);
void set_init(void);

/* hash.c */

short new_hash_entry(term, term);
short get_hash_entry(term, term);
term new_hash_table(int);

void warning(char *);
void fail(char *);

/* array.c */

int *new_ivector(int);
int **new_2D_iarray(int, int);
double *new_dvector(int);
double **new_2D_array(int, int);
double ***new_3D_array(int, int, int);
void matinv(double **, double **, int);
void matmul(double **, double **, double **, int , int, int);
void inner(double **, double **, double *, int, int);

/* table.c */

term get_table_defn(term);

/* frame.c */

void make_slot(term, char *, term);
term build_frame(char *, ...);
term slot_value(term, term);

/* p_system.c */

double get_time(void);
char *date_time(void);
