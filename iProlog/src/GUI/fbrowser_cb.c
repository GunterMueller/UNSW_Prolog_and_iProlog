#include <setjmp.h>
#include "prolog.h"
#include "forms.h"
#include "generic.h"
#include "frame.h"
#include "xforms_io.h"
#include "string_io.h"
#include "instance_display.h"

#ifdef LINUX
#define EXAMPLES_DIR	"/home/claude/iprolog/examples"
#else
#define EXAMPLES_DIR	"/Users/claude/Projects/iProlog/examples"
#endif

static jmp_buf ret_env;

static FD_generic_frame *fd_generic_frame = NULL;

term current_frame, current_slot, current_facet;
term current_instance, current_instance_slot;


/************************************************************************/
/*	callbacks and freeobj handles for form frame_browser		*/
/************************************************************************/

void
list_instances(void)
{
	term p;

	fl_clear_menu(fd_generic_frame -> instances_menu);

	fl_addto_menu(fd_generic_frame -> instances_menu, "New%l");

	for (p = PROC(current_frame); p != NULL; p = NEXT(p))
		fl_addto_menu(fd_generic_frame -> instances_menu, NAME(ARG(1, HEAD(p))));
}


void
generic_frame_list_cb(FL_OBJECT *ob, long data)
{
	int line = fl_get_browser(ob);
	const char *frame_name = fl_get_browser_line(ob, line);
	term p;

	current_frame = intern((char *) frame_name);
	current_slot = NULL;

	fl_clear_browser(fd_generic_frame -> generic_slot_list);
	fl_clear_browser(fd_generic_frame -> generic_facet_list);
	fl_set_input(fd_generic_frame -> daemon, "");

	fl_set_input(fd_generic_frame -> generic_name, NAME(current_frame));

	fl_set_input(fd_generic_frame -> generic_inherits, "");
	xprint(fd_generic_frame -> generic_inherits, INHERITS(current_frame));

	for (p = PLIST(current_frame); p != _nil; p = CDR(p))
		fl_addto_browser(fd_generic_frame -> generic_slot_list, NAME(SLOT_NAME(CAR(p))));

	list_instances();
}

void new_generic_button_cb(FL_OBJECT *ob, long data)
{
	const char *frame_name = fl_show_input("What is the name of the new generic frame?", "");
	current_frame = intern((char *) frame_name);

	if (FLAGS(current_frame) & GENERIC)
	{
		fl_show_alert("Alert", "A generic frame with the same name already exists", "", 1);
		return;
	}

	if (PROC(current_frame) != NULL)
	{
		fl_show_alert("Alert", "That name is already being used", "", 1);
		return;
	}

	FLAGS(current_frame) |= GENERIC;
	fl_addto_browser(fd_generic_frame -> generic_frame_list, frame_name);
	fl_clear_browser(fd_generic_frame -> generic_slot_list);
	fl_clear_browser(fd_generic_frame -> generic_facet_list);
	fl_set_input(fd_generic_frame -> daemon, "");
	fl_set_input(fd_generic_frame -> generic_name, frame_name);
	fl_set_input(fd_generic_frame -> generic_inherits, "[]");
	fl_show_form(fd_generic_frame -> generic_frame, FL_PLACE_CENTER, FL_FULLBORDER, "Generic Frame");
}

void
display_generic_button_cb(FL_OBJECT *ob, long data)
{
	extern term hashtable[];
	int i;
	term p;

	fl_clear_browser(fd_generic_frame -> generic_frame_list);

	for (i = 0; i < HASHSIZE; i++)
		for (p = hashtable[i]; p != 0; p = LINK(p))
			if (PLIST(p) != _nil && IS_GENERIC(p))
				fl_addto_browser(fd_generic_frame -> generic_frame_list, NAME(p));
}

static term last_file = NULL;

void
open_button_cb(FL_OBJECT *ob, long data)
{
	const char *file_name;

	file_name =	fl_show_fselector(
			       "Frame File",
			       EXAMPLES_DIR,
			       "*.plist",
			       "appt.plist"
			);

	if (file_name != NULL)
	{
		last_file = intern((char *) file_name);
		read_file(last_file);
		display_generic_button_cb(ob, data);
	}
}

void
save_button_cb(FL_OBJECT *ob, long data)
{
	const char *file_name;
	FILE *old_output = output;

	if (last_file == NULL)
		file_name = fl_show_input("Save as ...", "");
	else
		file_name = fl_show_input("Save as ...", NAME(last_file));

	if (file_name == NULL)
		return;

	if ((output = fopen(file_name, "w")) != NULL)
	{
		list_frames();
		fclose(output);
		last_file = intern((char *) file_name);
	}
	else
		fl_show_alert("Alert", "Couldn't open file for writing", "", 1);

	output = old_output;
}

void
quit_button_cb(FL_OBJECT *ob, long data)
{
	if (fl_form_is_visible(fd_generic_frame -> generic_frame))
		fl_hide_form(fd_generic_frame -> generic_frame);

	longjmp(ret_env, 1);
}



/************************************************************************/
/*	    callbacks and freeobj handles for form generic_frame	*/
/************************************************************************/

static void
print_conj(term daemon)
{
	FILE *old_output = output;

	output = xf_open(fd_generic_frame -> daemon);

	while (TYPE(daemon) == FN && ARG(0, daemon) == _comma)
	{
		prin(ARG(1, daemon));
		fputs(",\n", output);
		daemon = ARG(2, daemon);
	}

	print(daemon);

	fclose(output);
	output = old_output;
}

void
generic_facet_list_cb(FL_OBJECT *ob, long data)
{
	extern int display;
	int line = fl_get_browser(ob);
	const char *facet_name = fl_get_browser_line(ob, line);
	term daemon;

	if (current_slot == NULL)
		return;

	current_facet = intern((char *) facet_name);
	daemon = get_facet(current_frame, current_slot, current_facet);
	fl_set_input(fd_generic_frame -> daemon, "");

	if (daemon != NULL)
	{
		display = TRUE;
		print_conj(daemon);
	}
}

void
new_generic_slot_button_cb(FL_OBJECT *ob, long data)
{
	const char *slot_name = fl_show_input("What is the name of the new slot?", "");

	if (slot_name == NULL)
		return;

	current_slot = intern((char *) slot_name);
	current_facet = NULL;

	fl_addto_browser(fd_generic_frame -> generic_slot_list, slot_name);
	fl_clear_browser(fd_generic_frame -> generic_facet_list);
	fl_set_input(fd_generic_frame -> daemon, "");
}

void
daemon_cb(FL_OBJECT *ob, long data)
{
	/* fill-in code for callback */
}

void
generic_accept_button_cb(FL_OBJECT *ob, long data)
{
	term daemon = xread_term(fd_generic_frame -> daemon);

	if (daemon == NULL)
		return;
	put_facet(current_frame, current_slot, current_facet, make(daemon, NULL));
}

void
facet_menu_cb(FL_OBJECT *ob, long data)
{
	int item = fl_get_menu(ob);
	const char *facet_name;

	if (current_slot == NULL)
		return;

	facet_name = fl_get_menu_item_text(ob, item);
	current_facet = intern((char *) facet_name);
	if (! get_facet(current_frame, current_slot, current_facet))
	{
		fl_addto_browser(fd_generic_frame -> generic_facet_list, facet_name);

		switch (item)
		{
			case 1: /* range */
				put_facet(current_frame, current_slot, current_facet, intern("atom"));
				break;
			case 2: /* help */
				put_facet(current_frame, current_slot, current_facet, h_fn1(intern("message"), intern("Incorrect entry")));
				break;
			case 3: /* multivalued */
				put_facet(current_frame, current_slot, current_facet, _true);
				break;
			case 4: /* cache */
				put_facet(current_frame, current_slot, current_facet, _true);
				break;
			case 5: /* default */
				break;
			case 6: /* if new */
				put_facet(current_frame, current_slot, current_facet, intern("ask_user"));
				break;
			case 7: /* if needed */
				put_facet(current_frame, current_slot, current_facet, intern("ask_user"));
				break;
		}
	}
}

void
generic_slot_list_cb(FL_OBJECT *ob, long data)
{
	int line = fl_get_browser(ob);
	const char *slot_name = fl_get_browser_line(ob, line);
	term slot = getprop(current_frame, intern((char *) slot_name));
	term p;

	fl_clear_browser(fd_generic_frame -> generic_facet_list);
	fl_set_input(fd_generic_frame -> daemon, "");

	if (slot == NULL)
		return;

	current_slot = SLOT_NAME(slot);

	for (p = VALUE(slot); p != _nil; p = CDR(p))
		fl_addto_browser(fd_generic_frame -> generic_facet_list, NAME(FACET(CAR(p))));
}

void generic_name_cb(FL_OBJECT *ob, long data)
{
  /* fill-in code for callback */
}

void generic_remove_slot_button_cb(FL_OBJECT *ob, long data)
{
	extern int delete_slot();
	char buf[256];

	if (current_slot == NULL)
		return;

	sprintf(buf, "Are you sure you want to remove the slot \"%s\"?", NAME(current_slot));
	if (fl_show_question(buf, 1))
	{
		int line = fl_get_browser(fd_generic_frame -> generic_slot_list);

		delete_slot(current_frame, current_slot);

		fl_delete_browser_line(fd_generic_frame -> generic_slot_list, line);
		fl_clear_browser(fd_generic_frame -> generic_facet_list);
		fl_set_input(fd_generic_frame -> daemon, "");
	}
}

void generic_remove_facet_button_cb(FL_OBJECT *ob, long data)
{
	extern int delete_facet();
	char buf[256];

	if (current_slot == NULL || current_facet == NULL )
		return;

	sprintf(buf, "Are you sure you want to remove the facet \"%s\"?", NAME(current_facet));
	if (fl_show_question(buf, 1))
	{
		int line = fl_get_browser(fd_generic_frame -> generic_facet_list);

		delete_facet(current_frame, current_slot, current_facet);

		fl_delete_browser_line(fd_generic_frame -> generic_facet_list, line);
		fl_set_input(fd_generic_frame -> daemon, "");
	}
}

void generic_inherits_cb(FL_OBJECT *ob, long data)
{
	term inherits = xread(fd_generic_frame -> generic_inherits);

	INHERITS(current_frame) = make(inherits, NULL);
}

void generic_delete_button_cb(FL_OBJECT *ob, long data)
{
	extern int delete_frame();
	char buf[256];

	sprintf(buf, "Are you sure you want to remove the generic frame \"%s\", and all its instances?", NAME(current_frame));
	if (! fl_show_question(buf, 1))
		return;

	delete_frame(current_frame);
	fl_hide_form(fd_generic_frame -> generic_frame);
	display_generic_button_cb(NULL, 0);
}

void
instances_menu_cb(FL_OBJECT *ob, long data)
{
	int item = fl_get_menu(ob);
	const char *frame_name;

	if (item == 1)
	{
		frame_name = fl_show_input("What is the name of the new object?", "");
		current_instance = intern((char *) frame_name);
		make_instance(current_instance, gcons(current_frame, _nil), _nil, NULL);
		list_instances();
	}
	else
	{
		frame_name = fl_get_menu_item_text(ob, item);
		current_instance = intern((char *) frame_name);
	}

	display_instance(current_instance);
}


/************************************************************************/
/* 			     Ask user for input				*/
/************************************************************************/

extern term (*qa)(term, term, term *);

static term
x_q_and_a(term question, term def, term *frame)
{
	char *s, *q_buf, *d_buf;

	q_buf = message_to_string(question, frame);

	d_buf = (def != NULL) ? print_to_string(def) : "";

	s = (char *) fl_show_input(q_buf, d_buf);

	free(q_buf);
	if (*d_buf != '\0')
		free(d_buf);

	if (s == NULL)
		return NULL;

	return read_term_from_string(s);
}


/************************************************************************/
/*			Handle window close events			*/
/************************************************************************/

static int
close_browser(FL_FORM *form, void *data)
{
	qa = NULL;
	quit_button_cb(NULL, 0);
	return FL_OK;
}


static int
close_frame(FL_FORM *form, void *data)
{
	qa = NULL;
	fl_hide_form(form);
	return FL_OK;
}

/************************************************************************/
/*			 Module Initialisation				*/
/************************************************************************/

static int
p_frame_browser(void)
{
	if (fd_generic_frame == NULL)
	{
		fd_generic_frame = create_form_generic_frame();
		fl_set_form_atclose(fd_generic_frame -> generic_frame, close_browser, 0);
	}

	display_generic_button_cb(NULL, 0);
	fl_show_form(fd_generic_frame -> generic_frame, FL_PLACE_CENTER, FL_FULLBORDER, "Frame Browser");

	if (setjmp(ret_env))
	{
		qa = NULL;
		return TRUE;
	}

	qa = x_q_and_a;
	fl_do_forms();

	return FALSE;
}

int
frame_browser_running(void)
{
	return (fd_generic_frame != NULL && fl_form_is_visible(fd_generic_frame -> generic_frame));
}

void
frame_browser_init(void)
{
	new_pred(p_frame_browser, "frame_browser");
}
