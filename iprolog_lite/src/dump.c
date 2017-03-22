#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "prolog.h"


#define ERROR(msg)	{fprintf(stderr, "%s\n", msg); return(FALSE);}

/************************************************************************/
/* data_start must be the very first thing declared and initialised	*/
/* It's address is used as the start of the user's data when dumping	*/
/* in fact there is other system stuff before it, but no harm is done	*/
/************************************************************************/

static int data_start = 0;

/************************************************************************/
/* dump the user's data segment starting from data_start to the program	*/
/* break found by sbrk(0). A one word header give the data size is	*/
/* written out first.							*/
/************************************************************************/

static int
dump(char *fname)
{
	int fd;
	size_t data_size = (size_t)(sbrk(0)) - (size_t)(&data_start);

	if ((fd = creat(fname, 0600)) == -1)
		ERROR("Cannot create dump file");
	if (write(fd, &data_size, sizeof data_size) != sizeof data_size)
		ERROR("Cannot write dump file header");
	if (write(fd, &data_start, data_size) != data_size)
		ERROR("Cannot write dump file data");
	close(fd);
	return(TRUE);
}


/************************************************************************/
/* restore the user's data segment by first reading the segment size	*/
/* then reading the data and placing it at data_start. The program	*/
/* must be set large enough to accomodate the data			*/
/* return -1 on error, 0 otherwise					*/
/************************************************************************/

int
restore(char *fname)
{
	int fd;
	size_t data_size;

	if ((fd = open(fname, O_RDONLY)) == -1)
		ERROR("Could not open dump file");

	if (read(fd, &data_size, sizeof data_size) != sizeof data_size)
		ERROR("Could not read dump file header");

	brk((size_t)(&data_start) + data_size);

	if (read(fd, &data_start, data_size) != data_size)
		ERROR("Could not read dump file data");

	close(fd);
	return(TRUE);
}


/************************************/
/* The user accessible call to dump */
/************************************/

static int
p_dump(term goal, term * frame)
{
	term fname = check_arg(1, goal, frame, ATOM, IN);

	return(dump(NAME(fname)));
}


void
dump_init()
{
	new_pred(p_dump, "dump");
}
