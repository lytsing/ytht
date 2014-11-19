#include "bbs.h"
#include "ythtbbs.h"

struct bbsinfo bbsinfo;
time_t t;

int
printbnames(const struct userec *urec, char *arg)
{
	char *ptr, *ptr0;
	int i = 0;
	struct mmapfile mf;
	char path[100];
	if (!urec->userid[0] || urec->kickout != 0 || t - urec->lastlogin > 24 * 3600 * 10)
		return 0;
	sethomefile(path, urec->userid, "brc");
	if (t - file_time(path) > 24 * 3600 * 10)
		return 0;
	mmapfile(NULL, &mf);
	if (mmapfile(path, &mf) < 0)
		return 0;
	if (mf.size <= 50) {
		mmapfile(NULL, &mf);
		return 0;
	}
	ptr0 = mf.ptr + mf.size;
	for (ptr = mf.ptr; ptr < ptr0;) {
		if (!((struct onebrc_c *) ptr)->len)
			break;
		if (ptr + ((struct onebrc_c *) ptr)->len > ptr0)
			break;
		if (strncmp
		    (((struct onebrc_c *) ptr)->data, "#INFO", BRC_STRLEN - 1)) {
			printf("%s%s", i ? " " : "",
			       ((struct onebrc_c *) ptr)->data);
			i++;
			if (i > 15)
				break;
		}
		ptr += ((struct onebrc_c *) ptr)->len;
	}
	if (i)
		printf("\n");
	return 0;
}
int
main()
{
	if (initbbsinfo(&bbsinfo) < 0) {
		fprintf(stderr, "can't init bbinfo\n");
		return -1;
	}
	t = time(NULL);
	apply_passwd(printbnames, NULL);
	return 0;
}
