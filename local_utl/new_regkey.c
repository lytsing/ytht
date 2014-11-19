#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;

int
main(int argc, char *argv[])
{
	int i;
	unsigned int r;
	FILE *fp;
	char *ptr, font[1024], buf[1024];
	if (initbbsinfo(&bbsinfo) < 0) {
		perror("init error");
		return -1;
	}
	bbsinfo.ucachehashshm->keytime = time(NULL);
	memcpy(&(bbsinfo.ucachehashshm->oldregkey), &(bbsinfo.ucachehashshm->regkey), sizeof(int) * 4);
	for (i = 0; i < 4; i++) {
		getrandomint(&r);
		bbsinfo.ucachehashshm->regkey[i] = r;
	}
	fp = fopen("etc/figfonts.list", "r");
	if (fp == NULL)
		return 0;
	if (fgets(font, 1024, fp) == 0)
		return 0;
	r = r % atoi(font);
	i = 0;
	while (fgets(font, 1024, fp)) {
		if (i == r)
			break;
		i++;
	}
	ptr = strchr(font, '\n');
	if (ptr)
		*ptr = 0;
	for (i = 0; i <= 9; i++) {
		sprintf(buf, "/usr/bin/figlet -f %s %d > %s/etc/fonts/txt/%d", font, i, MY_BBS_HOME, i);
		system(buf);
	}
	return 0;
}
