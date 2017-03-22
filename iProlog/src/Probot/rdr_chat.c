/************************************************************************/
/*	Perform interactive RDR maintenance on chat rules		*/
/************************************************************************/

#include "prolog.h"
#include "probot.h"
#include "read_script.h"
#include "rdr.h"

static term _answer, _respond, _star, _var;


/************************************************************************/
/*		Compute Levenshtein distance between s and t		*/
/*									*/
/* The first row and column are initialized to denote distance from	*/
/* the empty string. The res are 0.					*/
/*									*/
/* Set the cost to 1 iff the ith term of s does not equal the jth of t.	*/
/* When the terms are equal there is no need to substitute,		*/
/* so the cost is 0.							*/
/*									*/
/* Cell d[i][j] equals the minimum of:					*/
/*									*/
/* - The cell immediately above plus 1					*/
/* - The cell immediately to the left plus 1				*/
/* - The cell diagonally above and to the left plus the cost		*/
/*									*/
/* We can either insert a new char, delete a char or			*/
/* substitute an existing char (with an associated cost)		*/
/*									*/
/* Finally, the Levenshtein distance equals the rightmost bottom	*/
/* cell of the matrix..							*/
/* d[i][j] denotes the distance between the substrings 1..i and 1..j	*/
/************************************************************************/

static int
minimum(int a, int b, int c)
{
	int min = a;

	if (b < min) min = b;
	if (c < min) min = c;

	return min;
}


int
levenshtein_distance(term s, term t)
{
	int cost, i, j;
	term p, q;
	int n = length(s, NULL);
	int m = length(t, NULL);
	int d[n+1][m+1];

	if (n == 0 || m == 0)
		return -1;

	for (j = 0; j <= m; j++)
		d[0][j] = j;

	for (i = 0; i <= n; i++)
		d[i][0] = i;

	for (i = 1; i <= n; i++)
		for (j = 1; j <= m; j++)
			d[i][j] = 0;

	for (i = 1, p = s; i <= n; i++, p = CDR(p))
	{
		for (j = 1, q = t; j <= m; j++, q = CDR(q))
		{
			cost = (CAR(p) != CAR(q));

			d[i][j] = minimum(d[i-1][j]+1, d[i][j-1]+1, d[i-1][j-1] + cost);
		}

	}
/*
	for (i = 0; i <= n; i++)
	{
		for (j = 0; j <= m; j++)
			printf("%3d", d[i][j]);

		printf("\n");
	}
*/
	return d[n][m];
}


static term
edit_distance(term goal, term *frame)
{
	term s = check_arg(1, goal, frame, LIST, IN);
	term t = check_arg(2, goal, frame, LIST, IN);
	int rval = levenshtein_distance(s, t);

	if (rval == -1)
		fail("Cannot find edit distance with empty list");

	return new_int(rval);
}


/************************************************************************/
/*		Look for common subsequences in a pair of lists		*/
/************************************************************************/

static term
sub_seq(term input, term response)
{
	term p, q;
	term L = _nil, *last = &L;
	term subst = _nil, *last_subst = &subst;

	for (p = response; p != _nil; p = CDR(p))
		for (q = input; q != _nil; q = CDR(q))
			if (CAR(p) == CAR(q))
			{
				*last = gcons(CAR(p), _nil);
				last = &CDR(*last);
			}
			else if (L != _nil)
			{
				*last_subst = gcons(L, _nil);
				last_subst = &CDR(*last_subst);
				L = _nil;
				last = &L;
			}
	
	if (L != _nil)
	{
		*last_subst = gcons(L, _nil);
		last_subst = &CDR(*last_subst);
	}

	return subst;
}


/************************************************************************/
/*		Generate pattern from common subsequences		*/
/************************************************************************/

static term
gen_pattern(term input, term subst, term marker)
{
	term p;
	term sent = input;
	int n_vars = 0;

	for (p = subst; p != _nil; p = CDR(p))
	{
		term L = _nil, *last = &L;
		term subseq = CAR(p);

		repeat
		{
			if (subseq == _nil)
			{
				if (marker == _var)
					*last = gcons(g_fn1(_var, new_int(++n_vars)), _nil);
				else
					*last = gcons(marker, _nil);
				last = &CDR(*last);

				subseq = CAR(p);
			}
			if (sent == _nil)
				break;
			if (CAR(subseq) == CAR(sent))
			{
				subseq = CDR(subseq);
				sent = CDR(sent);
				continue;
			}
			subseq = CAR(p);

			*last = gcons(CAR(sent), _nil);
			last = &CDR(*last);
			sent = CDR(sent);
		}
		sent = L;
	}

	return sent;
}


/************************************************************************/
/*		Look for common subsequences in a pair of lists		*/
/************************************************************************/

static int
p_seq_pattern(term goal, term *frame)
{
	term new_case = check_arg(1, goal, frame, LIST, IN);
	term old_case = check_arg(2, goal, frame, LIST, IN);
	term subst, pattern1, pattern2;

	subst = sub_seq(new_case, old_case);
	printf("Subst: "); print(subst);

	pattern1 = gen_pattern(new_case, subst, _star);
	printf("Pattern1: "); print(pattern1);

	pattern2 = gen_pattern(old_case, subst, _star);
	printf("Pattern2: "); print(pattern2);

	return TRUE;
}


/************************************************************************/
/*		  RDR interaction for learning chat rules		*/
/************************************************************************/

static int
p_rdr_chat(term goal, term *frame)
{
	term rule = check_arg(1, goal, frame, FN, IN);
        term conclusion, old_case, new_case;
        term *old_global = global;
        term subst = _nil, pattern = _nil, response = _nil;

	new_case = read_user_input();
	eval(rule, frame);
        old_case = last_case;

	if (yes_no(intern("this the correct response?"), ""))
		return TRUE;

	fprintf(output, "New case: "); print(new_case);
	fprintf(output, "Old case: "); print(old_case);

	printf("What is the correct response?\n");
	conclusion = read_sentence();

	subst = sub_seq(new_case, conclusion);
	printf("Subst: "); print(subst);

	pattern = gen_pattern(new_case, subst, _star);
	printf("Pattern: "); print(pattern);

	response = gen_pattern(conclusion, subst, _var);
	printf("Response: "); print(response);

        add_rdr(new_rule(
			 make(g_fn1(_answer, pattern), frame),
			 make(g_fn1(_respond, response), frame),
			 make(new_case, frame),
			 _anon,
			 _anon
	));
        print(rule);

        global = old_global;

        return TRUE;
}


/************************************************************************/
/*				Nearest Neighbour			*/
/************************************************************************/

static void
nconc(term *x, term y)
{
	while (*x != _nil)
		x = &(CDR(*x));
	*x = y;
}


static int
p_nn_chat(term goal, term *frame)
{
	term rules = check_arg(1, goal, frame, LIST, IN);
	term p, best_match, user_input, pattern, response, new_response, subst;
	int best_score = 1000;

	if ((user_input = read_sentence()) == _nil)
		return TRUE;

	for (p = rules; p != _nil; p = CDR(p))
	{
		int score = levenshtein_distance(user_input, ARG(1, CAR(p)));

		if (score < best_score)
		{
			best_score = score;
			best_match = CAR(p);
		}
	}

	write_response(ARG(2, best_match));

	if (yes_no(intern("this the correct response?"), ""))
		return FALSE;

	if (best_score == 0)
	{
		printf("It was the right response last time\n");
		return FALSE;
	}

	printf("What is the correct response?\n");
	new_response = read_sentence();

	subst = sub_seq(user_input, new_response);
	printf("Subst: "); print(subst);

	pattern = gen_pattern(user_input, subst, _star);
	printf("Pattern: "); print(pattern);

	response = gen_pattern(new_response, subst, _var);
	printf("Response: "); print(response);
	
	nconc(&rules, hcons(h_fn2(_arrow, make(pattern, frame), make(response, frame)), _nil));

	return FALSE;
}


/************************************************************************/
/*				Initialisation				*/
/************************************************************************/

void
rdr_chat_init(void)
{
	_answer = intern("answer");
	_respond = intern("respond");
	_star = intern("*");
	_var = intern("var");

	new_pred(p_rdr_chat, "rdr_chat");
	new_pred(p_seq_pattern, "seq_pattern");
	new_fsubr(edit_distance, "edit_distance");
	new_pred(p_nn_chat, "nn_chat");
}

