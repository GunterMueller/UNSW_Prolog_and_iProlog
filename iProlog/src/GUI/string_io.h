/* string_io.c */

FILE *str_ropen(void *buf);
FILE *str_wopen(char **buf);
term read_atom_from_string(char *str);
term read_term_from_string(char *str);
int probot_from_string(char *buf);
char *prin_to_string(term t);
char *print_to_string(term t);
char *message_to_string(term goal, term *frame);
