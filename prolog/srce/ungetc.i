#include <stdio.h>

int ungetc(c, iop)
int c;
register FILE *iop;
{
	if (c == EOF) return(EOF);

	if ((iop->_flag & _IOREAD) == 0 || iop->_ptr <= iop->_base)
		if (iop->_ptr == iop->_base && iop->_cnt == 0)
			++iop -> _ptr;
		else return(EOF);
	*--iop -> _ptr = c;
	++iop -> _cnt;
	return(c);
}