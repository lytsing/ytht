#include "ythtlib.h"
#include "nbstat.h"
#include "bbs.h"
#include <math.h>

#define BMSTAT MY_BBS_HOME"/0Announce/bmstat"
#define BMSTATA MY_BBS_HOME"/0Announce/groups/GROUP_0/syssecurity/bmstata"
#define BMSTAND MY_BBS_HOME"/etc/bmstand"
#define BMSTAT_HISTORY_FILE MY_BBS_HOME"/etc/bmstat.history"
#define MAX_RULE_NUM 256
#define MAX_ELEM_NUM 256

diction bmd;

struct bmstat {
	char board[24];
	char class[10];
	char userid[20];
	char noread;
	int inboard;
	int stay;
	int post;
	int digest;
	int mark;
	int range;
	int delete;
	int import;
	int move;
	int same;
	int deny;
	int unmark;
	int undigest;
	int boardscore;
	/* added by churinga: 1 char to indicate sec type (1st level) */
	char sec;
	/* added by cojie for enhanced bmstat */
	short bidx;
	char bmidx;
};

struct bmrule {
	char *elem[MAX_ELEM_NUM];
	int num;
	int condition;
	int limit;
} rule[MAX_RULE_NUM];

struct rulegrp {
	char name[24];
	int necessary;
	int nrule;
	int rule[MAX_RULE_NUM];
} grp[MAX_RULE_NUM];

int ngrp = -1;			/* -1 to indicate an error */

struct posttarget {
	char sec;
	char target[STRLEN];
	char unused[3];
} target[MAX_RULE_NUM];

int ntarget = 1;		/* target[0] is always used to store the default target */

void *bmstat_history_mem_rd = NULL;
void *bmstat_history_mem_wr = NULL;

void
bm_posted(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], title[128];
	char *tmp[2] = { board, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 2);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->post++;
		}
	}
}

void
bm_use(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], staytime[30];
	char *tmp[2] = { board, staytime };
	int i, j;
	i = mystrtok(other, ' ', tmp, 2);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->inboard++;
			data->stay += atoi(staytime);
		}
	}
}

void
bm_import(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], author[20], title[128];
	char *tmp[3] = { board, author, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->import++;
		}
	}
}

void
bm_move(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], path[512];
	char *tmp[2] = { board, path };
	int i, j;
	i = mystrtok(other, ' ', tmp, 2);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->move++;
		}
	}
}

void
bm_undigest(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], author[20], title[128];
	char *tmp[3] = { board, author, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->undigest++;
		}
	}
}

void
bm_digest(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], author[20], title[128];
	char *tmp[3] = { board, author, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->digest++;
		}
	}
}

void
bm_mark(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], author[20], title[128];
	char *tmp[3] = { board, author, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->mark++;
		}
	}
}

void
bm_unmark(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], author[20], title[128];
	char *tmp[3] = { board, author, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->unmark++;
		}
	}
}

void
bm_range(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], num1[10], num2[10];
	char *tmp[3] = { board, num1, num2 };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->range++;
		}
	}
}

void
bm_del(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], author[20], title[128];
	char *tmp[3] = { board, author, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 3);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->delete++;
		}
	}
}

void
bm_deny(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], denyuser[20];
	char *tmp[2] = { board, denyuser };
	int i, j;
	i = mystrtok(other, ' ', tmp, 2);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->deny++;
		}
	}
}

void
bm_same(int day, char *time, char *user, char *other)
{
	struct hword *a;
	struct bmstat *data;
	char buf[STRLEN];
	char board[30], title[128];
	char *tmp[2] = { board, title };
	int i, j;
	i = mystrtok(other, ' ', tmp, 2);
	for (j = 1; j <= 2; j++) {
		snprintf(buf, STRLEN - 1, "%d %s %s", j, board, user);
		a = finddic(bmd, buf);
		if (a != NULL) {
			data = a->value;
			data->same++;
		}
	}
}

struct action_f bm[] = {
	{"post", bm_posted},
	{"use", bm_use},
	{"import", bm_import},
	{"additem", bm_move},
	{"moveitem", bm_move},
	{"paste", bm_move},
	{"undigest", bm_undigest},
	{"digest", bm_digest},
	{"mark", bm_mark},
	{"unmark", bm_unmark},
	{"ranged", bm_range},
	{"del", bm_del},
	{"deny", bm_deny},
	{"sametitle", bm_same},
	{0}
};

int
bm_cmp(struct bmstat *b, struct bmstat *a)
{
	if (b->sec < a->sec)
		return -1;
	if (b->sec > a->sec)
		return 1;
	if (a->stay != b->stay)
		return (a->stay - b->stay);
	if (a->inboard != b->inboard)
		return (a->inboard - b->inboard);
	return a->post - b->post;
}

int
trim_blank(char *str)
{
	char *tmp;
	int cnt = 0;
	int i;
	int n = strlen(str);
	tmp = (char *) malloc(n + 1);

	for (i = 0; i < n; i++) {
		if (isspace(str[i]))
			continue;
		tmp[cnt++] = str[i];
	}
	tmp[cnt] = 0;
	strcpy(str, tmp);
	free(tmp);
	return cnt;
}

int
read_stand()
{
	int i, j;
	char tmp[256];
	int offset;
	const char *seps_plus = "+";
	const char *seps_colon = ":";
	const char *seps_eq = "=";
	char *token;
	FILE *fp_r;
	int cnt = 0;
	int exit = 0;

	fp_r = fopen(BMSTAND, "r");
	if (fp_r == NULL) {
		errlog("Can't open bmstand file!");
		return -1;
	}
	do {
		memset(&rule[cnt], 0, sizeof (struct bmrule));
		for (j = 0; j < 256; j++) {
			tmp[j] = fgetc(fp_r);
			if (tmp[j] == EOF) {
				tmp[j] = 0;
				exit = 1;
				break;
			}
			if (tmp[j] == '\n') {
				tmp[j] = 0;
				break;
			}
		}
		j = trim_blank(tmp);
		if (j == 0)	/* blank line with only a carriage char */
			continue;
		if (tmp[0] == '#')	/* comment */
			continue;
		if (tmp[0] == '$') {	/* post target rule */
			token = strstr(&tmp[1], seps_eq);
			if (token == NULL) {	/* bad format, need '=' */
				errlog
				    ("Bad file format: bmstand - post target specification needs '='!");
				return -1;
			}
			*token = 0;
			if (!strcmp(&tmp[1], "default")) {
				if (strlen(&token[1]) >= STRLEN) {
					errlog
					    ("Bad file format: bmstand - post target too long!");
					return -1;
				}
				strcpy(target[0].target, &token[1]);
			} else {
				if ((strlen(&tmp[1]) > 1) || (tmp[1] == 0)) {
					errlog
					    ("Bad file format: bmstand - a sec name should be only one char!");
					return -1;
				}
				target[ntarget].sec = tmp[1];
				if (strlen(&token[1]) >= STRLEN) {
					errlog
					    ("Bad file format: bmstand - post target too long!");
					return -1;
				}
				strcpy(target[ntarget].target, &token[1]);
				ntarget++;
				if (ntarget >= MAX_RULE_NUM) {
					errlog
					    ("Bad file format: bmstand - too many post targets!");
					return -1;
				}
			}
			continue;
		}
		if (tmp[0] == '[') {	/* group title */
			ngrp++;
			token = strstr(&tmp[1], seps_colon);
			if (token == NULL) {	/* no necessity sign, default is necessary */
				grp[ngrp].necessary = 1;
				tmp[strlen(tmp) - 1] = 0;
			} else {
				*token = 0;
				if (strlen(&tmp[1]) >= 20) {
					errlog
					    ("Bad file format: bmstand - group title too long!");
					fclose(fp_r);
					return -1;
				}
				if (*(token + 1) == '0')	/* not necessary */
					grp[ngrp].necessary = 0;
				else
					grp[ngrp].necessary = 1;
				strcpy(grp[ngrp].name, &tmp[1]);
				grp[ngrp].nrule = 0;
			}
			continue;
		}
		/* rule */
		/* decide whether there is condition */
		if (ngrp < 0) {
			errlog
			    ("Bad file format: bmstand - at least one rule was not assigned to a group!");
			fclose(fp_r);
			return -1;
		}
		token = strstr(tmp, seps_colon);
		if (token != NULL) {	/* has condition */
			*token = 0;
			rule[cnt].condition = atoi(tmp);
			offset = strlen(tmp) + 1;
		} else {
			rule[cnt].condition = -1;	/* no condition */
			offset = 0;
		}
		for (i = offset; i < j; i++)
			if (tmp[i] == '>')
				break;
		if (i == j) {
			errlog("Bad file format: bmstand");
			fclose(fp_r);
			return -1;
		}
		tmp[i] = 0;	/* seperate the string into two parts */
		token = strtok(&tmp[offset], seps_plus);
		while (token != NULL) {
			rule[cnt].elem[rule[cnt].num] =
			    (char *) malloc(strlen(token + 1));
			strcpy(rule[cnt].elem[rule[cnt].num], token);
			rule[cnt].num++;
			if (rule[cnt].num >= MAX_ELEM_NUM) {
				errlog
				    ("Bad file format: bmstand - too many elements in one rule!");
				fclose(fp_r);
				return -1;
			}
			token = strtok(NULL, seps_plus);
		}
		rule[cnt].limit = atoi(&tmp[i + 1]);
		grp[ngrp].rule[grp[ngrp].nrule] = cnt;
		grp[ngrp].nrule++;
		cnt++;
		if (cnt >= MAX_RULE_NUM) {
			errlog("Bad file format: bmstand - too many rules!");
			fclose(fp_r);
			return -1;
		}
	} while (!exit);
	fclose(fp_r);
	return cnt;
}

struct bm_eva *
get_bm_eva(struct bmstat *data, int fail_flag, int score_thisweek)
{
//从加载在内存中的bmstat history中返回一个bm的统计档案
	int bidx, bmidx;
	size_t offset;
	struct board_bmstat *brdbms_rd, *brdbms_wr;

	bidx = data->bidx;
	bmidx = data->bmidx;
	if (bidx < 0 || bidx >= MAXBOARD || bmidx >= BMNUM || bmidx < 0)	//越界
		return NULL;
	offset = bidx * sizeof (struct board_bmstat);
	brdbms_rd = bmstat_history_mem_rd + offset;
	brdbms_wr = bmstat_history_mem_wr + offset;

	/*  qsort() by sec 保证了一个版的版主们不会以交替sec身分进入此函数,于是
	   如果发现sec已被设置且与当前身分不符, 则说明该版所有版主已被处理一次,
	   故直接返回,避免统计2次 */
	if (brdbms_wr->sec != 0 && data->sec != brdbms_wr->sec)
		return &(brdbms_wr->bm[bmidx]);

	if (brdbms_wr->boardname[0] == 0) {	//若目标区域干净,直接写过去
		strcpy(brdbms_wr->boardname, data->board);
		brdbms_wr->boardscore = data->boardscore;
		brdbms_wr->sec = data->sec;
	}
	if (!strcmp(brdbms_rd->boardname, data->board)) {	//如果还是原来的版面
		int max_pos = (bmidx > 3) ? 16 : 4;	//大小版主
		int i;
		for (i = bmidx; i < max_pos; i++) {	//向后搜寻
			if (!strcmp(brdbms_rd->bm[i].userid, data->userid)) {
				memcpy(&(brdbms_wr->bm[bmidx]),
				       &(brdbms_rd->bm[i]),
				       sizeof (struct bm_eva));
				break;
			}
		}
		if (i == max_pos)	//找不到,即新版主
			strcpy(brdbms_wr->bm[bmidx].userid, data->userid);
	} else			//版面已变动,可直接写版主过去
		strcpy(brdbms_wr->bm[bmidx].userid, data->userid);
	//上面已经将版主档案放到了brdbms_wr

	if (brdbms_wr->bm[bmidx].leave)	//版主请假
		return &(brdbms_wr->bm[bmidx]);

	if (fail_flag == -1) {	// 如果本周考勤不合格
		brdbms_wr->bm[bmidx].total_week++;
		if (!brdbms_wr->bm[bmidx].last_pass)	// 上次也不合格
			brdbms_wr->bm[bmidx].week++;
		else		//上次合格,但是本次不合格
			brdbms_wr->bm[bmidx].week = 1;
		brdbms_wr->bm[bmidx].last_pass = 0;
	} else {		//本次合格,清除连续记录
		brdbms_wr->bm[bmidx].week = 0;
		brdbms_wr->bm[bmidx].last_pass = 1;
	}
	// 版主评分
	brdbms_wr->bm[bmidx].ave_score = (brdbms_wr->bm[bmidx].ave_score *
					  brdbms_wr->bm[bmidx].weight +
					  score_thisweek) /
	    (brdbms_wr->bm[bmidx].weight + 1);
	brdbms_wr->bm[bmidx].weight++;

	//下面两个if保证在brdbms_rd为空时正确执行
	if (brdbms_rd->boardname[0] == 0)
		strcpy(brdbms_rd->boardname, data->board);
	if (brdbms_rd->bm[bmidx].userid[0] == 0)
		memcpy(&(brdbms_rd->bm[bmidx]), &(brdbms_wr->bm[bmidx]),
		       sizeof (struct bm_eva));

	return &(brdbms_wr->bm[bmidx]);
}

int
load_bmstat_history_file()
{
	FILE *fp;
	size_t mem_size = sizeof (struct board_bmstat) * MAXBOARD;
	bmstat_history_mem_rd = malloc(mem_size);
	bmstat_history_mem_wr = malloc(mem_size);
	if (bmstat_history_mem_rd == NULL || bmstat_history_mem_wr == NULL) {
		errlog("Can't malloc for bmstat_history");
		exit(-1);
	}
	memset(bmstat_history_mem_rd, 0, mem_size);
	memset(bmstat_history_mem_wr, 0, mem_size);

	if (!file_exist(BMSTAT_HISTORY_FILE))
		return 0;
	fp = fopen(BMSTAT_HISTORY_FILE, "r");
	if (fp == NULL) {
		errlog("Can't open bmstat history file.");
		exit(-1);
	}
	fread(bmstat_history_mem_rd, sizeof (struct board_bmstat), MAXBOARD,
	      fp);
	fclose(fp);
	return 0;
}

int
save_bmstat_history_file()
{
	FILE *fp;
	char tmpfile[256];

	sprintf(tmpfile, "%s", MY_BBS_HOME "/bbstmpfs/tmp/bmstat.history.tmp");
	fp = fopen(tmpfile, "w");
	if (fp == NULL) {
		errlog("can not save bmstat history file.");
		exit(-1);
	}
	fwrite(bmstat_history_mem_wr, sizeof (struct board_bmstat), MAXBOARD,
	       fp);
	fclose(fp);
	rename(BMSTAT_HISTORY_FILE, MY_BBS_HOME "/etc/bmstat.history.bak");
	copyfile(tmpfile, BMSTAT_HISTORY_FILE);
	unlink(tmpfile);
	free(bmstat_history_mem_rd);
	free(bmstat_history_mem_wr);
	return 0;
}

float
limit_value(float value, float inf, float sup)
{
	if (value < inf)
		return inf;
	if (value > sup)
		return sup;
	return value;
}

int
make_bm_score(struct bmstat *data)
{
	int bidx;
	struct board_bmstat *brdbms_rd;
	float score_partI, score_partII = 0;
	float delta_score, weight;
	float time_score;
	float op1_score, op2_score, op3_score;

	bidx = data->bidx;
	if (bidx < 0 || bidx >= MAXBOARD)	//越界
		return -1;
	brdbms_rd = bmstat_history_mem_rd + bidx * sizeof (struct board_bmstat);

	//版面人气增长占 30%
	if (strcmp(brdbms_rd->boardname, data->board))	//新版
		score_partI = 20.0;
	else {
		delta_score = (data->boardscore - brdbms_rd->boardscore) /
		    (brdbms_rd->boardscore + 1.0);
		weight =
		    limit_value(sqrt(brdbms_rd->boardscore) / 36, 0.2, 5.0);
		score_partI =
		    limit_value(20 * (1 + delta_score * weight), 10, 30);
	}

	//停留时间占 10%
	time_score = limit_value(data->stay / 1260.0, 0, 10);
	if (data->stay > data->inboard * 3600)	//惩治挂版
		time_score /= 2.0;

	//版主操作占 60%
	op1_score =
	    (data->mark - data->unmark) + (data->digest - data->undigest) +
	    data->range + data->same + data->delete;
	op1_score = limit_value(op1_score, 0, 30);

	op2_score = data->post + data->deny;
	op2_score = limit_value(op2_score / 2.0, 0, 10);

	op3_score = data->import + data->move;
	op3_score = limit_value(op3_score * 2.0, 0, 20);

	if (data->bmidx < 4)	//大版主,精华区操作记分
		score_partII = time_score + op1_score + op2_score + op3_score;
	else
		score_partII = time_score + (op1_score + op2_score) * 1.5;
	//两部分之和即为版主评分
	return score_partI + score_partII + 0.5;
}

static void
send_warning_mail(char *boardname, struct bm_eva *bmeva)
{
	char content[256], title[STRLEN];

	snprintf(content, 255, "%s版版主%s:\n"
		 "    您在该版的版务考勤已经连续 \033[1;36m%d\033[0m 周不合格\n"
		 "    具体情况可到相应区务管理版或BM_Club版查看\n"
		 "    望今后能加强版务工作, " MY_BBS_NAME
		 " BBS 向您的辛勤工作表示感谢!\n\n", boardname, bmeva->userid,
		 bmeva->week);
	sprintf(title, "%s", "[系统通知]抱歉, 您的版务考勤不合格");
	system_mail_buf(content, strlen(content), bmeva->userid, title,
			"deliver");
}

int
check_rule(struct bmstat *data, int idx)
{
	int i, j, acc;
	int pass = 1;
	char *tmp;
	for (i = 0; i < grp[idx].nrule; i++) {
		acc = 0;	/* accumulator */
		if (rule[grp[idx].rule[i]].condition != -1)
			if (data->boardscore < rule[grp[idx].rule[i]].condition)
				continue;	/* condition not met */
		for (j = 0; j < rule[grp[idx].rule[i]].num; j++) {
			tmp = rule[grp[idx].rule[i]].elem[j];
			if (!strcmp(tmp, "m"))
				acc += data->mark;
			else if (!strcmp(tmp, "um"))
				acc += data->unmark;
			else if (!strcmp(tmp, "g"))
				acc += data->digest;
			else if (!strcmp(tmp, "ug"))
				acc += data->undigest;
			else if (!strcmp(tmp, "del"))
				acc += data->delete;
			else if (!strcmp(tmp, "post"))
				acc += data->post;
			else if (!strcmp(tmp, "enter"))
				acc += data->inboard;
			else if (!strcmp(tmp, "stay"))
				acc += data->stay;
			else if (!strcmp(tmp, "pop"))
				acc += data->boardscore;
			else if (!strcmp(tmp, "deny"))
				acc += data->deny;
			else if (!strcmp(tmp, "range"))
				acc += data->range;
			else if (!strcmp(tmp, "digest"))
				acc += data->import;
			else if (!strcmp(tmp, "move"))
				acc += data->move;
			else if (!strcmp(tmp, "same"))
				acc += data->same;
			else	/* not recognizable */
				return 0;
		}
		if (acc < rule[grp[idx].rule[i]].limit) {
			pass = 0;
			break;
		}
	}
	if (pass) {
		if (grp[idx].necessary)
			return 1;
		else
			return 2;
	}
	if (grp[idx].necessary)
		return -1;
	else
		return -2;
}

char *
select_post_target(char *fn, char sec)
{
	int i;
	if (ntarget == 1)
		strcpy(fn, target[0].target);
	else {
		for (i = 1; i < ntarget; i++)
			if (target[i].sec == sec) {
				strcpy(fn, target[i].target);
				return target[0].target;
			}
		strcpy(fn, target[0].target);
	}
	return NULL;
}

static void
gen_report_header(char currSec, FILE * fp_p, FILE * fp_p2, char *POST_NAME,
		  char *POST_PASS, char *fn)
{
	char format[16], buffer[1024];
	int j;

	snprintf(buffer, 1023, "#name: %s\n#pass: %s\n#board: %s\n"
		 "#title: 上周%c区版务考勤统计\n#localpost:\n\n"
		 "%c 区 版务 %s 考勤达标统计\n"
		 "按停留时间排序。 Y 为要求且达标，y 为不严格要求但达标，N 为不达标\n\n",
		 POST_NAME, POST_PASS, fn, currSec, currSec, timeperiod);
	fprintf(fp_p, "%s", buffer);
	fprintf(fp_p, "%-15s%-20s", "版主ID", "版面名称");	// 前两列是固定的:  userid, bname
	if (fp_p2) {
		fprintf(fp_p2, "%s", buffer);
		fprintf(fp_p2, "%-15s%-20s", "版主ID", "版面名称");
	}
	for (j = 0; j <= ngrp; j++) {
		sprintf(format, "%%-%ds", (int) (45 / (ngrp + 1)));
		fprintf(fp_p, format, grp[j].name);
		if (fp_p2)
			fprintf(fp_p2, format, grp[j].name);
	}
	fprintf(fp_p,
		"\n-------------------------------------------------------------------------------\n");
	if (fp_p2)
		fprintf(fp_p2,
			"\n-------------------------------------------------------------------------------\n");
}

void
bm_exit()
{
	int i, j, k, bmc, nrule, tmp, onvac;
	struct bmstat *data;
	FILE *fp1, *fp2, *fp;
	FILE *fp_p = NULL;	/* pipe for "mail box" */
	FILE *fp_p2 = NULL;
	char currSec = 0, *cross = NULL, fn[STRLEN];
	char POST_NAME[STRLEN], POST_PASS[STRLEN], buf[STRLEN];

	load_bmstat_history_file();
	target[0].sec = 0;
	strcpy(target[0].target, "BM_Club");
	readstrvalue(BMSTAND, "#[onVacation]", buf, 4);	//note: # is neccesary
	onvac = atoi(buf);
	nrule = read_stand();
#if 0
	printf("numrule: %d\n", nrule);
	printf("numgrp: %d\n", ngrp);
	for (i = 0; i <= ngrp; i++) {
		printf("grp#%d: name=%s, nrule=%d, nec=%d\n", i, grp[i].name,
		       grp[i].nrule, grp[i].necessary);
		for (j = 0; j < grp[i].nrule; j++) {
			printf("\trule#%d: cond=%d, limit=%d, nelem=%d\n",
			       grp[i].rule[j], rule[grp[i].rule[j]].condition,
			       rule[grp[i].rule[j]].limit,
			       rule[grp[i].rule[j]].num);
			for (k = 0; k < rule[grp[i].rule[j]].num; k++)
				printf("\t\telem#%d: name=%s\n", k,
				       rule[grp[i].rule[j]].elem[k]);
		}
	}
	for (i = 0; i < ntarget; i++) {
		printf("target#%d: sec=%c, target=%s\n", i, target[i].sec,
		       target[i].target);
	}
#endif
	fp = fopen(MY_BBS_HOME "/etc/bmstatpost", "r");
	if (fp == NULL) {
		errlog("Can't open poster info file!");
		exit(-1);
	}
	fscanf(fp, "%s", POST_NAME);
	fscanf(fp, "%s", POST_PASS);
	fclose(fp);
	fp1 = fopen(BMSTAT, "w");
	fp2 = fopen(BMSTATA, "w");
	if (fp1 == NULL || fp2 == NULL) {
		errlog("faint,can't open bmstat output!");
		exit(-1);
	}
	fp = fp1;
	for (i = 0; i < 2; i++) {
		fprintf(fp, "版主工作统计 %s 按分区归类，按停留时间排序.\n",
			timeperiod);
		fprintf(fp,
			"telnet方式阅读本文 按 / 可以向后搜索字符串,按 ? 可以向前搜索字符串.\n");
		fp = fp2;
	}
	bmc = getdic(bmd, sizeof (struct bmstat), (void *) &data);
	if (bmc < 0) {
		errlog("Can't malloc bm result!");
		exit(-1);
	}
	qsort(data, bmc, sizeof (struct bmstat), (void *) bm_cmp);
	for (i = 0; i < bmc; i++) {
		if (!goodgbid(data->userid)) {
			data++;
			continue;
		}
		if (data->noread) {
			fp = fp2;
		} else {
			fp = fp1;
			if (currSec != data->sec) {
				if (fp_p != NULL)
					pclose(fp_p);
				if (fp_p2 != NULL)
					pclose(fp_p2);
				currSec = data->sec;
				fprintf(fp,
					"\n\033[1;36m分区 %c 版主工作统计\033[m\n\n",
					currSec);
				cross = select_post_target(fn, currSec);
				fp_p = popen("/usr/bin/mail bbs", "w");	// need to change to pipe at last
				if (fp_p == NULL) {
					errlog("Can't open mail to write!");
					exit(-1);
				}
				fp_p2 = NULL;
				if (cross) {
					fp_p2 = popen("/usr/bin/mail bbs", "w");
					if (fp_p2 == NULL) {
						errlog
						    ("Can't open mail to write!");
						exit(-1);
					}
				}
				if (nrule > 0 && !onvac)
					gen_report_header(currSec, fp_p, fp_p2,
							  POST_NAME, POST_PASS,
							  fn);
			}
		}
		fprintf(fp,
			"%c区[%s] 版面 %s 版主 %s 停留时间 %d 秒 版面人气 %d\n",
			data->sec, data->class, data->board, data->userid,
			data->stay, data->boardscore);
		fprintf(fp, "进版 %4d 次 ", data->inboard);
		fprintf(fp, "版内发文 %4d 篇 ", data->post);
		fprintf(fp, "收入文摘 %4d 篇 ", data->digest);
		fprintf(fp, "去掉文摘 %4d 篇\n", data->undigest);
		fprintf(fp, "区段 %4d 次 ", data->range);
		fprintf(fp, "标记文章 %4d 篇 ", data->mark);
		fprintf(fp, "去掉标记 %4d 篇 ", data->unmark);
		fprintf(fp, "删除文章 %4d 篇\n", data->delete);
		fprintf(fp, "封禁 %4d 次 ", data->deny);
		fprintf(fp, "收入精华 %4d 篇 ", data->import);
		fprintf(fp, "整理精华 %4d 次 ", data->move);
		fprintf(fp, "相同主题 %4d 次\n\n", data->same);
		/* generate statistics */
		if ((nrule > 0) && (!data->noread) && !onvac) {
			struct bm_eva *bmeva = NULL;
			fprintf(fp_p, "%-15s%-20s", data->userid, data->board);
			if (fp_p2)
				fprintf(fp_p2, "%-15s%-20s", data->userid,
					data->board);
			for (j = 0; j <= ngrp; j++) {
				int m;
				char result[STRLEN] = "     ";
				tmp = check_rule(data, j);
				if (tmp == 0) {
					errlog
					    ("bmstand - unrecognizable rule tag!");
					pclose(fp_p);
					exit(-1);
				}
				// 必须确保规则Week先于Score执行,才能得到bmeva
				if (!strcmp(grp[j].name, "Week")) {
					int score = make_bm_score(data);
					bmeva = get_bm_eva(data, tmp, score);
					tmp = 3;
				} else if (!strcmp(grp[j].name, "Score")) {
					tmp = 4;
				}
				switch (tmp) {
				case 1:	//达标
					sprintf(result, "%-5s", "Y");
					break;
				case 2:	//超额完成
					sprintf(result, "%-5s", "y");
					break;
				case -1:	//不达标
					sprintf(result, "%-5s", "N");
					break;
				case 3:	//rule: week
					if (bmeva == NULL)
						break;
					sprintf(result, "%2d/%2d", bmeva->week,
						bmeva->total_week);
					if (bmeva->week >= 2)
						send_warning_mail(data->board,
								  bmeva);
					break;
				case 4:	//rule: Score
					if (bmeva != NULL)
						sprintf(result, "%5d",
							bmeva->ave_score);
					break;
				}
				//显示对齐
				k = 45 / (ngrp + 1);
				for (m = 5; m < k; m++)
					result[m] = ' ';
				result[m] = '\0';
				fprintf(fp_p, "%s", result);
				if (fp_p2)
					fprintf(fp_p2, "%s", result);
			}
			fprintf(fp_p, "\n");
			if (fp_p2)
				fprintf(fp_p2, "\n");
		}		/* end of generate statistics */
		data++;
	}
	fclose(fp1);
	fclose(fp2);
	save_bmstat_history_file();
}

void
bm_init()
{
	int i, k;
	struct hword *tmp, *tmp1;
	int numboards;

	numboards = brdshm->number;
	for (i = 0; i < numboards; i++) {
		if (bcache[i].header.filename[0]) {
			for (k = 0; k < BMNUM; k++) {
				if (bcache[i].header.bm[k][0] == 0)
					continue;
				tmp = malloc(sizeof (struct hword));
				if (tmp == NULL) {
					errlog("Can't malloc in bm_init!");
					exit(-1);
				}
				if (!strcmp(bcache[i].header.bm[k], "")
				    || !strcmp(bcache[i].header.bm[k], "SYSOP"))
					continue;
				snprintf(tmp->str, STRLEN - 1, "1 %s %s",
					 bcache[i].header.filename,
					 bcache[i].header.bm[k]);
				tmp->value = malloc(sizeof (struct bmstat));
				if (tmp->value == NULL) {
					errlog
					    ("Can't malloc value in bm_init!");
					exit(-1);
				}
				memset(tmp->value, 0, sizeof (struct bmstat));
				strncpy(((struct bmstat *) (tmp->value))->board,
					bcache[i].header.filename, 19);
				strncpy(((struct bmstat *) (tmp->value))->class,
					bcache[i].header.type, 4);
				strncpy(((struct
					  bmstat *) (tmp->value))->userid,
					bcache[i].header.bm[k], IDLEN);
				((struct bmstat *) (tmp->value))->noread =
				    boardnoread(&(bcache[i].header));
				((struct bmstat *) (tmp->value))->boardscore =
				    bcache[i].score;
				((struct bmstat *) (tmp->value))->sec =
				    bcache[i].header.sec1[0];
				((struct bmstat *) (tmp->value))->bidx = i;
				((struct bmstat *) (tmp->value))->bmidx = k;
				if ((strncmp
				     (bcache[i].header.sec1,
				      bcache[i].header.sec2, 3))
				    && (bcache[i].header.sec2[0] != 0)) {
					tmp1 = malloc(sizeof (struct hword));
					if (tmp1 == NULL) {
						errlog
						    ("Can't malloc value in bm_init!");
						exit(-1);
					}
					snprintf(tmp1->str, STRLEN - 1,
						 "2 %s %s",
						 bcache[i].header.filename,
						 bcache[i].header.bm[k]);
					tmp1->value =
					    malloc(sizeof (struct bmstat));
					if (tmp1->value == NULL) {
						errlog
						    ("Can't malloc in bm_init!");
						exit(-1);
					}
					memcpy(tmp1->value, tmp->value,
					       sizeof (struct bmstat));
					((struct bmstat *) (tmp1->value))->sec =
					    bcache[i].header.sec2[0];
					insertdic(bmd, tmp1);
				}
				insertdic(bmd, tmp);
			}
		}
	}
	register_stat(bm, bm_exit);
}
