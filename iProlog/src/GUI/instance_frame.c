/* Form definition file generated with fdesign. */

#include "forms.h"
#include <stdlib.h>
#include "instance_frame.h"

FD_instance_frame *create_form_instance_frame(void)
{
  FL_OBJECT *obj;
  FD_instance_frame *fdui = (FD_instance_frame *) fl_calloc(1, sizeof(*fdui));

  fdui->instance_frame = fl_bgn_form(FL_NO_BOX, 285, 455);
  obj = fl_add_box(FL_FLAT_BOX,0,0,285,455,"");
  obj = fl_add_box(FL_UP_BOX,0,0,320,30,"");
  obj = fl_add_menu(FL_PULLDOWN_MENU,5,5,80,20,"Menu");
  obj = fl_add_menu(FL_PULLDOWN_MENU,80,5,80,20,"Menu");
  obj = fl_add_box(FL_FRAME_BOX,0,30,320,90,"");
  obj = fl_add_input(FL_NORMAL_INPUT,90,40,180,30,"Frame Id:");
  obj = fl_add_input(FL_NORMAL_INPUT,90,80,180,30,"Isa:");
  obj = fl_add_input(FL_NORMAL_INPUT,90,130,180,30,"name:");
  obj = fl_add_input(FL_NORMAL_INPUT,90,170,180,30,"ph_level");
  obj = fl_add_input(FL_NORMAL_INPUT,90,210,180,30,"investigation");
  obj = fl_add_input(FL_NORMAL_INPUT,90,250,180,30,"this is a very long name that goes on forever");
  obj = fl_add_input(FL_NORMAL_INPUT,90,290,180,30,"Isa:");
  obj = fl_add_input(FL_NORMAL_INPUT,90,330,180,30,"Isa:");
  obj = fl_add_input(FL_NORMAL_INPUT,90,370,180,30,"Isa:");
  obj = fl_add_input(FL_NORMAL_INPUT,90,410,180,30,"Isa:");
  fl_end_form();

  fdui->instance_frame->fdui = fdui;

  return fdui;
}
/*---------------------------------------*/

