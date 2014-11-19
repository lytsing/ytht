#include <stdio.h>
#include "bbs.h"
#include "ythtlib.h"
#include "www.h"
#define MAXFILTER 100
#define NBRDITEM 3

struct lastmarkl {
	char title[NBRDITEM][100];
	int thread[NBRDITEM];
	int count;
	char board[30];
	char boardtitle[80];
	int bnum;
	int score;
	char sec1[10];
	char sec2[10];
} lastmarklist[MAXBOARD];

struct boardlist {
	int brdn;
	int used;
	int fakescore;
} boardlist[MAXBOARD];

char *encode_url(unsigned char *s);
char *nohtml(char *s);
char *userid_str(char *s);
int initfilter(char *filename);
int dofilter(char *str);
int readlastmark(struct lastmarkl *lm);
int testperm(struct boardmem *x);
int main(void);
int makeallseclastmark(const struct sectree *sec);
int printlastmarkline(FILE * fp, struct lastmarkl *lm, int i);
int printseclastmark(FILE * fp, const struct sectree *sec, int nline);

int numlastmarkb;
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
readlastmark(struct lastmarkl *lm)
{
	char buf[200], *ptr;
	FILE *fp;
	sprintf(buf, MY_BBS_HOME "/wwwtmp/lastmark/%s", lm->board);
	if ((fp = fopen(buf, "r")) == NULL)
		goto END;
	lm->count = 0;
	while (fgets(buf, 200, fp)) {
		if (dofilter(buf))
			continue;
		ptr = strchr(buf, '\t');
		if (ptr == NULL)
			break;
		ptr++;
		lm->thread[lm->count] = atoi(ptr);
		if (lm->thread[lm->count] == 0)
			break;
		ptr = strchr(ptr, '\t');
		if (!ptr)
			break;
		ptr++;
		strsncpy(lm->title[lm->count], ptr, sizeof (lm->title[0]));
		ptr = strchr(lm->title[lm->count], '\n');
		if (ptr)
			*ptr = 0;
		lm->count++;
		if (lm->count >= NBRDITEM)
			break;
	}
	fclose(fp);
      END:
	return lm->count;
}

int
makeboardlist(const struct sectree *sec, struct boardlist *bl, int max)
{
	int i, count = 0, l;
	char admin[40], admin2[40], bmt[40], bmt2[40];
	snprintf(admin, sizeof (admin), "%sadmin", sec->basestr);
	snprintf(admin2, sizeof (admin2), "%s_admin", sec->basestr);
	snprintf(bmt, sizeof (bmt), "%s_BMTraining", sec->basestr);
	snprintf(bmt2, sizeof (bmt2), "%sBMTraining", sec->basestr);
	l = strlen(sec->basestr);
	for (i = 0; i < numlastmarkb && count < max; i++) {
		if (strncmp(lastmarklist[i].sec1, sec->basestr, l) &&
		    (l == 1 || !lastmarklist[i].sec2 ||
		     strncmp(lastmarklist[i].sec2, sec->basestr, l)))
			continue;
		if (!strcasecmp(lastmarklist[i].board, admin) ||
		    !strcasecmp(lastmarklist[i].board, admin2) ||
		    !strcasecmp(lastmarklist[i].board, bmt) ||
		    !strcasecmp(lastmarklist[i].board, bmt2))
			continue;
		bl[count].brdn = i;
		bl[count].fakescore = lastmarklist[i].score;
		count++;
	}
	return count;
}

int
fakecmp(const void *p0, const void *p1)
{
	return ((struct boardlist *) p1)->fakescore -
	    ((struct boardlist *) p0)->fakescore;
}

void
fakesort(struct boardlist *bl, int count)
{
	int i;
	qsort(bl, count, sizeof (*bl), fakecmp);
	if (count < 3 + 2)
		return;
	for (i = 3; i < count; i++)
		bl[i].fakescore = rand();
	qsort(&bl[3], count - 3, sizeof (*bl), fakecmp);
}

int
testperm(struct boardmem *x)
{
	if (!x->header.filename[0])
		return 0;
	if (x->header.flag & CLUB_FLAG) {
		if ((x->header.flag & CLOSECLUB_FLAG)
		    && (x->header.flag & CLUBLEVEL_FLAG))
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
main()
{
	int i;
	struct boardmem x;
	if (initbbsinfo(&bbsinfo) < 0)
		return -1;
	shm_bcache = bbsinfo.bcacheshm;
	if (shm_bcache->number <= 0)
		return -1;
	initfilter(MY_BBS_HOME "/etc/filtertitle");
	srand(time(NULL));
	for (i = 0; i < shm_bcache->number; i++) {
		x = shm_bcache->bcache[i];
		if (!testperm(&x))
			continue;
		bzero(&lastmarklist[numlastmarkb], sizeof (lastmarklist[0]));
		strcpy(lastmarklist[numlastmarkb].board, x.header.filename);
		strcpy(lastmarklist[numlastmarkb].boardtitle, x.header.title);
		lastmarklist[numlastmarkb].bnum = i;
		lastmarklist[numlastmarkb].score = x.score;
		strsncpy(lastmarklist[numlastmarkb].sec1, x.header.sec1,
			 sizeof (lastmarklist[numlastmarkb].sec1));
		strsncpy(lastmarklist[numlastmarkb].sec2, x.header.sec2,
			 sizeof (lastmarklist[numlastmarkb].sec2));
		if (!(x.header.flag & CLOSECLUB_FLAG))
			readlastmark(&lastmarklist[numlastmarkb]);
		numlastmarkb++;
	}
	makeallseclastmark(&sectree);
	return 0;
}

int
printseclastmark(FILE * fp, const struct sectree *sec, int nline)
{
	int count, i, j, n, m, l, pcount;
	bzero(boardlist, sizeof (boardlist));
	count = makeboardlist(sec, boardlist, MAXBOARD);
	fakesort(boardlist, count);
	fprintf(fp, "btb('%s', '%s');", sec->basestr, scriptstr(sec->title));
	for (i = 0, n = 0, pcount = 0; i < NBRDITEM && n < nline; i++) {
		for (j = 0; j < count && n < nline; j++) {
			m = boardlist[j].brdn;
			if (lastmarklist[m].count <= i)
				continue;
			//printlastmarkline(fp, &lastmarklist[m], i);
			n++;
			if (i == 0)
				pcount++;
			boardlist[j].used++;
		}
	}
	if (strlen(sec->basestr) >= 2) {
		for (i = 0; i < count; i++) {
			m = boardlist[i].brdn;
			if (lastmarklist[m].count && !boardlist[i].used) {
				boardlist[i].used++;
				pcount++;
			}
		}
	}
	for (i = 0; i < count; i++) {
		m = boardlist[i].brdn;
		for (j = 0; j < boardlist[i].used; j++)
			printlastmarkline(fp, &lastmarklist[m], j);
	}
	if (pcount < count) {
		if (sec->parent != &sectree)
			l = 20;
		else
			l = 3;
		fprintf(fp, "ltb('%s',new Array(", sec->basestr);
		for (i = 0, j = 0; i < count && j < l; i++) {
			if (boardlist[i].used)
				continue;
			m = boardlist[i].brdn;
			fprintf(fp, "%s'%d','%s'", j ? "," : "",
				lastmarklist[m].bnum,
				scriptstr(lastmarklist[m].boardtitle));
			j++;
		}
		fprintf(fp, "));");
		n++;
	}

	if (!count)
		fprintf(fp,
			"document.write('<tr><td colspan=2>&nbsp;</td></tr>');");
	fprintf(fp, "etb();\n");
	return n;
}

int
makeallseclastmark(const struct sectree *sec)
{
	int i, n, nline, separate;
	const struct sectree *subsec;
	char buf[200], tmp[200];
	FILE *fp;
	if (!sec->introstr[0])
		return -1;
	sprintf(tmp, "%s/wwwtmp/lastmark.js.sec%s.new", MY_BBS_HOME,
		sec->basestr);
	printf("writting %s\n", tmp);
	fp = fopen(tmp, "w");
	if (!fp)
		return -1;
	fprintf(fp, "<!--\n"
		"document.write('<table><tr><td valign=top width=49%%>');\n");
	n = strlen(sec->introstr);
	if (strchr(sec->introstr, '|'))
		separate = strchr(sec->introstr, '|') - sec->introstr;
	else
		separate = n / 2;
	nline = 8;
	if (n <= 4 || sec->basestr[0])
		nline = 13;
	for (i = 0; i < n; i++) {
		if (i == separate)
			fprintf(fp,
				"document.write('</td><td valign=top width=49%%>');\n");
		if (sec->introstr[i] == '|')
			continue;
		sprintf(buf, "%s%c", sec->basestr, sec->introstr[i]);
		subsec = getsectree(buf);
		if (subsec == &sectree)
			continue;
		if (!strcmp(subsec->basestr, "T")
		    || !strcmp(subsec->basestr, "4"))
			printseclastmark(fp, subsec, 11);
		else
			printseclastmark(fp, subsec, nline);
	}
	fprintf(fp, "document.write('</td></tr></table>');\n//-->\n");
	fclose(fp);
	sprintf(buf, "%s/wwwtmp/lastmark.js.sec%s", MY_BBS_HOME, sec->basestr);
	rename(tmp, buf);
	for (i = 0; i < sec->nsubsec; i++)
		makeallseclastmark(sec->subsec[i]);
	return 0;
}

int
printlastmarkline(FILE * fp, struct lastmarkl *lm, int i)
{
	char *title = lm->title[i];
	fprintf(fp, "itb('%d','%s',%d,", lm->bnum, scriptstr(lm->boardtitle),
		lm->thread[i]);
	if (!strncmp(title, "[зЊди] ", 7) && strlen(title) > 20)
		title += 7;
	if (strlen(title) > 45)
		title[45] = 0;
	fprintf(fp, "'%s');", scriptstr(title));
	return 0;
}
