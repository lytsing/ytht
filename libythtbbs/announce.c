#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "ythtbbs.h"

char *
getbfroma(char *path)
{
	static char board[30];
	char *ptr;
	if (*path == '/')
		path++;
	if (!strncmp(path, "0Announce/", 10))
		path += 10;
	if (strncmp(path, "groups/GROUP_", 13))
		return "";
	ptr = strchr(path + 13, '/');
	if (!ptr)
		return "";
	strsncpy(board, ptr + 1, sizeof (board));
	ptr = strchr(board, '/');
	if (ptr)
		*ptr = 0;
	return board;
}

int
checktitle(char *str)
{
	int indextype = 0;
	if (strstr(str, "��������������") == str)
		indextype = 2;
	else if (strstr(str, "��������Ŀ¼������") == str)
		indextype = 2;
	else if (strstr(str, "������������������") == str)
		indextype = 1;
	else if (strstr(str, "������������������") == str)
		indextype = 3;
	return indextype;
}

int
do_testtime(int t, char *path0, int mode, int time)	//mode=0 ���������ļ� mode=1 ���� .Names �ļ�
{
	char names[1024], genbuf[1024], path00[1024];
	FILE *fp;
	int m;
	if (!lfile_isdir(path0))
		return 0;
	if (file_time(path0) >= t - time)
		return 1;
	sprintf(names, "%s/.Names", path0);
	if (file_time(names) >= t - time)
		return 1;
	fp = fopen(names, "r");
	if (!fp)
		return 0;
	while (fgets(genbuf, sizeof (genbuf), fp)) {
		if (!strncmp(genbuf, "Name=", 5)) {
			if (checktitle(genbuf + 5))
				continue;
			fgets(genbuf, 256, fp);
			if (strncmp("Path=~/", genbuf, 6))
				continue;
			for (m = 0; m < strlen(genbuf); m++)
				if (genbuf[m] <= 27)
					genbuf[m] = 0;
			if (!strcmp("Path=~/", genbuf))
				continue;
			sprintf(path00, "%s/%s", path0, genbuf + 7);
			for (m = 0; m < strlen(path00); m++)
				if (path00[m] <= 27)
					path00[m] = 0;
			if (checkautofile(genbuf + 7, path00))
				continue;
			if (!file_exist(path00))
				continue;
			if (lfile_isdir(path00)
			    && do_testtime(t, path00, mode, time)) {
				fclose(fp);
				return 1;
			} else if (!mode && file_time(path00) >= t - time) {
				fclose(fp);
				return 1;
			}
		}
	}
	fclose(fp);
	return 0;
}

struct autofile {
	const char *fname;
	const char *fpath;
	ino_t inode;
	dev_t dev;
};

static struct autofile autofile[] = {
	{"ctop", MY_BBS_HOME "/0Announce/groups/GROUP_0/Personal_Corpus/ctop"},
	{"7days", MY_BBS_HOME "/0Announce/groups/GROUP_0/BM_Club/7days"},
	{"bmlist", MY_BBS_HOME "/0Announce/groups/GROUP_0/BM_Club/bmlist"},
	{"boardlist",
	 MY_BBS_HOME "/0Announce/groups/GROUP_0/BM_Club/boardlist"},
	{NULL}
};

int
checkautofile(char *fname, char *fpath)
{
	static int inited = 0;
	int i;
	struct stat *st;
	if (!inited) {
		for (i = 0; autofile[i].fname; i++) {
			st = f_stat(autofile[i].fpath);
			autofile[i].dev = st->st_dev;
			autofile[i].inode = st->st_ino;
		}
		inited = 1;
	}
	for (i = 0; autofile[i].fname; i++) {
		if (strcmp(fname, autofile[i].fname))
			continue;
		st = f_stat(fpath);
		if (autofile[i].dev == st->st_dev &&
		    autofile[i].inode == st->st_ino)
			return 1;
	}
	return 0;
}
