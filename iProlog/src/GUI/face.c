/************************************************************************/
/*			Pipe to talking head				*/
/************************************************************************/

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include "prolog.h"


/************************************************************************/
/*		     Check if input stream has incoming data		*/
/************************************************************************/

static void
child_died(int i)
{
	dialog_in = stdin;
	dialog_out = stdout;
	fail("Child process terminated");
}


/************************************************************************/
/*		 	   Create pipes to talking head			*/
/************************************************************************/

static int
face(term goal, term *frame)
{
	signal(SIGCHLD, child_died);

	send_to("sophie", &dialog_out, &dialog_in);

	sleep(1);		/* child dies if this isn't here */
	return TRUE;
}


/************************************************************************/
/*		 	  Make the face say something			*/
/************************************************************************/

static int
speak(term goal, term *frame)
{
	term msg = check_arg(1, goal, frame, ANY, IN);
	FILE *old_output = output;

	output = dialog_out;
	print(msg);
	fflush(output);
	output = old_output;
	
	return TRUE;
}


/************************************************************************/
/* 			  Module initialisation				*/
/************************************************************************/

void
face_init(void)
{
	new_pred(face, "face");
	new_pred(speak, "speak");
}
