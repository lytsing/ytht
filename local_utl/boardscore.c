#include "ythtlib.h"
#include "bbs.h"
#include "nbstat.h"

#define BSSTAT        MY_BBS_HOME"/0Announce/bbslist/boardscore"
#define BRDSCORE_TOP5 MY_BBS_HOME"/0Announce/bbslist/boardtop5"
#define BRDSCORE_FILE MY_BBS_HOME"/etc/boardscore.history"
#define NOADDSCORE      MY_BBS_HOME"/no_add_boardscore"
diction bsstat;

int addscore;

struct bscore {
	char board[20];
	char expname[STRLEN];
	diction user;
	diction author;
	int person;
	int post;
	int stay;
	int noread;
	int score;
	int score_yesterday;
	int score_lastweek;
	char sec;		
};


struct bsincrease {
	float percent_ystd;
	float percent_lstwk;
};
/* to save the percentage of increasement */

struct sstay {
	int stay;
	int day;
};

struct bshistory {
	char boardname[21];
	int score_history[7];
};

struct UTMPFILE *shm_utmp;
void
bs_use(int day, char *time, char *user, char *other)
{
	struct hword *a, *tmp;
	struct bscore *data;
	char board[30], staytime[10];
	char *temp[2] = { board, staytime };
	int i, stay, countstay;
//      int hour = 2;
	i = mystrtok(other, ' ', temp, 2);
	a = finddic(bsstat, board);
	stay = atoi(staytime);
//      if (stay > hour * 3600)
//              stay = hour * 3600;
	if (a != NULL) {
		data = a->value;
		a = finddic(data->user, user);
		if (a == NULL) {
			tmp = malloc(sizeof (struct hword));
			snprintf(tmp->str, STRLEN, "%s", user);
			tmp->value = calloc(1, sizeof (struct sstay));
			((struct sstay *) (tmp->value))->stay = stay;
			((struct sstay *) (tmp->value))->day = day;
			countstay = stay;
			insertdic(data->user, tmp);
			data->person++;
		} else {
			countstay = stay;
			if (((struct sstay *) (a->value))->day != day) {
				((struct sstay *) (a->value))->stay = 0;
				((struct sstay *) (a->value))->day = day;
			}
			stay += ((struct sstay *) (a->value))->stay;
/*			if (stay > hour * 3600) {
				countstay =
				    hour * 3600 -
				    ((struct sstay *) (a->value))->stay;
				stay = hour * 3600;
			}
*/
			((struct sstay *) (a->value))->stay = stay;
		}
		data->stay += countstay;
	}
}

int
bs_cmp(struct bscore *b, struct bscore *a)
{
	return a->score - b->score;
}



int
bssec_cmp(struct bscore *b, struct bscore *a)
{
	return (b->sec - a->sec);
}

/* for boardscore compare by sec*/

void
bs_post(int day, char *time, char *user, char *other)
{
	struct hword *a, *tmp;
	struct bscore *data;
	char board[30], title[128];
	char *temp[2] = { board, title };
	int i;
	i = mystrtok(other, ' ', temp, 2);
	a = finddic(bsstat, board);
	if (a != NULL) {
		data = a->value;
		a = finddic(data->author, user);
		if (a == NULL) {
			tmp = malloc(sizeof (struct hword));
			snprintf(tmp->str, STRLEN, "%s", user);
			insertdic(data->author, tmp);
			data->post++;
		}
	}
}

struct action_f bs[] = {
	{"use", bs_use},
	{"post", bs_post},
	{0}
};

int
bs_genfile(int is_new)
{
	FILE *fp;
	struct bshistory *bsh;
	struct tm *t;
	time_t timer;
	int i, j;
	int numboards = brdshm->number;

	bsh = malloc(numboards * sizeof (struct bshistory));	//e.g.  1000 * 49 = 49KB
	if (bsh == NULL) {
		errlog("Can not malloc.(boardscore.c/bs_genfile()");
		return -1;
	}
	memset(bsh, 0, sizeof (struct bshistory));

	if (!is_new) {
		fp = fopen(BRDSCORE_FILE, "rb");
		if (fp == NULL) {
			errlog("Can not open file.(boardscore.c/bs_genfile())");
			return -1;
		}
		fread(bsh, sizeof (struct bshistory), numboards, fp);
		fclose(fp);
	}
	timer = time(NULL);
	t = localtime(&timer);
	for (i = 0; i < numboards; i++) {
		strcpy((bsh + i)->boardname, bcache[i].header.filename);
		if (is_new) {
			for (j = 0; j < 7; j++) {
				(bsh + i)->score_history[j] = bcache[i].score;
			}
		} else {
			(bsh + i)->score_history[t->tm_wday] = bcache[i].score;
		}
	}
	fp = fopen(BRDSCORE_FILE, "wb");
	if (fp == NULL) {
		errlog("Can not open file.(boardscore.c/bs_genfile())");
		return -1;
	}
	fwrite(bsh, sizeof (struct bshistory), numboards, fp);
	fclose(fp);
	return 0;
}

void
bs_exit()
{
	int buc, i, j, count, numboards;	
	int boards;
	struct bscore *data;
	FILE *fp = NULL, *fp_his = NULL;
	struct hword *a;
	struct bshistory bsh;
	char bslog[128];
	char tailbuf[256];
	struct tm *t;
	time_t timer;
	float percent_ystd, percent_lstwk, swaptmp, percentall_ystd, percentall_lstwk;
	int yy, mm, dd, ss, tt, cc;
	char currSec = 0;
	int seccount[MAXSUBSEC];
	int seccounthistory[MAXSUBSEC];	
	int bbsallcount=0,bbsallcount_ystd=0,bbsallcount_lstwk=0;
	char secname[MAXSUBSEC];
	int secnum;	
	struct bscore *pointbsi[5], *pointbsitmp;
	struct bsincrease bsi[5];

	buc = getdic(bsstat, sizeof (struct bscore), (void *) &data);
	if (buc < 0) {
		errlog("Can't malloc bu result!");
		exit(-1);
	}
	if (!file_exist(BRDSCORE_FILE)) {
		bs_genfile(1);
	}

	fp_his = fopen(BRDSCORE_FILE, "rb");
	if (fp_his == NULL) {
		errlog("Can not open file.(boardscore.c/bs_exit())");
		exit(-1);
	}
	timer = time(NULL);
	t = localtime(&timer);
	numboards = brdshm->number;
	for (i = 0; i < numboards; i++) {
		fread(&bsh, sizeof (struct bshistory), 1, fp_his);
		a = finddic(bsstat, bcache[i].header.filename);
		if (a != NULL) {
			data = a->value;
			data->score =
			    data->stay / 1000 + data->person * 2 +
			    data->post * 10;
			sprintf(bslog, MY_BBS_HOME "/boards/%s/boardscore",
				bcache[i].header.filename);
			if (addscore) {
				fp = fopen(bslog, "a");
				if (fp) {
					fprintf(fp, "%d %d %d %d\n",
						t->tm_year + 1900,
						t->tm_mon + 1, t->tm_mday,
						data->score);
					fclose(fp);
				}
			}
			sprintf(tailbuf,
				"/usr/bin/tail -n 7 %s > %s/bbstmpfs/tmp/boardscore.tail",
				bslog, MY_BBS_HOME);
			system(tailbuf);
			fp = fopen(MY_BBS_HOME "/bbstmpfs/tmp/boardscore.tail",
				   "r");
			tt = 0;
			cc = 0;
			if (fp) {
				while (fgets(tailbuf, sizeof (tailbuf), fp)) {
					if (4 !=
					    sscanf(tailbuf, "%d%d%d%d", &yy,
						   &mm, &dd, &ss))
						continue;
					cc++;
					tt += ss;
				}
				fclose(fp);
			}
			unlink(MY_BBS_HOME "/bbstmpfs/tmp/boardscore.tail");
			if (cc == 0)
				cc = 1;
			data->score = tt;
			bcache[i].score = data->score;
			if (!strcmp(bsh.boardname, data->board)) {
				data->score_yesterday =
				    bsh.score_history[(t->tm_wday + 6) % 7];
				data->score_lastweek =
				    bsh.score_history[t->tm_wday];
			}
		}
	}
	fclose(fp_his);

	fp = fopen(BSSTAT, "w");
	if (fp == NULL) {
		errlog("can't open bsstat output!");
		exit(-1);
	}
	fprintf(fp, "\033[1;37m名次 %-15.15s%-20.20s    %s\t%s\t%s\033[m\n",
		"讨论区名称", "中文叙述", "昨日人气", "较前日", "较上周");

	buc = getdic(bsstat, sizeof (struct bscore), (void *) &data);
	qsort(data, buc, sizeof (struct bscore), (void *) bs_cmp);
	count = 0;
	boards = 0;
	for (i = 0; i < buc; i++) {
		if (!data->noread) {
			if (data->score_yesterday != 0
			    && data->score_lastweek != 0) {
				percent_ystd =
				    (data->score -
				     data->score_yesterday) * 100.0 /
				    data->score_yesterday;
				percent_lstwk =
				    (data->score -
				     data->score_lastweek) * 100.0 /
				    data->score_lastweek;
				fprintf(fp,
					"\033[1m%4d\033[m %-15.15s%-20.20s    %8d    %5.1f%%\t%5.1f%%\n",
					i + 1, data->board, data->expname,
					data->score, percent_ystd,
					percent_lstwk);
			} else {
				fprintf(fp,
					"\033[1m%4d\033[m %-15.15s%-20.20s    %8d     --    \t--\n",
					i + 1, data->board, data->expname,
					data->score);
			}
			count += data->score;
			boards++;
		}
		data++;
	}
	fprintf(fp, "\033[1m%4d\033[m %-15.15s%-20.20s    %8d\n",
		boards, "Average", "平均", count / (boards ? boards : 1));
	fclose(fp);
	bs_genfile(0);

	

	data -= buc;
	qsort(data, buc, sizeof (struct bscore), (void *) bssec_cmp);

	fp = fopen(BRDSCORE_TOP5, "w");
	if (fp == NULL) {
		errlog("can't open boardscore_top5 output!");
		exit(-1);
	}

	fprintf(fp, "\033[1;37m名次 %-15.15s%-20.20s    %s\t%s\t%s\033[m\n",
		"讨论区名称", "中文叙述", "昨日人气", "较前日", "较上周");

	secnum = -1;
	memset(seccount, 0, sizeof (seccount));
	memset(seccounthistory, 0, sizeof (seccounthistory));
	for (i = 0; i < buc; i++, data++) {
		if (data->noread)
			continue;
		if (currSec != data->sec) {
			currSec = data->sec;
			if (secnum != -1) {
				for (j = 0; j < 5; j++) {
					if (bsi[j].percent_lstwk > 0.0001) {
						fprintf(fp,
							"\033[1m%4d\033[m %-15.15s%-20.20s    %8d    %5.1f%%\t%5.1f%%\n",
							j + 1,
							pointbsi[j]->board,
							pointbsi[j]->expname,
							pointbsi[j]->score,
							bsi[j].percent_ystd,
							bsi[j].percent_lstwk);
					} else {
						fprintf(fp,
							"---------------------------------------------------------\n");
					}
				}
			}
			fprintf(fp,
				"\n\033[1;36m%c区 版面增长率排名\033[m\n\n",
				currSec);
			secnum++;
			secname[secnum] = currSec;
			memset(bsi, 0, sizeof (bsi));
		}

		seccount[secnum] += data->score;
		seccounthistory[secnum] += data->score_lastweek;
		bbsallcount += data->score;
		bbsallcount_ystd += data->score_yesterday;
		bbsallcount_lstwk += data->score_lastweek;
		if (data->score_yesterday == 0 || data->score_lastweek == 0)
			continue;
		percent_ystd =
		    (data->score -
		     data->score_yesterday) * 100.0 / data->score_yesterday;
		percent_lstwk =
		    (data->score -
		     data->score_lastweek) * 100.0 / data->score_lastweek;
		if (data->score <= 3000
		    || percent_lstwk <= bsi[4].percent_lstwk)
			continue;
		//added by bridged
		if (((data->score > 30000) && (percent_ystd > 6) && (percent_lstwk > 15))
		    || ((data->score > 10000) && (percent_ystd > 9) && (percent_lstwk > 20))
		    || ((data->score > 3000) && (percent_ystd > 15) && (percent_lstwk > 30)))
			fprintf(fp,"\033[1;35m推荐：%s版(%s) 今日人气：%7d 增长 日：%5.1f%%\t周：%5.1f%%\033[m\n",
					data->board, data->expname, data->score, 
					percent_ystd, percent_lstwk);
				
		bsi[4].percent_lstwk = percent_lstwk;
		bsi[4].percent_ystd = percent_ystd;
		pointbsi[4] = data;
		for (j = 3; j >= 0; j--) {
			if (bsi[j].percent_lstwk < bsi[j + 1].percent_lstwk) {
				pointbsitmp = pointbsi[j + 1];
				pointbsi[j + 1] = pointbsi[j];
				pointbsi[j] = pointbsitmp;
				swaptmp = bsi[j + 1].percent_lstwk;
				bsi[j + 1].percent_lstwk = bsi[j].percent_lstwk;
				bsi[j].percent_lstwk = swaptmp;
				swaptmp = bsi[j + 1].percent_ystd;
				bsi[j + 1].percent_ystd = bsi[j].percent_ystd;
				bsi[j].percent_ystd = swaptmp;
			}
		}
		/*
		   fprintf(fp,
		   "\033[1m%4d\033[m %-15.15s%-20.20s    %8d    %5.1f%%\t%5.1f%%\n",
		   i + 1, data->board, data->expname,
		   data->score, percent_ystd,
		   percent_lstwk);
		 */
	}			//end for i=0;i<buc;i++

	for (j = 0; j < 5; j++) {
		if (bsi[j].percent_lstwk > 0.0001) {
			fprintf(fp,
				"\033[1m%4d\033[m %-15.15s%-20.20s    %8d    %5.1f%%\t%5.1f%%\n",
				j + 1,
				pointbsi[j]->board,
				pointbsi[j]->expname,
				pointbsi[j]->score,
				bsi[j].percent_ystd, bsi[j].percent_lstwk);
		} else {
			fprintf(fp,
				"---------------------------------------------------------\n");
		}
	}

	fprintf(fp, "\n\n\n\n各区人气增长率情况\n");
	fprintf(fp, "\n区名     %-15.15s%-10.10s%-10.10s\n\n","本周人气","上周人气","增长率");
	for (j = 0; j < MAXSUBSEC; j++) {
		if (seccount[j] != 0) {
			swaptmp =
			    (seccount[j] -
			     seccounthistory[j]) * 100.0 / seccounthistory[j];
			fprintf(fp, "\033[1m%c%15d \t%15d\t%8.3f%%\n", secname[j],
				seccount[j], seccounthistory[j], swaptmp);
		}
	}
        percentall_ystd = (bbsallcount - bbsallcount_ystd) * 100.0 / bbsallcount;
	percentall_lstwk = (bbsallcount - bbsallcount_lstwk) * 100.0 / bbsallcount;
	fprintf(fp, "\n\n\n全站人气总计： %15d。昨日增长： %8.3f%%，上周增长： %8.3f%%。\n", 
			bbsallcount, percentall_ystd, percentall_lstwk);
	fclose(fp);
	bbsinfo.utmpshm->mc.ave_score = count / (boards ? boards : 1);

}

void
bs_init(int day)
{
	int i;
	struct hword *tmp;
	int numboards;
	numboards = brdshm->number;

	addscore = !file_exist(NOADDSCORE);
	for (i = 0; i < numboards; i++) {
		if (!bcache[i].header.filename[0])
			continue;
		tmp = malloc(sizeof (struct hword));
		if (tmp == NULL) {
			errlog("Can't malloc in board_init!");
			exit(-1);
		}
		snprintf(tmp->str, 20, "%s", bcache[i].header.filename);
		tmp->value = calloc(1, sizeof (struct bscore));
		if (tmp->value == NULL) {
			errlog("Can't malloc value in board_init!");
			exit(-1);
		}
		strcpy(((struct bscore *) (tmp->value))->board, tmp->str);
		snprintf(((struct bscore *) (tmp->value))->expname, STRLEN,
			 "[%s] %s", bcache[i].header.type,
			 bcache[i].header.title);
		((struct bscore *) (tmp->value))->noread =
		    boardnoread(&(bcache[i].header));
		((struct bscore *) (tmp->value))->sec =
		    bcache[i].header.sec1[0];
		insertdic(bsstat, tmp);
	}
	register_stat(bs, bs_exit);
}
