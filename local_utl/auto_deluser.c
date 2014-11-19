#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;
time_t now_t;

int test_deluser(struct userec *u, char *arg)
{
	char userid[IDLEN+1];
	if (u->userid[0] == 0)
		return 0;
	if (u->kickout !=0  && (now_t - u->kickout) > 86400 - 3600) {
		strcpy(userid, u->userid);
		deluserec(userid);
		tracelog("system kill %s", userid);
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
	now_t = time(NULL);
	apply_passwd((void *)test_deluser, NULL);
	return 0;
}
