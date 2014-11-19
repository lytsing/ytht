#include "bbs.h"
#include "stdio.h"
#include "ythtlib.h"
#include "ythtbbs.h"

int
main()
{
	FILE *fd1;
	struct boardheader rec;
	char bmbuf[IDLEN * 4 + 4];
	int size1 = sizeof (rec);
	fd1 = fopen(MY_BBS_HOME "/.BOARDS", "r");
	while (fread(&rec, size1, 1, fd1) == 1) {
		if (!(rec.level & PERM_POSTMASK) && !(rec.level & PERM_NOZAP)
		    && rec.level != 0)
			continue;
		if (rec.flag & CLOSECLUB_FLAG)
			continue;
		printf("%-18s%-24s %s\n", rec.filename,
		       rec.title, bm2str(bmbuf, &rec));
	}
	fclose(fd1);
	return 0;
}

