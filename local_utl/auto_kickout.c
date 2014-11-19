#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;

int test_kickout(struct userec *u, char *arg)
{
	if (u->userid[0] == 0)
		return 0;
	if (countlife(u) < 0 && u->kickout == 0) 
		kickoutuserec(u->userid);
	return 0;
}

int
main(int argc, char *argv[])
{

	if (initbbsinfo(&bbsinfo) < 0) {
		perror("init error");
		return -1;
	}
	apply_passwd((void *)test_kickout, NULL);
	return 0;
}
