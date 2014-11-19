//查找版面最靠后的mark文章      ylsdd 2002/4/3
#include "bbs.h"
#include "ythtlib.h"
#include "ythtbbs.h"
#include "www.h"

#define MAXFOUNDD 9
#define MAXAUTHOR 5
struct markeditem {
	char title[100];
	int n;
	char author[MAXAUTHOR][16];
	int thread;
	int marked;
};

struct markedlist {
	int n;
	int nmarked;
	struct markeditem mi[MAXFOUNDD];
};

int
denyUser(char *author)
{
	char buf[256], *ptr;
	FILE *fp;
	int retv = 0;
	if (!strcmp(author, "Anonymous"))
		return 1;

	fp = fopen("deny_users", "rt");
	if (!fp)
		return 0;
	while (fgets(buf, sizeof (buf), fp)) {
		ptr = strtok(buf, " \t\n\r");
		if (!ptr)
			continue;
		if (!strcasecmp(ptr, author)) {
			retv = 1;
			break;
		}
	}
	fclose(fp);
	return retv;
}

int
addmiauthor(struct markeditem *mi, char *author)
{
	int i;
	for (i = 0; i < mi->n; i++) {
		if (!strncmp(mi->author[i], author, sizeof (mi->author[i]))) {
			return mi->n;
		}
	}
	if (i >= MAXAUTHOR)
		return -1;
	strsncpy(mi->author[i], author, sizeof (mi->author[i]));
	mi->n++;
	return mi->n;
}

int
addmarkedlist(struct markedlist *ml, char *title, char *author, int thread,
	      int marked)
{
	int i;
	if (!marked && ml->n >= MAXFOUNDD)
		return ml->nmarked;
	if (marked && ml->n > ml->nmarked) {
		for (i = ml->n - 1; i >= 0; i--) {
			if (!ml->mi[i].marked)
				break;
		}
		for (; i < ml->n - 1; i++)
			ml->mi[i] = ml->mi[i + 1];
		bzero(&ml->mi[i], sizeof (ml->mi[0]));
		ml->n--;
	}
	for (i = 0; i < ml->n; i++) {
		if (thread == ml->mi[i].thread) {
			if (marked && !ml->mi[i].marked) {
				ml->mi[i].n = 0;
				ml->mi[i].marked = marked;
				ml->nmarked++;
			} else if (!marked && ml->mi[i].marked)
				return ml->nmarked;
			addmiauthor(&ml->mi[i], author);
			return ml->nmarked;
		}
	}
	if (i >= MAXFOUNDD)
		return ml->nmarked;
	strsncpy(ml->mi[i].title, title, sizeof (ml->mi[0].title));
	addmiauthor(&ml->mi[i], author);
	ml->mi[i].thread = thread;
	ml->mi[i].marked = marked;
	ml->n++;
	if (marked)
		ml->nmarked++;
	return ml->nmarked;
}

int
searchLastMark(char *filename, struct markedlist *ml, int addscore)
{
	struct fileheader fhdr;
	int fd, total, n, old = 0;
	time_t now;

	bzero(ml, sizeof (*ml));

	if ((total = file_size(filename) / sizeof (fhdr)) <= 0)
		return 0;

	time(&now);
	if ((fd = open(filename, O_RDONLY, 0)) == -1) {
		return 0;
	}
	for (n = total - 1; n >= 0 && total - n < 3000; n--) {
		if (lseek(fd, n * sizeof (fhdr), SEEK_SET) < 0)
			break;
		if (read(fd, &fhdr, sizeof (fhdr)) != sizeof (fhdr))
			break;
		if (now - fhdr.filetime > 3600 * 24 * 6) {
			old++;
			if (old > 4)
				break;
			continue;
		}
		if (!strcmp(fhdr.owner, "deliver")
		    && !(fhdr.accessed & FH_DIGEST)
		    && !(fhdr.accessed & FH_MARKED))
			continue;
		if (fhdr.owner[0] == '-'	//|| !strcmp(fhdr.owner, "deliver")
		    || strstr(fhdr.title, "[警告]")
		    || (fhdr.accessed & FH_DANGEROUS))
			continue;
		if (bytenum(fhdr.sizebyte) <= 150)
			continue;
		if (denyUser(fh2owner(&fhdr)))
			continue;
		if ((fhdr.accessed & FH_DIGEST) || (fhdr.accessed & FH_MARKED)
		    || (fhdr.hasvoted > 1 + addscore
			&& fhdr.staravg50 * (int) fhdr.hasvoted / 50 >
			4 + addscore * 2)) {
			if (addmarkedlist
			    (ml, fhdr.title, fh2owner(&fhdr), fhdr.thread,
			     1) >= MAXFOUNDD)
				break;
		} else if (bytenum(fhdr.sizebyte) > 200
			   || fhdr.thread == fhdr.filetime) {
			addmarkedlist(ml, fhdr.title, fh2owner(&fhdr),
				      fhdr.thread, 0);
		}
	}
	close(fd);
	return ml->n;
}

int
main()
{
	int b_fd, fdaux, i, bnum = -1;
	struct boardheader bh;
	int size, foundd, addscore;
	char buf[200], recfile[200];
	struct markedlist ml;
	struct boardaux boardaux;
	struct bbsinfo bbsinfo;
	FILE *fp;

	initbbsinfo(&bbsinfo);
	mkdir("wwwtmp/lastmark", 0770);
	size = sizeof (bh);

	chdir(MY_BBS_HOME);
	if ((b_fd = open(BOARDS, O_RDONLY)) == -1)
		return -1;
	if ((fdaux = open(BOARDAUX, O_RDWR | O_CREAT, 0660)) < 0)
		return -1;
	for (bnum = 0; read(b_fd, &bh, size) == size && bnum < MAXBOARD; bnum++) {
		if (!bh.filename[0])
			continue;
		sprintf(buf, MY_BBS_HOME "/boards/%s/.DIR", bh.filename);
		sprintf(recfile, MY_BBS_HOME "/wwwtmp/lastmark/%s",
			bh.filename);
		if (file_time(buf) < file_time(recfile)
		    && time(NULL) - file_time(recfile) < 3600 * 12)
			continue;
		fp = fopen(recfile, "w");
		lseek(fdaux, bnum * sizeof (boardaux), SEEK_SET);
		read(fdaux, &boardaux, sizeof (boardaux));
		boardaux.nlastmark = 0;
		addscore = 1;
		if (bbsinfo.bcacheshm->bcache[bnum].score > 10000)
			addscore++;
		if (bbsinfo.bcacheshm->bcache[bnum].score > 50000)
			addscore++;
		searchLastMark(buf, &ml, addscore);
		if (ml.n > 0) {
			int nunmarked = 0;
			for (foundd = 0;
			     foundd < ml.n && boardaux.nlastmark < MAXLASTMARK;
			     foundd++) {
				char buf[60];
				struct lastmark *lm =
				    &boardaux.lastmark[boardaux.nlastmark];
				buf[0] = 0;
				i = ml.mi[foundd].n;
				while (i > 0) {
					i--;
					if (strlen(buf) +
					    strlen(ml.mi[foundd].author[i]) +
					    2 >= sizeof (buf))
						break;
					strcat(buf, ml.mi[foundd].author[i]);
					if (i)
						strcat(buf, " ");
				}
				strsncpy(lm->authors, buf,
					 sizeof (lm->authors));
				strsncpy(lm->title, ml.mi[foundd].title,
					 sizeof (lm->title));
				lm->thread = ml.mi[foundd].thread;
				lm->marked = ml.mi[foundd].marked;
				if (lm->marked || nunmarked < 6) {
					boardaux.nlastmark++;
					if (!lm->marked)
						nunmarked++;
				}
				if (ml.mi[foundd].marked) {
					fputs(buf, fp);
					fprintf(fp, "\t%d",
						ml.mi[foundd].thread);
					fprintf(fp, "\t%s\n",
						ml.mi[foundd].title);
				}
			}
		}
		fclose(fp);
		lseek(fdaux, bnum * sizeof (boardaux), SEEK_SET);
		write(fdaux, &boardaux, sizeof (boardaux));
	}
	close(b_fd);
	return 0;
}
