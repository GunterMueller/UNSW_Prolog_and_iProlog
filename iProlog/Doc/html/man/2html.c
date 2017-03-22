#include <stdio.h>

#define TITLE	 	text[4]
#define NAME		text[14]
#define SYNOPSIS	text[22]
#define DESCRIPTION	text[30]

char *text[37] =
{
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2//EN\">",
	"<HTML>",
	"<HEAD>",
	"<TITLE>",
	NULL,
	"</TITLE>"
	"<META NAME=\"GENERATOR\" CONTENT=\"Mozilla/3.0Gold (X11; I; Linux 1.3.20 i586) [Netscape]\">",
	"<META NAME=\"Author\" CONTENT=\"Claude Sammut\">",
	"</HEAD>",
	"<BODY>",
	"",
	"<H2>NAME</H2>",
	"",
	"<UL>",
	"<P>",
	NULL,
	"</P>",
	"</UL>",
	"",
	"<H2>SYNOPSIS</H2>",
	"",
	"<UL>",
	"<P>",
	NULL,
	"</P>",
	"</UL>",
	"",
	"<H2>DESCRIPTION </H2>",
	"",
	"<UL>",
	"<P>",
	NULL,
	"</P>",
	"</UL>",
	"",
	"</BODY>",
	"</HTML>",
	NULL
};


print_string(FILE *output, char **s)
{
	while (*s != NULL)
	{
		fputs(*s++, output);
		fputc('\n', output);
	}
}

read_string(FILE *input, char *s, char terminator)
{
	int c;

	while ((c = fgetc(input)) != terminator)
		*s++ = c;
	*s = '\0';
}

main(int argc, char **argv)
{
	FILE *input, *output;
	char html_file[64];
	char synopsis[120];
	char description[512];

	if (argc != 2)
	{
		fprintf(stderr, "Usage: fred <file name>\n");
		exit(0);
	}

	if ((input = fopen(argv[1], "r")) == NULL)
	{
		fprintf(stderr, "File not found: %s\n", argv[1]);
		exit(0);
	}

	sprintf(html_file, "%s.html", argv[1]);

	if ((output = fopen(html_file, "w")) == NULL)
	{
		fprintf(stderr, "Can't open output file: %s\n", argv[1]);
		exit(0);
	}

	TITLE = argv[1];
	NAME = argv[1];
	SYNOPSIS = synopsis;
	DESCRIPTION = description;

	read_string(input, synopsis, '\n');
	read_string(input, description, EOF);

	print_string(output, text);
}
