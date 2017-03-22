/************************************************************************/
/*	Initialise all modules. Must be called before anything else	*/
/************************************************************************/

void
version(char **version_id)
{
	void	set_init(), bayes_init(), atms_init(), aq_init(), id_init(),
		induct_init(), rt_init(), bp_init(), duce_init(), scw_init();

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
	
	*version_id = "iProlog ML (15 November 2007)";
}
