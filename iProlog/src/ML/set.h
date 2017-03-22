/* set.c */

term new_set(int, term);
term copy_set(term);
void clear_set(term);
void set_add(term, short);
int set_intersection(term, term, term);
int set_union(term, term, term);
int set_diff(term, term, term);
int null_set(term);
int set_eq(term, term);
int empty_set(term);
int set_contains(term, term);
int disj_contains(term, term);
int set_cardinality(term);
int intersection_size(term, term);
int distance(term, term);
void print_set(term);
void set_init(void);
void free_disj(term);
void print_disj(term);
