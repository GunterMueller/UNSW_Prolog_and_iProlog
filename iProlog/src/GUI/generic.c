/* Form definition file generated with fdesign. */

#include "forms.h"
#include <stdlib.h>
#include "generic.h"

static FL_PUP_ENTRY fdmenu_facet_menu_0[] =
{ 
    /*  itemtext   callback  shortcut   mode */
    { "range",	0,	"",	 FL_PUP_NONE},
    { "help",	0,	"",	 FL_PUP_NONE},
    { "multivalued",	0,	"",	 FL_PUP_NONE},
    { "cache",	0,	"",	 FL_PUP_NONE},
    { "default",	0,	"",	 FL_PUP_NONE},
    { "if_new",	0,	"",	 FL_PUP_NONE},
    { "if_needed",	0,	"",	 FL_PUP_NONE},
    { "if_added",	0,	"",	 FL_PUP_NONE},
    { "if_replaced",	0,	"",	 FL_PUP_NONE},
    { "if_removed",	0,	"",	 FL_PUP_NONE},
    {0}
};

FD_generic_frame *create_form_generic_frame(void)
{
  FL_OBJECT *obj;
  FD_generic_frame *fdui = (FD_generic_frame *) fl_calloc(1, sizeof(*fdui));

  fdui->generic_frame = fl_bgn_form(FL_NO_BOX, 795, 385);
  obj = fl_add_box(FL_FLAT_BOX,0,0,795,385,"");
    fl_set_object_resize(obj, FL_RESIZE_NONE);
  fdui->generic_facet_list = obj = fl_add_browser(FL_HOLD_BROWSER,280,80,120,210,"Facets");
    fl_set_object_color(obj,FL_WHITE,FL_MCOL);
    fl_set_object_lalign(obj,FL_ALIGN_TOP);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_facet_list_cb,0);
  fdui->new_generic_slot_button = obj = fl_add_button(FL_NORMAL_BUTTON,150,300,120,30,"New Slot");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,new_generic_slot_button_cb,0);
  fdui->daemon = obj = fl_add_input(FL_MULTILINE_INPUT,410,80,370,210,"Daemon");
    fl_set_object_color(obj,FL_WHITE,FL_WHITE);
    fl_set_object_lalign(obj,FL_ALIGN_TOP_LEFT);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,daemon_cb,0);
  fdui->generic_slot_list = obj = fl_add_browser(FL_HOLD_BROWSER,150,80,120,210,"Slots");
    fl_set_object_color(obj,FL_WHITE,FL_MCOL);
    fl_set_object_lalign(obj,FL_ALIGN_TOP);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_slot_list_cb,0);
  fdui->generic_name = obj = fl_add_input(FL_NORMAL_INPUT,190,25,160,30,"Name:");
    fl_set_object_color(obj,FL_WHITE,FL_WHITE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_name_cb,0);
  fdui->generic_inherits = obj = fl_add_input(FL_NORMAL_INPUT,410,25,230,30,"Inherits:");
    fl_set_object_color(obj,FL_WHITE,FL_WHITE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_inherits_cb,0);
  fdui->generic_delete_button = obj = fl_add_button(FL_NORMAL_BUTTON,20,340,120,30,"Delete Frame");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_delete_button_cb,0);
  fdui->generic_accept_button = obj = fl_add_button(FL_NORMAL_BUTTON,660,300,120,30,"Accept");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_accept_button_cb,0);
  fdui->instances_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,660,25,120,30,"Instances");
    fl_set_object_boxtype(obj,FL_UP_BOX);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,instances_menu_cb,0);
  fdui->facet_menu = obj = fl_add_menu(FL_PULLDOWN_MENU,280,300,120,30,"Facets");
    fl_set_object_boxtype(obj,FL_UP_BOX);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,facet_menu_cb,0);
    fl_set_menu_entries(obj, fdmenu_facet_menu_0);
  fdui->generic_remove_slot_button = obj = fl_add_button(FL_NORMAL_BUTTON,150,340,120,30,"Remove Slot");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_remove_slot_button_cb,0);
  fdui->generic_remove_facet_button = obj = fl_add_button(FL_NORMAL_BUTTON,280,340,120,30,"Remove Facet");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_remove_facet_button_cb,0);
  fdui->generic_frame_list = obj = fl_add_browser(FL_HOLD_BROWSER,20,80,120,210,"Generic Frames");
    fl_set_object_color(obj,FL_WHITE,FL_MCOL);
    fl_set_object_lalign(obj,FL_ALIGN_TOP);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,generic_frame_list_cb,0);
  fdui->new_generic_button = obj = fl_add_button(FL_NORMAL_BUTTON,20,300,120,30,"New Frame");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,new_generic_button_cb,0);
  fdui->dispay_generic_frames = obj = fl_add_button(FL_NORMAL_BUTTON,20,25,120,30,"Display Frames");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,display_generic_button_cb,0);
  fdui->quit_button = obj = fl_add_button(FL_NORMAL_BUTTON,660,340,120,30,"Quit");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,quit_button_cb,0);
  fdui->save_button = obj = fl_add_button(FL_NORMAL_BUTTON,480,340,120,30,"Save");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,save_button_cb,0);
  fdui->open_button = obj = fl_add_button(FL_NORMAL_BUTTON,480,300,120,30,"Open");
    fl_set_object_lstyle(obj,FL_BOLD_STYLE);
    fl_set_object_resize(obj, FL_RESIZE_NONE);
    fl_set_object_callback(obj,open_button_cb,0);
  fl_end_form();

  fdui->generic_frame->fdui = fdui;

  return fdui;
}
/*---------------------------------------*/

