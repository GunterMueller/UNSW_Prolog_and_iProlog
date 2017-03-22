/* rdr.c */

term new_rule(term cond, term concl, term because, term except, term alt);
void add_rdr(term rule);
int yes_no(term x, char *question);
void add_condition(term cond);
void make_cond(term var, term old, term new);
term make_conj(int n_selected);
void fix_rdr(int (*rdr_interaction)(), term new_case, term *frame);

extern term _rdr, _if, _then, _else, _except, _because, _and, _or;
extern term _gt, _lt, _eq, _ne, _dont_know;

extern int  was_true;
extern term anon_struct;
extern term last_rule;
extern term last_case;

extern term condition_list[];
extern term selected_conds[];
extern int n_conditions;
