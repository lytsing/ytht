/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu
    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
                        Peng Piaw Foong, ppfoong@csie.ncu.edu.tw
    Copyright (C) 1999, Zhou Lin, kcn@cic.tsinghua.edu.cn
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <sys/mman.h>
#include "bbs.h"
#include "common.h"
#include "bbstelnet.h"

extern int page, range;
extern char IScurrBM;
static char *const vote_type[] = { "是非", "单选", "复选", "数字", "问答" };
struct votebal currvote;
extern int numboards;
char controlfile[STRLEN];
unsigned int result[33];
int vnum;
int voted_flag;
FILE *sug;
int multivotestroll;

static int cmpvuid(char *userid, struct ballot *uv);
static void setvoteflag(char *bname, int flag);
static void setcontrolfile(void);
static int count_result(struct ballot *ptr);
static int count_log(struct votelog *ptr);
static void get_result_title(FILE * fp);
static int compareip(struct votelog *a, struct votelog *b);
static void mk_result(void);
static int get_vitems(struct votebal *bal);
static int vote_check(int bits);
static int showvoteitems(unsigned int pbits, int i, int flag);
static void show_voteing_title(void);
static int getsug(struct ballot *uv);
static void strollvote(struct ballot *uv, struct votebal *vote, int s);
static int multivote(struct ballot *uv);
static int valuevote(struct ballot *uv);
static int valid_voter(char *board, char *name);
static void user_vote(int num);
static void voteexp(void);
static int printvote(struct votebal *ent);
static void dele_vote(void);
static int vote_results(char *bname);
static void vote_title(void);
static int vote_key(int ch, int allnum, int pagenum);
static int Show_Votes(void);
static int b_suckinfile(FILE * fp, char *fname);

static int
cmpvuid(userid, uv)
char *userid;
struct ballot *uv;
{
	int i;
	i = strlen(userid) + 1;
	return !memcmp(userid, uv->uid, i > IDLEN ? IDLEN : i);
}

static void
setvoteflag(bname, flag)
char *bname;
int flag;
{
	int pos;
	struct boardheader fh;

	pos =
	    new_search_record(BOARDS, &fh, sizeof (fh), (void *) cmpbnames,
			      bname);
	if ((fh.flag & VOTE_FLAG)) {
		if (flag)
			return;
	} else {
		if (!flag)
			return;
	}
	if (flag == 0)
		fh.flag = fh.flag & ~VOTE_FLAG;
	else
		fh.flag = fh.flag | VOTE_FLAG;
	if (substitute_record(BOARDS, &fh, sizeof (fh), pos) == -1)
		prints("Error updating BOARDS file...\n");
	reload_boards();
}

void
makevdir(bname)
char *bname;
{
	struct stat st;
	char buf[STRLEN];

	sprintf(buf, "vote/%s", bname);
	if (stat(buf, &st) != 0)
		mkdir(buf, 0777);
}

void
setvfile(buf, bname, filename)
char *buf, *bname, *filename;
{
	sprintf(buf, "vote/%s/%s", bname, filename);
}

static void
setcontrolfile()
{
	setvfile(controlfile, currboard, "control");
}

static int
b_suckinfile(fp, fname)
FILE *fp;
char *fname;
{
	char inbuf[256];
	FILE *sfp;
	if ((sfp = fopen(fname, "r")) == NULL)
		return -1;
	while (fgets(inbuf, sizeof (inbuf), sfp) != NULL)
		fputs(inbuf, fp);
	fclose(sfp);
	sfp = 0;
	return 0;
}

int
b_closepolls()
{
	char buf[80];
	time_t now, nextpoll;
	int i, end;

	now = time(NULL);
	resolve_boards();

	if (now < bbsinfo.bcacheshm->pollvote) {
		return 0;
	}
	bbsinfo.bcacheshm->pollvote = now + 300;	//quick fix for multi user enter this function

	move(t_lines - 1, 0);
	prints("对不起，系统关闭投票中，请稍候...");
	refresh();

	nextpoll = now + 7 * 3600;

	strcpy(buf, currboard);
	for (i = 0; i < bbsinfo.bcacheshm->number; i++) {
		strcpy(currboard, (&(bbsinfo.bcache[i]))->header.filename);
		setcontrolfile();
		end = get_num_records(controlfile, sizeof (currvote));
		for (vnum = end; vnum >= 1; vnum--) {
			time_t closetime;
			get_record(controlfile, &currvote, sizeof (currvote),
				   vnum);
			closetime =
			    currvote.opendate + currvote.maxdays * 86400;
			if (now > closetime)
				mk_result();
			else if (nextpoll > closetime)
				nextpoll = closetime + 300;
		}
	}
	strcpy(currboard, buf);

	bbsinfo.bcacheshm->pollvote = nextpoll;
	return 0;
}

static int
count_result(ptr)
struct ballot *ptr;
{
	int i;

	if (ptr == NULL) {
		if (sug != NULL) {
			fclose(sug);
			sug = NULL;
		}
		return 0;
	}
	if (ptr->msg[0][0] != '\0') {
		if (currvote.type == VOTE_ASKING) {
			fprintf(sug, "\033[1m%.12s \033[m的作答如下：\n",
				ptr->uid);
		} else
			fprintf(sug, "\033[1m%.12s \033[m的建议如下：\n",
				ptr->uid);
		for (i = 0; i < 3; i++)
			fprintf(sug, "%s\n", ptr->msg[i]);
	}
	result[32]++;
	if (currvote.type == VOTE_ASKING) {
		return 0;
	}
	if (currvote.type != VOTE_VALUE) {
		for (i = 0; i < 32; i++) {
			if ((ptr->voted >> i) & 1)
				(result[i])++;
		}

	} else {
		result[31] += ptr->voted;
		result[(ptr->voted * 10) / (currvote.maxtkt + 1)]++;
	}
	return 0;
}

static int
count_log(ptr)
struct votelog *ptr;
{
	if (ptr == NULL) {
		if (sug != NULL) {
			fclose(sug);
			sug = NULL;
		}
		return 0;
	}
	fprintf(sug, "%12s   %16s   %s %s\n", ptr->uid, ptr->ip,
		Ctime(ptr->votetime), (ptr->voted == 0) ? "0" : "");
	return 0;
}

static void
get_result_title(FILE * fp)
{
	char buf[STRLEN], *tktDesc[] =
	    { NULL, NULL, NULL, "可投票数", "值不可超过" };

	fprintf(fp,
		"⊙ 投票开启于：\033[1m%.24s\033[m  类别：\033[1m%s\033[m\n",
		ctime(&currvote.opendate), vote_type[currvote.type - 1]);
	fprintf(fp, "⊙ 主题：\033[1m%s\033[m\n", currvote.title);
	if (currvote.type == VOTE_VALUE || currvote.type == VOTE_MULTI)
		fprintf(fp, "⊙ 此次投票的%s：\033[1m%d\033[m\n\n",
			tktDesc[(int) currvote.type], currvote.maxtkt);
	fprintf(fp, "⊙ 票选题目描述：\n\n");
	sprintf(buf, "vote/%s/desc.%ld", currboard,
		(long int) currvote.opendate);
	b_suckinfile(fp, buf);
}

static int
compareip(a, b)
struct votelog *a, *b;
{
	return cmpIP(a->ip, b->ip);
}

static void
mk_result()
{
	char fname[STRLEN], nname[STRLEN];
	char sugname[STRLEN];
	char logfname[STRLEN], sortedlogfname[STRLEN];
	char title[STRLEN];
	int i;
	int postout = 0;
	unsigned int total = 0;
	int j, percent;
	char *blk[] = { "█", "▉", "▊", "▋", "▌", "▍", "▎", "▏", " " };

	setcontrolfile();
	sprintf(fname, "vote/%s/flag.%ld", currboard,
		(long int) currvote.opendate);
	count_result(NULL);
	sprintf(sugname, "vote/%s/tmp.%d", currboard, uinfo.pid);

	if ((sug = fopen(sugname, "w")) == NULL) {
		errlog("open vote tmp file error %d", errno);
		prints("Error: 结束投票错误...\n");
		pressanykey();
		return;
	}
	memset(result, 0, sizeof (result));
	if (apply_record(fname, (void *) count_result, sizeof (struct ballot))
	    == -1) {
		errlog("Vote apply flag error");
	}
	fprintf(sug,
		"\033[1;44;36m——————————————┤使用者%s├——————————————\033[m\n\n\n",
		(currvote.type != VOTE_ASKING) ? "建议或意见" : "此次的作答");
	fclose(sug);
	sprintf(nname, "vote/%s/results", currboard);
	if ((sug = fopen(nname, "w")) == NULL) {
		errlog("open vote newresult file error %d", errno);
		prints("Error: 结束投票错误...\n");
		return;
	}

	get_result_title(sug);

	fprintf(sug, "** 投票结果:\n\n");
	if (currvote.type == VOTE_VALUE) {
		total = result[32];
		for (i = 0; i < 10; i++) {
			fprintf(sug,
				"\033[1m  %4d\033[m 到 \033[1m%4d\033[m 之间有 \033[1m%4d\033[m 票  约占 ",
				(i * currvote.maxtkt) / 10 + ((i == 0) ? 0 : 1),
				((i + 1) * currvote.maxtkt) / 10, result[i]);
			percent = 0.5 +
			    (result[i] * 100.) / ((total <= 0) ? 1 : total);
			fprintf(sug, "%3d%%", percent);
			fprintf(sug, "\033[3%dm", 1 + i % 7);
			for (j = percent / 16; j; j--)
				fprintf(sug, "%s", blk[0]);
			fprintf(sug, "%s\033[m\n", blk[8 - (percent % 16) / 2]);
		}
		fprintf(sug, "此次投票结果平均值是: \033[1m%d\033[m\n",
			result[31] / ((total <= 0) ? 1 : total));
	} else if (currvote.type == VOTE_ASKING) {
		total = result[32];
	} else {
		for (i = 0; i < currvote.totalitems; i++) {
			total += result[i];
		}
		for (i = 0; i < currvote.totalitems; i++) {
			fprintf(sug, "(%c) %-40s  %4d 票  约占 ",
				'A' + i, currvote.items[i], result[i]);
			percent =
			    (result[i] * 100) / ((total <= 0) ? 1 : total);
			fprintf(sug, "%3d%%", percent);
			fprintf(sug, "\033[3%dm", 1 + i % 7);
			for (j = percent / 16; j; j--)
				fprintf(sug, "%s", blk[0]);
			fprintf(sug, "%s\033[m\n", blk[8 - (percent % 16) / 2]);
		}
	}
	fprintf(sug, "\n投票总人数 = \033[1m%d\033[m 人\n", result[32]);
	fprintf(sug, "投票总票数 =\033[1m %d\033[m 票\n\n", total);
	fprintf(sug,
		"\033[1;44;36m——————————————┤使用者%s├——————————————\033[m\n\n\n",
		(currvote.type != VOTE_ASKING) ? "建议或意见" : "此次的作答");
	b_suckinfile(sug, sugname);
	unlink(sugname);
	fclose(sug);

	sug = NULL;

	resolve_boards();
	for (i = 0; i < numboards; i++)
		if (!strncmp
		    (currboard, bbsinfo.bcache[i].header.filename, STRLEN))
			break;
	if (i != numboards)
		if (normal_board(currboard)) {
			if ((bbsinfo.bcache[i].header.flag & CLOSECLUB_FLAG) ==
			    0)
				postout = 1;
			else
				postout = 0;
		}
	if (currvote.flag & VOTE_FLAG_OPENED) {
		char *mem = NULL;
		struct stat buf;
		int fd;

		sprintf(fname, "vote/%s/newlog.%ld", currboard,
			(long int) currvote.opendate);
		sprintf(logfname, "vote/%s/log", currboard);
		if ((sug = fopen(logfname, "w")) == NULL) {
			errlog("open vote tmp file error %d", errno);
			prints("Error: 结束投票错误...\n");
			pressanykey();
			return;
		}
		fprintf(sug, "%12s   %16s   %24s\n", "ID", "IP", "投票时间");
		apply_record(fname, (void *) count_log,
			     sizeof (struct votelog));
		fclose(sug);
		if ((fd = open(fname, O_RDWR, 0644)) != -1) {
			flock(fd, LOCK_EX);
			fstat(fd, &buf);
			MMAP_TRY {
				mem =
				    mmap(0, buf.st_size, PROT_READ | PROT_WRITE,
					 MAP_FILE | MAP_SHARED, fd, 0);
				qsort(mem,
				      buf.st_size / sizeof (struct votelog),
				      sizeof (struct votelog),
				      (void *) compareip);
			}
			MMAP_CATCH {
			}
			MMAP_END munmap(mem, buf.st_size);
			flock(fd, LOCK_UN);
			close(fd);
		}
		sprintf(sortedlogfname, "vote/%s/slog", currboard);
		if ((sug = fopen(sortedlogfname, "w")) == NULL) {
			errlog("open vote tmp file error %d", errno);
			prints("Error: 结束投票错误...\n");
			pressanykey();
			return;
		}
		fprintf(sug, "%12s   %16s   %24s\n", "ID", "IP", "投票时间");
		apply_record(fname, (void *) count_log,
			     sizeof (struct votelog));
		fclose(sug);
		sug = NULL;
	}
	if (postout) {
		sprintf(title, "[公告] %s 版的投票结果", currboard);
		postfile(nname, "vote", title, 1);
		if (currvote.flag & VOTE_FLAG_OPENED) {
			sprintf(title, "[公告] %s 版的投票参与情况", currboard);
			postfile(logfname, "vote", title, 1);
			sprintf(title, "[公告] %s 版的投票参与情况(by IP)",
				currboard);
			postfile(sortedlogfname, "vote", title, 1);
		}
	}
	if (strncmp(currboard, "vote", STRLEN)) {
		snprintf(title, STRLEN, "[投票结果] %s", currvote.title);
		postfile(nname, currboard, title, 1);
		if (currvote.flag & VOTE_FLAG_OPENED) {
			sprintf(title, "[公告] %s 版的投票参与情况", currboard);
			postfile(logfname, currboard, title, 1);
			sprintf(title, "[公告] %s 版的投票参与情况(by IP)",
				currboard);
			postfile(sortedlogfname, currboard, title, 1);
		}

	}
	dele_vote();
	return;
}

static int
get_vitems(bal)
struct votebal *bal;
{
	int num;
	char buf[STRLEN];

	move(3, 0);
	prints("请依序输入可选择项, 按 ENTER 完成设定.\n");
	num = 0;
	for (num = 0; num < 32; num++) {
		sprintf(buf, "%c) ", num + 'A');
		getdata((num % 16) + 4, (num / 16) * 40, buf, bal->items[num],
			36, DOECHO, YEA);
		if (strlen(bal->items[num]) == 0) {
			if (num > 1)
				break;
			num = 0;
		}

	}
	bal->totalitems = num;
	return num;
}

int
vote_maintain(bname)
char *bname;
{
	char buf[STRLEN * 2];
	struct votebal *ball = &currvote;
	int retv;

	setcontrolfile();
	if (!USERPERM(currentuser, PERM_OVOTE))
		if (!IScurrBM) {
			return 0;
		}
	stand_title("开启投票箱");
	makevdir(bname);
	for (;;) {
		getdata(2, 0,
			"(1)是非, (2)单选, (3)复选, (4)数值 (5)问答 (6)取消 ? : ",
			genbuf, 2, DOECHO, YEA);
		genbuf[0] -= '0';
		if (genbuf[0] == 6) {
			prints("取消此次投票\n");
			sleep(1);
			return FULLUPDATE;
		}
		if (genbuf[0] < 1 || genbuf[0] > 5)
			continue;
		ball->type = (int) genbuf[0];
		break;
	}
	if (askyn("此次投票结果是否公开", NA, NA))
		ball->flag |= VOTE_FLAG_OPENED;
	else
		ball->flag &= ~VOTE_FLAG_OPENED;
	if (askyn("此次投票是否限制穿梭 IP", NA, NA))
		ball->flag |= VOTE_FLAG_LIMITIP;
	else
		ball->flag &= ~VOTE_FLAG_LIMITIP;
	ball->flag &= ~VOTE_FLAG_LIMITED;
	if (USERPERM(currentuser, PERM_SYSOP)) {
		if (askyn
		    ("此次投票是否限制投票人? (需要先由系统维护生成名单)", NA,
		     NA))
			ball->flag |= VOTE_FLAG_LIMITED;
		else
			ball->flag &= ~VOTE_FLAG_LIMITED;
	}
	ball->opendate = time(NULL);
	prints("请按任何键开始编辑此次 [投票的描述]: \n");
	igetkey();
	setvfile(genbuf, bname, "desc");
	sprintf(buf, "%s.%ld", genbuf, (long int) ball->opendate);

	retv = vedit(buf, NA, YEA);
	if (retv == -1) {
		clear();
		prints("取消此次投票\n");
		pressreturn();
		return FULLUPDATE;
	}

	clear();
	getdata(0, 0, "此次投票所须天数 (默认1天): ", buf, 4, DOECHO, YEA);

	if (*buf == '\n' || atoi(buf) == 0 || *buf == '\0')
		strcpy(buf, "1");

	ball->maxdays = atoi(buf);
	if (999 == ball->maxdays) {
		prints("真是变态...\n");
		pressanykey();
	}
	for (;;) {
		getdata(1, 0, "投票箱的标题: ", ball->title, 61, DOECHO, YEA);
		if (strlen(ball->title) > 0)
			break;
		bell();
	}
	switch (ball->type) {
	case VOTE_YN:
		ball->maxtkt = 0;
		strcpy(ball->items[0], "赞成  （是的）");
		strcpy(ball->items[1], "不赞成（不是）");
		strcpy(ball->items[2], "没意见（不清楚）");
		ball->maxtkt = 1;
		ball->totalitems = 3;
		break;
	case VOTE_SINGLE:
		get_vitems(ball);
		ball->maxtkt = 1;
		break;
	case VOTE_MULTI:
		get_vitems(ball);
		for (;;) {
			getdata(21, 0, "一个人最多几票? [2]: ", buf, 5, DOECHO,
				YEA);
			ball->maxtkt = atoi(buf);
			if (ball->maxtkt <= 1)
				ball->maxtkt = 2;
			if (ball->maxtkt > ball->totalitems)
				continue;
			break;
		}
		break;
	case VOTE_VALUE:
		for (;;) {
			getdata(3, 0, "输入数值最大不得超过 [100] : ", buf, 4,
				DOECHO, YEA);
			ball->maxtkt = atoi(buf);
			if (ball->maxtkt <= 0)
				ball->maxtkt = 100;
			break;
		}
		break;
	case VOTE_ASKING:
/*                    getdata(3,0,"此问答题作答行数之限制 :",buf,3,DOECHO,YEA) ;
                    ball->maxtkt = atof(buf) ;
                    if(ball->maxtkt <= 0) ball->maxtkt = 10;*/
		ball->maxtkt = 0;
		currvote.totalitems = 0;
		break;
	default:
		ball->maxtkt = 1;
		break;
	}
	setvoteflag(currboard, 1);
	clear();
	strcpy(ball->userid, currentuser->userid);
	if (append_record(controlfile, ball, sizeof (*ball)) == -1) {
		prints("发生严重的错误，无法开启投票，请通告站长");
		errlog("Append Control file Error!! board=%s", currboard);
	} else {
		char votename[STRLEN];
		int i;

		prints("投票箱开启了！\n");
		range++;;
		sprintf(votename, "tmp/votetmp.%s.%05d", currentuser->userid,
			uinfo.pid);
		if ((sug = fopen(votename, "w")) != NULL) {
			sprintf(buf, "[通知] %s 举办投票：%s", currboard,
				ball->title);
			get_result_title(sug);
			if (ball->type != VOTE_ASKING
			    && ball->type != VOTE_VALUE) {
				fprintf(sug, "\n【\033[1m选项如下\033[m】\n");
				for (i = 0; i < ball->totalitems; i++) {
					fprintf(sug,
						"(\033[1m%c\033[m) %-40s\n",
						'A' + i, ball->items[i]);
				}
			}
			fclose(sug);
			sug = NULL;
			resolve_boards();
			for (i = 0; i < numboards; i++)
				if (!strncmp
				    (currboard,
				     bbsinfo.bcache[i].header.filename, STRLEN))
					break;
			if (i != numboards)
				if (normal_board(currboard)) {
					if ((bbsinfo.bcache[i].
					     header.flag & CLOSECLUB_FLAG) == 0)
						postfile(votename, "vote", buf,
							 1);
				}
			postfile(votename, currboard, buf, 1);
			unlink(votename);
		}
	}
	pressreturn();
	return FULLUPDATE;
}

static int
vote_check(bits)
int bits;
{
	int i, count;

	for (i = count = 0; i < 32; i++) {
		if ((bits >> i) & 1)
			count++;
	}
	return count;
}

static int
showvoteitems(pbits, i, flag)
unsigned int pbits;
int i, flag;
{
	char buf[STRLEN];
	int count;

	if (flag == YEA) {
		count = vote_check(pbits);
		if (count > currvote.maxtkt)
			return NA;
		move(2, 0);
		clrtoeol();
		prints("您已经投了 \033[1m%d\033[m 票。\n"
		       "\033[1;32m修改前次投票：先选相同选项取消原投票，"
		       "然后重新选择所要的选项。\033[m", count);
	}

	sprintf(buf, "%c.%2.2s%-36.36s", 'A' + i,
		((pbits >> i) & 1 ? "□" : "  "), currvote.items[i]);
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints(buf);
	return YEA;
}

static void
show_voteing_title()
{
	time_t closedate;
	char buf[STRLEN];

	if (currvote.type != VOTE_VALUE && currvote.type != VOTE_ASKING)
		sprintf(buf, "可投票数: \033[1m%d\033[m 票", currvote.maxtkt);
	else
		buf[0] = '\0';
	closedate = currvote.opendate + currvote.maxdays * 86400;
	prints("投票将结束于: \033[1m%24s\033[m  %s  %s\n",
	       ctime(&closedate), buf,
	       (voted_flag) ? "(\033[5;1m修改前次投票\033[m)" : "");
	prints("投票主题是: \033[1m%-50s\033[m类型: \033[1m%s\033[m \n",
	       currvote.title, vote_type[currvote.type - 1]);
}

static int
getsug(uv)
struct ballot *uv;
{
	int i, line;
	int lastline = 0;

	move(0, 0);
	clrtobot();
	if (currvote.type == VOTE_ASKING) {
		show_voteing_title();
		line = 3;
		prints("请填入您的作答(三行):\n");
	} else {
		line = 1;
		prints("请填入您宝贵的意见(三行):\n");
	}
	move(line, 0);
	for (i = 0; i < 3; i++) {
		prints(": %s\n", uv->msg[i]);
		if (uv->msg[i][0])
			lastline = i + 1;
	}
	for (i = 0; i < 3; i++) {
		getdata(line + i, 0, ": ", uv->msg[i], STRLEN - 2, DOECHO, NA);
		if (uv->msg[i][0] == '\0') {
			if (lastline && i + 1 == lastline) {
				break;
			} else if (0 == lastline)
				break;
		}
	}
	return i;
}

static void
strollvote(struct ballot *uv, struct votebal *vote, int s)
{
	struct ballot nuv;
	struct votebal nvote;
	int i, j;
	memcpy(&nuv, uv, sizeof (nuv));
	memcpy(&nvote, vote, sizeof (nvote));
	nuv.voted = 0;
	for (i = 0; i < vote->totalitems; i++) {
		j = (i + s) % vote->totalitems;
		if (j < 0)
			j += vote->totalitems;
		if (uv->voted & (1 << i))
			nuv.voted |= (1 << j);
		strcpy(nvote.items[j], vote->items[i]);
	}
	memcpy(uv, &nuv, sizeof (nuv));
	memcpy(vote, &nvote, sizeof (nvote));
}

static int
multivote(uv)
struct ballot *uv;
{
	unsigned int i;
	int multivotestroll = time(NULL) % currvote.totalitems;
	i = uv->voted;

	move(0, 0);
	show_voteing_title();

	strollvote(uv, &currvote, multivotestroll);
	if (vote_check(uv->voted) > currvote.maxtkt)	//修正先前bug的错误结果
		uv->voted = 0;
	uv->voted =
	    setperms(uv->voted, "选票", currvote.totalitems, showvoteitems, 1);
	strollvote(uv, &currvote, -multivotestroll);

	if (uv->voted == i)
		return -1;
	return 1;
}

static int
valuevote(uv)
struct ballot *uv;
{
	unsigned int chs;
	char buf[10];

	chs = uv->voted;
	move(0, 0);
	show_voteing_title();
	prints("此次作答的值不能超过 \033[1m%d\033[m", currvote.maxtkt);
	if (uv->voted != 0)
		sprintf(buf, "%d", uv->voted);
	else
		memset(buf, 0, sizeof (buf));
	do {
		getdata(3, 0, "请输入一个值? [0]: ", buf, 5, DOECHO, NA);
		uv->voted = abs(atoi(buf));
	} while (uv->voted > currvote.maxtkt && buf[0] != '\n'
		 && buf[0] != '\0');
	if (buf[0] == '\n' || buf[0] == '\0' || uv->voted == chs)
		return -1;
	return 1;
}

static int
valid_voter(char *board, char *name)
{
	FILE *in;
	char buf[100];
	int i;
	sprintf(genbuf, "%s/%s.validlist", MY_BBS_HOME, board);
	in = fopen(genbuf, "r");
	if (in != NULL) {
		while (fgets(buf, 80, in)) {
			i = strlen(buf);
			if (buf[i - 1] == '\n')
				buf[i - 1] = 0;
			//prints(buf);
			if (!strcmp(buf, name)) {
				fclose(in);
				return 1;
			}
		}
		fclose(in);
	}
	return 0;
}

static void
user_vote(num)
int num;
{
	char fname[STRLEN], bname[STRLEN];
	char buf[STRLEN];
	struct ballot uservote, tmpbal;
	int votevalue;
	int aborted = NA, pos;

	move(t_lines - 2, 0);
	get_record(controlfile, &currvote, sizeof (struct votebal), num);
	if (!(currentuser->userlevel & PERM_LOGINOK)) {
		prints("对不起, 您还没有通过注册呢\n");
		pressanykey();
		return;
	}
	if (currentuser->firstlogin >= currvote.opendate - 7 * 86400) {
		prints
		    ("对不起, 开这个投票时您注册还没满 7 天, 没有资格参加哦!\n");
		pressanykey();
		return;
	}
	if ((currvote.flag & VOTE_FLAG_LIMITIP) && invalid_voteIP(realfromhost)) {
		prints("对不起，您从穿梭站连来，不能投票\n");
		pressanykey();
		return;
	}
	if (currvote.flag & VOTE_FLAG_LIMITED) {
		if (!valid_voter(currboard, currentuser->userid)) {
			prints("对不起, 您没有选举权\n");
			pressanykey();
			return;
		}
	}
	if (currvote.flag & VOTE_FLAG_OPENED) {
		prints("请注意，该投票结束后将公布投票ID、IP、投票时间");
		pressanykey();
	}

	sprintf(fname, "vote/%s/flag.%ld", currboard,
		(long int) currvote.opendate);
	if ((pos =
	     search_record(fname, &uservote, sizeof (uservote),
			   (void *) cmpvuid, currentuser->userid)) <= 0) {
		(void) memset(&uservote, 0, sizeof (uservote));
		voted_flag = NA;
	} else {
		voted_flag = YEA;
	}
	strcpy(uservote.uid, currentuser->userid);
	sprintf(bname, "desc.%ld", (long int) currvote.opendate);
	setvfile(buf, currboard, bname);
	ansimore(buf, YEA);
	move(0, 0);
	clrtobot();
	switch (currvote.type) {
	case VOTE_SINGLE:
	case VOTE_MULTI:
	case VOTE_YN:
		votevalue = multivote(&uservote);
		if (votevalue == -1)
			aborted = YEA;
		break;
	case VOTE_VALUE:
		votevalue = valuevote(&uservote);
		if (votevalue == -1)
			aborted = YEA;
		break;
	case VOTE_ASKING:
		uservote.voted = 0;
		aborted = !getsug(&uservote);
		break;
	}
	clear();
	if (aborted == YEA) {
		prints("保留 【\033[1m%s\033[m】原来的的投票。\n",
		       currvote.title);
	} else {
		if (currvote.type != VOTE_ASKING)
			getsug(&uservote);
		pos =
		    search_record(fname, &tmpbal, sizeof (tmpbal),
				  (void *) cmpvuid, currentuser->userid);
		if (pos) {
			substitute_record(fname, &uservote, sizeof (uservote),
					  pos);
		} else if (append_record(fname, &uservote, sizeof (uservote)) ==
			   -1) {
			move(2, 0);
			clrtoeol();
			prints("投票失败! 请通知站长参加那一个选项投票\n");
			pressreturn();
		}
		prints("\n已经帮您投入票箱中...\n");
		if (currvote.flag & VOTE_FLAG_OPENED) {
			char votelogfile[STRLEN];
			struct votelog log;
			strcpy(log.uid, currentuser->userid);
			log.uid[IDLEN] = 0;
			log.votetime = time(NULL);
			log.voted = uservote.voted;
			strcpy(log.ip, fromhost);
			sprintf(votelogfile, "vote/%s/newlog.%ld", currboard,
				(long int) currvote.opendate);
			append_record(votelogfile, &log, sizeof (log));
		}
		if (!strcmp(currboard, "SM_Election")) {
			int now;
			now = time(NULL);
			sprintf(buf, "%s %s %s", currentuser->userid,
				fromhost, Ctime(now));
			addtofile(MY_BBS_HOME "/vote.log", buf);
		}
	}
	pressanykey();
	return;
}

static void
voteexp()
{
	clrtoeol();
	prints
	    ("\033[1;44m编号 开启投票箱者 开启日 %-37s   类别 天数 人数\033[m\n",
	     "投票主题");
}

static int
printvote(ent)
struct votebal *ent;
{
	static int i;
	struct ballot uservote;
	char buf[STRLEN + 10], *date;
	char flagname[STRLEN];
	int num_voted;

	if (ent == NULL) {
		move(2, 0);
		voteexp();
		i = 0;
		return 0;
	}
	i++;
	if (i > page + 19 || i > range)
		return QUIT;
	else if (i <= page)
		return 0;
	sprintf(buf, "flag.%ld", (long int) ent->opendate);
	setvfile(flagname, currboard, buf);
	if (search_record
	    (flagname, &uservote, sizeof (uservote), (void *) cmpvuid,
	     currentuser->userid) <= 0) {
		voted_flag = NA;
	} else
		voted_flag = YEA;
	num_voted = get_num_records(flagname, sizeof (struct ballot));
	date = ctime(&ent->opendate) + 4;
	sprintf(buf,
		" %s%3d %-12.12s %-6.6s %-37.37s %c %-4.4s %3d  %4d\033[m\n",
		(voted_flag == NA) ? "\033[1m" : "", i, ent->userid, date,
		ent->title, ent->flag & VOTE_FLAG_OPENED ? 'O' : ' ',
		vote_type[ent->type - 1], ent->maxdays, num_voted);
	prints("%s", buf);
	return 0;
}

static void
dele_vote()
{
	char buf[STRLEN];
	int num = 1;
	struct votebal tmpvote;
	while (get_record(controlfile, &tmpvote, sizeof (struct votebal), num)
	       == 0) {
		if (currvote.opendate == tmpvote.opendate) {
			if (delete_record(controlfile, sizeof (currvote), num)
			    == -1) {
				prints("发生错误，请通知站长....");
				pressanykey();
			}
			range--;
			sprintf(buf, "vote/%s/flag.%ld", currboard,
				(long int) currvote.opendate);
			deltree(buf);
			sprintf(buf, "vote/%s/desc.%ld", currboard,
				(long int) currvote.opendate);
			deltree(buf);
			sprintf(buf, "vote/%s/newlog.%ld", currboard,
				(long int) currvote.opendate);
			deltree(buf);
			break;
		}
		num++;
	}
	if (get_num_records(controlfile, sizeof (currvote)) == 0) {
		setvoteflag(currboard, 0);
	}
}

static int
vote_results(bname)
char *bname;
{
	char buf[STRLEN];
	setvfile(buf, bname, "results");
	if (ansimore(buf, YEA) == -1) {
		move(3, 0);
		prints("目前没有任何投票的结果。\n");
		clrtobot();
		pressreturn();
	} else
		clear();
	return FULLUPDATE;
}

int
b_vote_maintain()
{
	return vote_maintain(currboard);
}

static void
vote_title()
{

	docmdtitle("[投票箱列表]",
		   "[\033[1;32m←\033[m,\033[1;32me\033[m] 离开 [\033[1;32mh\033[m] 求助 [\033[1;32m→\033[m,\033[1;32mr <cr>\033[m] 进行投票 [\033[1;32m↑\033[m,\033[1;32m↓\033[m] 上,下选择 \033[1m高亮度\033[m表示尚未投票");
	update_endline();
}

static int
vote_key(ch, allnum, pagenum)
int ch;
int allnum, pagenum;
{
	int deal = 0, ans;
	char buf[STRLEN];

	switch (ch) {
	case 'v':
	case 'V':
	case '\n':
	case '\r':
	case 'r':
	case KEY_RIGHT:
		user_vote(allnum + 1);
		deal = 1;
		break;
	case 'R':
		vote_results(currboard);
		deal = 1;
		break;
	case 'H':
	case 'h':
		show_help("help/votehelp");
		deal = 1;
		break;
	case 'A':
	case 'a':
		if (!IScurrBM)
			return YEA;
		vote_maintain(currboard);
		deal = 1;
		break;
	case 'O':
	case 'o':
		if (!IScurrBM)
			return YEA;
		clear();
		deal = 1;
		get_record(controlfile, &currvote, sizeof (struct votebal),
			   allnum + 1);
		prints("\033[5;1;31m警告!!\033[m\n");
		prints("投票箱标题：\033[1m%s\033[m\n", currvote.title);
		ans = askyn("你确定要提早结束这个投票吗", NA, NA);

		if (ans != 1) {
			move(2, 0);
			prints("取消删除行动\n");
			pressreturn();
			clear();
			break;
		}
		mk_result();
		sprintf(buf, "提早结束投票 %s", currvote.title);
		securityreport(buf, buf);
		break;
	case 'D':
	case 'd':
		if (!USERPERM(currentuser, PERM_OVOTE))
			if (!IScurrBM) {
				return 1;
			}
		deal = 1;
		get_record(controlfile, &currvote, sizeof (struct votebal),
			   allnum + 1);
		clear();
		prints("\033[5;1;31m警告!!\033[m\n");
		prints("投票箱标题：\033[1m%s\033[m\n", currvote.title);
		ans = askyn("您确定要强制关闭这个投票吗", NA, NA);

		if (ans != 1) {
			move(2, 0);
			prints("取消删除行动\n");
			pressreturn();
			clear();
			break;
		}
		sprintf(buf, "强制关闭投票 %s", currvote.title);
		securityreport(buf, buf);
		dele_vote();
		break;
	default:
		return 0;
	}
	if (deal) {
		Show_Votes();
		vote_title();
	}
	return 1;
}

static int
Show_Votes()
{

	move(3, 0);
	clrtobot();
	printvote(NULL);
	setcontrolfile();
	if (apply_record
	    (controlfile, (void *) printvote, sizeof (struct votebal)) == -1) {
		prints("错误，没有投票箱开启....");
		pressreturn();
		return -1;
	}
	clrtobot();
	return 0;
}

int
b_vote()
{
	int num_of_vote;
	int voting;
	int bnum;

	if (!USERPERM(currentuser, PERM_VOTE))
		return -1;

	if (currentuser->stay < 1800 && currentuser->numdays < 7) {
		return -1;
	}

	bnum = getbnum(currboard);
	if (!haspostperm(bnum)) {
		return -1;
	}
	if (club_board(currboard, bnum)) {
		if (!clubtest(currboard) && !USERPERM(currentuser, PERM_SYSOP)) {
			return -1;
		}
	}
	setcontrolfile();
	num_of_vote = get_num_records(controlfile, sizeof (struct votebal));
	if (num_of_vote == 0) {
		move(3, 0);
		clrtobot();
		prints("抱歉, 目前并没有任何投票举行。\n");
		pressreturn();
		setvoteflag(currboard, 0);
		return FULLUPDATE;
	}
	setlistrange(num_of_vote);
	clear();
	voting =
	    choose(NA, 0, vote_title, vote_key, Show_Votes, (void *) user_vote);
	clear();
	return /*user_vote( currboard ) */ FULLUPDATE;
}

int
b_results()
{
	return vote_results(currboard);
}

void
m_vote()
{
	char buf[STRLEN];
	strcpy(buf, currboard);
	strcpy(currboard, DEFAULTBOARD);
	modify_user_mode(ADMIN);
	vote_maintain(DEFAULTBOARD);
	strcpy(currboard, buf);
	return;
}

void
x_vote()
{
	char buf[STRLEN];
	modify_user_mode(XMENU);
	strcpy(buf, currboard);
	strcpy(currboard, "sysop");
	b_vote();
	strcpy(currboard, buf);
	return;
}

int
x_results()
{
	modify_user_mode(XMENU);
	return vote_results("sysop");
}
