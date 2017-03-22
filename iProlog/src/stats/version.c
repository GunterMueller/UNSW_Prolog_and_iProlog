/************************************************************************/
/*	This is the main program for the entire iProlog system_init	*/
/************************************************************************/

void
version(char **version_id)
{
	void	set_init(), bayes_init(), atms_init(), aq_init(), id_init(),
		induct_init(), rt_init(), bp_init(), duce_init(), scw_init();

	void	discrim_init(), line_init(), mreg_init();


	set_init();
	bayes_init();
	atms_init();
	aq_init();
	id_init();
	induct_init();
	rt_init();
	bp_init();
	duce_init();
	scw_init();

	discrim_init();
	line_init();
	mreg_init();
	
	*version_id = "iProlog STATS (15 November 2007)";
}
