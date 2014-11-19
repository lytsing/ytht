#include <stdio.h>
#include "bbs.h"
#include "ythtlib.h"
#include "www.h"
#define MAXFILTER 100
#define MAXUPDATE 200

struct lastmarkl {
	char title[100];
	char board[30];
	char boardtitle[80];
	int thread;
	int filetime;
	char sec[10];
	int brdn;
} lastmarklist[MAXUPDATE + 1];

char filterstr[MAXFILTER][60];
int nfilterstr = 0;

struct bbsinfo bbsinfo;
struct BCACHE *shm_bcache;

int
initfilter(char *filename)
{
	FILE *fp;
	char *ptr, *p;
	fp = fopen(filename, "r");
	if (!fp)
		return -1;
	for (nfilterstr = 0; nfilterstr < MAXFILTER; nfilterstr++) {
		ptr = filterstr[nfilterstr];
		if (fgets
		    (filterstr[nfilterstr], sizeof (filterstr[nfilterstr]),
		     fp) == NULL)
			break;
		if ((p = strchr(ptr, '\n')) != NULL)
			*p = 0;
		if ((p = strchr(ptr, '\r')) != NULL)
			*p = 0;
		while (ptr[0] == ' ')
			memmove(&ptr[0], &ptr[1], strlen(ptr));
		while (ptr[0] != 0 && ptr[strlen(ptr) - 1] == ' ')
			ptr[strlen(ptr) - 1] = 0;
		if (!ptr[0])
			nfilterstr--;
	}
	fclose(fp);
	return 0;
}

int
dofilter(char *str)
{
	int i;
	if (!nfilterstr)
		return 0;
	for (i = 0; i < nfilterstr; i++) {
		if (strstr(str, filterstr[i]))
			break;
	}
	if (i < nfilterstr)
		return 1;
	return 0;
}

int
testperm(struct boardmem *x)
{
	if (x->header.flag & CLUB_FLAG) {
		if ((x->header.flag & CLOSECLUB_FLAG))
			return 0;
		return 1;
	}
	if (x->header.level == 0)
		return 1;
	if (x->header.level & (PERM_POSTMASK | PERM_NOZAP))
		return 1;
	return 0;
}

int
cmpfiletime(struct lastmarkl *a, struct lastmarkl *b)
{
	return b->filetime - a->filetime;
}

int
printitem(FILE * fp, int i)
{
	const struct sectree *sec;
	sec = getsectree(lastmarklist[i].sec);
	fprintf(fp, "<tr><td><a href='bbsnt?th=%d&B=%s&T=%s'>",
		lastmarklist[i].thread, lastmarklist[i].board,
		encode_url(lastmarklist[i].title));
	fprintf(fp, "%s</a></td><td><a href='home?B=%s' class=blk>",
		void1(nohtml(lastmarklist[i].title)), lastmarklist[i].board);
	fprintf(fp,
		"<nobr>%s</nobr></a></td><td><a href='boa?secstr=%s'><nobr>%s</nobr></a></td></tr>\n",
		lastmarklist[i].boardtitle, lastmarklist[i].sec,
		shm_bcache->bcache[lastmarklist[i].brdn].header.type);
	return 0;
}

int
testsubsec(int i)
{
	struct boardmem x;
	char *ptr;
	x = shm_bcache->bcache[lastmarklist[i].brdn];
	if (x.header.sec1[0] == 'Y')
		ptr = x.header.sec1;
	else
		ptr = x.header.sec2;
	if (!strncmp(ptr, "YH", 2) || !strncmp(ptr, "YW", 2))
		return 0;
	return 1;

}

int
doprint(char *filename, int num, int addhead, int doublecol)
{
	FILE *fp;
	int i, count, col = 0;

	fp = fopen(filename, "w");
	if (fp == NULL)
		return -1;
	if (addhead)
		fprintf(fp,
			"<br><center><FONT class=f3><b>--== 最新更新 ==--</b></FONT><br><br>");
	if (doublecol)
		fprintf(fp, "<table><tr><td valign=top>");
      HALF:
	fprintf(fp,
		"<table border=1><tr><td>标题</td><td>版面</td><td><nobr>分类</nobr></td></tr>\n");
	for (i = 0, count = 0; i < MAXUPDATE && count < num; i++) {
		if (!doublecol || testsubsec(i) == col) {
			printitem(fp, i);
			count++;
		}
	}
	fprintf(fp, "</table>\n");
	if (doublecol && !col) {
		fprintf(fp, "</td><td valign=top>");
		col = 1;
		goto HALF;
	}
	if (doublecol)
		fprintf(fp, "</td></tr></table>");
	if (addhead)
		fprintf(fp, "</center>");
	fclose(fp);
	return 0;
}

int
main()
{
	int i, j;
	struct boardmem x;
	struct mmapfile mf = { ptr:NULL };
	int now_t, total, start, count;
	struct fileheader *ptr;
	char path[80];
	now_t = time(NULL);
	if(initbbsinfo(&bbsinfo)<0)
		exit(1);
	shm_bcache = bbsinfo.bcacheshm;
	initfilter(MY_BBS_HOME "/etc/filtertitle");
	for (i = 0; i < shm_bcache->number; i++) {
		x = shm_bcache->bcache[i];
		if (x.header.sec1[0] != 'Y' && x.header.sec2[0] != 'Y')
			continue;
		if (!testperm(&x))
			continue;
		sprintf(path, MY_BBS_HOME "/boards/%s/.DIR", x.header.filename);
		if (mmapfile(path, &mf) < 0)
			continue;
		total = mf.size / sizeof (struct fileheader);	//共 0 - total 条目
		start = Search_Bin((struct fileheader *)mf.ptr, now_t - 500000, 0, total - 1);
		if (start < 0) {
			start = -(start + 1);
		}
		if (start >= total) {	//没有新文章
			mmapfile(NULL, &mf);
			continue;
		}
		for (j = total - 1, count = 0; j >= start && count < 15; j--) {
			ptr = (struct fileheader *) mf.ptr + j;

			if (!(ptr->accessed & FH_MARKED)
			    || !(ptr->filetime == ptr->thread)
			    || dofilter(ptr->title))
				continue;
			if (ptr->filetime <=
			    lastmarklist[MAXUPDATE - 1].filetime) break;

			strncpy(lastmarklist[MAXUPDATE].title, ptr->title, 60);
			strncpy(lastmarklist[MAXUPDATE].board,
				x.header.filename, 30);
			strncpy(lastmarklist[MAXUPDATE].boardtitle,
				x.header.title, 30);
			lastmarklist[MAXUPDATE].thread = ptr->thread;
			lastmarklist[MAXUPDATE].filetime = ptr->filetime;
			strncpy(lastmarklist[MAXUPDATE].sec, x.header.sec1, 5);
			lastmarklist[MAXUPDATE].brdn = i;
			qsort(lastmarklist, MAXUPDATE + 1,
			      sizeof (struct lastmarkl), (void *) cmpfiletime);
			count++;
		}
		mmapfile(NULL, &mf);
	}
	doprint(MY_BBS_HOME "/wwwtmp/lastupdate.secY.new", 14, 0, 1);
	rename(MY_BBS_HOME "/wwwtmp/lastupdate.secY.new",
	       MY_BBS_HOME "/wwwtmp/lastupdate.secY");
	doprint(MY_BBS_HOME "/wwwtmp/longupdate.secY.new", 200, 1, 0);
	rename(MY_BBS_HOME "/wwwtmp/longupdate.secY.new",
	       MY_BBS_HOME "/wwwtmp/longupdate.secY");
	return 0;
}
