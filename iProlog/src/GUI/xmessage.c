#include "forms.h"
#include "prolog.h"
#include "string_io.h"


static int
p_xmessage(term goal, term *frame)
{
	FILE *old_output = output;
	char *buf;

	output = str_wopen(&buf);
	p_print(goal, frame);
	fl_show_messages(buf);
	fclose(output);
	output = old_output;
	free(buf);

	return TRUE;
}


static int
p_x_yes_no(term goal, term *frame)
{
	term msg = check_arg(1, goal, frame, ATOM, IN);

	return fl_show_question(NAME(msg), 1) ? TRUE : FALSE;
}


static int
p_xalert(term goal, term *frame)
{
	term msg = check_arg(1, goal, frame, ATOM, IN);

	fl_show_alert("Alert", NAME(msg), "", 1);
	return TRUE;
}


term
f_xquestion(term goal, term *frame)
{
	char *s, *buf;
	term rval;

	buf = message_to_string(goal, frame);
	s = (char *) fl_show_input(buf, "");
	if (s == NULL)
		return NULL;
	rval = read_atom_from_string(s);
	free(buf);
	return rval;
}


/************************************************************************/
/*		   Finalise all modules. Called on exit			*/
/************************************************************************/

static void
finalise(void)
{
	fl_finish();
}


/************************************************************************/
/*			    XFORMS Intialisation                       	*/
/************************************************************************/

void
xforms_init(void)
{
	int argc = 1;
	char *argv[] = {"iProlog"};

	/* register finalise to be called on exit */
	atexit(finalise);

	fl_set_border_width(1); 
	fl_initialize(&argc, argv, 0, 0, 0);

	new_pred(p_xmessage, "message");
	new_pred(p_x_yes_no, "yes_no_box");
	new_pred(p_xalert, "alert_box");
	new_subr(f_xquestion, "question_box");
}
