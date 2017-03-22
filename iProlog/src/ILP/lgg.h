/* lgg.c */

int count_neg_cover(term, term);
int covers_neg(term c, term neglist);
int count_cover(term c, term clause_list);
int mark_covered(term c, term clause_list);
int match_head(term h1, term *f1, term h2, term *f2);
term lgg_clause(term c1, term c2);
