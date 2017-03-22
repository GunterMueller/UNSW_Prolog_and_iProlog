#include "forms.h"
#include "prolog.h"

static void *last_cookie;

static ssize_t
xf_read(void *cookie, char *buf, size_t size)
{
	const char *str;

	if (cookie == last_cookie)
		return EOF;
	else
		last_cookie = cookie;

	str = fl_get_input((FL_OBJECT *) cookie);

	strncpy(buf, str, size);

	return strlen(buf);
}


static ssize_t
xf_read_selection(void *cookie, char *buf, size_t size)
{
	const char *str;
	int start, end;

	str = fl_get_input((FL_OBJECT *) cookie);
	fl_get_input_selected_range((FL_OBJECT *) cookie, &start, &end);

	if (start == end)
		strncpy(buf, str, size);
	else if (end - start < size - 2)
	{
		strncpy(buf, &str[start], end - start);
		buf[end-start]='\n';
		buf[end-start+1]='\0';
	}
	else
		return -1;

	return strlen(buf);
}


static ssize_t
xf_read_browser(void *cookie, char *buf, size_t size)
{
	int line = fl_get_browser((FL_OBJECT *) cookie);
	const char *str = fl_get_browser_line((FL_OBJECT *) cookie, line);

	strncpy(buf, str, size);

	return strlen(buf);
}


static ssize_t
xf_write(void *cookie, const char *buf, size_t size)
{
	const char *in = fl_get_input((FL_OBJECT *) cookie);
	size_t insize = strlen(in);
	char *tmp = malloc(insize + size + 1);

	strcpy(tmp, in);
	strncpy(&tmp[insize], buf, size);
	tmp[insize + size] = '\0';
	fl_set_input((FL_OBJECT *) cookie, (const char *) tmp);
	free(tmp);

	return strlen(buf);
}


static ssize_t
xf_write_browser(void *cookie, const char *buf, size_t size)
{
	char *tmp = malloc(size + 1);

	strncpy(tmp, buf, size);
	tmp[size] = '\0';
	fl_addto_browser((FL_OBJECT *) cookie, (const char *) tmp);
	free(tmp);

	return strlen(buf);
}


static int
xf_close(void *cookie)
{
	return 0;
}


FILE *
xf_open(FL_OBJECT *input_field)
{
	FILE *rval;

#ifdef _GNU_SOURCE
	static cookie_io_functions_t io_fns = {xf_read, xf_write, NULL, xf_close};

	rval = fopencookie(input_field, "r+", io_fns);
#else
	rval = funopen(input_field, xf_read, xf_write, NULL, xf_close);
#endif
	setlinebuf(rval);

	last_cookie = NULL;
	return rval;
}


FILE *
xf_open_selection(FL_OBJECT *input_field)
{
	FILE *rval;

#ifdef _GNU_SOURCE
	static cookie_io_functions_t sel_fns = {xf_read_selection, xf_write, NULL, xf_close};

	rval = fopencookie(input_field, "r+", sel_fns);
#else
	rval = funopen(input_field, xf_read_selection, xf_write, NULL, xf_close);
#endif
	setlinebuf(rval);

	return rval;
}


FILE *
xf_open_browser(FL_OBJECT *input_field)
{
	FILE *rval;

#ifdef _GNU_SOURCE
	static cookie_io_functions_t browser_fns = {xf_read_browser, xf_write_browser, NULL, xf_close};

	rval = fopencookie(input_field, "r+", browser_fns);
#else
	rval = funopen(input_field, xf_read_browser, xf_write_browser, NULL, xf_close);
#endif
	setlinebuf(rval);

	return rval;
}


term
xread(FL_OBJECT *input_field)
{
	term rval;
	FILE *old_input = input;

	input = xf_open(input_field);
	clear_input();
	rval = p_read();
	xf_close(input);
	input = old_input;

	return rval;
}


term
xread_term(FL_OBJECT *input_field)
{
	term rval;
	FILE *old_input = input;

	input = xf_open(input_field);
	clear_input();
	rval = read_term();
	xf_close(input);
	input = old_input;

	return rval;
}


term
xread_selection(FL_OBJECT *input_field)
{
	term rval;
	FILE *old_input = input;

	input = xf_open_selection(input_field);
	clear_input();
	rval = read_term();
	xf_close(input);
	input = old_input;

	return rval;
}


void
xprint(FL_OBJECT *input_field, term t)
{
	FILE *old_output = output;

	output = xf_open(input_field);
	print(t);
	xf_close(output);
	output = old_output;
}


void
xprint_browser(FL_OBJECT *input_field, term t)
{
	FILE *old_output = output;

	output = xf_open_browser(input_field);
	prin(t);
	fflush(output);
	xf_close(output);
	output = old_output;
}
