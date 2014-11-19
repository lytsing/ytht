#include <stdlib.h>
#include "his.h"
#include "bbslib.h"

int
main(argc, argv)
int argc;
char **argv;
{
	long size, entry;
	if (argc > 1) {
		entry = atol(argv[1]);
	} else {
		entry = 100000;
	}
	size = dbzsize(entry);
	/*if (chdir(XINDEXDIR) != 0) {
	   fprintf(stderr,"can't chdir to %s\n",XINDEXDIR);
	   exit(-1);
	   } */
	dbzfresh(HISTORY, size, '\t', 0, 1);
	dbmclose();
	return 0;
}
