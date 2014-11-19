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
		printf("%-14s �ϴ���վʱ���� %s",urec->userid,ctime(&(urec->lastlogin)));
	return 0;
}

int
main(int argc, char *argv[])
{
	
	if (initbbsinfo(&bbsinfo) < 0) {
		printf("���ܴ�ϵͳ�ļ�,��֪ͨϵͳά��!\n");
		return -1;
	}
	now_t=time(NULL);
	printf("����û�����ߵİ���:\n\n");
	apply_passwd(bm7check, NULL);
	return 0;
}
