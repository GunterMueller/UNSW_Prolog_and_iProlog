#include "forms.h"
#include "fbrowser.h"

int main(int argc, char *argv[])
{
   FD_frame_browser *fd_frame_browser;
   FD_generic_frame *fd_generic_frame;
   FD_instance_frame *fd_instance_frame;

   fl_initialize(&argc, argv, 0, 0, 0);
   fd_frame_browser = create_form_frame_browser();
   fd_generic_frame = create_form_generic_frame();
   fd_instance_frame = create_form_instance_frame();

   /* fill-in form initialization code */

   /* show the first form */
   fl_show_form(fd_frame_browser->frame_browser,FL_PLACE_CENTERFREE,FL_FULLBORDER,"frame_browser");
   fl_do_forms();
   return 0;
}
