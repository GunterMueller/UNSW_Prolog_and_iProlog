#include <stdio.h>
#include <stdarg.h>
#include "prolog.h"

void speak(char *);
void set_output_box(char *);

extern void *facial_animation;


/************************************************************************/
/*			    Send string to GUI				*/
/************************************************************************/

void set_output_box(char *);

void
reply(const char *fmt, ...)
{
	va_list arg;
	char buf[1024];

	va_start(arg, fmt);
	(void) vsprintf(buf, fmt, arg);
	va_end(arg);

	if (facial_animation != NULL)
	{
		set_output_box(buf);
		speak(buf);
	}
	else
	{
		fputs(buf, output);
		fflush(output);
	}
}
