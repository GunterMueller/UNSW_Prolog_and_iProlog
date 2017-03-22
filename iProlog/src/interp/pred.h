/* pred.c */

term new_pred(int (*)(), char *);
term new_fpred(int (*)(), char *);
term new_subr(term (*)(), char *);
term new_fsubr(term (*)(), char *);
term check_arg(int, term, term *, int, int);
void check_arity(term, int);
