/************************************************************************/
/*	Implementation of de Kleer's ATMS algorithm using bit sets	*/
/************************************************************************/


#include "prolog.h"
#include "set.h"
#include "hash.h"

#define MAX_NODES		64

/* node access macros */

#define CONSEQUENTS(x)		PLIST(x)
#define JUSTIFICATIONS(x)	PROC(x)
#define NODE_LABEL(x)		INHERITS(x)

static term hash_table, index[MAX_NODES];
static term empty, nogood_db = NULL, free_list = NULL;

static void propagate(term, term);


/************************************************************************/
/*	Create new ATMS nodes, environments and	justifications		*/
/************************************************************************/

static term
new_node(term x)
{
	int i = new_hash_entry(x, hash_table);

	if (index[i] != NULL)
		return index[i];
	else
	{
		if (CONSEQUENTS(x) != _nil)
			free_term(CONSEQUENTS(x));
		if (JUSTIFICATIONS(x) != NULL)
			free_term(JUSTIFICATIONS(x));
		if (NODE_LABEL(x) != NULL)
			free_term(NODE_LABEL(x));

		CONSEQUENTS(x)		= _nil;
		JUSTIFICATIONS(x)	= NULL;
		NODE_LABEL(x)		= NULL;

		index[i] = x;
		return x;
	}
}


static void
delete_node(term x)
{
	int i = get_hash_entry(x, hash_table);

	if (i == -1)
		return;

	if (index[i] == NULL)
		return;

	index[i] = NULL;
	delete_hash_entry(i, hash_table);
}


static term
new_environment(void)
{
	if (free_list != NULL)
	{
		term rval = free_list;
		clear_set(rval);
		NEXT(rval) = NULL;

		free_list = NEXT(free_list);
		return rval;
	}
	return new_set(ARITY(hash_table)/BITS_IN_WORD, hash_table);
}


static void
free_environment(term *L)
{
	term e = *L;
	term x = NEXT(e);

	NEXT(e) = free_list;
	free_list = e;
	*L = x;
}


/************************************************************************/
/*    Create a new clause for justification and update consequents	*/
/************************************************************************/

static void
append1(term *list, term new_element)
{
	term *p;

	for (p = list; *p != _nil; p = &CDR(*p))
		if (CAR(*p) == new_element)
			return;

	*p = hcons(new_element, _nil);
}


static term
new_justification(term cl)
{
	int i;
	term c, rval = mkclause(cl, NULL);

	if (TYPE(HEAD(rval)) != ATOM)
	{
		free_term(rval);
		fail("ATMS only works with propositional clauses");
	}

	c = new_node(HEAD(rval));

	for (i = 1; GOAL(i, rval) != NULL; i++)
	{
		if (TYPE(GOAL(i, rval)) != ATOM)
		{
			delete_node(c);
			free_term(rval);
			fail("ATMS only works with propositional clauses");
		}

		new_node(GOAL(i, rval));
		append1(&CONSEQUENTS(GOAL(i, rval)), c);
	}

	add_clause(rval, FALSE);

	return rval;
}


/************************************************************************/
/*			compute the product of two labels		*/
/************************************************************************/

static term
product(term label1, term label2)
{
	term rval = NULL;
	term x, y, *z = &rval;

	for (x = label1; x != NULL; x = NEXT(x))
		for (y = label2; y != NULL; y = NEXT(y))
		{
			*z = new_environment();
			set_union(x, y, *z);
			z = &NEXT(*z);
		}
	*z = NULL;
	return rval;
}


/************************************************************************/
/*	Destructively append one label onto the end of another		*/
/************************************************************************/

static void
nconc(term *label1, term label2)
{
	while (*label1 != NULL)
		label1 = &NEXT(*label1);
	*label1 = label2;
}


/************************************************************************/
/*		Check if environment e is in the nogood data base	*/
/************************************************************************/

static int
consistent(term e)
{
	term p;

	for (p = nogood_db; p != NULL; p = NEXT(p))
		if (set_contains(e, p))
			return FALSE;
	return TRUE;
}


/************************************************************************/
/*		Check if environment p is contained in label L		*/
/************************************************************************/

static int
minimal(term p, term L)
{
	term q;

	for (q = L; q != NULL; q = NEXT(q))
		if (p != q && set_contains(p, q))
			return FALSE;
	return TRUE;
}


/************************************************************************/
/*	     Make label L1 is minimal with respect to label L2		*/
/************************************************************************/

static void
min_wrt(term *L1, term L2)
{
	while (*L1 != NULL)
		if (minimal(*L1, L2))
			L1 = &NEXT(*L1);
		else
			free_environment(L1);
}


/************************************************************************/
/*	Remove inconsistent and redundant environments from label L	*/
/************************************************************************/

static void
minimise(term *L)
{
	term start = *L;

	while (*L != NULL)
		if (consistent(*L) && minimal(*L, start))
			L = &NEXT(*L);
		else
			free_environment(L);
}


/************************************************************************/
/* Insert environment e into label L ensuring that no redundant		*/
/* environments are kept in the label					*/
/************************************************************************/

static void
insert(term e, term *L)
{
	while (*L != NULL)
	{
		if (set_contains(*L, e))
			return;
		else if (set_contains(e, *L))
			free_environment(L);
		else
			L = &NEXT(*L);
	}
	*L = e;
	NEXT(e) = NULL;
}


/************************************************************************/
/*		Remove environments containing e from label L		*/
/************************************************************************/

static void
remove_env(term e, term *L)
{
	while (*L != NULL)
		if (set_contains(*L, e))
			free_environment(L);
		else
			L = &NEXT(*L);
}


/************************************************************************/
/*				Update no goods	 			*/
/************************************************************************/

static void
nogood(term L)
{
	int i;
	term p, q;

	for (p = L; p != NULL; p = q)
	{
		q = NEXT(p);
		insert(p, &nogood_db);

		for (i = 0; i < MAX_NODES; i++)
			if (index[i] != NULL)
				remove_env(p, &NODE_LABEL(index[i]));
	}
}


/************************************************************************/
/*		Update consequents of new node				*/
/*		Recursively propagate changes in labels			*/
/************************************************************************/

static void
update(term L, term n)
{
	term p, cl;

	if (n == _false)
	{
		nogood(L);
		return;
	}

	min_wrt(&NODE_LABEL(n), L);
	min_wrt(&L, NODE_LABEL(n));
	nconc(&NODE_LABEL(n), L);

	for (p = CONSEQUENTS(n); p != _nil; p = CDR(p))
		for (cl = JUSTIFICATIONS(CAR(p)); cl != NULL; cl = NEXT(cl))
			propagate(cl, L);
}


/************************************************************************/
/*	Compute all the products. Retain only the minimal ones		*/
/************************************************************************/

static term
weave(term antecedents, term p)
{
	int i;

	for (i= 1; GOAL(i, antecedents) != NULL; i++)
		p = product(p, NODE_LABEL(GOAL(i, antecedents)));

	minimise(&p);
	return p;
}


/************************************************************************/
/* Propagate changes to ATMS resulting from a new justification being	*/
/* added. The initial call to propagate should have L = empty		*/
/************************************************************************/

static void
propagate(term j, term L)
{
	update(weave(j, L), HEAD(j));
}


/************************************************************************/
/*		Prolog hooks to start ATMS propagation			*/
/************************************************************************/

static int
assume(term goal, term *frame)
{
	term assumption = check_arg(1, goal, frame, ANY, IN);
	term e = new_environment();
	term n = new_node(assumption);

	if (TYPE(assumption) != ATOM)
		fail("ATMS only works with propositional clauses");

	set_add(e, new_hash_entry(assumption, hash_table));
	NODE_LABEL(n) = e;
	return TRUE;
}


static int
atms(term goal, term *frame)
{
	term cl = check_arg(1, goal, frame, FN, IN);
	term j = new_justification(cl);

	propagate(j, empty);
	return TRUE;
}


/************************************************************************/
/*			Is there support for a belief?			*/
/************************************************************************/

static int
believe(term goal, term *frame)
{
	term belief = check_arg(1, goal, frame, ATOM, IN);

	if (NODE_LABEL(belief) != NULL && TYPE(NODE_LABEL(belief)) != SET)
		fail("This is not a belief known to the ATMS");

	return (NODE_LABEL(belief) != NULL && ! empty_set(NODE_LABEL(belief)));
}


/************************************************************************/
/*			Examine nodes in the ATMS network		*/
/************************************************************************/

static int
belief_bound(term goal, term *frame)
{
	term bel = check_arg(1, goal, frame, ATOM, IN);
	term consequents = check_arg(2, goal, frame, LIST, OUT);
	term justifications = check_arg(3, goal, frame, LIST, OUT);
	term label = check_arg(4, goal, frame, LIST, OUT);
	term j = _nil, L =  _nil;
	term p, *q;

	for (p = JUSTIFICATIONS(bel), q = &j; p != NULL; p = NEXT(p))
	{
		*q = hcons(p, _nil);
		q = &CDR(*q);
	}

	for (p = NODE_LABEL(bel), q = &L; p != NULL; p = NEXT(p))
	{
		*q = hcons(p, _nil);
		q = &CDR(*q);
	}
	
	return	unify(consequents, frame, CONSEQUENTS(bel), frame) &&
		unify(justifications, frame, j, frame) &&
		unify(label, frame, L, frame);
}


static int
belief_unbound(term goal, term *frame)
{
	term bel = check_arg(1, goal, frame, ATOM, OUT);
	term consequents = check_arg(2, goal, frame, LIST, OUT);
	term justifications = check_arg(3, goal, frame, LIST, OUT);
	term label = check_arg(4, goal, frame, LIST, OUT);
	int i;

	for (i = 0; i != MAX_NODES; i++)
	{
		term x = index[i];

		if (x != NULL)
		{
			term *old_global = global;
			term j = _nil, L =  _nil;
			term p, *q;

			for (p = JUSTIFICATIONS(x), q = &j; p != NULL; p = NEXT(p))
			{
				*q = hcons(p, _nil);
				q = &CDR(*q);
			}

			for (p = NODE_LABEL(x), q = &L; p != NULL; p = NEXT(p))
			{
				*q = hcons(p, _nil);
				q = &CDR(*q);
			}
			
			if (unify(bel, frame, x, frame)
			&&  unify(consequents, frame, CONSEQUENTS(x), frame)
			&&  unify(justifications, frame, j, frame)
			&&  unify(label, frame, L, frame))
			{
				if (rest_of_clause())
					break;
				_untrail();
			}
			global = old_global;
		}
	}
	return FALSE;
}


static int
belief(term goal, term *frame)
{
	term bel = check_arg(1, goal, frame, ATOM, OUT);

	if (TYPE(bel) == ATOM)
		return belief_bound(goal, frame);
	else
		return belief_unbound(goal, frame);
}


/************************************************************************/
/*			    Initialise ATMS				*/
/************************************************************************/

void
atms_init(void)
{
	int i;

	for (i = 0; i != MAX_NODES; i++)
		index[i] = NULL;

	hash_table = new_hash_table(MAX_NODES);
	empty = new_environment();

	new_pred(atms,		"atms");
	new_pred(assume,	"assume");
	new_pred(believe,	"believe");
	new_pred(belief,	"belief");
}

