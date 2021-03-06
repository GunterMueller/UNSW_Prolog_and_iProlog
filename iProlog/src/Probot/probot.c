/************************************************************************/
/*			    Probot scripting engine			*/
/************************************************************************/

#include <ctype.h>
#include "prolog.h"
#include "probot.h"
#include "read_script.h"
#include "frame.h"

#define MAX_VARS	10

#define CONTEXT(x)	ARG(0, x)
#define TIME_STAMP(x)	ARG(1, x)
#define PATTERN(x)	ARG(2, x)
#define RESPONSE(x)	ARG(3, x)

static term _rule, _star, _init;
static term _non_terminal, _nextof, _anyof, _var;
static term _topic, _filter, _backup;
static term *new_frame;

static int time_stamp = 0;


/************************************************************************/
/*			probot pattern matching routine			*/
/************************************************************************/

static int non_terminal(term, term, term *, term *);
static int match_pattern(term, term, term *, term *);
static term *pos_var = NULL;
static int nvars;

static int
match_string(char *pattern, char *input)
{
	int tilde = 0;

/*	fprintf(stderr, "Match %s AND %s\n", pattern, input);
*/ 
	for (;;)
	{
		char *last = pattern;

		for (;;)
		{
			if (*pattern == '\0' && *input == '\0')
				return 1;

			if (*pattern == '~')
			{
				pattern++;
				tilde++;
				break;
			}

			if (*input == '\0')
				return 0;

			if (tolower(*pattern++) != tolower(*input++))
			{
				if (! tilde)
					return 0;

				pattern = last;
				break;
			}
		}
	}
}


static int
match_any_pattern(term pattern, term input, term *tree, term *cont)
{
	if (TYPE(CAR(pattern)) == FN)
	{
		if (ARG(0, CAR(pattern)) == _non_terminal)
		{
			if (non_terminal(ARG(1, CAR(pattern)), input, tree, cont))
				return TRUE;
		}
		else /* This is an "any of" construct */
		{
			term p;
			int old_nvars = nvars;

			for (p = ARG(1, CAR(pattern)); p != _nil; p = CDR(p))
				if (match_pattern(CAR(p), input, tree, cont))
					return TRUE;
				else
					nvars = old_nvars;
		}
	}
	else if (TYPE(CAR(pattern)) == ATOM && TYPE(CAR(input)) == ATOM)
	{
		if (match_string(NAME(CAR(pattern)), NAME(CAR(input))))
		{
			*tree = CAR(input);
			*cont = CDR(input);
			return TRUE;
		}
	}
	else if (term_compare(CAR(pattern), NULL, CAR(input), NULL) == 0)
	{

		*tree = CAR(input);
		*cont = CDR(input);
		return TRUE;
		
	}
	
	return FALSE;
}


static int
match_pattern(term pattern, term input, term *tree, term *continuation)
{
	int skip = FALSE;
	term L = _nil, *last = &L;
	term rval = _nil, *end = &rval;
	term tmp, cont;

	repeat
	{
#ifdef DEBUG
		fprintf(output, "PATTERN: "); print(pattern);
		fprintf(output, "INPUT:   "); print(input);
		fflush(output);
#endif
		if (pattern == _nil)
		{
			if (skip)
			{
				pos_var[nvars++] = input;
				*end = gcons(input, _nil);
				input = _nil;
			}
			*tree = rval;
			*continuation = input;
			return TRUE;
		}
		else if (CAR(pattern) == _nil)
			pattern = CDR(pattern);
		else if (CAR(pattern) == _star)
		{
			skip = TRUE;
			pattern = CDR(pattern);
		}
		else if (TYPE(CAR(pattern)) == FN
		     && ARG(0, CAR(pattern)) != _anyof
		     && ARG(0, CAR(pattern)) != _nextof
		     && ARG(0, CAR(pattern)) != _non_terminal)
		{
			if (skip)
			{
				pos_var[nvars++] = input;
				*end = gcons(input, _nil);
				input = _nil;
				skip = FALSE;
			}
			if (eval(CAR(pattern), new_frame) != _true)
				return FALSE;
			pattern = CDR(pattern);
		}
		else if (input == _nil)
			return FALSE;
		else if (match_any_pattern(pattern, input, &tmp, &cont))
		{
			if (skip)
			{
				*end = gcons(L, _nil);
				end = &CDR(*end);
				pos_var[nvars++] = L;
				L = _nil;
				last = &L;
				skip = FALSE;
			}
			*end = gcons(tmp, _nil);
			end = &CDR(*end);

			if (TYPE(CAR(pattern)) != ATOM)
				pos_var[nvars++] = tmp;

			pattern = CDR(pattern);
			input = cont;
		}
		else if (skip)
		{
			*last = gcons(CAR(input), _nil);
			last = &CDR(*last);
			input = CDR(input);
		}
		else
			return FALSE;
	}
}


/************************************************************************/
/*			probot response generation			*/
/************************************************************************/

static term current_context;

static term
nextof(term *L)
{
	term *p, rval = CAR(*L);

	for (p = L; *p != _nil; p = &CDR(*p));

	*p = *L;
	*L = CDR(*L);
	CDR(*p) = _nil;
	return rval;
}


static term
anyof(term L)
{
	int r, length = 0;
	term p;

	for (p = L; p != _nil; p = CDR(p))
		length++;

	r = (int)((double)(length)*rand()/(RAND_MAX+1.0));

	while (r--)
		L = CDR(L);

	return CAR(L);
}


static int gen_expression(term);

static void
gen_sentence(term sentence)
{
	while (sentence != _nil)
	{
		term x = CAR(sentence);

		DEREF(x);

		gen_expression(x);

		sentence = CDR(sentence);
		DEREF(sentence);
	}
}

static term *cursor = NULL;

static int
gen_expression(term x)
{
	extern int display;

	display = FALSE;

	switch (TYPE(x))
	{
		case FREE:
		case BOUND:
			x = PNAME(x);
		case INT:
		case REAL:
		case ATOM:
			if (x == _nil)
				break;
			*cursor = gcons(x, _nil);
			cursor = &CDR(*cursor);
			break;
		case LIST:
			gen_sentence(x);
			break;
		case FN:
			if (ARG(0, x) == _anyof)
				while (! gen_expression(anyof(ARG(1, x))));
			else if (ARG(0, x) == _nextof)
				while (! gen_expression(nextof(&ARG(1, x))));
			else if (ARG(0, x) == _arrow)
			{
				if (eval(CAR(ARG(1, x)), new_frame) == _true)
					gen_expression(ARG(2, x));
				else
					return 0;
			}
				else if (ARG(0, x) == _var)
				{
					term expr = ARG(1, x);

					if (TYPE(expr) == INT)
					{
						if (IVAL(expr) >= nvars)
							fail("Position variable out of range");
						gen_expression(pos_var[IVAL(expr)]);
					}
					else
					{
						term tmp = eval(expr, new_frame);

						if (tmp == NULL || tmp == _false)
							fail("probot couldn't evaluate an expression");
						*cursor = gcons(tmp, _nil);
						cursor = &CDR(*cursor);
					}
				}
				else
					eval(x, new_frame);
			break;
		default:
			fputs("\n*** ", output);
			print(x);
			fflush(output);
			fail("Bad response expression");
			break;
	}
	return 1;
}


/************************************************************************/
/*		Generate response and return expression			*/
/*		Store old cursor for recursive call to gen_response	*/
/************************************************************************/

static term
gen_response(term x)
{
	term *old_cursor = cursor;
	term rval = _nil;

	cursor = &rval;
	gen_expression(x);
	cursor = old_cursor;
	return rval;
}


/************************************************************************/
/*			Write response to terminal			*/
/************************************************************************/

static void
write_sentence(term x)
{
	while (x != _nil)
	{
		switch (TYPE(CAR(x)))
		{
			case ATOM:	if (! ispunct(NAME(CAR(x))[0]))
				fputc(' ', output);
				prin(CAR(x));
				break;
			case LIST:	write_sentence(CAR(x));
				break;
			default:	fputc(' ', output);
				prin(CAR(x));
				break;
		}
		x = CDR(x);
	}
}


void
write_response(term x)
{
	term t = gen_response(x);

	if (cursor == NULL)
	{
		FILE *old_input = input;
		FILE *old_output = output;

		input = dialog_in;
		output = dialog_out;

		write_sentence(t);
		fputc('\n', output);
		fflush(output);

		input = old_input;
		output = old_output;
	}
	else
	{
		*cursor = gcons(t, _nil);
		cursor = &CDR(*cursor);
	}
}


/************************************************************************/
/*	    Call probot recursively for non-terminal symbols		*/
/************************************************************************/

static term
fill0(term sentence, term stop)
{
	term rval, *L = &rval, p;
		for (p = sentence; p != stop; p = CDR(p))
		{
			*L = gcons(CAR(p), _nil);
			L = &CDR(*L);
		}
			return rval;
}

static int
non_terminal(term context, term sentence, term *tree, term *cont)
{
	extern term *global;
	term *old_global = global;
	int old_nvars = nvars;
	term *old_pos_var = pos_var;
	term loc_var[MAX_VARS];
	term p;

	pos_var = loc_var;

	for (p = PROC(context); p != NULL; p = NEXT(p))
	{
		pos_var[0] = sentence;
		nvars = 1;
#ifdef DEBUG
		fprintf(stderr, "NON-TERMINAL: %s\n", NAME(context));
#endif
		if (match_pattern(PATTERN(HEAD(p)), sentence, tree, cont))
		{
//			pos_var[0] = *tree;
			if (*cont != _nil)
				pos_var[0] = fill0(sentence, *cont);

			if (RESPONSE(HEAD(p)) != _nil)
				*tree = gen_response(RESPONSE(HEAD(p)));
			else							// Don't know
				*tree = pos_var[0];				// about this

			pos_var = old_pos_var;
			nvars = old_nvars;
			return TRUE;
		}
		else
			global = old_global;
	}

	pos_var = old_pos_var;
	nvars = old_nvars;
	return FALSE;
}


/************************************************************************/
/*			Top level probot routines			*/
/************************************************************************/

static int
probot(term context, term sentence)
{
	term *old_global = global;
	term p, tree, cont;
	int rval = FALSE;
	int old_nvars = nvars;
	term *old_pos_var = pos_var;
	term loc_var[MAX_VARS];

//	new_frame = local;
	pos_var = loc_var;
/*
	fprintf(stderr, "\nCONTEXT: %s\n", NAME(context));
*/
	for (p = PROC(context); p != NULL; p = NEXT(p))
	{
		term frame[NVARS(p)];

		new_frame = frame;
		pos_var[0] = sentence;
		nvars = 1;

//		MAKE_FRAME(NVARS(p));
		
		if (match_pattern(PATTERN(HEAD(p)), sentence, &tree, &cont) && cont == _nil)
		{
			IVAL(TIME_STAMP(HEAD(p))) = ++time_stamp;
			write_response(RESPONSE(HEAD(p)));
			rval = TRUE;
			break;
		}
//		local = new_frame;
	}

	pos_var = old_pos_var;
	nvars = old_nvars;
	global = old_global;
//	local = new_frame;
	return rval;
}


static int
scan(term con, term sentence)
{
	term p;

	for (p = PROC(con); p != NULL; p = NEXT(p))
		if (probot(ARG(1, HEAD(p)), sentence))
			return 1;
	return 0;
}


void
start_probot(term goal, term *frame)
{
//	FILE *old_output = output;
//	output = stderr;
//	rprint(goal, frame);
//	output = old_output;

	if (TYPE(goal) == FN)
	{
		current_context = check_arg(1, goal, frame, ATOM, IN);

		if (ARITY(goal) == 2)
		{
			term init = check_arg(2, goal, frame, LIST, IN);
			probot(current_context, init);
		}
	}
	else
		current_context = _rule;
}


int
process_sentence(void)
{
	term sentence = read_sentence();

	if (sentence == _nil) return 0;
	if (scan(_filter, sentence)) return 1;
	if (probot(current_context, sentence)) return 1;
	if (scan(_backup, sentence)) return 1;
	return 2;
}


static int
p_probot(term goal, term *frame)
{
	start_probot(goal, frame);

	repeat
	{
		switch (process_sentence())
		{
			case 0: return TRUE;
			case 1: continue;
			case 2: return FALSE;
		}
	}

	return FALSE;
}


/************************************************************************/
/*   Built-in predicate to goto new context, no filter and backup	*/
/************************************************************************/

static int
p_goto(term goal, term *frame)
{
	current_context = check_arg(1, goal, frame, ATOM, IN);

	if (ARITY(goal) == 2)
	{
		term init = check_arg(2, goal, frame, LIST, IN);

		probot(current_context, init);
	}

	return TRUE;
}

/************************************************************************/
/*   Push and pop topics (with filters and backups) on and off a stack	*/
/************************************************************************/

static int
p_new_topic(term goal, term *frame)
{
	term topic = check_arg(1, goal, frame, ATOM, IN);
	term filter = check_arg(2, goal, frame, ATOM, IN);
	term backup = check_arg(3, goal, frame, ATOM, IN);
	term p;
	
	for (p = PROC(_topic); p != NULL; p = NEXT(p))
		if (ARG(1, HEAD(p)) == topic)
			return TRUE;

	add_clause(new_unit(h_fn1(_topic, topic)), TRUE);
	add_clause(new_unit(h_fn1(_filter, filter)), TRUE);
	add_clause(new_unit(h_fn1(_backup, backup)), TRUE);

	return TRUE;
}


static int
p_pop_topic(term goal, term *frame)
{
	term init = check_arg(1, goal, frame, LIST, IN);
	term p;

	if ((p = PROC(_topic)) != NULL)
	{
		PROC(_topic) = NEXT(p);
		free_term(p);
	}
	if ((p = PROC(_filter)) != NULL)
	{
		PROC(_filter) = NEXT(p);
		free_term(p);
	}
	if ((p = PROC(_backup)) != NULL)
	{
		PROC(_backup) = NEXT(p);
		free_term(p);
	}

	if (PROC(_topic) == NULL)
		fail("No topics left on stack");

	current_context = ARG(1, HEAD(PROC(_topic)));

	if (scan(_filter, init))
		return TRUE;
	if (probot(current_context, init))
		return TRUE;
	if (scan(_backup, init))
		return TRUE;

	return TRUE;
}


/************************************************************************/
/* 	"^" function for evaluating expressions in a response		*/
/************************************************************************/

static term
p_var(term expr, term *frame)
{
	term x = check_arg(1, expr, frame, ANY, IN);

	if (TYPE(x) == INT)
	{
		int i = IVAL(x);

		if (i > nvars)
			fail("Position variable out of range");

		return pos_var[i];
	}
	else
	{
		term tmp = eval(x, new_frame);

		if (tmp == NULL || tmp == _false)
			fail("probot couldn't evaluate an expression");

		return tmp;
	}
}


/************************************************************************/
/* 		    Make an arbitrary choice of response		*/
/************************************************************************/

static int
p_cycle(term goal, term *frame)
{
	extern term *global;
	term *old_global = global;
	term p;
	int rval = FALSE;

	nvars = 0;
/*
	fprintf(stderr, "\nCONTEXT: %s\n", NAME(current_context));
*/
	for (p = PROC(current_context); p != NULL; p = NEXT(p))
		if (IVAL(TIME_STAMP(HEAD(p))) == 0)
		{
			IVAL(TIME_STAMP(HEAD(p))) = ++time_stamp;
			write_response(RESPONSE(HEAD(p)));
			rval = TRUE;
			break;
		}

	global = old_global;
	return rval;
}


/************************************************************************/
/* 	Built-in succeeds if there are more responses available		*/
/************************************************************************/

static int
more_responses(term goal, term *frame)
{
	term p;

	for (p = PROC(current_context); p != NULL; p = NEXT(p))
		if (IVAL(TIME_STAMP(HEAD(p))) == 0)
			return TRUE;

	return FALSE;
}

	
/************************************************************************/
/* Fetch an input previously stored in memory and process it in given	*/
/* context								*/
/************************************************************************/

static term _memory;

static int
p_recall_memory(term goal, term *frame)
{
	term context = check_arg(1, goal, frame, ATOM, IN);
	term p = PROC(_memory);

	if (p == NULL)
		return FALSE;
	if (TYPE(HEAD(p)) != FN || ARITY(HEAD(p)) != 1)
		fail("Bad memory");

	if (probot(context, ARG(1, HEAD(p))))
	{
		PROC(_memory) = NEXT(p);
		free_term(p);
		return TRUE;
	}

	return FALSE;
}


/************************************************************************/
/* 	Check if a predicate is defined					*/
/*	Useful to see if something is stored in memory			*/
/************************************************************************/

static int
p_have(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, ATOM, IN);

	return (PROC(x) != NULL);
}


/************************************************************************/
/*   Use pattern matcher and response generator for asking questions	*/
/************************************************************************/

static term user_input = NULL;
static term answer;

term
read_user_input(void)
{
	return user_input = read_sentence();
}


static int
p_answer(term goal, term *frame)
{
	term p = check_arg(1, goal, frame, LIST, IN);
	term tree, cont;
	int rval = FALSE;
	static term loc_var[MAX_VARS];

	if (user_input == NULL)
		fail("Can only match on the answer to a question");

	pos_var = loc_var;
	pos_var[0] = user_input;
	nvars = 1;

	if (match_pattern(p, user_input, &tree, &cont) && cont == _nil)
		rval = TRUE;
	
	return rval;
}


static int
p_respond(term goal, term *frame)
{
	term x = check_arg(1, goal, frame, LIST, IN);

	write_response(x);

	return TRUE;
}


static int
p_return(term goal, term *frame)
{
	answer = check_arg(1, goal, frame, ANY, IN);

	return TRUE;
}


static term
f_question(term goal, term *frame)
{
	extern int check_range();
	term q = check_arg(1, goal, frame, LIST, IN);
	term a = check_arg(2, goal, frame, ANY, IN);
	term *old_global = global;
	int old_nvars = nvars;
	term *old_pos_var = pos_var;

	do
	{
		write_response(q);
		answer = NULL;
		global = old_global;
		user_input = read_sentence();
		eval(a, frame);
	}
	while (answer == NULL || ! check_range(answer));

	pos_var = old_pos_var;
	nvars = old_nvars;

	return answer;
}


static term
f_probot(term goal, term *frame)
{
	extern int check_range();
	int old_nvars = nvars;
	term *old_pos_var = pos_var;
	term context = check_arg(1, goal, frame, ATOM, IN);
	term init = check_arg(2, goal, frame, LIST, IN);
	term *oldcursor = cursor; 
	cursor = NULL; 

	answer = NULL;
	probot(context, init);

	do
	{
		term sentence = read_sentence();

		probot(context, sentence);
	}
	while (answer == NULL || ! check_range(answer));

	pos_var = old_pos_var;
	nvars = old_nvars;
	cursor = oldcursor; 
	return answer;
}


static int
p_match(term goal, term *frame)
{
	term sentence = check_arg(1, goal, frame, LIST, IN);
	term rules = check_arg(2, goal, frame, ANY, IN);
	term *old_global = global;
	int old_nvars = nvars;
	term *old_pos_var = pos_var;
	int rval;

	global = old_global;
	user_input = sentence;
	rval = eval(rules, frame) == _true;
	
	pos_var = old_pos_var;
	nvars = old_nvars;

	return rval;
}


/************************************************************************/
/*			   Module Initialisation			*/
/************************************************************************/

void
probot_init(void)
{
	extern int rdr_chat_init();

	rdr_chat_init();

	_rule					= intern("$probot");

	current_context				= _rule;
	_init					= hcons(intern("init"), _nil);

	_non_terminal				= intern("\\");
	_nextof					= intern("nextof");
	_anyof					= intern("anyof");
	_var					= intern("var");
	_star					= intern("*");

	_memory					= intern("memory");

	_topic					= intern("topic");
	_filter					= intern("filter");
	_backup					= intern("backup");

	new_pred(p_probot,			"probot");
	new_pred(p_goto,			"goto");
	new_pred(p_new_topic,			"new_topic");
	new_pred(p_pop_topic,			"pop_topic");

	defop(300, FX, new_subr(p_var,		"^"));

	new_pred(p_cycle,			"cycle_responses");
	new_pred(more_responses,		"more_responses");
	new_pred(p_have,			"have");
	new_pred(p_recall_memory,		"recall_memory");

	defop(300, FX, new_fpred(p_answer,	"answer"));
	defop(300, FX, new_fpred(p_respond,	"respond"));
	defop(700, FX, new_pred(p_return,	"return"));
	new_fsubr(f_question,			"question");
	new_subr(f_probot,			"query");
	new_pred(p_match,			"match");
}

