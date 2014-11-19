#include "ythtlib.h"
#include "nbstat.h"
#include "bbs.h"
#include "sysrecord.h"

diction amd;

struct amstat {
	char userid[20];
	int pass;
	int reject;
	int skip;
	int del;
};


void
am_pass(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct amstat *data;
	int npass, nreject, nskip, ndel;
	sscanf(other, "%d %d %d %d", &npass, &nreject, &nskip, &ndel);
	a = finddic(amd, user);
	if (a != NULL) {
		data = a->value;
		data->pass+=npass;
		data->reject+=nreject;
		data->skip+=nskip;
		data->del+=ndel;
	}
}

struct action_f am[] = {
	{"pass", am_pass},
	{0}
};

int
am_cmp(struct amstat *a, struct amstat *b)
{
	int totala, totalb;
	totala=a->pass + a->reject + a->skip + a->del;
	totalb=b->pass + b->reject + b->skip + b->del;
	if(totala!=totalb)
		return (totalb-totala);
	return (b->pass - a->pass);
}

void
am_exit()
{
	FILE *fp;
	int amc, i, total, pass=0, reject=0, skip=0, del=0;
	char tmpfile[80];
	struct amstat *data;
	amc = getdic(amd, sizeof (struct amstat), (void *) &data);
	if (amc < 0) {
		errlog("Can't malloc am result!");
		exit(-1);
	}
	qsort(data, amc, sizeof (struct amstat), (void *) am_cmp);
	sprintf(tmpfile, "%s%s", MY_BBS_HOME, "/bbstmpfs/tmp/amstat");
	fp = fopen(tmpfile, "w");
	if (!fp) {
		errlog("can't open mail in am_exit!");
		exit(-1);
	}
	fprintf(fp, "    帐号管理员       通过   退回   删除   合计   通过率   跳过\n");
	fprintf(fp, "----------------------------------------------------------------\n");
	for(i=0; i<amc; i++)
	{
		total=data->pass + data->reject + data->del;
		fprintf(fp, " %2d %-14s %6d %6d %6d %6d ", i+1, data->userid, data->pass, data->reject, data->del, total);
		if(total)
			fprintf(fp, "%7.2f%%", data->pass * 100.0 / total);
		else
			fprintf(fp, "  --.--%%");
		fprintf(fp, " %6d\n", data->skip);
		pass+=data->pass;
		reject+=data->reject;
		skip+=data->skip;
		del+=data->del;
		data++;
	}
	total=pass+reject+del;
	fprintf(fp, "----------------------------------------------------------------\n");
	fprintf(fp, "    %-14s %6d %6d %6d %6d %7.2f%% %6d\n", "总计", pass, reject, del, total, pass*100.0/total, skip);
	fclose(fp);
	postfile(tmpfile, "deliver", "IDmanage", "上周帐号管理员工作情况统计");
	unlink(tmpfile);
}

void
am_init()
{
	struct hword *tmp;
	struct userec x;
	FILE *fp;
	if((fp=fopen(MY_BBS_HOME "/" PASSFILE, "r"))==NULL){
		errlog("Can't open PASSFILE in am_init!");
		exit(-1);
	}
	while(fread(&x, 1, sizeof(struct userec), fp)){
		if(!USERPERM(&x, PERM_ACCOUNTS) || USERPERM(&x, PERM_SYSOP))
			continue;
		tmp = malloc(sizeof (struct hword));
		if (tmp == NULL) {
			errlog("Can't malloc in am_init!");
			exit(-1);
		}
		strncpy(tmp->str, x.userid, STRLEN - 1);
		tmp->value = malloc(sizeof (struct amstat));
		if (tmp->value == NULL) {
			errlog("Can't malloc value in am_init!");
			exit(-1);
		}
		memset(tmp->value, 0, sizeof (struct amstat));
		strncpy(((struct amstat *) (tmp->value))->userid,
				x.userid, IDLEN);
		insertdic(amd, tmp);
	}
	fclose(fp);
	register_stat(am, am_exit);
}
