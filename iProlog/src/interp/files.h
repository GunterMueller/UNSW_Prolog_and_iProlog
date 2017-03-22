/* files.c */

void reset_io(void);
term add_stream(term, term, FILE *);
term get_stream(term a, term mode);
int close_stream(term strm);
void add_to_proc_list(term);
int read_file(term);
int input_file(char *);
