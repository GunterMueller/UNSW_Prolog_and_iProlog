#include "forms.h"
#include "rdr.h"

int main(int argc, char *argv[])
{
   FD_rdr *fd_rdr;

   fl_initialize(&argc, argv, 0, 0, 0);
   fd_rdr = create_form_rdr();

   /* fill-in form initialization code */

   /* show the first form */
   fl_show_form(fd_rdr->rdr,FL_PLACE_CENTER,FL_FULLBORDER,"rdr");
   fl_do_forms();
   return 0;
}
