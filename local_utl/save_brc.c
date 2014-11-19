#include "bbs.h"
#include "ythtlib.h"
#include "ythtbbs.h"

int
copy_brc(char *ent, char *toent)
{
	int fd;
	char buf[BRC_MAXSIZE];
	int size;
	fd = open(ent, O_RDONLY);
	if (fd < 0)
		return -1;
	size = read(fd, buf, BRC_MAXSIZE);
	close(fd);
	if (size <= 0)
		return -1;
	fd = open(toent, O_WRONLY | O_CREAT, 0660);
	if (fd < 0)
		return -1;
	size = write(fd, buf, size);
	if (size > 0)
		ftruncate(fd, size);
	close(fd);
	if (size <= 0)
		return -1;
	return 0;
}

int
do_save_brc(char *path, int all)
{
	char ent[1024], newent[1024];
	time_t nowtime;
	int t1a;
	DIR *dirp;
	struct dirent *direntp;

	nowtime = time(NULL);

	printf("processing %s\n", path);
	dirp = opendir(path);
	if (dirp == NULL)
		return 0;
	while ((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_name[1] == 0)
			continue;
		sprintf(ent, "%s/%s", path, direntp->d_name);
		if (strchr(direntp->d_name, '.')) {
			if (direntp->d_name[0] != '.'
			    && nowtime - file_rtime(ent) > 3600)
				unlink(ent);
			continue;
		}
		sethomefile(newent, direntp->d_name, "brc");
		//sethomefile(tmpent, direntp->d_name, "brc.tmp");
		//printf("%s\nnewent %s\ntmpent %s\n", ent, newent, tmpent);
//              t1 = file_time(ent);
		t1a = file_rtime(ent);
//              t2 = file_time(newent);
/*		if (nowtime - t1 > 500 && t1 >= t2)
		{
		}*/
		if (all || nowtime - t1a > 7200) {
			printf("%s\n", direntp->d_name);
			if (copy_brc(ent, newent) < 0)
				continue;
			if (nowtime - t1a > 7200)
				unlink(ent);
		}
	}
	closedir(dirp);
	return 0;
}

int
main(int argc, char *argv[])
{
	char path[1024];
	time_t nowtime;
	int all = 0, i;

	if (argc >= 2 && !strcmp(argv[1], "all"))
		all = 1;
	chdir(MY_BBS_HOME);
	nowtime = time(NULL);
	printf("save_brc is running~\n");
	printf("\033[1mbbs home=%s now time = %s\033[0m\n", MY_BBS_HOME,
	       ctime(&nowtime));

	for (i = 'A'; i <= 'Z'; i++) {
		sprintf(path, PATHTMPBRC "/%c", i);
		do_save_brc(path, all);
	}
	for (i = '0'; i <= '9'; i++) {
		sprintf(path, PATHTMPBRC "/%c", i);
		do_save_brc(path, all);
	}
	return 0;
}
