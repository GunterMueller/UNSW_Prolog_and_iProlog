/************************************************************************/
/*		Initialisation for full version of iProlog		*/
/************************************************************************/


void
version(char **version_id)
{
	void	frame_init(), rdr_init(), print_rule_init();
	void	probot_init(), script_init();
	void	set_init(), bayes_init(), atms_init(), aq_init(), id_init();
	void	induct_init(), rt_init(), bp_init(), duce_init(), scw_init();
	void	refine_init(), lgg_init(), golem_init(), progol_init();
	void	clause_diff_init();
	void	discrim_init(), line_init(), mreg_init();
	void	image_init();
	void	xforms_init(), frame_browser_init(), rdr_GUI_init(),
		instance_init();

	frame_init();
	rdr_init();
	print_rule_init();

	probot_init();
	script_init();

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

	refine_init();
	lgg_init();
	golem_init();
	progol_init();
	clause_diff_init();

	discrim_init();
	line_init();
	mreg_init();

	image_init();

	xforms_init();
	frame_browser_init();
	rdr_GUI_init();
	instance_init();

	*version_id = "iProlog - all modules (15 November 2007)";
}
