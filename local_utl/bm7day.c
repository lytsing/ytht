#include "bbs.h"
#include "ythtbbs.h"

time_t now_t; 
struct bbsinfo bbsinfo;

int bm7check(const struct userec *urec, char *arg)
{
	if (urec->userid[0] == 0 || urec->kickout != 0)
		return 0;
	if (!USERPERM(urec, PERM_BOARDS))
		return 0;
	if (urec->lastlogin < now_t - 7 * 86400)
		printf("%-14s 上次上站时间是 %s",urec->userid,ctime(&(urec->lastlogin)));
	return 0;
}

int
main(int argc, char *argv[])
{
	
	if (initbbsinfo(&bbsinfo) < 0) {
		printf("不能打开系统文件,请通知系统维护!\n");
		return -1;
	}
	now_t=time(NULL);
	printf("七天没有上线的版主:\n\n");
	apply_passwd(bm7check, NULL);
	return 0;
}
