#include "bbslib.h"

int
updatelastpost(char *board)
{
	struct boardmem *bptr;
	bptr = getbcache(board);
	if (bptr == NULL)
		return -1;
	getlastpost(bptr->header.filename, &bptr->lastpost, &bptr->total);
	return 0;
}
