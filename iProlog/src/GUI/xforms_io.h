/* xforms_io.c */

FILE *xf_open(FL_OBJECT *input_field);
FILE *xf_open_selection(FL_OBJECT *input_field);
FILE *xf_open_browser(FL_OBJECT *input_field);
term xread(FL_OBJECT *input_field);
term xread_term(FL_OBJECT *input_field);
term xread_selection(FL_OBJECT *input_field);
void xprint(FL_OBJECT *input_field, term t);
void xprint_browser(FL_OBJECT *input_field, term t);
