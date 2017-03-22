/************************************************************************/
/*	Initialise all modules. Must be called before anything else	*/
/************************************************************************/

void
version(char **version_id)
{
	void	refine_init(), lgg_init(), golem_init(), progol_init(),
		clause_diff_init();

	refine_init();
	lgg_init();
	golem_init();
	progol_init();
	clause_diff_init();
	
	*version_id = "iProlog ILP (15 November 2007)";
}
