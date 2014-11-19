#include "innbbsconf.h"
#include "bbslib.h"

extern void mkhistory(char *);

int
main(argc, argv)
int argc;
char *argv[];
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s history-file\n", argv[0]);
		exit(1);
	}
	initial_bbs(NULL);
	mkhistory(argv[1]);
	return 0;
}
