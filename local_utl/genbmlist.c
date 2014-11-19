#include "bbs.h"
#define LISTSIZE 2000
int count = 0;
char bmlist[LISTSIZE][30];

int
inlist(char *id)
{
	int i;
	for (i = 0; i < count; i++) {
		if (!strcmp(id, bmlist[i]))
			return 1;
	}
	return 0;
}

int
addbms(struct boardheader *rec)
{
	int i;
	for (i = 0; i < BMNUM; i++) {
		if (rec->bm[i][0] && !inlist(rec->bm[i])) {
			strcpy(bmlist[count], rec->bm[i]);
			count++;
		}
	}
	return 0;
}

main(int argn, char **argv)
{
	char *secstr = "";
	const struct sectree *sec;
	struct boardheader rec;
	int fd, l, i;
	if (argn >= 2)
		secstr = argv[1];
	sec = getsectree(secstr);
	if (strcmp(sec->basestr, secstr)) {
		printf("错误的分区字符串!\n");
		return -1;
	}
	secstr = sec->basestr;
	l = strlen(secstr);
	fd = open(MY_BBS_HOME "/.BOARDS", O_RDONLY);
	if (fd <= 0) {
		printf("无法打开.BOARDS文件!\n");
		return -1;
	}
	while (sizeof (rec) == read(fd, &rec, sizeof (rec))) {
		if (!strncmp(rec.sec1, secstr, l) ||
		    !strncmp(rec.sec2, secstr, l)) {
			addbms(&rec);
		}
	}
	close(fd);
	for (i = 0; i < count; i++) {
		printf("%s\n", bmlist[i]);
	}
}
