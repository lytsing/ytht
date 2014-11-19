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

    Copyright (C) 1999, KCN,Zhou Lin, kcn@cic.tsinghua.edu.cn
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "bbstelnet.h"

char currmaildir[STRLEN];
int mailmode, mailhour;
static int mailallmode = 0;
#define maxrecp 300

static int do_send(char *userid, char *title);
static int read_mail(struct fileheader *fptr);
static int read_new_mail(struct fileheader *fptr);
static int mailtitle(void);
static char *maildoent(int num, struct fileheader *ent, char buf[512]);
static int mail_read(int ent, struct fileheader *fileinfo, char *direct);
static int mail_del(int ent, struct fileheader *fileinfo, char *direct);
static int mail_spam(int ent, struct fileheader *fileinfo, char *direct);
static int mail_del_range(int ent, struct fileheader *fileinfo, char *direct);
static int mail_mark(int ent, struct fileheader *fileinfo, char *direct);
static int mail_markdel(int ent, struct fileheader *fileinfo, char *direct);
static int do_gsend(char *userid[], char *title, int num, char *maillist);
static void getmailinfo(char *path, struct fileheader *rst);
static int mail_rjunk(void);
static int m_cancel_1(struct fileheader *fh, char *receiver);
static int check_mail_perm();
static int show_user_notes();
static int mailone(struct userec *uentp, char *arg);
static int gensecBMs(const struct sectree *sec, char *bmfile);

#ifdef INTERNET_EMAIL
#endif

char *
email_domain()
{
	return MY_BBS_DOMAIN;
}

static int
check_mail_perm()
{
	char *ptr;
	ptr = check_mailperm(currentuser);
	if (!ptr)
		return 0;
	clrtoeol();
	prints("%s\n", ptr);
	return -1;
}

int
chkmail()
{
	static time_t lasttime = 0;
	static int ismail = 0;
	struct stat st;
	int fd;
	int i, offset;
	int numfiles;
	int accessed;
	extern char currmaildir[STRLEN];

	if (!USERPERM(currentuser, PERM_BASIC)) {
		return 0;
	}
	offset = (int) &((struct fileheader *) 0)->accessed;

	if (!ismail && now_t - lasttime < 10)
		return 0;
	if (stat(currmaildir, &st) < 0)
		return (ismail = 0);
	if (lasttime >= st.st_mtime)
		return ismail;

	if ((fd = open(currmaildir, O_RDONLY)) < 0)
		return (ismail = 0);
	lasttime = st.st_mtime;
	numfiles = st.st_size / sizeof (struct fileheader);
	if (numfiles <= 0) {
		close(fd);
		return (ismail = 0);
	}
	lseek(fd, (st.st_size - (sizeof (struct fileheader) - offset)),
	      SEEK_SET);
	for (i = 0; i < numfiles && i < 10; i++) {
		read(fd, &accessed, sizeof (accessed));
		if (!(accessed & FH_READ)) {
			close(fd);
			return (ismail = 1);
		}
		lseek(fd, -sizeof (struct fileheader) - sizeof (accessed),
		      SEEK_CUR);
	}
	close(fd);
	return (ismail = 0);
}

int
check_query_mail(qry_mail_dir)
char qry_mail_dir[STRLEN];
{
	struct stat st;
	int fd;
	int offset;
	int numfiles;
	int accessed;

	offset = (int) &((struct fileheader *) 0)->accessed;
	if ((fd = open(qry_mail_dir, O_RDONLY)) < 0)
		return 0;
	fstat(fd, &st);
	numfiles = st.st_size / sizeof (struct fileheader);
	if (numfiles <= 0) {
		close(fd);
		return 0;
	}
	lseek(fd, (st.st_size - (sizeof (struct fileheader) - offset)),
	      SEEK_SET);
/*离线查询新信只要查询最后一封是否为新信，其他并不重要*/
/*Modify by SmallPig*/
	read(fd, &accessed, sizeof (accessed));
	if (!(accessed & FH_READ)) {
		close(fd);
		return YEA;
	}
	close(fd);
	return NA;
}

int
mailall()
{
	char ans[4], fname[STRLEN], title[STRLEN];
	char doc[6][STRLEN], buf[STRLEN], str[7];
	int i;
	int hour;
	int save_in_mail;

	strcpy(title, "没主题");
	modify_user_mode(SMAIL);
	clear();
	move(0, 0);
	sprintf(fname, "tmp/mailall.%s", currentuser->userid);
	prints("你要寄给所有的：\n");
	prints("(0) 放弃\n");
	strcpy(doc[0], "(1) 尚未通过身份确认的使用者");
	strcpy(doc[1], "(2) 所有通过身份确认的使用者");
	strcpy(doc[2], "(3) 所有的版主");
	strcpy(doc[3], "(4) 本站智囊团");
	strcpy(doc[4], "(5) 所有本组织会员");
	strcpy(doc[5], "(6) 所有SYSOP");
	for (i = 0; i < 6; i++)
		prints("%s\n", doc[i]);
	getdata(9, 0, "请输入模式 (0~6)? [0]: ", ans, 2, DOECHO, YEA);
	if (ans[0] - '0' < 1 || ans[0] - '0' > 6) {
		return NA;
	}
	getdata(10, 0, "请输入上站时间下限(按小时)[0]: ", str, 5, DOECHO, YEA);
	hour = atoi(str);
	sprintf(buf, "是否确定寄给%s 并且上站时间不小于%d小时?",
		doc[ans[0] - '0' - 1], hour);
	move(11, 0);
	if (askyn(buf, NA, NA) == NA)
		return NA;
	save_in_mail = in_mail;
	in_mail = YEA;
	header.reply_mode = NA;
	strcpy(header.title, "没主题");
	strcpy(header.ds, doc[ans[0] - '0' - 1]);
	header.postboard = NA;
	if (post_header(&header))
		sprintf(save_title, "[Type %c 公告] %.60s", ans[0],
			header.title);
	setquotefile("");
	do_quote(fname, header.include_mode);
	if (vedit(fname, NA, NA) == -1) {
		in_mail = save_in_mail;
		unlink(fname);
		clear();
		return -2;
	}
	add_loginfo(fname);
	move(t_lines - 1, 0);
	clrtoeol();
	prints
	    ("\033[5;1;32;44m正在寄件中，请稍候.....                                                        \033[m");
	refresh();
	mailtoall(ans[0] - '0', hour);
	move(t_lines - 1, 0);
	clrtoeol();
	unlink(fname);
	in_mail = save_in_mail;
	return 0;
}

static int gensecBMs(const struct sectree *sec, char *bmfile) {
        FILE *fp;
        int i, j, len;
	
        fp = fopen(bmfile, "w");
        if (NULL == fp)
                return -1;
	len = strlen(sec->basestr);
        for (i = 0; i < numboards; i++) {
                if (!strncmp(bbsinfo.bcache[i].header.sec1, sec->basestr, len) &&
		     strlen(bbsinfo.bcache[i].header.sec1) >= len) {
                        for (j = 0; j < BMNUM; j++) {
                                if (bbsinfo.bcache[i].header.bm[j][0] != '\0')
                        	        fprintf(fp, "%s\n", bbsinfo.bcache[i].header.bm[j]);
			}
                }
        }
        fclose(fp);
        return 0;
}

int mailsecbm() {
	const struct sectree *sec;
	char secbuf[8], buf[STRLEN], bmfile[256];
	int cnt;
	
	modify_user_mode(SMAIL);
	clear();
	stand_title("寄信给区内版主");
	getdata(1, 0, "请输入区代号:", secbuf, 4, DOECHO, YEA);
	if (secbuf[0] == '\0')
		return -1;
	move(2, 0);
	if ((sec  = getsectree(secbuf)) == &sectree) {
		prints("错误的分区!");
		pressanykey();
		return -1;
	}
	if (!USERPERM(currentuser, PERM_SYSOP) && !issecm(secbuf, currentuser->userid)) {
		prints("您无权在该区寄出版主群体信件.");
		pressanykey();
		return -1;
	}
	sprintf(bmfile, MY_BBS_HOME "/bbstmpfs/tmp/%s.%d.sml", currentuser->userid, uinfo.pid);
	if (gensecBMs(sec, bmfile) < 0)
		return -1;
	if ((cnt = listfilecontent(bmfile)) <= 0)
		return 0;
	sprintf(buf, "确实要给%s区的所有版主寄出群体信件吗", secbuf);
	move(2, 0);
	if (askyn(buf, YEA, NA) == NA)
		return -2;
	do_gsend(NULL, NULL, cnt, bmfile);
	sprintf(buf, "%s区版主名单", secbuf);
	system_mail_file(bmfile, currentuser->userid, buf, currentuser->userid);
	unlink(bmfile);
	return 0;
}

#ifdef INTERNET_EMAIL

void
m_internet()
{
	char receiver[STRLEN];

	if (check_mail_perm()) {
		pressreturn();
		return;
	}
	modify_user_mode(SMAIL);

	getdata(1, 0, "收信人E-mail：", receiver, 65, DOECHO, YEA);
	sprintf(genbuf, ".bbs@%s", email_domain());
	if (strstr(receiver, genbuf)
	    || strstr(receiver, ".bbs@localhost")) {
		move(3, 0);
		prints("站内信件, 请用 (S)end 指令来寄\n");
		pressreturn();
	} else if (!invalidaddr(receiver)) {
		*quote_file = '\0';
		clear();
		do_send(receiver, NULL);
	} else {
		move(3, 0);
		prints("收信人不正确, 请重新选取指令\n");
		pressreturn();
	}
	clear();
}
#endif

void
m_init()
{
	sprintf(currmaildir, "mail/%c/%s/%s", mytoupper(currentuser->userid[0]),
		currentuser->userid, DOT_DIR);
}

static int
do_send(userid, title)
char *userid, *title;
{
	struct stat st;
	char filepath[STRLEN];
	char save_title2[STRLEN];
	int internet_mail = 0;
	char uid[80];
	int save_in_mail;
	struct userec *lookupuser;

	strcpy(uid, userid);
	/* I hate go to , but I use it again for the noodle code :-) */
	if (strchr(userid, '@')) {
		internet_mail = YEA;
		goto edit_mail_file;
	}
	/* end of kludge for internet mail */

	if (getuser(userid, &lookupuser) == 0)
		return -1;
	strncpy(uid, lookupuser->userid, IDLEN + 1);
	uid[IDLEN + 1] = 0;
	if (inoverride(currentuser->userid, uid, "rejects"))
		return -3;
/*add by KCN :) */

	if (!(lookupuser->userlevel & PERM_READMAIL))
		return -3;
	setmailfile(filepath, uid, "");
	if (stat(filepath, &st) == -1) {
		if (mkdir(filepath, 0775) == -1)
			return -1;
	} else {
		if (!(st.st_mode & S_IFDIR))
			return -1;
	}
      edit_mail_file:
	if (title == NULL) {
		header.reply_mode = NA;
		strcpy(header.title, "没主题");
	} else {
		header.reply_mode = YEA;
		strcpy(header.title, title);
	}
	header.postboard = NA;
	save_in_mail = in_mail;
	in_mail = YEA;

	setuserfile(genbuf, "signatures");
	ansimore2(genbuf, NA, 0, 18);
	strcpy(header.ds, uid);
	if (post_header(&header)) {
		strsncpy(save_title, header.title, sizeof (save_title));
		sprintf(save_title2, "{%.16s} %.60s", uid, header.title);
	}
	sprintf(filepath, "bbstmpfs/tmp/mail%05d", uinfo.pid);
	do_quote(filepath, header.include_mode);

	if (internet_mail) {
		int res;
		if (vedit(filepath, NA, YEA) == -1) {
			unlink(filepath);
			clear();
			in_mail = save_in_mail;
			return -2;
		}
		add_loginfo(filepath);
		clear();
		prints("信件即将寄给 %s \n", uid);
		prints("标题为： %s \n", header.title);
		if (askyn("确定要寄出吗", YEA, NA) == NA) {
			prints("\n信件已取消...\n");
			res = -2;
		} else {
			if (askyn("是否备份给自己", NA, NA) == YEA)
				mail_file(filepath, currentuser->userid,
					  save_title2, currentuser->userid);
			prints("请稍候, 信件传递中...\n");
			refresh();
			res = bbs_sendmail(filepath, header.title, uid, currentuser->userid, 0);
		}
		unlink(filepath);
		tracelog("%s netmail %s", currentuser->userid, uid);
		in_mail = save_in_mail;
		return res;
	} else {
		if (vedit(filepath, NA, NA) == -1) {
			unlink(filepath);
			clear();
			in_mail = save_in_mail;
			return -2;
		}
		add_loginfo(filepath);
		clear();
		if (askyn("是否备份给自己", NA, NA) == YEA)
			mail_file(filepath, currentuser->userid, save_title2,
				  currentuser->userid);
		setmailfile(genbuf, uid, DOT_DIR);
		if (mail_file(filepath, uid, save_title, currentuser->userid) ==
		    -1) {
			in_mail = save_in_mail;
			return -1;
		}
		unlink(filepath);
		tracelog("%s mail %s", currentuser->userid, uid);
		in_mail = save_in_mail;
		return 0;
	}
}

int
m_send(userid)
char userid[];
{
	char uident[STRLEN];
	// 永远可以给 SYSOP 发信
	if (userid == NULL || strcmp(userid, "SYSOP")) {
		if (check_mail_perm()) {
			pressreturn();
			return 0;
		}
	}
	modify_user_mode(SMAIL);
	if (			/*(uinfo.mode != LUSERS && uinfo.mode != LAUSERS
				   && uinfo.mode != FRIEND && uinfo.mode != GMENU)
				   || */ userid == NULL) {
		move(1, 0);
		clrtoeol();
		usercomplete("收信人： ", uident);
		if (uident[0] == '\0') {
			return FULLUPDATE;
		}
	} else
		strcpy(uident, userid);
	clear();
	*quote_file = '\0';
	switch (do_send(uident, NULL)) {
	case -1:
		prints("收信者不正确\n");
		break;
	case -2:
		prints("取消\n");
		break;
	case -3:
		prints("[%s] 无法收信\n", uident);
		break;
	default:
		prints("信件已寄出\n");
	}
	pressreturn();
	return FULLUPDATE;
}

int
M_send()
{
	if (!USERPERM(currentuser, PERM_LOGINOK))
		return 0;
	return m_send(NULL);
}
static int
read_mail(fptr)
struct fileheader *fptr;
{
	setmailfile(genbuf, currentuser->userid, fh2fname(fptr));
	ansimore(genbuf, NA);
	fptr->accessed |= FH_READ;
	return 0;
}

int mrd;

static int
read_new_mail(fptr)
struct fileheader *fptr;
{
	static int idc;
	char done = NA, delete_it;
	char fname[256];

	if (fptr == NULL) {
		idc = 0;
		return 0;
	}
	idc++;
	if ((fptr->accessed & FH_READ))
		return 0;
	prints("读取 %s 寄来的 '%s' ?\n", fptr->owner, fptr->title);
	getdata(1, 0, "(Y)读取 (N)不读 (Q)离开 [Y]: ", genbuf, 2, DOECHO, YEA);
	if (genbuf[0] == 'q' || genbuf[0] == 'Q') {
		clear();
		return QUIT;
	}
	if (genbuf[0] != 'y' && genbuf[0] != 'Y' && genbuf[0] != '\0') {
		clear();
		return 0;
	}
	read_mail(fptr);
	mrd = 1;
	delete_it = NA;
	while (!done) {
		move(t_lines - 1, 0);
		prints("(R)回信, (D)删除, (G)继续 ? [G]: ");
		switch (egetch()) {
		case 'R':
		case 'r':
			*quote_file = '\0';
			mail_reply(idc, fptr, currmaildir);
			break;
		case 'D':
		case 'd':
			delete_it = YEA;
		default:
			done = YEA;
		}
		if (!done) {
			setmailfile(fname, currentuser->userid, fh2fname(fptr));
			ansimore(fname, NA);	/* re-read */
		}
	}
	if (delete_it) {
		clear();
		prints("删除信件 '%s' ", fptr->title);
		getdata(1, 0, "(Y)确定 ,(N)取消 [N]: ", genbuf, 2, DOECHO, YEA);
		if (genbuf[0] == 'Y' || genbuf[0] == 'y') {	/* if not yes quit */
			fptr->accessed |= FH_DEL;
		}
	}
	if (substitute_record(currmaildir, fptr, sizeof (*fptr), idc))
		return -1;
	clear();
	return 0;
}

int
m_new()
{
	clear();
	mrd = 0;
	modify_user_mode(RMAIL);
	read_new_mail(NULL);
	if (!file_exist(currmaildir)
	    || apply_record(currmaildir, (void *) read_new_mail,
			    sizeof (struct fileheader)) == -1) {
		clear();
		move(0, 0);
		prints("No new messages\n\n\n");
		return -1;
	}
	clear();
	move(0, 0);
	if (mrd)
		prints("No more messages.\n\n\n");
	else
		prints("No new messages.\n\n\n");
	return -1;
}

static int
mailtitle()
{
	showtitle("邮件选单    ", MY_BBS_NAME);

	prints
	    ("离开[\033[1;32m←\033[m,\033[1;32me\033[m]  选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m]  阅读信件[\033[1;32m→\033[m,\033[1;32mRtn\033[m]  回信[\033[1;32mR\033[m]  砍信／清除旧信[\033[1;32md\033[m,\033[1;32mD\033[m]  求助[\033[1;32mh\033[m]\033[m\n");
	prints("\033[1;44m编号   %-12s %6s  %-28s %d %s %d %s\033[m\n",
	       "发信者", "日  期", "标  题          当前信箱容量",
	       max_mailsize(currentuser), "k,已用空间",
	       get_mailsize(currentuser), "k");
	clrtobot();
	return 0;
}

static char *
maildoent(num, ent, buf)
int num;
struct fileheader *ent;
char buf[512];
{
	char b2[512];
	char status;
	char *date;
	extern char ReadPost[];
	extern char ReplyPost[];
	char c1[8];
	char c2[8];
	int same = NA;
	//add by gluon
	char replymark;
	char delmark;
	//end

	if (ent->filetime > 740000000) {
		time_t t = ent->filetime;
		date = ctime(&t) + 4;
	} else {
		date = "";
	}

	strcpy(c2, "\033[1;33m");
	strcpy(c1, "\033[1;36m");
	if (!strcmp(ReadPost, ent->title) || !strcmp(ReplyPost, ent->title))
		same = YEA;
	strsncpy(b2, ent->owner, sizeof (b2));

	//add by gluon
	/* Added by deardragon 1999.11.15 给已回信件加上 "回信" 标记 ('R') */
	if (ent->accessed & FH_REPLIED)	/* 邮件已回复 */
		replymark = 'R';
	else
		replymark = ' ';
	if (ent->accessed & FH_DEL)
		delmark = 'X';
	else
		delmark = ' ';
	/* Added End. */
	//end

	if (ent->accessed & FH_READ) {
		if (ent->accessed & FH_MARKED)
			status = 'm';
		else
			status = ' ';
	} else {
		if (ent->accessed & FH_MARKED)
			status = 'M';
		else
			status = 'N';
	}
	if (!strncmp("Re:", ent->title, 3)) {
		sprintf(buf,
			" %s%3d\033[m%c%c%c %-12.12s %6.6s %c%s%.50s\033[m",
			same ? c1 : "", num, delmark, replymark, status, b2,
			date, ent->accessed & FH_ATTACHED ? '@' : ' ',
			same ? c1 : "", ent->title);
	} else {
		sprintf(buf,
			" %s%3d\033[m%c%c%c %-12.12s %6.6s %c★ %s%.47s\033[m",
			same ? c2 : "", num, delmark, replymark, status, b2,
			date, ent->accessed & FH_ATTACHED ? '@' : ' ',
			same ? c2 : "", ent->title);
	}
	return buf;
}

static int
mail_read(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char notgenbuf[128];
	int readnext;
	char done = NA, delete_it, replied;

	clear();
	readnext = NA;
	setqtitle(fileinfo->title);
	directfile(notgenbuf, direct, fh2fname(fileinfo));
	delete_it = replied = NA;
	while (!done) {
		ansimore(notgenbuf, NA);
		move(t_lines - 1, 0);
		prints("(R)回信, (D)删除, (G)继续? [G]: ");
		switch (egetch()) {
		case 'R':
		case 'r':
			replied = YEA;
			*quote_file = '\0';
			mail_reply(ent, fileinfo, direct);
			break;
		case ' ':
		case 'j':
		case KEY_RIGHT:
		case KEY_DOWN:
		case KEY_PGDN:
			done = YEA;
			readnext = YEA;
			break;
		case 'D':
		case 'd':
			delete_it = YEA;
		default:
			done = YEA;
		}
	}
	if (!delete_it && !(fileinfo->accessed & FH_READ)) {
		fileinfo->accessed |= FH_READ;
		substitute_record(currmaildir, fileinfo, sizeof (*fileinfo),
				  ent);
	}
	if (delete_it)
		return mail_del(ent, fileinfo, direct);
	if (readnext == YEA)
		return READ_NEXT;
	return FULLUPDATE;
}

 /*ARGSUSED*/ int
mail_reply(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char uid[STRLEN];
	char title[STRLEN];
	char *t;
	clear();

	sprintf(genbuf, "MAILER-DAEMON@%s", email_domain());
	if (strstr(fileinfo->owner, genbuf)) {
		ansimore("help/mailerror-explain", YEA);
		return FULLUPDATE;
	}
	if (check_mail_perm()) {
		pressreturn();
		return 0;
	}
	clear();
	modify_user_mode(SMAIL);
	strsncpy(uid, fh2owner(fileinfo), sizeof (uid));
	if (strchr(uid, '.')) {
		char filename[STRLEN];
		directfile(filename, direct, fh2fname(fileinfo));
		if (!getdocauthor(filename, uid, sizeof (uid))) {
			prints("无法投递\n");
			pressreturn();
			return FULLUPDATE;
		}
	}
	if ((t = strchr(uid, ' ')) != NULL)
		*t = '\0';
	if (strncasecmp(fileinfo->title, "Re:", 3))
		strcpy(title, "Re: ");
	else
		title[0] = '\0';
	strncat(title, fileinfo->title, sizeof (title) - 5);

	if (quote_file[0] == '\0')
		setmailfile(quote_file, currentuser->userid,
			    fh2fname(fileinfo));
	strcpy(quote_user, fileinfo->owner);
	switch (do_send(uid, title)) {
	case -1:
		prints("无法投递\n");
		break;
	case -2:
		prints("取消回信\n");
		break;
	case -3:
		prints("[%s] 无法收信\n", uid);
		break;
	default:
		//add by gluon
		/* Added bye deardragon 1999.11.15 给已回信件加上 "回信" 标记 ('R') */
		if (ent >= 0) {
			fileinfo->accessed |= FH_REPLIED;
			substitute_record(direct, fileinfo, sizeof (*fileinfo),
					  ent);
		}
		/* Added End. */
		//end
		prints("信件已寄出\n");
	}
	pressreturn();
	return FULLUPDATE;
}

static int
mail_del(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char buf[512];

	clear();
	if (-2 == get_mailsize(currentuser)) {
		errlog("strange user %s", currentuser->userid);
		prints("邮箱内部错误,请联系系统维护");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	prints("如果这是一封站外的垃圾邮件，请您使用 b 键来进行删除，以便我们改进系统。\n");
	prints("删除信件 [%s] ", fileinfo->title);
	getdata(2, 0, "(Yes, or No) [N]: ", genbuf, 2, DOECHO, YEA);
	if (genbuf[0] != 'Y' && genbuf[0] != 'y') {	/* if not yes quit */
		move(3, 0);
		prints("Quitting Delete Mail\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	currfiletime = fileinfo->filetime;
	directfile(buf, direct, fh2fname(fileinfo));
	if (!delete_dir_callback
	    (direct, ent, currfiletime, (void *) update_mailsize_down,
	     currentuser->userid)) {
		deltree(buf);
		return DIRCHANGED;
	}
	move(3, 0);
	prints("Delete failed\n");
	pressreturn();
	clear();
	return FULLUPDATE;
}

#ifdef INTERNET_EMAIL

int
mail_forward(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char buf[STRLEN];
	if (!USERPERM(currentuser, PERM_FORWARD)) {
		return DONOTHING;
	}
	directfile(buf, direct, fh2fname(fileinfo));
	switch (doforward(buf, fileinfo->title, 0)) {
	case 0:
		prints("文章转寄完成!\n");
		break;
	case -1:
		prints("转寄失败: 系统发生错误.\n");
		break;
	case -2:
		prints("转寄失败: 不正确的收信地址.\n");
		break;
	default:
		prints("取消转寄...\n");
	}
	do_delay(1);
	pressreturn();
	clear();
	return FULLUPDATE;
}

int
mail_u_forward(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char buf[STRLEN];
	if (!USERPERM(currentuser, PERM_FORWARD)) {
		return DONOTHING;
	}
	directfile(buf, direct, fh2fname(fileinfo));
	switch (doforward(buf, fileinfo->title, 1)) {
	case 0:
		prints("文章转寄完成!\n");
		break;
	case -1:
		prints("转寄失败: 系统发生错误.\n");
		break;
	case -2:
		prints("转寄失败: 不正确的收信地址.\n");
		break;
	default:
		prints("取消转寄...\n");
	}
	do_delay(1);
	pressreturn();
	clear();
	return FULLUPDATE;
}

#endif

static int
mail_del_range(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	if (-2 == get_mailsize(currentuser)) {
		errlog("strange user %s", currentuser->userid);
		prints("邮箱内部错误,请联系系统维护");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	return (del_range(ent, fileinfo, direct));
}

static int
mail_mark(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	if (fileinfo->accessed & FH_MARKED)
		fileinfo->accessed &= ~FH_MARKED;
	else
		fileinfo->accessed |= FH_MARKED;
	substitute_record(currmaildir, fileinfo, sizeof (*fileinfo), ent);
	return (PARTUPDATE);
}

static int
mail_markdel(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	if (fileinfo->accessed & FH_DEL)
		fileinfo->accessed &= ~FH_DEL;
	else
		fileinfo->accessed |= FH_DEL;
	substitute_record(currmaildir, fileinfo, sizeof (*fileinfo), ent);
	return (PARTUPDATE);
}

struct one_key mail_comms[] = {
	{'d', mail_del, "删除信件"},
	{'D', mail_del_range, "区段删除"},
	{Ctrl('P'), M_send, "发送信件"},
	{'E', edit_post, "编辑信件"},
	{'r', mail_read, "阅读信件"},
	{'R', mail_reply, "回复信件"},
	{'m', mail_mark, "mark信件"},
	{'i', Save_post, "将文章存入暂存档"},
	{'I', Import_post, "将信件放入精华区"},
	{'x', into_announce, "进入精华区"},
	{KEY_TAB, show_user_notes, "查看用户备忘录"},
#ifdef INTERNET_EMAIL
	{'F', mail_forward, "转寄信件"},
	{'U', mail_u_forward, "uuencode 转寄"},
#endif
	{'a', auth_search_down, "向后搜索作者"},
	{'A', auth_search_up, "向前搜索作者"},
	{'/', t_search_down, "向后搜索标题"},
	{'?', t_search_up, "向前搜索标题"},
	{'\'', post_search_down, "向后搜索内容"},
	{'\"', post_search_up, "向前搜索内容"},
	{']', thread_down, "向后同主题"},
	{'[', thread_up, "向前同主题"},
	{Ctrl('A'), show_author, "作者简介"},
	{'\\', SR_last, "最后一篇同主题文章"},
	{'=', SR_first, "第一篇同主题文章"},
	{'L', show_allmsgs, "查看消息"},
	{Ctrl('C'), do_cross, "转贴文章"},
	{'n', SR_first_new, "主题未读的第一篇"},
	{'p', SR_read, "相同主题的阅读"},
	{Ctrl('U'), SR_author, "相同作者阅读"},
	{'h', mailreadhelp, "查看帮助"},
	{'!', Q_Goodbye, "快速离站"},
	{'S', s_msg, "传送讯息"},
	{'c', t_friends, "查看好友"},
	{'C', friend_author, "增加作者为好友"},
	{Ctrl('E'), mail_rjunk, "翻拣垃圾"},
	{'t', mail_markdel, "标记删除信件"},
	{Ctrl('Y'), zmodem_sendfile, "ZMODEM 下载"},
	{'b', mail_spam, "垃圾信件"}, 
	{'\0', NULL, ""}
};

int
m_read()
{
	int savemode;
	int save_in_mail;
	char savedir[255];
	int savedmode;
	save_in_mail = in_mail;
	in_mail = YEA;
	savedmode = digestmode;
	strsncpy(savedir, currdirect, sizeof (savedir));
	savemode = uinfo.mode;
	m_init();
	i_read(RMAIL, currmaildir, mailtitle, (void *) maildoent, mail_comms,
	       sizeof (struct fileheader));
	modify_user_mode(savemode);
	in_mail = save_in_mail;
	digestmode = savedmode;
	strsncpy(currdirect, savedir, sizeof (currdirect));
	return 0;
}

int
g_send()
{
	char uident[13], tmp[3];
	int cnt, i, n, fmode = NA;
	int keepgoing = 0;
	int key;
	char maillist[STRLEN];
	struct userec *lookupuser;

	if (check_mail_perm()) {
		pressreturn();
		return 0;
	}
	modify_user_mode(SMAIL);
	*quote_file = '\0';
	clear();
	sethomefile(maillist, currentuser->userid, "maillist");
	cnt = listfilecontent(maillist);
	while (1) {
		if (cnt > maxrecp - 10) {
			move(2, 0);
			prints("目前限制寄信给 \033[1m%d\033[m 人", maxrecp);
		}
		if (!keepgoing) {
			getdata(0, 0,
				"(A)增加 (D)删除 (I)引入好友 (C)清除目前名单 (E)放弃 (S)寄出? [S]： ",
				tmp, 2, DOECHO, YEA);
		}
		if (tmp[0] == '\n' || tmp[0] == '\0' || tmp[0] == 's'
		    || tmp[0] == 'S') {
			break;
		}
		if (tmp[0] == 'a' || tmp[0] == 'd' || tmp[0] == 'A'
		    || tmp[0] == 'D') {
			move(1, 0);
			if (tmp[0] == 'a' || tmp[0] == 'A')
				usercomplete
				    ("请依次输入要添加的使用者代号(只按 ENTER 结束输入): ",
				     uident);
			else
				namecomplete
				    ("请依次输入要删除的使用者代号(只按 ENTER 结束输入): ",
				     uident);
			move(1, 0);
			clrtoeol();
			if (uident[0] == '\0') {
				keepgoing = 0;
				continue;
			}
			keepgoing = 1;
			if (!getuser(uident, &lookupuser)) {
				lookupuser = NULL;
				move(2, 0);
				prints("这个使用者代号是错误的.\n");
				continue;
			}
		}
		switch (tmp[0]) {
		case 'A':
		case 'a':
			if (!lookupuser
			    || !(lookupuser->userlevel & PERM_READMAIL)) {
				move(2, 0);
				prints("无法送信给: \033[1m%s\033[m\n",
				       lookupuser->userid);
				break;
			} else if (seek_in_file(maillist, uident)) {
				move(2, 0);
				prints("已经列为收件人之一 \n");
				break;
			}
			addtofile(maillist, uident);
			cnt++;
			break;
		case 'E':
		case 'e':
		case 'Q':
		case 'q':
			cnt = 0;
			break;
		case 'D':
		case 'd':
			{
				if (seek_in_file(maillist, uident)) {
					del_from_file(maillist, uident);
					cnt--;
				}
				break;
			}
		case 'I':
		case 'i':
			n = 0;
			clear();
			for (i = cnt; i < maxrecp && n < uinfo.fnum; i++) {
				move(2, 0);
				clrtoeol();
				getuserbynum(uinfo.friend[n], &lookupuser);
				if (lookupuser == NULL)
					continue;
				prints("%s\n", lookupuser->userid);
				move(3, 0);
				n++;
				prints
				    ("(A)全部加入 (Y)加入 (N)不加入 (Q)结束? [Y]:");
				if (!fmode)
					key = igetkey();
				else
					key = 'Y';
				if (key == 'q' || key == 'Q')
					break;
				if (key == 'A' || key == 'a') {
					fmode = YEA;
					key = 'Y';
				}
				if (key == '\0' || key == '\n' || key == 'y'
				    || key == 'Y') {
					if (!getuser
					    (lookupuser->userid, &lookupuser)) {
						move(4, 0);
						prints
						    ("这个使用者代号是错误的.\n");
						i--;
						continue;
					} else
					    if (!
						(lookupuser->userlevel &
						 PERM_READMAIL)) {
						move(4, 0);
						prints
						    ("无法送信给: \033[1m%s\033[m\n",
						     lookupuser->userid);
						i--;
						continue;
					} else
					    if (seek_in_file
						(maillist,
						 lookupuser->userid)) {
						i--;
						continue;
					}
					addtofile(maillist,
						  lookupuser->userid);
					cnt++;
				}
			}
			fmode = NA;
			clear();
			break;
		case 'C':
		case 'c':
			unlink(maillist);
			cnt = 0;
			break;
		}
		if (strchr("EeQq", tmp[0]))
			break;
		move(5, 0);
		clrtobot();
		if (cnt > maxrecp)
			cnt = maxrecp;
		move(3, 0);
		clrtobot();
		if (keepgoing)
			ansimore2(maillist, 0, 3, t_lines - 4);
		else
			listfilecontent(maillist);
	}
	FreeNameList();
	if (cnt > 0) {
		switch (do_gsend(NULL, NULL, cnt, maillist)) {
		case -1:
			prints("信件目录错误\n");
			break;
		case -2:
			prints("取消\n");
			break;
		default:
			prints("信件已寄出\n");
		}
		pressreturn();
	}
	return 0;
}

/*Add by SmallPig*/

static int
do_gsend(userid, title, num, maillist)
char *userid[], *title, *maillist;
int num;
{
	char filepath[STRLEN], tmpfile[STRLEN];
	int cnt, save_in_mail;
	FILE *mp = NULL;
	struct userec *lookupuser;

	save_in_mail = in_mail;
	in_mail = YEA;
	sprintf(genbuf, "%s (%s)", currentuser->userid, currentuser->username);
	header.reply_mode = NA;
	strcpy(header.title, "没主题");
	strcpy(header.ds, "寄信给一群人");
	header.postboard = NA;
	sprintf(tmpfile, MY_BBS_HOME "/bbstmpfs/tmp/gsend.%s.%05d", currentuser->userid, uinfo.pid);
	if (post_header(&header)) {
		sprintf(save_title, "[群体信件] %.60s", header.title);
	}
	setquotefile("");
	do_quote(tmpfile, header.include_mode);
	if (vedit(tmpfile, NA, NA) == -1) {
		unlink(tmpfile);
		clear();
		in_mail = save_in_mail;
		return -2;
	}
	add_loginfo(tmpfile);
	clear();
	prints("\033[5;1;32m正在寄件中，请稍候...\033[m");
	if (maillist != NULL) {
		if ((mp = fopen(maillist, "r")) == NULL) {
			in_mail = save_in_mail;
			unlink(tmpfile);
			return -3;
		}
	}
	for (cnt = 0; cnt < num; cnt++) {
		char uid[13];
		char buf[STRLEN];

		if (maillist == NULL) {
			getuserbynum(uinfo.friend[cnt], &lookupuser);
			if (lookupuser)
				strcpy(uid, lookupuser->userid);
		} else {
			if (fgets(buf, STRLEN, mp) != NULL) {
				if (strtok(buf, " \n\r\t") != NULL)
					strcpy(uid, buf);
				else
					continue;
			} else
				break;
		}
		sprintf(filepath, "mail/%c/%s", mytoupper(uid[0]), uid);
		mail_file(tmpfile, uid, save_title, currentuser->userid);
	}
	unlink(tmpfile);
	clear();
	if (maillist != NULL)
		fclose(mp);
	in_mail = save_in_mail;
	return 0;
}

/*Add by SmallPig*/
int
ov_send()
{
	int all, i;
	struct userec *lookupuser;

	if (check_mail_perm()) {
		pressreturn();
		return 0;
	}
	modify_user_mode(SMAIL);
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints
	    ("寄信给好友名单中的人，目前本站限制仅可以寄给 \033[1m%d\033[m 位。\n",
	     maxrecp);
	if (uinfo.fnum <= 0) {
		prints("你并没有设定好友。\n");
		pressanykey();
		clear();
		return 0;
	} else {
		prints("名单如下：\n");
	}
	all = (uinfo.fnum >= maxrecp) ? maxrecp : uinfo.fnum;
	for (i = 0; i < all; i++) {
		if (getuserbynum(uinfo.friend[i], &lookupuser) <= 0)
			continue;
		prints("%-12s ", lookupuser->userid);
		if ((i + 1) % 6 == 0)
			outc('\n');
	}
	pressanykey();
	switch (do_gsend(NULL, NULL, all, NULL)) {
	case -1:
		prints("信件目录错误\n");
		break;
	case -2:
		prints("信件取消\n");
		break;
	default:
		prints("信件已寄出\n");
	}
	pressreturn();
	return 0;
}

int
club_send()
{
	int all;
	char maillist[STRLEN];
	if (check_mail_perm()) {
		pressreturn();
		return 0;
	}
	modify_user_mode(SMAIL);
	move(1, 0);
	clrtobot();
	move(2, 0);
	setbfile(maillist, currboard, "club_users");
	all = listfilecontent(maillist);
	FreeNameList();
	switch (do_gsend(NULL, NULL, all, maillist)) {
	case -1:
		prints("信件目录错误\n");
		break;
	case -2:
		prints("信件取消\n");
		break;
	default:
		prints("信件已寄出\n");
	}
	pressreturn();
	modify_user_mode(READING);
	return 0;
}

#ifdef INTERNET_EMAIL
int
doforward(filepath, oldtitle, mode)
char *filepath, *oldtitle;
int mode;
{
	static char address[STRLEN];
	char fname[STRLEN], tmpfname[STRLEN];
	char receiver[STRLEN];
	char title[STRLEN];
	int return_no;
	time_t now;
	int filter_ansi;
	struct userdata currentdata;
	struct userec *lookupuser;

	clear();
	if (address[0] == '\0') {
		loaduserdata(currentuser->userid, &currentdata);
		strncpy(address, currentdata.email, STRLEN);
		address[STRLEN - 1] = 0;
	}
	if (USERPERM(currentuser, PERM_SETADDR)) {
		prints
		    ("请直接按 Enter 接受括号内提示的地址, 或者输入其他地址\n");
		prints("把信件转寄给 [%s]\n", address);
		getdata(2, 0, "==> ", receiver, 70, DOECHO, YEA);
		if (receiver[0] != '\0') {
			strncpy(address, receiver, STRLEN);
			address[STRLEN - 1] = 0;
		}
	}
	sprintf(genbuf, ".bbs@%s", email_domain());
	if (strstr(address, genbuf)
	    || strstr(address, ".bbs@localhost")) {
		char *pos = strchr(address, '.');
		*pos = '\0';
	}
	if (check_mail_perm()) {
		pressreturn();
		return -1;
	}
	sprintf(genbuf, "确定将文章寄给 %s 吗", address);
	if (askyn(genbuf, YEA, NA) == 0)
		return 1;
	if (invalidaddr(address)) {
		if (!getuser(address, &lookupuser)
		    || check_mail_perm())
			return -2;
		if (inoverride
		    (currentuser->userid, lookupuser->userid, "rejects"))
			return -3;
	}
	sprintf(tmpfname, "tmp/forward.%s.%05d", currentuser->userid,
		uinfo.pid);
	copyfile(filepath, tmpfname);
	if (askyn("是否修改文章内容", NA, NA) == 1) {
		if (vedit(tmpfname, NA, NA) == -1) {
			if (askyn("是否寄出未修改的文章", YEA, NA) == 0) {
				unlink(tmpfname);
				clear();
				return 1;
			}
		} else if (ADD_EDITMARK) {
			now = time(NULL);
			add_edit_mark(tmpfname, currentuser->userid, now,
				      fromhost);
		}
		clear();
	}
	add_crossinfo(tmpfname, 2);
	prints("转寄信件给 %s, 请稍候....\n", address);
	refresh();

	if (mode == 0)
		strcpy(fname, tmpfname);
	else if (mode == 1) {
		FILE *fr, *fw;
		char uuname[40];
		sprintf(fname, "bbstmpfs/tmp/file.uu%05d", uinfo.pid);
		fr = fopen(tmpfname, "r");
		if (NULL == fr) {
			unlink(tmpfname);
			return -3;
		}
		fw = fopen(fname, "w");
		if (NULL == fw) {
			fclose(fr);
			unlink(tmpfname);
			return -4;
		}
		snprintf(uuname, sizeof (uuname), "%s-BBSMAIL.%d", MY_BBS_ID,
			 (int) time(NULL));
		uuencode(fr, fw, file_size(tmpfname), uuname);
		unlink(tmpfname);
		fclose(fw);
		fclose(fr);
	}
	sprintf(title, "[转寄] %.70s", oldtitle);
	if (!strpbrk(address, "@.")) {
		return_no =
		    mail_file(fname, lookupuser->userid, title,
			      currentuser->userid);
		if (return_no != -1)
			return_no = 0;
	} else {
		filter_ansi = askyn("过滤 ANSI 控制码", NA, NA);
		return_no = bbs_sendmail(fname, title, address, currentuser->userid, filter_ansi);
	}
	if (mode == 1) {
		unlink(fname);
	}
	unlink(tmpfname);
	return (return_no);
}

#endif

static void
getmailinfo(char *path, struct fileheader *rst)
{
	FILE *fp;
	char *p, buf1[256], buf2[256];
	rst->sizebyte = numbyte(file_size(path));
	strcpy(rst->title, "无题");
	strcpy(rst->owner, "deliver");
	if ((fp = fopen(path, "r")) == NULL)
		return;
	buf1[0] = 0;
	while (fgets(buf2, sizeof (buf2), fp) != NULL) {
		if ((strncmp(buf1, "寄信人: ", 8)
		     && strncmp(buf1, "发信人: ", 8))
		    || (strncmp(buf2, "标  题: ", 8)
			&& strncmp(buf2, "标　题: ", 8))) {
			strcpy(buf1, buf2);
			continue;
		}
		p = strchr(buf1 + 8, ' ');
		if (p)
			*p = 0;
		if ((p = strchr(buf1 + 8, '(')))
			*p = 0;
		if ((p = strchr(buf1 + 8, '\n')))
			*p = 0;
		fh_setowner(rst, buf1 + 8, 0);
		if ((p = strchr(buf2 + 8, '\n')))
			*p = 0;
		if ((p = strchr(buf2 + 8, '\r')))
			*p = 0;
		strsncpy(rst->title, buf2 + 8, sizeof (rst->title));
		break;
	}
	fclose(fp);
	return;
}

static int
mail_rjunk()
{
	DIR *dirp;
	struct dirent *direntp;
	int len, count;
	char buf[256], rpath[256], npath[256];
	struct fileheader rstmsg;

	clear();
	getdata(1, 0, "要拣垃圾吗?(Yes, or No) [N]: ", genbuf, 2, DOECHO, YEA);
	if (genbuf[0] != 'Y' && genbuf[0] != 'y') {
		move(2, 0);
		prints("哦~~~~~,怕脏啊...\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	len =
	    sprintf(buf, "%c/%s/", mytoupper(currentuser->userid[0]),
		    currentuser->userid);
	normalize(buf);
	dirp = opendir(MY_BBS_HOME "/mail/.junk");
	if (dirp == NULL)
		return FULLUPDATE;
	count = 0;
	while ((direntp = readdir(dirp)) != NULL) {
		if (strncmp(buf, direntp->d_name, len)
		    || strncmp(direntp->d_name + len, "M.", 2))
			continue;
		sprintf(rpath, MY_BBS_HOME "/mail/.junk/%s", direntp->d_name);
		bzero(&rstmsg, sizeof (struct fileheader));
		getmailinfo(npath, &rstmsg);
		if (system_mail_file
		    (rpath, currentuser->userid, rstmsg.title,
		     rstmsg.owner) >= 0) {
			unlink(rpath);
			count++;
		}
	}
	closedir(dirp);
	if (count) {
		prints("哦,拣了 %d 件垃圾回来.\n", count);
	} else
		prints("哎呀,这年头垃圾也难找...\n");
	pressreturn();
	return FULLUPDATE;
}

static int
m_cancel_1(struct fileheader *fh, char *receiver)
{
	char buf[256];
	FILE *fp;
	time_t now;
	if (strncmp(currentuser->userid, fh2owner(fh), IDLEN + 1)
	    || (fh->accessed & FH_READ))
		return 0;
	snprintf(buf, sizeof (buf), "您要撤回邮件<%.50s>吗?", fh->title);
	if (YEA == askyn(buf, NA, NA)) {
		setmailfile(buf, receiver, fh2fname(fh));
		system_mail_file(buf, currentuser->userid,
				 "[系统]撤回的邮件备份", currentuser->userid);
		fp = fopen(buf, "r+");
		if (NULL == fp) {
			prints("不能打开文件写,请联系系统维护!");
			return 2;
		}
		keepoldheader(fp, SKIPHEADER);
		now = time(0);
		fprintf(fp, "本文已经于 %s 被 %s 撤回\n", Ctime(now),
			currentuser->userid);
		ftruncate(fileno(fp), ftell(fp));
		fclose(fp);
		return 1;
	}
	return 0;
}

int
m_cancel(userid)
char userid[];
{
	char uident[STRLEN], buf[STRLEN];

	if (check_mail_perm()) {
		pressreturn();
		return 0;
	}
	if ((uinfo.mode != LUSERS && uinfo.mode != LAUSERS
	     && uinfo.mode != FRIEND && uinfo.mode != GMENU)
	    || userid == NULL) {
		move(1, 0);
		clrtoeol();
		modify_user_mode(SMAIL);
		usercomplete("撤回给谁的信件： ", uident);
		if (uident[0] == '\0') {
			return FULLUPDATE;
		} else if (!strcmp(currentuser->userid, uident)) {
			prints("自己给自己写信就够变态了,居然还要自己撤回啊?");
			pressreturn();
			return FULLUPDATE;
		}
	} else
		strcpy(uident, userid);
	clear();
	setmailfile(buf, uident, ".DIR");
	if (!new_apply_record
	    (buf, sizeof (struct fileheader), (void *) m_cancel_1, uident))
		prints("没有找到可以撤回的信件\n");
	pressreturn();
	return FULLUPDATE;
}

int
post_reply(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char uid[STRLEN];
	char title[STRLEN];
	struct userec *lookupuser;
	if (!strcmp(currentuser->userid, "guest"))
		return DONOTHING;

	clear();
	if (!USERPERM(currentuser, PERM_LOGINOK)) {
		pressreturn();
		return FULLUPDATE;
	}
	if (check_mail_perm()) {
		pressreturn();
		return FULLUPDATE;
	}
	modify_user_mode(SMAIL);
/* indicate the quote file/user */
	setbfile(quote_file, currboard, fh2fname(fileinfo));
	strcpy(quote_user, fh2owner(fileinfo));
/* find the author */
	if (!getuser(quote_user, &lookupuser)) {
		getdocauthor(quote_file, uid, sizeof (uid));
		if (invalidaddr(uid)) {
			prints("Error: Cannot find Author ... \n");
			pressreturn();
		}
	} else
		strsncpy(uid, quote_user, sizeof (uid));
	/* make the title */
	if (toupper(fileinfo->title[0]) != 'R'
	    || fileinfo->title[1] != 'e' || fileinfo->title[2] != ':')
		strcpy(title, "Re: ");
	else
		title[0] = '\0';
	strncat(title, fileinfo->title, STRLEN - 5);
/* edit, then send the mail */
	switch (do_send(uid, title)) {
	case -1:
		prints("系统无法送信\n");
		break;
	case -2:
		prints("送信动作已经中止\n");
		break;
	case -3:
		prints("使用者 '%s' 无法收信\n", uid);
		break;
	default:
		prints("信件已成功地寄给原作者 %s\n", uid);
	}
	pressreturn();
	return FULLUPDATE;
}

static int
show_user_notes()
{
	char buf[256];
	setuserfile(buf, "notes");
	if (file_isfile(buf)) {
		ansimore(buf, YEA);
		return FULLUPDATE;
	}
	clear();
	move(10, 15);
	prints("您尚未在 InfoEdit->WriteFile 编辑个人备忘录。\n");
	pressanykey();
	return FULLUPDATE;
}

static int
mailone(uentp, arg)
struct userec *uentp;
char *arg;
{
	char filename[STRLEN];

	if (uentp->userid[0] == 0 || uentp->kickout != 0)
		return 1;
	sprintf(filename, "%s/mail/S/SYSOP/M.%d.A", MY_BBS_HOME, mailallmode);
	if (uentp->stay / 60 / 60 < mailhour)
		return 1;
	if (!strcmp(uentp->userid, "SYSOP"))
		return 1;
	if ((uentp->userlevel == PERM_BASIC && mailmode == 1) ||
	    (uentp->userlevel & PERM_POST && mailmode == 2) ||
	    (uentp->userlevel & PERM_BOARDS && mailmode == 3) ||
	    (uentp->userlevel & PERM_ARBITRATE && mailmode == 4) ||
	    (uentp->userdefine & DEF_SEEWELC1 && mailmode == 5) ||
	    (uentp->userlevel & PERM_SYSOP && mailmode == 6)) {
		system_mail_link(filename, uentp->userid, save_title,
				 currentuser->userid);
	}
	return 0;
}

int
mailtoall(mode, hour)
int mode, hour;
{
	char filename[STRLEN];
	sprintf(filename, "tmp/mailall.%s", currentuser->userid);
	mailmode = mode;
	mailhour = hour;
	mailallmode =
	    system_mail_file(filename, "SYSOP", save_title,
			     currentuser->userid);
	if (mailallmode == -1) {
		prints("SYSOP 信箱故障，请联系系统维护人员。");
		pressreturn();
		return -1;
	}
	if (apply_passwd((void *) mailone, NULL) == -1) {
		prints("No Users Exist");
		pressreturn();
		return -1;
	}
	mailallmode = 0;
	return 0;
}

int
bogomail()
{
	modify_user_mode(RMAIL);
	clear();
	move(10,0);
	prints("想查看垃圾邮件箱，请访问：\n");
	prints("http://%s/" SMAGIC "%s/bbsspam\n", MY_BBS_DOMAIN, temp_sessionid);
	pressreturn();
	return 0;
}

static int
mail_spam(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char buf[512];
	struct yspam_ctx *yctx;
	clear();
	if(-2==get_mailsize(currentuser)){
		errlog("strange user %s", currentuser->userid);
		prints("邮箱内部错误,请联系系统维护");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	prints("这真的是一封垃圾邮件 [%s] ", fileinfo->title);
	getdata(1, 0, "(Yes, or No) [N]: ", genbuf, 2, DOECHO, YEA);
	if (genbuf[0] != 'Y' && genbuf[0] != 'y') {	/* if not yes quit */
		move(2, 0);
		prints("Quitting Delete Mail\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	currfiletime = fileinfo->filetime;
	directfile(buf, direct, fh2fname(fileinfo));
	yctx = yspam_init("127.0.0.1");
	if (yspam_feed_spam(yctx, currentuser->userid, fh2fname(fileinfo)) != 0) {
		yspam_fini(yctx);
		clear();
		move(2, 0);
		prints("反馈失败，可能不是站外信件，请使用删除功能");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	yspam_fini(yctx);
	if (!delete_dir_callback(direct, ent, currfiletime, (void *) update_mailsize_down, currentuser->userid)) {
		unlink(buf);
		return DIRCHANGED;
	}
	move(2, 0);
	prints("Delete failed\n");
	pressreturn();
	clear();
	return FULLUPDATE;
}
