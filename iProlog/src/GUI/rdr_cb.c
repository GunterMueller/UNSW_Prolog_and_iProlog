#include <setjmp.h>
#include "forms.h"
#include "rdr_GUI.h"
#include "prolog.h"
#include "rdr.h"
#include "rdr_plist.h"
#include "xforms_io.h"

#define EXCEPT(x)	ARG(4, x)
#define ALT(x)		ARG(5, x)

#define MAX_COND 20

extern term *global;

static FD_rdr *fd_rdr = NULL;
static jmp_buf ret_env;
static term *old_global;

static term rule_slot, new_case, old_case, conclusion, whole_rule;


/************************************************************************/
/*   Handle window close from close button or window manager close	*/
/************************************************************************/

void
close_button_cb(FL_OBJECT *ob, long data)
{
	fl_hide_form(fd_rdr -> rdr);

	longjmp(ret_env, 1);
}


static int
close_rdr_GUI(FL_FORM *ob, void *dummy)
{
	fl_hide_form(ob);

	longjmp(ret_env, 1);
	return FL_OK;
}


/************************************************************************/
/*		  Place a condition on condition browser		*/
/************************************************************************/

static void
show_conditions(void)
{
	int i;

	for (i = 0; i < n_conditions; i++)
		xprint_browser(fd_rdr -> conditions, condition_list[i]);	
}


/************************************************************************/
/*		   Put conclusions in choice pull-down			*/
/************************************************************************/

static void
show_conclusions(term inherits, term slot, term conclusion)
{
	extern term get_facet();
	int choice, i = 0;
	term p, conc_list;

	if (inherits == _nil || TYPE(inherits) != LIST)
		return;

	conc_list = get_facet(CAR(inherits), slot, intern("range"));
	whole_rule = get_facet(CAR(inherits), slot, intern("if_needed"));

	fl_clear_choice(fd_rdr -> conclusion);

	for (p = conc_list; p != _nil; p = CDR(p))
	{
		i++;
		fl_addto_choice(fd_rdr -> conclusion, NAME(CAR(p)));
		if (CAR(p) == conclusion)
			choice = i;
	}

	fl_set_choice(fd_rdr -> conclusion, choice);
}

/************************************************************************/
/*			Show a case is a browser			*/
/************************************************************************/

static void
show_case(FL_OBJECT *browser, term plist)
{
	term p;

	fl_clear_browser(browser);

	for (p = plist; p != _nil; p = CDR(p))
		if (CAR(CAR(p)) != rule_slot)
			xprint_browser(browser, CAR(p));
}


/************************************************************************/
/* Call back for conclusions pull-down					*/
/* Calls routine to construct difference list				*/
/* conclusions_cb is also called explicitly from rdr_GUI		*/
/************************************************************************/

void
conclusion_cb(FL_OBJECT *ob, long data)
{
	fl_clear_browser(fd_rdr -> conditions);
	n_conditions = 0;
	plist_diff(rule_slot, PLIST(old_case), PLIST(new_case));
	show_conditions();
}


/************************************************************************/
/* Call back for make rule button					*/
/* Finds selected conditions and constructs new rule			*/
/************************************************************************/

void
make_button_cb(FL_OBJECT *ob, long data)
{
	int i, n = 0;
	int total_lines = fl_get_browser_maxline(fd_rdr -> conditions);
	term conclusion = intern((char *) fl_get_choice_text(fd_rdr -> conclusion));

	for (i = 1; i <= total_lines; i++)
		if (fl_isselected_browser_line(fd_rdr -> conditions, i))
			selected_conds[n++] = condition_list[i-1];

	if (n == 0)
	{
		fl_show_alert("Alert", "You must select at least one condition", "", 1);
		return;
	}

	add_rdr(new_rule(
		 make(make_conj(n), NULL),
		 conclusion,
		 new_case,
		 _anon,
		 _anon
	));
	
	fl_set_input(fd_rdr -> new_rule, "");
	xprint(fd_rdr -> new_rule, whole_rule);

	global = old_global;
}


/************************************************************************/
/*			Invokde RDR maintenance GUI			*/
/************************************************************************/

static int
rdr_GUI(term goal, term *frame)
{
	char buf[256];

	if (fd_rdr == NULL)
	{
		fd_rdr = create_form_rdr();
		fl_set_form_atclose(fd_rdr -> rdr, close_rdr_GUI, 0);
	}
	else
	{
		fl_clear_browser(fd_rdr -> conditions);
		n_conditions = 0;
	}

	old_global = global;
	new_case = copy(check_arg(1, goal, frame, FN, IN), frame);
	conclusion = eval(new_case, frame);
	rule_slot = ARG(0, new_case);
	new_case = ARG(1, new_case);
	old_case = last_case;

	show_case(fd_rdr -> cornerstone_case, PLIST(old_case));
	show_case(fd_rdr -> new_case, PLIST(new_case));
	show_conclusions(INHERITS(new_case), rule_slot, conclusion);
	conclusion_cb(NULL, 0);
	
	sprintf(buf, "New Case: %s", NAME(new_case));
	fl_set_object_label(fd_rdr -> new_case, buf);
	sprintf(buf, "Cornerstone Case: %s", NAME(old_case));
	fl_set_object_label(fd_rdr -> cornerstone_case, buf);
	fl_show_form(fd_rdr -> rdr, FL_PLACE_CENTER, FL_FULLBORDER, "Ripple Down Rules");

	if (setjmp(ret_env))
		return TRUE;
	else
		fl_do_forms();

	return FALSE;
}


/************************************************************************/
/*   	Dummy call back in case it's ever needed in the future		*/
/************************************************************************/

void
conditions_cb(FL_OBJECT *ob, long data)
{
	/* fill-in code for callback */
}


/************************************************************************/
/*			Invokde RDR maintenance GUI			*/
/************************************************************************/

void
rdr_GUI_init(void)
{
	new_pred(rdr_GUI, "rdr_GUI");
}
