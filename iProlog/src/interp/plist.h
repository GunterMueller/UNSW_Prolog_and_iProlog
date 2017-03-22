/* plist.c */

#define PROPERTY(x)	ARG(0, x)
#define VALUE(x)	ARG(1, x)

term getprop(term, term);
term getpropval(term, term);
term putprop(term, term, term);
int remprop(term, term);
term build_plist(char *, ...);
term build_plist_named(char *, ...);
