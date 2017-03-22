/* Form definition file generated with fdesign. */

#include <stdlib.h>
#include "prolog.h"
#include "forms.h"

term getprop(term, term);
term freplace(term, term, term);
int fremove(term, term);
term fget(term, term);
int list_properties(void);
void xprint(FL_OBJECT *, term);
term xread(FL_OBJECT *);
term xread_term(FL_OBJECT *);
term xread_selection(FL_OBJECT *);

void display_instance(term);

extern int display;


typedef struct
{
	FL_FORM *instance_display;
	void *vdata;
	char *cdata;
	long  ldata;
	term instance;
	FL_OBJECT *frame_menu;
	FL_OBJECT *new_slot_menu;
	FL_OBJECT *frame_id;
	FL_OBJECT *isa;
	FL_OBJECT **slot;
} FD_instance_display;


/************************************************************************/
/* 		   Do whatever has to be done at the end		*/
/************************************************************************/

static int
atclose(FL_FORM *form, void *dummy)
{
	FD_instance_display *fdui = form -> fdui;

	fl_hide_form(form);
	fl_free(fdui -> slot);
	fl_free(fdui);
	fl_free_form(form);

	return FL_OK;
}


/************************************************************************/
/* 		   		Call backs				*/
/************************************************************************/

static void delete_instance(FD_instance_display *fdui)
{
	extern void list_instances();
	extern int delete_frame();
	char buf[256];

	sprintf(buf, "Are you sure you want to remove the instance \"%s\"?", NAME(fdui -> instance));
	if (! fl_show_question(buf, 1))
		return;

	delete_frame(fdui -> instance);
	atclose(fdui -> instance_display, NULL);
	list_instances();
}

static void
frame_menu_cb(FL_OBJECT *ob, long data)
{
	FD_instance_display *fdui = (FD_instance_display *) data;
	int item = fl_get_menu(ob);

	switch (item)
	{
		case 1: /* delete frame */
			delete_instance(fdui);
			break;
	}
}

static void
new_slot_cb(FL_OBJECT *ob, long data)
{
	FD_instance_display *fdui = (FD_instance_display *) data;
	term instance = fdui -> instance;
	int item = fl_get_menu(ob);
	term slot, val;

	slot = intern((char *) fl_get_menu_item_text(ob, item));
	if ((val = fget(instance, slot)) != NULL)
	{
		atclose(fdui -> instance_display, NULL);
		display_instance(instance);
	}
}

static void
frame_id_cb(FL_OBJECT *ob, long data)
{
  /* fill-in code for callback */
}

static void
isa_cb(FL_OBJECT *ob, long data)
{
	FD_instance_display *fdui = (FD_instance_display *) data;
	term inherits = xread_term(fdui -> isa);

	INHERITS(fdui -> instance) = make(inherits, NULL);
}

static void
slot_cb(FL_OBJECT *obj, long data)
{
	FD_instance_display *fdui = (FD_instance_display *) data;
	term instance = fdui -> instance;
	term slot = intern(obj -> label);
	term val = xread_term(obj);

	if (val == _end_of_file)
	{
		fremove(instance, slot);
		atclose(fdui -> instance_display, NULL);
		display_instance(instance);
	}
	else if (! freplace(instance, slot, val))
	{
		fl_set_input(obj, "");
		xprint(obj, CDR(getprop(instance, slot)));
	}
}

static void
slot_expand_cb(FL_OBJECT *ob, long data)
{
	term val;

	val = xread_selection(ob);
	if (val == NULL || TYPE(val) != ATOM)
		return;

	display_instance(val);
}


/************************************************************************/
/* 		   		Menu Declarations			*/
/************************************************************************/


static FL_PUP_ENTRY fdmenu_frame_menu_0[] =
{ 
	/*  itemtext   callback  shortcut   mode */
	{ "Delete Frame",	0,	"",	 FL_PUP_NONE},
	{0}
};


/************************************************************************/
/*	Put all the names of all inherited slots into slot menu		*/
/************************************************************************/

static void
list_slots(FL_OBJECT *menu, term instance, term inherits)
{
	for (; inherits != _nil; inherits = CDR(inherits))
	{
		term obj = CAR(inherits);
		term p;

		list_slots(menu, instance, INHERITS(obj));

		for (p = PLIST(obj); p != _nil; p = CDR(p))
		{
			term slot_name = CAR(CAR(p));
			if (getprop(instance, slot_name) == NULL)
				fl_addto_menu(menu, NAME(slot_name));
		}
	}
}


/************************************************************************/
/* 		   	Create Instance Frame Display			*/
/************************************************************************/

static FD_instance_display *
create_form_instance_display(term instance)
{
	term p;
	int i, n_slots = 0;
	FL_OBJECT *obj;
	FD_instance_display *fdui = (FD_instance_display *) fl_calloc(1, sizeof(*fdui));

	for (p = PLIST(instance); p != _nil; p = CDR(p))
		n_slots++;
	
	fdui -> instance = instance;
	fdui -> slot = fl_calloc(n_slots, sizeof (FL_OBJECT *));

	fdui -> instance_display = fl_bgn_form(FL_NO_BOX, 400, 130 +n_slots*40);

	obj = fl_add_box(FL_FLAT_BOX, 0 , 0, 400, 130+n_slots*40,"");
	obj = fl_add_box(FL_UP_BOX, 0, 0, 400, 30, "");
	obj = fl_add_box(FL_FRAME_BOX, 0, 30, 400, 90, "");

	fdui -> frame_menu = obj = fl_add_menu(FL_PULLDOWN_MENU, 5, 5, 60, 20, "Frame");
	fl_set_object_callback(obj, frame_menu_cb, (long) fdui);
	fl_set_menu_entries(obj, fdmenu_frame_menu_0);

	fdui -> new_slot_menu = obj = fl_add_menu(FL_PULLDOWN_MENU, 60, 5, 60, 20, "New Slot");
	fl_set_object_callback(obj, new_slot_cb, (long) fdui);
	list_slots(obj, instance, INHERITS(instance));

	fdui -> frame_id = obj = fl_add_input(FL_NORMAL_INPUT, 110, 40, 270, 30, "Frame Id");
	fl_set_object_color(obj, FL_WHITE, FL_WHITE);
	fl_set_object_lsize(obj, FL_NORMAL_SIZE);
	fl_set_object_callback(obj, frame_id_cb, (long) fdui);
	fl_set_input(obj, NAME(instance));

	fdui -> isa = obj = fl_add_input(FL_NORMAL_INPUT, 110, 80, 270, 30, "Isa");
	fl_set_object_color(obj, FL_WHITE, FL_WHITE);
	fl_set_object_lsize(obj, FL_NORMAL_SIZE);
	fl_set_object_callback(obj, isa_cb, (long) fdui);
	xprint(obj, INHERITS(instance));

	for (i = 0, p = PLIST(instance); p != _nil; p = CDR(p), i++)
	{
		term slot_name = CAR(CAR(p));
		term val = CDR(CAR(p));

		fdui -> slot[i] = obj = fl_add_input(FL_NORMAL_INPUT, 110, 130+i*40, 270, 30, NAME(slot_name));
		fl_set_object_color(obj, FL_WHITE, FL_WHITE);
		fl_set_object_lsize(obj, FL_NORMAL_SIZE);
		if (TYPE(val) == ATOM && PLIST(val) != _nil)
		{
			fl_set_object_callback(obj, slot_expand_cb, (long) fdui);
			fl_set_input_return(obj, FL_RETURN_END);
		}
		else
			fl_set_object_callback(obj, slot_cb, (long) fdui);
		display = TRUE;
		xprint(obj, CDR(CAR(p)));
	}

	fl_end_form();

	fl_set_form_atclose(fdui -> instance_display, atclose, NULL);

	fdui->instance_display->fdui = fdui;

	return fdui;
}


/************************************************************************/
/* 		   	Create Instance Frame Display			*/
/************************************************************************/

void
display_instance(term instance)
{
	FD_instance_display *inst;

	inst = create_form_instance_display(instance);

	fl_show_form(
		inst -> instance_display,
		FL_PLACE_FREE,
		FL_FULLBORDER,
		NAME(instance)
	);
}


/************************************************************************/
/* 		   	    Prolog Predicates				*/
/************************************************************************/

static int
p_display_instance(term goal, term *frame)
{
	term instance = check_arg(1, goal, frame, ATOM, IN);

	display_instance(instance);
	fl_do_forms();

	return TRUE;
}


/************************************************************************/
/* 		   	Module Initialisation				*/
/************************************************************************/

void
instance_init(void)
{
	new_pred(p_display_instance, "display_instance");
}
