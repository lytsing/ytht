#include "bbs.h"

struct bbsinfo bbsinfo;

int
main(int argc, char *argv[])
{
	int bnum=0, i;
	struct user_info *uentp;

	if (initbbsinfo(&bbsinfo) < 0) {
		printf("not init bbsinfo!\n");
		return 0;
	}
	if (argc != 2) {
		printf("no engough argments!\n");
		return 0;
	}
	for (i = 0; i < MAXBOARD; i++) {
		if (strcmp(bbsinfo.bcacheshm->bcache[i].header.filename, argv[1]))
			continue;
		bnum = i;
		break;
	}
	if (i == MAXBOARD) {
		printf("not found board!\n");
		return 0;
	}
	for (i = 0; i < USHM_SIZE; i++) {
		uentp = &(bbsinfo.utmpshm->uinfo[i]);
		if (uentp->active && uentp->pid && uentp->curboard == bnum + 1)
			printf("%d %s %s\n", uentp->pid, uentp->userid, uentp->from);
	}
	return 0;
}
