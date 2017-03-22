#include <unistd.h>
#include <prolog.h>

term read_atom(void);
term p_read(void);
void rprint(term, term *);
void rprin(term, term *);

extern int display;

static FILE *old_input, *old_output;

static void
open_pipe(void)
{
	int pipe_fd[2];

	old_input = input;
	old_output = output;

	pipe(pipe_fd);

	input = fdopen(pipe_fd[0], "r");
	output = fdopen(pipe_fd[1], "w");
}

static void
close_pipe(void)
{
	fclose(input);

	input = old_input;
	output = old_output;
}


term
read_atom_from_string(char *str)
{
	term rval;

	open_pipe();

	fputs(str, output);
	fclose(output);

	rval = read_atom();

	close_pipe();
	return rval;
}

term
read_term_from_string(char *str)
{
	term rval;

	open_pipe();

	fputs(str, output);
	fputc('.', output);
	fclose(output);

	rval = p_read();
	close_pipe();
	return ARG(1, rval);
}

void
prin_to_string(term expr, term *frame, char *buf, size_t count)
{
	int n;

	open_pipe();

	rprin(expr, frame);
	fclose(output);

	n = fread(buf, (size_t) 1, count, input);
	if (n < count-1)
		buf[n] = '\0';
	else
		buf[count-1] = '\0';

	close_pipe();
}

void
print_to_string(term expr, term *frame, char *buf, size_t count)
{
	int n;

	open_pipe();

	rprint(expr, frame);
	fclose(output);

	n = fread(buf, (size_t) 1, count, input);
	if (n < count-1)
		buf[n] = '\0';
	else
		buf[count-1] = '\0';

	close_pipe();
}

void
message_to_string(term goal, term *frame, char *buf, size_t count)
{
	int i, n;

	open_pipe();

	display = FALSE;
	if (ARITY(goal) > 0)
		for (i = 1; i <= ARITY(goal); i++)
			rprin(ARG(i, goal), frame);
	else
		rprin(goal, frame);
	fclose(output);

	n = fread(buf, (size_t) 1, count, input);
	if (n < count-1)
		buf[n] = '\0';
	else
		buf[count-1] = '\0';

	close_pipe();
}
