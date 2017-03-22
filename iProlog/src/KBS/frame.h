/* frame.c */

#define SLOT_NAME(x)	ARG(0, x)
#define VALUE(x)	ARG(1, x)

#define FACET(x)	ARG(0, x)
#define DAEMON(x)	ARG(1, x)

term get_facet(term object, term slot, term facet);
void put_facet(term obj, term prop, term facet, term daemon);
int check_range(term val);
term fget(term obj, term prop);
int fremove(term obj, term prop);
int freplace(term obj, term prop, term val, term *frame);
void make_instance(term obj, term inherits, term slots, term *frame);
int delete_frame(term obj);
int delete_slot(term obj, term prop);
int delete_facet(term obj, term prop, term facet);
int list_frames(void);
void print_frame(term x);
void frame_init(void);
