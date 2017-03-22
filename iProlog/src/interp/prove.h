/* prove.c */

#define INTERACTIVE
/*
#define MAKE_FRAME(x)\
	if ((local += x) >= local_end)\
		fail("Out of memory. Too many procedures called.");
*/

typedef struct query_struct *query;
typedef struct env_struct *env;

typedef struct query_struct
{
	query previous_query;
	env old_top_of_stack;
	env query_env;
	term query_vars;
	term result;
	int how_many;
	void (*new_result)();
} query_struct;

typedef struct env_struct
{
	env parent;
	term *goals;
	term *frame;
	term *global;
	term trail;
	env prev;
	int cut;
} env_struct;

//extern term *local, *local_end, *global;
extern term *global;
extern env top_of_stack;


/************************************************************************/
/*			    	Prototypes				*/
/************************************************************************/

//term *new_local_frame(int);
int cond(term *, term *);
int rest_of_clause(void);
void untrail(term);
void _untrail(void);
void lush(term, term, int);
term call_prove(term *, term *, term, int, void (*)(), int);
void backtrace(void);
void directive(term, term);
void trace_print(char *, env, term);
void cut(env);
int prove(term *, env);

