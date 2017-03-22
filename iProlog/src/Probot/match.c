/************************************************************************/
/*			Hooks for clausal representation		*/
/************************************************************************/

static int
p_match(term goal, term *frame)
{
	term tree;
	int old_nvars = nvars;
	term loc_var[MAX_POS_VAR];
	term *old_pos_var = pos_var;
	int rval = FALSE;
	term pattern = check_arg(1, goal, frame, LIST, IN);
	term sentence = check_arg(2, goal, frame, LIST, IN);
	term out_vars = check_arg(3, goal, frame, FN, OUT);

	pos_var = loc_var;
	pos_var[0] = sentence;
	nvars = 1;

	if (match_pattern(pattern, sentence, &tree) == _nil)
	{
		int i;
		term fn = new_g_fn(nvars);

		ARG(0, fn) = _var;
		for (i = 0; i < nvars; i++)
			ARG(i+1, fn) = pos_var[i];

		unify(out_vars, frame, fn, frame);
		rval = TRUE;
	}

	pos_var = old_pos_var;
	nvars = old_nvars;
	return rval;
}
