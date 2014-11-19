#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;

int testbm(const struct userec *rec, char *arg) {
	if (rec->userid[0] != 0 && rec->kickout != 0) {
		if (rec->userlevel & PERM_BOARDS) {
			printf("%s\n", rec->userid);
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{

	if (initbbsinfo(&bbsinfo) < 0) {
		perror("init error");
		return -1;
	}
	apply_passwd(testbm, NULL);
	return 0;
}
