#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "ythtlib.h"
#include "ythtbbs.h"

#define MAXDEPTH 6
int ncachetitle = 0;
char cachetitles[MAXDEPTH][300];

int
makeindex(FILE * wfp, char *path, char *prefix, int indextype, int depth)
{
	FILE *fp;
	char filen[800], str[500];
	char title[100], *ptr;
	struct stat sbuf;
	int i, n, hide = 0;
	time_t t;
	while ((ptr = strstr(prefix, "  ")))
		memcpy(ptr, "��", 2);

	//printf("======%d=%d=====\n",depth, indextype ); 
	t = time(NULL);

	if (depth >= MAXDEPTH)
		return 0;

	sprintf(filen, "%s/.Names", path);
	fp = fopen(filen, "r");
	if (fp == NULL)
		return -1;

	n = 0;
	while (fgets(str, 500, fp) != NULL) {
		if (strstr(str, "Name=") == str) {
			bzero(title, sizeof (title));
			strcpy(title, str + strlen("Name="));
			if (strncmp(title, "<HIDE>", 6) == 0 ||
			    strstr(title + 38, "(BM: SYSOPS)") != NULL ||
			    strstr(title + 38, "(BM: BMS)") != NULL) {
				hide = 1;
			} else
				hide = 0;
			title[38] = 0;
			for (i = strlen(title); i >= 0; i--) {
				if (strchr("\n\t\r", title[i]) != NULL) {
					title[i] = 0;
				}
			}
			for (i = strlen(title) - 1; i >= 0 && title[i] == ' ';
			     i--)
				title[i] = 0;
			continue;
		}
		if (hide)
			continue;
		if (strstr(str, "Path=~") != str)
			continue;
		n++;
		for (i = strlen(str); i >= strlen("Path=~"); i--) {
			if (strchr("\n\t\r /.", str[i]) == NULL)
				break;
			str[i] = '\0';
		}
		if (strlen(str) == strlen("Path=~"))
			continue;
		sprintf(filen, "%.300s%.200s", path, str + strlen("Path=~"));
		//printf("filen: %s\n", filen);
		if (lstat(filen, &sbuf) < 0)
			continue;
		if (S_ISDIR(sbuf.st_mode)) {
			char buf[500];
			int thisnew = 0;
			if (t - sbuf.st_mtime < 24 * 3600 * 3)
				thisnew = 1;
			if (indextype == 3) {
				if (thisnew) {
					for (i = 0; i < ncachetitle; i++)
						fprintf(wfp, "%s",
							cachetitles[i]);
					ncachetitle = 0;
					fprintf(wfp,
						"��%s%3d.[\033[33mĿ¼\033[m]%s\n",
						prefix, n, title);
				} else
					sprintf(cachetitles[ncachetitle++],
						"��%s%3d.[\033[33mĿ¼\033[m]%s\n",
						prefix, n, title);
			} else if (indextype == 2 && thisnew)
				fprintf(wfp, "��%s%3d.[\033[33mĿ¼\033[m]%s\n",
					prefix, n, title);
			else
				fprintf(wfp, "��%s%3d.[\033[33mĿ¼\033[m]%s\n",
					prefix, n, title);

			sprintf(buf, "%s%3d.", prefix, n);
			makeindex(wfp, filen, buf, indextype, depth + 1);
			if (!thisnew && indextype == 3 && ncachetitle > 0)
				ncachetitle--;
		} else if (S_ISREG(sbuf.st_mode)) {
			int thisnew = 0;
			if (t - sbuf.st_mtime < 24 * 3600 * 3
			    && !checktitle(title)) thisnew = 1;
			if (indextype == 3) {
				if (thisnew) {
					for (i = 0; i < ncachetitle; i++)
						fprintf(wfp, "%s",
							cachetitles[i]);
					ncachetitle = 0;
					fprintf(wfp,
						"��%s%3d.[\033[33m�ļ�\033[m]%s\n",
						prefix, n, title);
				}
			} else if (indextype == 1)
				fprintf(wfp, "%s%s%3d.[\033[33m�ļ�\033[m]%s\n",
					thisnew ? "��" : "��", prefix, n,
					title);
		} else
			continue;
	}
	fclose(fp);
	return 0;
}

int
searchindexfile(char *path)
{
	FILE *fp, *wfp;
	char filen[800], str[500];
	struct stat sbuf;
	int indextype, i;
	time_t t = time(NULL);
	//printf("search %s\n",path);
	sprintf(filen, "%s/.Names", path);
	fp = fopen(filen, "r");
	if (fp == NULL) {
		return -1;
	}

	while (fgets(str, 500, fp) != NULL) {
		if (strstr(str, "Name=") != str)
			continue;
		indextype = checktitle(str + 5);
		if (fgets(str, 500, fp) == NULL)
			break;
		if (strstr(str, "Path=~") != str)
			break;
		for (i = strlen(str); i >= strlen("Path=~"); i--) {
			if (strchr("\n\t\r /.", str[i]) == NULL)
				break;
			str[i] = '\0';
		}
		if (strlen(str) == strlen("Path=~"))
			continue;
		sprintf(filen, "%.300s%.200s", path, str + strlen("Path=~"));
		//printf("filen: %s\n", filen);
		if (lstat(filen, &sbuf) < 0)
			continue;
		if (S_ISDIR(sbuf.st_mode)) {
			searchindexfile(filen);
			continue;
		}
		if (!S_ISREG(sbuf.st_mode) || !indextype)
			continue;
		/*if (file_time(filen)!=0 && (do_testtime(file_time(filen), path, 1, 20*60)==0)) {
		   printf("no need to update");
		   continue;
		   } */
		wfp = fopen(filen, "w");
		if (wfp == NULL)
			continue;
		switch (indextype) {
		case 1:
			fprintf(wfp, "�������������� �� %s", ctime(&t));
			fprintf(wfp, "����ݸ��£������ڣ�\n");
			break;
		case 2:
			fprintf(wfp, "������Ŀ¼���� �� %s", ctime(&t));
			fprintf(wfp, "����ݸ��£������ڣ�\n");
			break;
		case 3:
			fprintf(wfp, "�������������� �� %s", ctime(&t));
			break;
		}
		fprintf(wfp, "����������������������������������������\n");
		ncachetitle = 0;
		makeindex(wfp, path, "", indextype, 0);
		fprintf(wfp, "����������������������������������������\n");
		fclose(wfp);
	}
	return fclose(fp);
}

int
main(int argc, char *argv[])
{
	//searchindexfile("/home/bbs/0Announce/groups/GROUP_0/test");
	if (argc < 2)
		return -1;
	searchindexfile(argv[1]);
	return 0;
}
