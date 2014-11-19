#include "ythtlib.h"
#include "nbstat.h"
#include "bbs.h"

#define BUSTAT MY_BBS_HOME"/0Announce/bbslist/board3"
#define BUSSTAT MY_BBS_HOME"/0Announce/groups/GROUP_0/syssecurity/board3"

diction bustat;

struct buser {
	char board[20];
	char expname[STRLEN];
	diction user;
	int used;
	int noread;
};

void
bu_use(int day, char *time, char *user, char *other)
{
	struct hword *a, *tmp;
	struct buser *data;
	char board[30], staytime[10];
	char *temp[2] = { board, staytime };
	int i;
	i = mystrtok(other, ' ', temp, 2);
	a = finddic(bustat, board);
	if (a != NULL) {
		data = a->value;
		a = finddic(data->user, user);
		if (a == NULL) {
			tmp = malloc(sizeof (struct hword));
			snprintf(tmp->str, STRLEN, "%s", user);
			insertdic(data->user, tmp);
			data->used++;
		}
	}
}

struct action_f bu[] = {
	{"use", bu_use},
	{0}
};

int
bu_cmp(struct buser *b, struct buser *a)
{
	return a->used - b->used;
}

void
bu_exit()
{
	int i, buc, count;
	struct buser *data;
	FILE *fp1, *fp2, *fp;

	fp1 = fopen(BUSTAT, "w");
	fp2 = fopen(BUSSTAT, "w");
	if (fp1 == NULL || fp2 == NULL) {
		errlog("faint,can't open bustat output!");
		exit(-1);
	}
	fp = fp1;
	for (i = 0; i < 2; i++) {
		fprintf(fp, "\033[1;37m���� %-15.15s%-28.28s %5s  \033[m\n",
			"����������", "��������", "����ID��");
		fp = fp2;
	}
	buc = getdic(bustat, sizeof (struct buser), (void *) &data);
	if (buc < 0) {
		errlog("Can't malloc bu result!");
		exit(-1);
	}
	qsort(data, buc, sizeof (struct buser), (void *)bu_cmp);
	count = 0;
	for (i = 0; i < buc; i++) {
		if (data->noread)
			fp = fp2;
		else
			fp = fp1;
		fprintf(fp, "\033[1m%4d\033[m %-15.15s%-28.28s %5d \n", i + 1,
			data->board, data->expname, data->used);
		count += data->used;
		data++;
	}
	fp = fp1;
	for (i = 0; i < 2; i++) {
		fprintf(fp, "\033[1m%4d\033[m %-15.15s%-28.28s %5d \n", buc,
			"Average", "ƽ��", count / buc);
		fp = fp2;
	}
	fclose(fp1);
	fclose(fp2);
}

void
bu_init()
{
	int i;
	struct hword *tmp;
	int numboards;
	numboards = brdshm->number;
	for (i = 0; i < numboards; i++) {
		if (!bcache[i].header.filename[0])
			continue;
		tmp = malloc(sizeof (struct hword));
		if (tmp == NULL) {
			errlog("Can't malloc in board_init!");
			exit(-1);
		}
		snprintf(tmp->str, 20, "%s", bcache[i].header.filename);
		tmp->value = calloc(1, sizeof (struct buser));
		if (tmp->value == NULL) {
			errlog("Can't malloc value in board_init!");
			exit(-1);
		}
		strcpy(((struct buser *) (tmp->value))->board, tmp->str);
		snprintf(((struct buser *) (tmp->value))->expname, STRLEN, "%s",
			 bcache[i].header.title);
		((struct buser *) (tmp->value))->noread =
		    boardnoread(&(bcache[i].header));
		insertdic(bustat, tmp);
	}
	register_stat(bu, bu_exit);
}
