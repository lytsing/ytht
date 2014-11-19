//查找和删除丢失到board目录下的文章     ylsdd   2001/6/16

#include "bbs.h"
#include "ythtbbs.h"
#define MAXFILE 500
#define MINAGE 10000		//at least 10000 sec old
#define HASHSIZE 30
int nfile[HASHSIZE];
char allpost[HASHSIZE][MAXFILE][20];
int refcount[HASHSIZE][MAXFILE];
char otherfile[200];
int allfile = 0, allref = 0, alllost = 0, unknownfn = 0, nindexitem = 0,
    nstrangeitem = 0;
time_t nowtime;

int
hash(char *postname)
{
	int i = atoi(postname + 2);
	return i % HASHSIZE;
}

int
fixabnormalfn(char *path, char *file)
{
	char newfilepath[256], oldfilepath[256];
	int l, t;
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		return 0;
	if (!isdigit(file[3]))
		return 0;
	if (strlen(file) >= 20)
		return 0;
	l = strlen(file);
	if (file[l - 1] == 'A')
		return 0;
	t = atoi(file + 2) + 1;
	sprintf(oldfilepath, "%s/%s", path, file);
	strcpy(newfilepath, path);
	t = trycreatefile(newfilepath, "M.%d.A", t, 100);
	if (t <= 0)
		return -1;
	return rename(oldfilepath, newfilepath);
}

int
ispostfilename(char *file)
{
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		return 0;
	if (!strstr(file, ".A"))
		return 0;
	if (!isdigit(file[3]))
		return 0;
	if (strlen(file) >= 20)
		return 0;
	return 1;
}

int
countfile(struct fileheader *fhdr, void *farg)
{
	int i, h;
	char *fname;
	nindexitem++;
	if (fhdr->filetime == 0) {
		nstrangeitem++;
		return 0;
	}
	fname = fh2fname(fhdr);
	h = hash(fname);
	for (i = 0; i < nfile[h]; i++) {
		if (strcmp(allpost[h][i], fname))
			continue;
		refcount[h][i]++;
		break;
	}
	return 0;
}

int
getallpost(char *path)
{
	DIR *dirp;
	struct dirent *direntp;
	int h;
	dirp = opendir(path);
	if (dirp == NULL)
		return -1;
	while ((direntp = readdir(dirp)) != NULL) {
		if (direntp->d_name[0] == '.')
			continue;
		fixabnormalfn(path, direntp->d_name);
		if (ispostfilename(direntp->d_name)) {
			h = hash(direntp->d_name);
			if (nfile[h] >= MAXFILE) {
				printf("啊?!");
				exit(0);
			}
			refcount[h][nfile[h]] = 0;
			strcpy(allpost[h][nfile[h]++], direntp->d_name);
			continue;
		}
		if (!strcmp(direntp->d_name, "deny_users"))
			continue;
		if (!strcmp(direntp->d_name, "deny_anony"))
			continue;
		unknownfn++;
		if (strlen(otherfile) + strlen(direntp->d_name) + 1 <
		    sizeof (otherfile)) {
			strcat(otherfile, " ");
			strcat(otherfile, direntp->d_name);
		}
	}
	closedir(dirp);
	return 0;
}

int
useindexfile(char *filename)
{
	return new_apply_record(filename, sizeof (struct fileheader),
				(void *) countfile, NULL);
}

int
save_lost(char *path)
{
	int i, h, total, totalref, lost, t;
	char buf[MAXPATHLEN];
	char *user;
	user = strrchr(path, '/');
	if (user == NULL)
		return 0;
	user++;
	if (*user == 0)
		return 0;
	for (h = 0, total = 0, totalref = 0, lost = 0; h < HASHSIZE; h++) {
		total += nfile[h];
		for (i = 0; i < nfile[h]; i++) {
			totalref += refcount[h][i];
			if (!refcount[h][i]) {
				lost++;
				t = atoi(allpost[h][i] + 2);
				if (nowtime - t < MINAGE) {
					continue;
				}
				sprintf(buf, "%s/%s", path, allpost[h][i]);
				if (file_size(buf) == 0) {
					unlink(buf);
					continue;
				}
				if (system_mail_file(buf, user, "系统发现的遗失文件", "deliver") >= 0)
					unlink(buf);
			}
		}
	}
	allfile += total;
	allref += totalref;
	alllost += lost;
	if (lost > 0)
		printf("%s: total %d, refcount %d, %d file(s) was lost\n", path,
		       total, totalref, lost);
	if (strlen(otherfile) > 0)
		printf("%s\n", otherfile);
	return 0;
}

int
find_lost_mail(char *path)
{
	char buf[1024];
	int i;
	for (i = 0; i < HASHSIZE; i++) {
		nfile[i] = 0;
	}
	otherfile[0] = 0;
	if (getallpost(path) < 0)
		return -1;
	sprintf(buf, "%s/.DIR", path);
	if (file_isfile(buf))
		if (useindexfile(buf) < 0)
			return -1;
	sprintf(buf, "%s/.DIGEST", path);
	if (file_isfile(buf))
		if (useindexfile(buf) < 0)
			return -1;
	sprintf(buf, "%s/.DELETED", path);
	if (file_isfile(buf))
		if (useindexfile(buf) < 0)
			return -1;
	sprintf(buf, "%s/.JUNK", path);
	if (file_isfile(buf))
		if (useindexfile(buf) < 0)
			return -1;
	save_lost(path);
	return 0;
}

struct bbsinfo bbsinfo;

int
main()
{
	char path[1024], ent[1024];
	DIR *dirp;
	struct dirent *direntp;
	struct stat st;
	char ch;

	if (initbbsinfo(&bbsinfo) < 0) {
		fprintf(stderr, "can't init bbinfo\n");
		return -1;
	}
	chdir(MY_BBS_HOME);
	nowtime = time(NULL);
	printf("find_lost_mail is running~\n");
	printf("\033[1mbbs home=%s now time = %s\033[0m\n", MY_BBS_HOME,
	       ctime(&nowtime));
	for (ch = 'A'; ch <= 'Z'; ch++) {
		sprintf(path, MY_BBS_HOME "/mail/%c", ch);
		printf("processing %s\n", path);
		dirp = opendir(path);
		if (dirp == NULL)
			continue;
		while ((direntp = readdir(dirp)) != NULL) {
			if (direntp->d_name[0] == '.')
				continue;
			sprintf(ent, "%s/%s", path, direntp->d_name);
			if (!strcmp(direntp->d_name, "delete")
			    || !strcmp(direntp->d_name, "SYSOP"))
				continue;
			if (!(stat(ent, &st) == 0 && S_ISDIR(st.st_mode)))
				continue;
			if (find_lost_mail(ent) < 0)
				printf(" %s failed\n", direntp->d_name);
		}
		closedir(dirp);
	}
	printf("allfile %d, allref %d, alllost %d\n", allfile, allref, alllost);
	printf("unknownfn %d, nindexitem %d, nstrangeitem %d\n", unknownfn,
	       nindexitem, nstrangeitem);
	return 0;
}
