#include "prolog.h"

#define COPIED(p)		(FLAGS(p) & COPY)

extern term trail;


term
unbind(term t, term *frame)
{
	repeat
	{
		switch (TYPE(t))
		{
		   case ANON:
		   case FREE:
				fail("Tried to unbind a free variable");
		   case BOUND:
				t = frame[OFFSET(t)];
				break;
		   case REF:
				if ((t = POINTER(t)) == NULL)
					fail("Tried to dereference an unbound variable");
				break;
		   default:
				return(t);
		}
	}
}


void
make_ref(term t, term *frame)
{
	term rval;

	switch (TYPE(t))
	{
	   case FREE:
	   		frame[OFFSET(t)] = rval = galloc(sizeof(ref));
	   		TYPE(rval) = REF;
			FLAGS(rval) = COPY;
			POINTER(rval) = NULL;
			return;
	   case LIST:
	   case FN:
			if (! COPIED(t))
			{
				int i, a = ARITY(t);

				for (i = 0; i <= a; i++)
					make_ref(ARG(i, t), frame);

				return;
			}
	   default:
			return;
	}
}


term
copy(term t, term *frame)
{
	term rval;

	switch (TYPE(t))
	{
	   case ANON:
	   		rval = galloc(sizeof(ref));
	   		TYPE(rval) = REF;
			FLAGS(rval) = COPY;
			POINTER(rval) = NULL;
			return(rval);
	   case FREE:
	   		frame[OFFSET(t)] = rval = galloc(sizeof(ref));
	   		TYPE(rval) = REF;
			FLAGS(rval) = COPY;
			POINTER(rval) = NULL;
			return(rval);
	   case BOUND:
	   		return(frame[OFFSET(t)]);
	   case INT:
			rval =  galloc(sizeof(integer));
			TYPE(rval) = INT;
			FLAGS(rval) = COPY;
			IVAL(rval) = IVAL(t);
			return(rval);
	   case REAL:
			rval =  galloc(sizeof(real));
			TYPE(rval) = REAL;
			FLAGS(rval) = COPY;
			RVAL(rval) = RVAL(t);
			return(rval);
	   case LIST:
	   case FN:
			if (! COPIED(t))
			{
				int i, a = ARITY(t);

		   		rval = galloc(sizeof(compterm) + a * WORD_LENGTH);
				TYPE(rval) = TYPE(t);
				FLAGS(rval) = COPY;
				ARITY(rval) = a;

				for (i = 0; i <= a; i++)
					ARG(i, rval) = copy(ARG(i, t), frame);

				return(rval);
			}
	   default:
			return(t);
	}
}


int
unify(term t1, term *f1, term t2, term *f2)
{
L2:

#ifdef DEBUG
	fprintf(output, "======= UNIFY ========\n");
	rprint(t1, f1);
	rprint(t2, f2);
	fflush(output);
#endif

	switch (TYPE(t2))
	{
	   case ANON:
/*
 * 	   		if (TYPE(t1) == FREE)
 * 				f1[OFFSET(t1)] = copy(t2, f2);
 */
 			make_ref(t1, f1);
			return(TRUE);
	   case FREE:
			f2[OFFSET(t2)] = copy(t1, f1);
			return(TRUE);
	   case BOUND:
			t2 = f2[OFFSET(t2)];
			goto L2;
	   case REF:
			if (POINTER(t2) == NULL)
			{
				POINTER(t2) = copy(t1, f1);
				TRAIL(t2) = trail;
				trail = t2;
				return(TRUE);
			}
			t2 = POINTER(t2);
			goto L2;
	}

L1:	if (t1 == t2) return(TRUE);

	switch (TYPE(t1))
	{
	   case ANON:
/*
 * 	   		if (TYPE(t2) == FREE)
 * 				f2[OFFSET(t2)] = copy(t1, f1);
 */
 			make_ref(t2, f2);
			return(TRUE);
	   case FREE:
			f1[OFFSET(t1)] = copy(t2, f2);
			return(TRUE);
	   case BOUND:
			t1 = f1[OFFSET(t1)];
			goto L1;
	   case REF:
			if (POINTER(t1) == NULL)
			{
				POINTER(t1) = copy(t2, f2);
				TRAIL(t1) = trail;
				trail = t1;
				return(TRUE);
			}
			t1 = POINTER(t1);
			goto L1;
	   case ATOM:
			if (TYPE(t2) == ATOM)
				return(t1 == t2);
			return(FALSE);
	   case INT:
			if (TYPE(t2) == INT)
				return(IVAL(t1) == IVAL(t2));
			if (TYPE(t2) == REAL)
				return((double) IVAL(t1) == RVAL(t2));
			return(FALSE);
	   case REAL:
			if (TYPE(t2) == REAL)
				return(RVAL(t1) == RVAL(t2));
			if (TYPE(t2) == INT)
				return(RVAL(t1) == (double) IVAL(t2));
			return(FALSE);
	   case FN:
			if (TYPE(t2) == FN && ARITY(t1) == ARITY(t2))
			{
				int i;

				for (i = 0; i <= ARITY(t1); i++)
					if (! unify(ARG(i, t1), f1, ARG(i, t2), f2))
						return(FALSE);
				return(TRUE);
			}
			return(FALSE);
	   case LIST:
			if (TYPE(t2) == LIST && unify(CAR(t1), f1, CAR(t2), f2))
			{
				t1 = CDR(t1);
				t2 = CDR(t2);
				goto L2;
			}
			return(FALSE);
	   default:
			return(FALSE);
	}
}
