/************************************************************************/
/*		Initialisation for Probot version of iProlog		*/
/************************************************************************/

#include <stdio.h>

void
version(char **version_id)
{
	void	frame_init(), rdr_init(), print_rule_init();
	void	probot_init(), script_init();

	frame_init();
	rdr_init();
	print_rule_init();

	probot_init();
	script_init();
	
	*version_id = "Probot (15 November 2007)\n";
/*
	ungetc('\n', stdin);	// This is needed to force an initial prompt
				// when Probot is called from command line file
*/
}
