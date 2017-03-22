#include "forms.h"
#include "fbrowser.h"
#include "prolog.h"

#define SLOT_NAME(x)	ARG(0, x)
#define VALUE(x)	ARG(1, x)

#define FACET(x)	ARG(0, x)
#define DAEMON(x)	ARG(1, x)

term get_slot(term, term);
term get_facet(term, term, term);
void print_to_string(term, term *, char *, size_t);

static FD_frame_browser *fd_frame_browser;
static FD_generic_frame *fd_generic_frame;
static FD_instance_frame *fd_instance_frame;


void
browse_generic(void)
{
	extern term hashtable[];
	int i;
	term p;

	fl_clear_browser(fd_frame_browser -> generic_frame_list);

	for (i = 0; i < HASHSIZE; i++)
		for (p = hashtable[i]; p != 0; p = LINK(p))
			if (PLIST(p) != _nil && IS_GENERIC(p))
				fl_addto_browser(fd_frame_browser -> generic_frame_list, NAME(p));
}

static void
list_instances(term obj)
{
	term p;

	fl_clear_menu(fd_generic_frame -> instances_menu);

	for (p = PROC(obj); p != NULL; p = NEXT(p))
		fl_addto_menu(fd_generic_frame -> instances_menu, NAME(ARG(1, HEAD(p))));
}


void
browse_slots(term obj)
{
	term p;
	char buf[256];

	fl_clear_browser(fd_generic_frame -> generic_slot_list);
	fl_clear_browser(fd_generic_frame -> generic_facet_list);
	fl_set_input(fd_generic_frame -> daemon, "");

	fl_set_input(fd_generic_frame -> generic_name, NAME(obj));

	print_to_string(INHERITS(obj), NULL, buf, 256);
	fl_set_input(fd_generic_frame -> generic_inherits, buf);
	
	for (p = PLIST(obj); p != _nil; p = CDR(p))
		fl_addto_browser(fd_generic_frame -> generic_slot_list, NAME(SLOT_NAME(CAR(p))));

	list_instances(obj);
	fl_show_form(fd_generic_frame -> generic_frame, FL_PLACE_CENTERFREE, FL_FULLBORDER, "generic_frame");
}

void
browse_facets(term obj, term prop)
{
	term p;
	term slot = get_slot(obj, prop);

	fl_clear_browser(fd_generic_frame -> generic_facet_list);
	fl_set_input(fd_generic_frame -> daemon, "");

	for (p = VALUE(slot); p != _nil; p = CDR(p))
		fl_addto_browser(fd_generic_frame -> generic_facet_list, NAME(FACET(CAR(p))));
}


void
browse_daemon(term obj, term prop, term facet)
{
	extern int display;
	term daemon = get_facet(obj, prop, facet);
	char buf[1024];

	if (daemon == NULL)
		fl_set_input(fd_generic_frame -> daemon, "");
	else
	{
		display = TRUE;
		print_to_string(daemon, NULL, buf, 1024);
		fl_set_input(fd_generic_frame -> daemon, buf);
	}
}


void
browse_instance(term obj)
{
	term p;
	char buf[256];

	fl_clear_browser(fd_instance_frame -> instance_slot_list);
	fl_set_input(fd_instance_frame -> value, "");

	fl_set_input(fd_instance_frame -> instance_name, NAME(obj));

	print_to_string(INHERITS(obj), NULL, buf, 256);
	fl_set_input(fd_instance_frame -> instance_inherits, buf);

	for (p = PLIST(obj); p != _nil; p = CDR(p))
		fl_addto_browser(fd_instance_frame -> instance_slot_list, NAME(SLOT_NAME(CAR(p))));

	fl_show_form(fd_instance_frame -> instance_frame, FL_PLACE_CENTERFREE, FL_FULLBORDER, "instance_frame");
}

void
browse_values(term obj, term prop)
{
	extern int display;
	char buf[1024];
	term slot = get_slot(obj, prop);

	fl_set_input(fd_instance_frame -> value, "");

	display = TRUE;
	print_to_string(VALUE(slot), NULL, buf, 1024);
	fl_set_input(fd_instance_frame -> value, buf);
}


static int
new_frame_browser(term goal, term *frame)
{
	fd_frame_browser = create_form_frame_browser();
	fd_generic_frame = create_form_generic_frame();
	fd_instance_frame = create_form_instance_frame();
	
	/* fill-in form initialization code */

	/* show the first form */
	fl_show_form(fd_frame_browser -> frame_browser, FL_PLACE_CENTERFREE, FL_FULLBORDER, "frame_browser");
	fl_do_forms();
	return TRUE;
}


void
frame_browser_init(void)
{
	new_pred(new_frame_browser, "frame_browser");
}