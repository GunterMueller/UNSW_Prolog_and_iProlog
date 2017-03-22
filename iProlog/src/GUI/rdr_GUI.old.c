/* Form definition file generated with fdesign. */

#include "forms.h"
#include <stdlib.h>
#include "rdr.h"

static FL_PUP_ENTRY fdchoice_new_conclusion_0[] =
{ 
    /*  itemtext   callback  shortcut   mode */
    { "None",	0,	"",	 FL_PUP_NONE},
    { "Soft",	0,	"",	 FL_PUP_NONE},
    { "Hard",	0,	"",	 FL_PUP_NONE},
    {0}
};

static FL_PUP_ENTRY fdchoice_old_conclusion_1[] =
{ 
    /*  itemtext   callback  shortcut   mode */
    { "None",	0,	"",	 FL_PUP_NONE},
    { "Soft",	0,	"",	 FL_PUP_NONE},
    { "Hard",	0,	"",	 FL_PUP_NONE},
    {0}
};

FD_rdr *create_form_rdr(void)
{
  FL_OBJECT *obj;
  FD_rdr *fdui = (FD_rdr *) fl_calloc(1, sizeof(*fdui));

  fdui->rdr = fl_bgn_form(FL_NO_BOX, 525, 545);
  obj = fl_add_box(FL_FLAT_BOX,0,0,525,545,"");
    fl_set_object_resize(obj, FL_RESIZE_NONE);
  fdui->cornerstone_case = obj = fl_add_browser(FL_NORMAL_BROWSER,20,30,240,190,"Cornerstone Case:");
    fl_set_object_color(obj,FL_WHITE,FL_YELLOW);
    fl_set_object_lalign(obj,FL_ALIGN_TOP_LEFT);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
  fdui->conditions = obj = fl_add_browser(FL_MULTI_BROWSER,20,300,490,80,"Conditions:");
    fl_set_object_color(obj,FL_WHITE,FL_DARKER_COL1);
    fl_set_object_lalign(obj,FL_ALIGN_TOP_LEFT);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,conditions_cb,0);
  fdui->make_button = obj = fl_add_button(FL_NORMAL_BUTTON,320,505,100,30,"Make New Rule");
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,make_button_cb,0);
  fdui->cancel_button = obj = fl_add_button(FL_NORMAL_BUTTON,115,505,100,30,"Cancel");
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,cancel_button_cb,0);
  fdui->new_conclusion = obj = fl_add_choice(FL_NORMAL_CHOICE2,330,230,160,30,"Conclusion:");
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,new_conclusion_cb,0);
    fl_set_choice_entries(obj, fdchoice_new_conclusion_0);
    fl_set_choice(obj,1);
  fdui->new_case = obj = fl_add_browser(FL_NORMAL_BROWSER,270,30,240,190,"New Case:");
    fl_set_object_color(obj,FL_WHITE,FL_YELLOW);
    fl_set_object_lalign(obj,FL_ALIGN_TOP_LEFT);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
  fdui->old_conclusion = obj = fl_add_choice(FL_NORMAL_CHOICE,80,230,160,30,"Conclusion:");
    fl_set_object_boxtype(obj,FL_UP_BOX);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_choice_entries(obj, fdchoice_old_conclusion_1);
    fl_set_choice(obj,1);
  fdui->new_rule = obj = fl_add_input(FL_MULTILINE_INPUT,20,405,490,90,"New Rule:");
    fl_set_object_color(obj,FL_WHITE,FL_WHITE);
    fl_set_object_lalign(obj,FL_ALIGN_TOP_LEFT);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
  fl_end_form();

  fdui->rdr->fdui = fdui;

  return fdui;
}
/*---------------------------------------*/

