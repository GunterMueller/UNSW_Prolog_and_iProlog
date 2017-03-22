/* make.c */

term new_var(int, int, term);
term new_ref(void);
term new_clause(int);
term new_unit(term);
term new_chunk(chunk_spec *, void *);
term gcons(term, term);
term hcons(term, term);
term new_h_fn(int);
term new_g_fn(int);
term h_fn1(term, term);
term g_fn1(term, term);
term h_fn2(term, term, term);
term g_fn2(term, term, term);
term mkclause(term, term *);
term add_clause(term, int);
term make(term, term *);
void check_last_clause(term);
int count_vars(term);
term cache_predicate(term);
