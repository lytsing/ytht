#include "bbs.h"
#include "www.h"
#include <math.h>
#define NBOARD 15

struct bbsinfo bbsinfo;
int matrix1[MAXBOARD][MAXBOARD];
float contri[MAXBOARD];
int recommcount[MAXBOARD];
int contriorder[MAXBOARD];

int
cmpcontri(const void *a, const void *b)
{
	return -contri[*(int *) a] + contri[*(int *) b];
}

int
getboards(char *buf, char boards[NBOARD][30])
{
	char *ptr;
	int i = 0;
	ptr = strtok(buf, "\t \n\r");
	while (ptr) {
		strncpy(boards[i], ptr, 30);
		boards[i][29] = 0;
		i++;
		if (i >= NBOARD)
			break;
		ptr = strtok(NULL, "\t \n\r");
	}
	return i;
}

int
testperm(struct boardmem *x)
{
	if (!x->header.filename[0])
		return 0;
	if (x->header.flag & CLUB_FLAG) {
		if ((x->header.flag & CLOSECLUB_FLAG))
			return 0;
		return 1;
	}
	if (x->header.level == 0)
		return 1;
	if (x->header.level & PERM_NOZAP)
		return 1;
	return 0;
}

int
getboardid(char *board)
{
	int i;
	for (i = 0; i < MAXBOARD; i++) {
		if (!strncmp
		    (board, bbsinfo.bcache[i].header.filename, BRC_STRLEN - 1))
			break;
	}
	if (i == MAXBOARD)
		return -1;
	return i;
}

int
maxfinder(int op, int id, float value)
{
	static int ids[40];
	static float values[40];
	static int n = 0;
	float temp;
	int i, j, tempid;
	switch (op) {
	case 0:
		break;
	case 1:
		n = 0;
		return 0;
	case 2:
		return n;
	case 3:
		return ids[id];
	}

	for (i = 0; i < n; i++) {
		if (value > values[i])
			break;
	}
	if (i < 40) {
		for (j = i; j < 40 && j <= n; j++) {
			temp = values[j];
			tempid = ids[j];
			values[j] = value;
			ids[j] = id;
			value = temp;
			id = tempid;
		}
		if (n < 40)
			n++;
	}
	return 0;
}

int
main()
{
	char buf[1000];
	char boards[NBOARD][30];
	int boardid[NBOARD];
	int num, i, j, k, fdaux;
	FILE *fp;
	struct boardaux boardaux;
	chdir(MY_BBS_HOME);
	if (initbbsinfo(&bbsinfo) < 0) {
		errlog("Failed to attach shm.");
		return -1;
	}
	if (uhash_uptime() == 0) {
		errlog("Failed to access uhash.");
		return -1;
	}

	if ((fdaux = open(BOARDAUX, O_RDWR | O_CREAT, 0660)) < 0) {
		errlog("Can't open " BOARDAUX ".");
		return -1;
	}

	while (fgets(buf, sizeof (buf), stdin)) {
		num = getboards(buf, boards);
		for (i = 0, j = 0; i < num; i++, j++) {
			boardid[j] = getboardid(boards[i]);
			if (boardid[j] < 0)
				j--;
		}
		num = j;
		for (i = 0; i < num; i++)
			contri[boardid[i]] += 1;
		for (i = 0; i < num; i++) {
			for (j = i + 1; j < num; j++) {
				matrix1[boardid[i]][boardid[j]]++;
				matrix1[boardid[j]][boardid[i]]++;
			}
		}
	}
	for (i = 0; i < MAXBOARD; i++) {
		contriorder[i] = i;
	}
	qsort(contriorder, MAXBOARD, sizeof (contriorder[0]), cmpcontri);

	for (k = 0; k < MAXBOARD; k++) {
		i = contriorder[k];
		if (!contri[i])
			break;
		maxfinder(1, 0, 0);
		for (j = 0; j < MAXBOARD; j++) {
			if (!testperm(&bbsinfo.bcache[j])
			    || recommcount[j] >= 10)
				continue;
			maxfinder(0, j,
				  (matrix1[i][j] - 4) / (contri[i] / 17 +
							 contri[j]));
		}
		num = maxfinder(2, 0, 0);
		if (!num)
			continue;

		lseek(fdaux, i * sizeof (boardaux), SEEK_SET);
		read(fdaux, &boardaux, sizeof (boardaux));
		boardaux.nrelate = 0;

		sprintf(buf, MY_BBS_HOME "/wwwtmp/relate/%s",
			bbsinfo.bcache[i].header.filename);
		fp = fopen(buf, "w");
		if (!fp)
			continue;
		fprintf(fp, "∆‰À¸∞Ê√Ê");

		for (j = 0; j < num && j < 6; j++) {
			int n;
			if (j == 5 && num > 7)
				j = 7;
			n = maxfinder(3, j, 0);
			strsncpy(boardaux.relate[boardaux.nrelate].filename,
				 bbsinfo.bcache[n].header.filename,
				 sizeof (boardaux.
					 relate[boardaux.nrelate].filename));
			strsncpy(boardaux.relate[boardaux.nrelate].title,
				 bbsinfo.bcache[n].header.title,
				 sizeof (boardaux.
					 relate[boardaux.nrelate].title));
			boardaux.nrelate++;
			fprintf(fp, " <a href=bbsdoc?B=%s>&lt;%s&gt;</a>",
				bbsinfo.bcache[n].header.filename,
				nohtml(bbsinfo.bcache[n].header.title));
			recommcount[n]++;
		}
		fprintf(fp, "\n");
		fclose(fp);
		lseek(fdaux, i * sizeof (boardaux), SEEK_SET);
		write(fdaux, &boardaux, sizeof (boardaux));
		printf("%d\n", i);
	}
#if 0
	for (i = 0; i < MAXBOARD; i++) {
		if (!contri[i] || !testperm(&bbsinfo.bcache[i]))
			continue;
		printf("%d\t%d\t%d\t%s\n", recommcount[i], (int) contri[i],
		       (int) (recommcount[i] * 1e5 / contri[i]),
		       bbsinfo.bcache[i].header.filename);
	}
#endif
	close(fdaux);
	return 0;
}
