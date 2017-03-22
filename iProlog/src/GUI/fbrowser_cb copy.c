#include "forms.h"
#include "fbrowser.h"
#include "prolog.h"

void browse_generic(void);
void browse_slots(term);
void browse_facets(term, term);

term current_frame, current_slot, current_facet;

/* callbacks and freeobj handles for form fbrowser */

void frame_menu_cb(FL_OBJECT *ob, long data)
{
	int item = fl_get_menu(ob);

	switch (item)
	{
		case 1:	/* Generic Frames */
			browse_generic();
			break;
	}
}

void slots_menu_cb(FL_OBJECT *ob, long data)
{
  /* fill-in code for callback */
}

void frame_cb(FL_OBJECT *ob, long data)
{
	int line = fl_get_browser(ob);
	char *buf = fl_get_browser_line(ob, line);
	current_frame = intern(buf);
	browse_slots(current_frame);
}

void slot_cb(FL_OBJECT *ob, long data)
{
	int line = fl_get_browser(ob);
	char *buf = fl_get_browser_line(ob, line);
	current_slot = intern(buf);
	browse_facets(current_frame, current_slot);
}

void facet_cb(FL_OBJECT *ob, long data)
{
	int line = fl_get_browser(ob);
	char *buf = fl_get_browser_line(ob, line);
	current_facet = intern(buf);
	browse_daemon(current_frame, current_slot, current_facet);
}

void inherits_cb(FL_OBJECT *ob, long data)
{
  /* fill-in code for callback */
}

void value_cb(FL_OBJECT *ob, long data)
{
  /* fill-in code for callback */
}

void file_menu_cb(FL_OBJECT *ob, long data)
{
 	int item = fl_get_menu(ob);

	switch (item)
	{
	   case 5:	/* Quit */
	   		exit(0);
	}
}


void facets_menu_cb(FL_OBJECT *ob, long data)
{
	/* fill-in code for callback */
}


void instance_menu_cb(FL_OBJECT *ob, long data)
{
	/* fill-in code for callback */
}



/* callbacks and freeobj handles for form request */
void request_cb(FL_OBJECT *ob, long data)
{
  /* fill-in code for callback */
}



