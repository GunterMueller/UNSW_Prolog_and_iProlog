/************************************************************************/
/*	Initialise all modules. Must be called before anything else	*/
/************************************************************************/

void
version(char **version_id)
{
	void frame_init(), rdr_init(), print_rule_init();

	frame_init();
	rdr_init();
	print_rule_init();
	
	*version_id = "iProlog KBS (15 November 2007)";
}
