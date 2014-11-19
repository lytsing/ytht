#include "bbs.h"
int
main()
{
	struct bbsinfo bbsinfo;
	struct UTMPFILE *utmpshm;
	struct tm *now;
	time_t nowtime;
	if (initbbsinfo(&bbsinfo)<0)
		exit(1);
	utmpshm = bbsinfo.utmpshm;
	nowtime = time(0);
	now = localtime(&nowtime);
	printf("%d %d\n", now->tm_hour, utmpshm->activeuser);
	return 0;
}
