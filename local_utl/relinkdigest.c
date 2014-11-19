#include "ythtbbs.h"
#include "ythtlib.h"
#include <sys/types.h>
#include <dirent.h>

static void
process_board(char *path)
{
	char buf[256];
	char buf2[256];
	FILE *fp;
	struct fileheader a;
	struct stat st1, st2;
	snprintf(buf, sizeof (buf), "%s/.DIGEST", path);
	fp = fopen(buf, "r");
	if (NULL == fp)
		return;
	while (fread(&a, 1, sizeof (struct fileheader), fp) ==
	       sizeof (struct fileheader)) {
		snprintf(buf, sizeof (buf), "%s/%s", path, fh2fname(&a));
		a.accessed &= ~FH_ISDIGEST;
		snprintf(buf2, sizeof (buf2), "%s/%s", path, fh2fname(&a));
		if (strcmp(buf, buf2) && !stat(buf, &st1) && !stat(buf2, &st2)) {
			if (st1.st_ino == st2.st_ino
			    || st1.st_size != st2.st_size)
				continue;
			printf("merge %s %s\n", buf, buf2);
			unlink(buf);
			link(buf2, buf);
		}
	}
	fclose(fp);
}

int
main(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *a;
	char buf[256];

	chdir(MY_BBS_HOME);
	dir = opendir("boards");
	if (NULL == dir) {
		printf("can't open boards directory!\n");
		return -1;
	}
	while ((a = readdir(dir))) {
		snprintf(buf, sizeof (buf), "boards/%s", a->d_name);
		if (strlen(a->d_name) > 40 || a->d_name[0] == '.'
		    || lfile_islnk(buf) || !file_isdir(buf)) {
			printf("skip %s\n", a->d_name);
			continue;
		}
		process_board(buf);
	}
	closedir(dir);
	return 0;
}
