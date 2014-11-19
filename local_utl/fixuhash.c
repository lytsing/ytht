#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;
int
main(void)
{
	int i;
	if (initbbsinfo(&bbsinfo) < 0)
		goto quit;
	if (load_ucache() < 0)
		goto quit;
	for (i = 0; i < 4; i++) {
		getrandomint(&(bbsinfo.ucachehashshm->regkey[i]));
		getrandomint(&(bbsinfo.ucachehashshm->oldregkey[i]));
	}
	bbsinfo.ucachehashshm->keytime = time(NULL);
	bbsinfo.ucachehashshm->uptime = time(NULL);
	return 0;

      quit:
	printf("E!\n");
	return -1;
}
