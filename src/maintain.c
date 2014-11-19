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

char cexplain[STRLEN];
char lookgrp[30];

static int valid_brdname(char *brd);
static char *chgrp(void);
static int freeclubnum(void);
static int setsecstr(char *buf, int ln);
static void anno_title(char *buf, struct boardheader *bh);
//static void domailclean(struct fileheader *fhdrp);
//static int cleanmail(struct userec *urec);
static void trace_state(int flag, char *name, int size);
static int touchfile(char *filename);
static int scan_register_form(char *regfile);

int
check_systempasswd()
{
	FILE *pass;
	char passbuf[20], prepass[STRLEN];

	clear();
	if ((pass = fopen("etc/.syspasswd", "r")) != NULL) {
		fgets(prepass, STRLEN, pass);
		fclose(pass);
		prepass[strlen(prepass) - 1] = '\0';
		getdata(1, 0, "请输入系统密码: ", passbuf, 19, NOECHO, YEA);
		if (passbuf[0] == '\0' || passbuf[0] == '\n')
			return NA;
		if (!checkpasswd_des(prepass, passbuf)) {
			move(2, 0);
			prints("错误的系统密码...");
			securityreport("系统密码输入错误...",
				       "系统密码输入错误...");
			pressanykey();
			return NA;
		}
	}
	return YEA;
}

static void
admin_b_report(char *title, char *content, struct boardheader *fh)
{
	char class;
	int i;

	strcpy(currboard, fh->filename);
	deliverreport(title, content);
	if (!normal_board(fh->filename))
		return;
	strcpy(currboard, "Board");
	deliverreport(title, content);
	for (i = 0, class = fh->sec1[0];
	     i < 2 && !(i == 1 && class == fh->sec1[0]);
	     i++, class = fh->sec2[0]) {
		if (!class)
			break;
		if (class > '0' && class <= '9')
			sprintf(currboard, "%cadmin", class);
		else
			sprintf(currboard, "%c_admin", class);
		deliverreport(title, content);
	}
}

int
setsystempasswd()
{
	FILE *pass;
	char passbuf[20], prepass[20];

	modify_user_mode(ADMIN);
	if (strcmp(currentuser->userid, "SYSOP"))
		return -1;
	if (!check_systempasswd())
		return -1;
	getdata(2, 0, "请输入新的系统密码: ", passbuf, 19, NOECHO, YEA);
	getdata(3, 0, "确认新的系统密码: ", prepass, 19, NOECHO, YEA);
	if (strcmp(passbuf, prepass))
		return -1;
	if (passbuf[0] == '\0' || passbuf[0] == '\n')
		return NA;
	if ((pass = fopen("etc/.syspasswd", "w")) == NULL) {
		move(4, 0);
		prints("系统密码无法设定....");
		pressanykey();
		return -1;
	}
	fprintf(pass, "%s\n", genpasswd_des(passbuf));
	fclose(pass);
	move(4, 0);
	prints("系统密码设定完成....");
	pressanykey();
	return 0;
}

void
penaltyreport(title, str)
char *title;
char *str;
{
	FILE *se;
	char fname[STRLEN];
	int savemode;

	savemode = uinfo.mode;
	sprintf(fname, "bbstmpfs/tmp/penalty.%s.%05d", currentuser->userid,
		uinfo.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "%s", str);
		fclose(se);
		postfile(fname, "Penalty", title, 1);
		unlink(fname);
		modify_user_mode(savemode);
	}
}

void
deliverreport(title, str)
char *title;
char *str;
{
	FILE *se;
	char fname[STRLEN];
	int savemode;

	snprintf(fname, sizeof (fname), "boards/%s", currboard);
	if (!file_isdir(fname))
		return;
	savemode = uinfo.mode;
	sprintf(fname, "bbstmpfs/tmp/deliver.%s.%05d", currentuser->userid,
		uinfo.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "%s", str);
		fclose(se);
		postfile(fname, currboard, title, 1);
		unlink(fname);
		modify_user_mode(savemode);
	}
}

void
securityreport(str, content)
char *str;
char *content;
{
	FILE *se;
	char fname[STRLEN];
	int savemode;

	savemode = uinfo.mode;
	//report(str);
	sprintf(fname, "bbstmpfs/tmp/security.%s.%05d", currentuser->userid,
		uinfo.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "系统安全记录系统\n原因：\n%s\n", content);
		fprintf(se, "以下是部分个人资料\n");
		fprintf(se, "登录自: %s", realfromhost);
		fclose(se);
		postfile(fname, "syssecurity", str, 2);
		unlink(fname);
		modify_user_mode(savemode);
	}
}

int
get_grp(seekstr)
char seekstr[STRLEN];
{
	FILE *fp;
	char buf[STRLEN];
	char *namep;

	if ((fp = fopen("0Announce/.Search", "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		namep = strtok(buf, ": \n\r\t");
		if (namep != NULL && strcasecmp(namep, seekstr) == 0) {
			fclose(fp);
			strtok(NULL, "/");
			namep = strtok(NULL, "/");
			if (strlen(namep) < 30) {
				strcpy(lookgrp, namep);
				return 1;
			} else
				return 0;
		}
	}
	fclose(fp);
	return 0;
}

void
stand_title(title)
char *title;
{
	clear();
	//standout();
	prints(title);
	//standend();
}

int
m_info()
{
	struct userec urec;
	int id;
	struct userec *lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("修改使用者代号");
	move(1, 0);
	usercomplete("请输入使用者代号: ", genbuf);
	if (*genbuf == '\0') {
		clear();
		return -1;
	}

	if (!(id = getuser(genbuf, &lookupuser))) {
		move(3, 0);
		prints("错误的使用者代号");
		clrtoeol();
		pressreturn();
		clear();
		return -1;
	}
	memcpy(&urec, lookupuser, sizeof (urec));
	sprintf(genbuf, "修改 %s 的资料", lookupuser->userid);
	securityreport(genbuf, genbuf);

	move(1, 0);
	clrtobot();
	disply_userinfo(&urec, 1);
	uinfo_query(&urec, 1);
	return 0;
}

static int
valid_brdname(brd)
char *brd;
{
	char ch;

	ch = *brd++;
	if (!isalnum(ch) && ch != '_')
		return 0;
	while ((ch = *brd++) != '\0') {
		if (!isalnum(ch) && ch != '_')
			return 0;
	}
	return 1;
}

static char *
chgrp()
{
	int i, ch;
	static char buf[STRLEN];
	char ans[6];

	clear();
	move(2, 0);
	prints("选择精华区的目录\n\n");
	for (i = 0; i < sectree.nsubsec; i++) {
		prints("\033[1;32m%2d\033[m. %-20s                GROUP_%c\n",
		       i, sectree.subsec[i]->title,
		       sectree.subsec[i]->basestr[0]);
	}
	sprintf(buf, "请输入你的选择(0~%d): ", --i);
	while (1) {
		getdata(i + 6, 0, buf, ans, 4, DOECHO, YEA);
		if (!isdigit(ans[0]))
			continue;
		ch = atoi(ans);
		if (ch < 0 || ch > i || ans[0] == '\r' || ans[0] == '\0')
			continue;
		else
			break;
	}
	strcpy(cexplain, sectree.subsec[ch]->title);
	snprintf(buf, sizeof (buf), "GROUP_%c", sectree.subsec[ch]->basestr[0]);
	return buf;
}

static int
freeclubnum()
{
	FILE *fp;
	int club[CLUB_SIZE];
	int i;
	struct boardheader rec;
	bzero(club, sizeof (club));
	if ((fp = fopen(BOARDS, "r")) == NULL) {
		return -1;
	}
	while (!feof(fp)) {
		fread(&rec, sizeof (struct boardheader), 1, fp);
		if (rec.clubnum != 0)
			club[rec.clubnum / (8 * sizeof (int))] |=
			    (1 << (rec.clubnum % (8 * sizeof (int))));
	}
	fclose(fp);
	for (i = 1; i < 8 * sizeof (int) * CLUB_SIZE; i++)
		if ((~club[i / (8 * sizeof (int))]) &
		    (1 << (i % (8 * sizeof (int))))) {
			return i;
		}
	return -1;
}

static int
setsecstr(char *buf, int ln)
{
	const struct sectree *sec;
	int i = 0, ch, len, choose = 0;
	sec = getsectree(buf);
	move(ln, 0);
	clrtobot();
	while (1) {
		prints
		    ("=======当前分区选择: \033[31m%s\033[0;1m %s\033[m =======\n",
		     sec->basestr, sec->title);
		if (sec->parent) {
			prints(" (\033[4;33m#\033[0m) 回上级分区\n");
			prints(" (\033[4;33m%%\033[0m) 就放在这里\n");
		}
		prints
		    (" (\033[4;33m*\033[0m) 保持原来设定(可用回车选定本项)\n");
		len = strlen(sec->basestr);
		for (i = 0; i < sec->nsubsec; i++) {
			if (i && !(i % 3))
				prints("\n");
			ch = sec->subsec[i]->basestr[len];
			prints
			    (" (\033[4;33m%c\033[0m) \033[31;1m %s\033[0m",
			     ch, sec->subsec[i]->title);
		}
		prints("\n请按括号内的字母选择");
		while (1) {
			ch = igetkey();
			if (ch == '\n' || ch == '\r')
				ch = '*';
			if (sec->parent == NULL && (ch == '#' || ch == '%'))
				continue;
			for (i = 0; i < sec->nsubsec; i++) {
				if (sec->subsec[i]->basestr[len] == ch) {
					choose = i;
					break;
				}
			}
			if (ch != '#' && ch != '*' && ch != '%'
			    && i == sec->nsubsec)
				continue;
			break;
		}
		move(ln, 0);
		clrtobot();
		switch (ch) {
		case '#':
			sec = sec->parent;
			break;
		case '%':
			strcpy(buf, sec->basestr);
			return 0;
		case '*':
			sec = getsectree(buf);
			strcpy(buf, sec->basestr);
			return 0;
		default:
			sec = sec->subsec[choose];
		}
	}
}

int
m_newbrd()
{
	struct boardheader newboard;
	char ans[4];
	char vbuf[100];
	char *group;
	int bid;
	int now;

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("开启新讨论区");
	move(1, 0);
	prints("如果是俱乐部版面,请将分区连接设置为C");
	memset(&newboard, 0, sizeof (newboard));
	while (1) {
		getdata(2, 0, "◆ 讨论区名称: ", newboard.filename, 18, DOECHO,
			YEA);
		if (newboard.filename[0] != 0) {
			struct boardheader dh;
			if (new_search_record
			    (BOARDS, &dh, sizeof (dh), (void *) cmpbnames,
			     newboard.filename)) {
				prints("\n错误! 此讨论区已经存在!!");
				pressanykey();
				return -1;
			}
		} else
			return -1;
		if (valid_brdname(newboard.filename))
			break;
		prints("\n不合法名称!!");
	}
	getdata(3, 0, "◆ 讨论区中文名: ", newboard.title,
		sizeof (newboard.title), DOECHO, YEA);
	if (newboard.title[0] == '\0')
		return -1;
	strcpy(vbuf, "vote/");
	strcat(vbuf, newboard.filename);
	setbpath(genbuf, newboard.filename);
	if (getbnum(newboard.filename) > 0 || mkdir(genbuf, 0777) == -1
	    || mkdir(vbuf, 0777) == -1) {
		prints("\n错误的讨论区名称!!\n");
		pressreturn();
		rmdir(vbuf);
		rmdir(genbuf);
		clear();
		return -1;
	}
	sprintf(genbuf, "ftphome/root/boards/%s", newboard.filename);
	mkdir(genbuf, 0777);
	move(4, 0);
	prints("◆ 选择主分区: ");
	while (1) {
		genbuf[0] = 0;
		setsecstr(genbuf, 10);
		if (genbuf[0] != '\0')
			break;
	}
	move(4, 0);
	prints("◆ 主分区设定: %s", genbuf);
	newboard.secnumber1 = genbuf[0];
	strsncpy(newboard.sec1, genbuf, sizeof (newboard.sec1));
	move(5, 0);
	prints("◆ 选择分区链接: ");
	genbuf[0] = 0;
	setsecstr(genbuf, 10);
	move(5, 0);
	prints("◆ 分区链接设定: %s", genbuf);
	newboard.secnumber2 = genbuf[0];
	strsncpy(newboard.sec2, genbuf, sizeof (newboard.sec2));
	ansimore2("etc/boardref", NA, 8, 11);
	while (1) {
		getdata(6, 0, "◆ 讨论区分类(4字):", newboard.type,
			sizeof (newboard.type), DOECHO, YEA);
		if (strlen(newboard.type) == 4)
			break;
	}
	getdata(7, 0, "◆ 3天旧文章数上限(0则默认3000): ", genbuf, 5,
		DOECHO, YEA);
	newboard.limitchar = atoi(genbuf) / 100;
	move(7, 0);
	prints("◆ 3天旧文章数上限(0则默认3000): %d\n",
	       newboard.limitchar * 100);
	move(8, 0);
	clrtobot();
	if (newboard.secnumber2 == 'C') {
		newboard.flag &= ~ANONY_FLAG;
		newboard.level = 0;
		if (askyn("◆ 是否是开放式俱乐部", YEA, NA) == YEA) {
			newboard.flag |= CLUB_FLAG;
			newboard.flag &= ~CLOSECLUB_FLAG;
			newboard.clubnum = 0;
		} else {
			move(9, 0);
			if (askyn("◆ 是否是完全close俱乐部", YEA, NA) == YEA) {
				if ((newboard.clubnum = freeclubnum()) == -1) {
					prints("没有空的俱乐部位置了");
					pressreturn();
					clear();
					return -1;
				}
				newboard.flag |= CLUBLEVEL_FLAG;
				newboard.flag |= CLOSECLUB_FLAG;
			} else {
				newboard.clubnum = 0;
				newboard.flag |= CLOSECLUB_FLAG;
				newboard.flag &= ~CLUBLEVEL_FLAG;
			}
		}
	} else {
		if (askyn("◆ 是否限制存取权利", NA, NA) == YEA) {
			getdata(9, 0, "◆ 限制 Read/Post? [R]: ", ans, 2,
				DOECHO, YEA);
			if (*ans == 'P' || *ans == 'p')
				newboard.level = PERM_POSTMASK;
			else
				newboard.level = 0;
			move(1, 0);
			clrtobot();
			move(2, 0);
			prints("◆ 设定 %s 权利. 讨论区: '%s'\n",
			       (newboard.level & PERM_POSTMASK ? "POST" :
				"READ"), newboard.filename);
			newboard.level =
			    setperms(newboard.level, "权限", NUMPERMS,
				     showperminfo, 0);
			clear();
		} else
			newboard.level = 0;

		move(9, 0);
		if (askyn("◆ 是否加入匿名版", NA, NA) == YEA)
			newboard.flag |= ANONY_FLAG;
		else
			newboard.flag &= ~ANONY_FLAG;
	}
	move(10, 0);
	if (askyn("◆ 是否是cn.bbs转信版面", NA, NA) == YEA)
		newboard.flag |= INNBBSD_FLAG;
	else
		newboard.flag &= ~INNBBSD_FLAG;

	if (askyn("◆ 是否是点对点转信版面", NA, NA) == YEA)
		newboard.flag2 |= NJUINN_FLAG;
	else
		newboard.flag2 &= ~NJUINN_FLAG;

	if (askyn("◆ 是否是有人盯着的版面", NA, NA) == YEA)
		newboard.flag2 |= WATCH_FLAG;
	else
		newboard.flag2 &= ~WATCH_FLAG;

	if (askyn("◆ 是否是需要进行内容检查的版面", NA, NA) == YEA)
		newboard.flag |= IS1984_FLAG;
	else
		newboard.flag &= ~IS1984_FLAG;
	if (askyn("◆ 版面内容是否可能和政治相关", NA, NA) == YEA)
		newboard.flag |= POLITICAL_FLAG;
	else
		newboard.flag &= ~POLITICAL_FLAG;
	now = time(NULL);
	newboard.board_mtime = now;
	newboard.board_ctime = now;

	if ((bid = getbnum("")) > 0) {
		substitute_record(BOARDS, &newboard, sizeof (newboard), bid);
	} else if (append_record(BOARDS, &newboard, sizeof (newboard)) == -1) {
		pressreturn();
		clear();
		return -1;
	}

	reload_boards();
	update_postboards();

	group = chgrp();
	sprintf(vbuf, "%-38.38s", newboard.title);
	if (group != NULL) {
		if (add_grp(group, cexplain, newboard.filename, vbuf) == -1)
			prints("\n成立精华区失败....\n");
		else
			prints("已经置入精华区...\n");
	}

	prints("\n新讨论区成立\n");
	{
		char secu[STRLEN];
		sprintf(secu, "成立新版：%s", newboard.filename);
		securityreport(secu, secu);
	}
	pressreturn();
	clear();
	return 0;
}

static void
anno_title(buf, bh)
char *buf;
struct boardheader *bh;
{
	char bm[IDLEN * 4 + 4];	//放四个版务
	sprintf(buf, "%-38.38s", bh->title);
	if (bh->bm[0][0] == 0)
		return;
	else {
		//fixme, 这里的sizeof看不出任何意义
		strncat(buf, "(BM: ", STRLEN - sizeof (buf) - 2);
		bm2str(bm, bh);
		strncat(buf, bm, STRLEN - sizeof (buf) - 2);
	}
	strcat(buf, ")");
	return;
}

int
m_editbrd()
{
	char bname[STRLEN], buf[STRLEN], oldtitle[STRLEN], vbuf[256], *group;
	char oldpath[STRLEN], newpath[STRLEN], tmp_grp[30];
	int pos, noidboard, a_mv, isclub, innboard, is1984, njuinn, watched;
	int political;
	struct boardheader fh, newfh;

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("修改讨论区资讯");
	move(1, 0);
	make_blist_full();
	namecomplete("输入讨论区名称: ", bname);
	FreeNameList();
	if (*bname == '\0') {
		move(2, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	pos =
	    new_search_record(BOARDS, &fh, sizeof (fh), (void *) cmpbnames,
			      bname);
	if (!pos) {
		move(2, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	noidboard = fh.flag & ANONY_FLAG;
	isclub = (fh.flag & CLUB_FLAG);
	innboard = (fh.flag & INNBBSD_FLAG) ? YEA : NA;
	njuinn = (fh.flag2 & NJUINN_FLAG) ? YEA : NA;
	watched = (fh.flag2 & WATCH_FLAG) ? YEA : NA;
	is1984 = fh.flag & IS1984_FLAG;
	political = fh.flag & POLITICAL_FLAG;
	move(2, 0);
	memcpy(&newfh, &fh, sizeof (newfh));
	prints("讨论区名称: %s", fh.filename);
	move(2, 40);
	prints("讨论区说明: %s\n", fh.title);
	prints
	    ("匿名讨论区: %s  俱乐部版面: %s  cn.bbs转信讨论区: %s 点对点转信讨论区: %s 文章数上限: %d%s\n %s人盯",
	     (noidboard) ? "Yes" : "No", (isclub) ? "Yes" : "No",
	     (innboard) ? "Yes" : "No", (njuinn) ? "Yes" : "No",
	     fh.limitchar * 100, fh.limitchar ? "" : "(默认值)",
	     watched ? "有" : "无");
	strcpy(oldtitle, fh.title);
	prints("限制 %s 权利: %s",
	       (fh.level & PERM_POSTMASK) ? "POST" :
	       (fh.level & PERM_NOZAP) ? "ZAP" : "READ",
	       (fh.level & ~PERM_POSTMASK) == 0 ? "不设限" : "有设限");
	prints(" %s进行人工文章审查", is1984 ? "要" : "不");
	if (political)
		prints(" 内容可能和政治相关");
	move(5, 0);
	if (askyn("◆ 是否更改以上资讯", NA, NA) == NA) {
		clear();
		return 0;
	}

	move(6, 0);
	prints("直接按 <Return> 不修改此栏资讯...");
      enterbname:
	getdata(7, 0, "◆ 新讨论区名称: ", genbuf, 18, DOECHO, YEA);
	if (genbuf[0] != 0) {
		struct boardheader dh;
		if (new_search_record
		    (BOARDS, &dh, sizeof (dh), (void *) cmpbnames, genbuf)) {
			prints("错误! 此讨论区已经存在!!");
			goto enterbname;
		}
		if (valid_brdname(genbuf)) {
			strsncpy(newfh.filename, genbuf,
				 sizeof (newfh.filename));
			strcpy(bname, genbuf);
		} else {
			prints("不合法的讨论区名称!");
			goto enterbname;
		}
	}
	getdata(8, 0, "◆ 新讨论区中文名: ", genbuf, 24, DOECHO, YEA);
	if (genbuf[0] != 0)
		strsncpy(newfh.title, genbuf, sizeof (newfh.title));
	strcpy(genbuf, newfh.sec1);
	move(9, 0);
	prints("◆ 选择新分区: %s", genbuf);
	setsecstr(genbuf, 12);
	if (genbuf[0] != 0) {
		newfh.secnumber1 = genbuf[0];
		strsncpy(newfh.sec1, genbuf, sizeof (newfh.sec1));
	}
	move(9, 0);
	prints("新分区设定: %s", newfh.sec1);
	clrtoeol();
	move(10, 0);
	strcpy(genbuf, newfh.sec2);
	prints("◆ 选择新分区链接: %s", genbuf);
	setsecstr(genbuf, 12);
	newfh.secnumber2 = genbuf[0];
	strsncpy(newfh.sec2, genbuf, sizeof (newfh.sec2));
	move(10, 0);
	prints("新分区链接设定: %s", newfh.sec2);
	clrtoeol();
	ansimore2("etc/boardref", NA, 13, 7);
	getdata(11, 0, "◆ 新讨论区分类(4字): ", genbuf, 5, DOECHO, YEA);
	if (genbuf[0] != 0)
		strsncpy(newfh.type, genbuf, sizeof (newfh.type));
	clrtobot();
	sprintf(genbuf, "%d", newfh.limitchar * 100);
	getdata(12, 0, "◆ 3天旧文章数上限(0则默认3000): ", genbuf, 5,
		DOECHO, NA);
	newfh.limitchar = atoi(genbuf) / 100;
	move(12, 0);
	prints("◆ 3天旧文章数上限(0则默认3000): %d\n", newfh.limitchar * 100);
	move(13, 0);
	if (askyn("◆ 是否是cn.bbs转信版面", innboard, NA) == YEA)
		newfh.flag |= INNBBSD_FLAG;
	else
		newfh.flag &= ~INNBBSD_FLAG;
	if (askyn("◆ 是否是点对点转信版面", njuinn, NA) == YEA)
		newfh.flag2 |= NJUINN_FLAG;
	else
		newfh.flag2 &= ~NJUINN_FLAG;
	if (askyn("◆ 是否是有人盯版面", watched, NA) == YEA)
		newfh.flag2 |= WATCH_FLAG;
	else
		newfh.flag2 &= ~WATCH_FLAG;
	if (askyn("◆ 是否是需要进行内容检查的版面", is1984, NA) == YEA)
		newfh.flag |= IS1984_FLAG;
	else
		newfh.flag &= ~IS1984_FLAG;
	if (askyn("◆ 版面内容是否可能和政治相关", political, NA) == YEA)
		newfh.flag |= POLITICAL_FLAG;
	else
		newfh.flag &= ~POLITICAL_FLAG;

	genbuf[0] = 0;
	if (askyn("◆ 是否移动精华区的位置", NA, NA) == YEA)
		a_mv = 2;
	else
		a_mv = 0;
	move(17, 0);
	if (newfh.secnumber2 == 'C')	//是俱乐部版面
	{
		if (askyn
		    ("◆ 是否是开放式俱乐部", !(newfh.flag & CLOSECLUB_FLAG),
		     newfh.flag & CLOSECLUB_FLAG) == YEA) {
			newfh.flag |= CLUB_FLAG;
			newfh.flag &= ~CLOSECLUB_FLAG;
			newfh.clubnum = 0;
		} else {
			move(18, 0);
			if (askyn
			    ("◆ 是否是完全close俱乐部", newfh.clubnum != 0,
			     newfh.clubnum == 0) == YEA) {
				if (fh.clubnum)
					newfh.clubnum = fh.clubnum;
				else {
					if ((newfh.clubnum =
					     freeclubnum()) == -1) {
						prints("没有空的俱乐部位置了");
						pressreturn();
						clear();
						return -1;
					}
				}
				newfh.flag |= CLUBLEVEL_FLAG;
				newfh.flag |= CLOSECLUB_FLAG;
			} else {
				newfh.clubnum = 0;
				newfh.flag |= CLOSECLUB_FLAG;
				newfh.flag &= ~CLUBLEVEL_FLAG;
			}
		}
		if (fh.clubnum != 0 && newfh.clubnum == 0) {
			if (clubmember_num(fh.filename) != 0) {
				move(22, 0);
				prints("请先清空俱乐部成员");
				pressreturn();
				clear();
				return -1;
			}

		}
		newfh.flag &= ~ANONY_FLAG;
		newfh.level = 0;
		getdata(19, 0, "◆ 确定要更改吗? (Y/N) [N]: ", genbuf, 4,
			DOECHO, YEA);
	} else {
		newfh.clubnum = 0;
		newfh.flag &= ~CLUBLEVEL_FLAG;
		newfh.flag &= ~CLOSECLUB_FLAG;
		if (fh.clubnum != 0 && newfh.clubnum == 0) {
			if (clubmember_num(fh.filename) != 0) {
				move(22, 0);
				prints("请先清空俱乐部成员");
				pressreturn();
				clear();
				return -1;
			}

		}
		sprintf(buf, "◆ 匿名版 (Y/N)? [%c]: ",
			(noidboard) ? 'Y' : 'N');
		getdata(17, 0, buf, genbuf, 4, DOECHO, YEA);
		if (*genbuf == 'y' || *genbuf == 'Y' || *genbuf == 'N'
		    || *genbuf == 'n') {
			if (*genbuf == 'y' || *genbuf == 'Y')
				newfh.flag |= ANONY_FLAG;
			else
				newfh.flag &= ~ANONY_FLAG;
		}
		if (askyn("◆ 是否更改存取权限", NA, NA) == YEA) {
			char ans[4];
			sprintf(genbuf,
				"◆ 限制 (R)阅读 或 (P)张贴 文章 [%c]: ",
				(newfh.level & PERM_POSTMASK ? 'P' : 'R'));
			getdata(19, 0, genbuf, ans, 2, DOECHO, YEA);
			if ((newfh.level & PERM_POSTMASK)
			    && (*ans == 'R' || *ans == 'r'))
				newfh.level &= ~PERM_POSTMASK;
			else if (!(newfh.level & PERM_POSTMASK)
				 && (*ans == 'P' || *ans == 'p'))
				newfh.level |= PERM_POSTMASK;
			clear();
			move(2, 0);
			prints("◆ 设定 %s '%s' 讨论区的权限\n",
			       newfh.level & PERM_POSTMASK ? "张贴" :
			       "阅读", newfh.filename);
			newfh.level =
			    setperms(newfh.level, "权限", NUMPERMS,
				     showperminfo, 0);
			clear();
			getdata(0, 0, "◆ 确定要更改吗? (Y/N) [N]: ",
				genbuf, 4, DOECHO, YEA);
		} else {
			getdata(19, 0, "◆ 确定要更改吗? (Y/N) [N]: ",
				genbuf, 4, DOECHO, YEA);
		}
	}
	clear();
	if (*genbuf == 'Y' || *genbuf == 'y') {
		{
			char secu[STRLEN];
			sprintf(secu, "修改讨论区: %s(%s)", fh.filename,
				newfh.filename);
			securityreport(secu, secu);
		}
		newfh.board_mtime = time(NULL);
		if (strcmp(fh.filename, newfh.filename)) {
			char old[256], tar[256];
			a_mv = 1;
			setbpath(old, fh.filename);
			setbpath(tar, newfh.filename);
			rename(old, tar);
			sprintf(old, "boards/.backnumbers/%s", fh.filename);
			sprintf(tar, "boards/.backnumbers/%s", newfh.filename);
			rename(old, tar);
			sprintf(old, "vote/%s", fh.filename);
			sprintf(tar, "vote/%s", newfh.filename);
			rename(old, tar);
			if (seek_in_file("etc/junkboards", fh.filename)) {
				del_from_file("etc/junkboards", fh.filename);
				addtofile("etc/junkboards", newfh.filename);
			}
		}
		get_grp(fh.filename);
		anno_title(vbuf, &newfh);
		edit_grp(fh.filename, lookgrp, oldtitle, vbuf);
		if (a_mv >= 1) {
			group = chgrp();
			get_grp(fh.filename);
			strcpy(tmp_grp, lookgrp);
			if (strcmp(tmp_grp, group) || a_mv != 2) {
				del_from_file("0Announce/.Search", fh.filename);
				if (group != NULL) {
					if (add_grp
					    (group, cexplain,
					     newfh.filename, vbuf) == -1)
						prints
						    ("\n成立精华区失败....\n");
					else
						prints("已经置入精华区...\n");
					sprintf(newpath,
						"0Announce/groups/%s/%s",
						group, newfh.filename);
					sprintf(oldpath,
						"0Announce/groups/%s/%s",
						tmp_grp, fh.filename);
					if (file_isdir(oldpath)) {
						deltree(newpath);
					}
					if(rename(oldpath, newpath)<0) {
						errlog("rename failed %s", strerror(errno));
					}
						
					del_grp(tmp_grp, fh.filename, fh.title);
				}
			}
		}
		substitute_record(BOARDS, &newfh, sizeof (newfh), pos);
		reload_boards();
		update_postboards();
	}
	clear();
	return 0;
}

FILE *cleanlog;
char curruser[IDLEN + 2];

static void
trace_state(flag, name, size)
int flag, size;
char *name;
{
	char buf[STRLEN];

	if (flag != -1) {
		sprintf(buf, "ON (size = %d)", size);
	} else {
		strcpy(buf, "OFF");
	}
	prints("%s记录 %s\n", name, buf);
}

static int
touchfile(filename)
char *filename;
{
	int fd;

	if ((fd = open(filename, O_RDWR | O_CREAT, 0600)) > 0) {
		close(fd);
	}
	return fd;
}

int
m_trace()
{
	struct stat ostatb, cstatb;
	int otflag, ctflag, done = 0;
	char ans[3];
	char *msg;

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("Set Trace Options");
	while (!done) {
		move(2, 0);
		otflag = stat("trace", &ostatb);
		ctflag = stat("trace.chatd", &cstatb);
		prints("目前设定:\n");
		trace_state(otflag, "一般", ostatb.st_size);
		trace_state(ctflag, "聊天", cstatb.st_size);
		move(9, 0);
		prints("<1> 切换一般记录\n");
		prints("<2> 切换聊天记录\n");
		getdata(12, 0, "请选择 (1/2/Exit) [E]: ", ans, 2, DOECHO, YEA);

		switch (ans[0]) {
		case '1':
			if (otflag) {
				touchfile("trace");
				msg = "一般记录 ON";
			} else {
				rename("trace", "trace.old");
				msg = "一般记录 OFF";
			}
			break;
		case '2':
			if (ctflag) {
				touchfile("trace.chatd");
				msg = "聊天记录 ON";
			} else {
				rename("trace.chatd", "trace.chatd.old");
				msg = "聊天记录 OFF";
			}
			break;
		default:
			msg = NULL;
			done = 1;
		}
		move(t_lines - 2, 0);
		if (msg) {
			prints("%s\n", msg);
			//report(msg);
		}
	}
	clear();
	return 0;
}

static int
scan_register_form(regfile)
char *regfile;
{
	static char *const field[] = { "usernum", "userid", "realname", "dept",
		"addr", "phone", "assoc", "rereg", NULL
	};
	static char *const finfo[] =
	    { "帐号位置", "申请帐号", "真实姓名", "学校系级",
		"目前住址", "连络电话", "毕业学校", "非首次填写", NULL
	};
	static char *const reason[] = { "请重新填写您的中文真实姓名。",
		"请重新填写学校系级或工作部门。",
		"请重新填写完整居住地址。",
		"资料多项不合格，请重新填写。",
		"缩写或简写名词不能算详细",
		"信箱或email不能作为有效地址。",
		"请勿用拼音或英文填写注册单。", NULL
	};
	struct userec urec;
	struct userdata udata;
	FILE *fn, *fout, *freg, *scanreglog, *fp;
	struct stat st;
	time_t dtime;
	struct tm *t;
	char fdata[8][STRLEN];
	char fname[STRLEN], buf[STRLEN], logname[STRLEN];
	char ans[5], dellog[STRLEN * 8], *ptr, *uid;
	int n, unum, lockfd, numline = 0, j, percent, loglength;
	time_t now;
	int npass = 0, nreject = 0, nskip = 0, ndel = 0, nresult = 0;
	const char *blk[] = {
		"█", "▉", "▊", "▋", "▌", "▍", "▎", "▏", " "
	};
	struct userec *lookupuser;

	uid = currentuser->userid;
	stand_title("依序设定所有新注册资料");
	sprintf(fname, "%s.tmp", regfile);
	move(2, 0);
	lockfd = openlockfile(".lock_new_register", O_RDONLY, LOCK_EX);
	if (file_isfile(fname)) {
		close(lockfd);
		if (stat(fname, &st) != -1 && st.st_atime > time(0) - 86400) {
			prints
			    ("其他 SYSOP 正在查看注册申请单, 请检查使用者状态.\n");
			pressreturn();
			return -1;
		} else {
			prints
			    ("其他 SYSOP 正在查看注册申请单, 请检查使用者状态.\n");
			pressreturn();
			return -1;
		}
	} else {
		rename(regfile, fname);
		close(lockfd);
	}
	if ((fn = fopen(fname, "r")) == NULL) {
		move(2, 0);
		prints("系统错误, 无法读取注册资料档, 请联系系统维护\n");
		pressreturn();
		return -1;
	}

	sprintf(logname, "bbstmpfs/tmp/telnetscanreglog.%d.%s", uinfo.pid,
		currentuser->userid);
	if ((scanreglog = fopen(logname, "w")) != NULL) {
		fprintf(scanreglog, "删除和跳过的有：\n");
	} else {
		errlog("open scanreg tmpfile error.");
	}
	memset(fdata, 0, sizeof (fdata));
	while (fgets(genbuf, STRLEN, fn) != NULL) {
		if ((ptr = (char *) strstr(genbuf, ": ")) != NULL) {
			*ptr = '\0';
			for (n = 0; field[n] != NULL; n++) {
				if (strcmp(genbuf, field[n]) == 0) {
					strcpy(fdata[n], ptr + 2);
					if ((ptr =
					     (char *) strchr(fdata[n],
							     '\n')) != NULL)
						*ptr = '\0';
				}
			}
			numline++;
			continue;
		}
		if (numline == 0)
			continue;
		numline = 0;

		if ((unum = getuser(fdata[1], &lookupuser)) == 0) {
			move(2, 0);
			clrtobot();
			prints("系统错误, 查无此帐号.\n\n");
			for (n = 0; field[n] != NULL; n++)
				prints("%s     : %s\n", finfo[n], fdata[n]);
			pressreturn();
			memset(fdata, 0, sizeof (fdata));
			continue;
		}
		memcpy(&urec, lookupuser, sizeof (urec));
		loaduserdata(urec.userid, &udata);
		move(1, 0);
		prints("帐号位置     : %d\n", unum);
		disply_userinfo(&urec, 1);
		move(15, 0);
		printdash(NULL);
		clrtobot();
		sprintf(dellog, "注册信息如下: \n");
		loglength = 0;
		for (n = 0; field[n] != NULL; n++) {
			if (loglength >= sizeof (dellog))
				break;
			loglength +=
			    snprintf(dellog + loglength,
				     sizeof (dellog) - loglength, "%s:%s\n",
				     finfo[n], fdata[n]);
		}
		if (urec.userlevel & PERM_LOGINOK) {
			move(t_lines - 1, 0);
			prints("此帐号不需再填写注册单.\n");
			igetkey();
			ans[0] = 0;
		} else {
			getdata(t_lines - 1, 0,
				"是否接受此资料 (Y/N/Q/Del/Skip)? [S]: ", ans,
				3, DOECHO, YEA);
		}
		move(1, 0);
		clrtobot();
		switch (ans[0]) {
		case 0:
			break;
		case 'D':
		case 'd':
			prints
			    ("不再接受来自此用户名的注册,此用户将被封禁上站权限.\n");
			urec.userlevel &= ~PERM_DEFAULT;
			updateuserec(&urec, 0);
			sprintf(genbuf, "%s 删除 %s 的注册单",
				currentuser->userid, urec.userid);
			securityreport(genbuf, dellog);
			sprintf(buf, "bbstmpfs/tmp/scanregdellog.%d.%s",
				getpid(), currentuser->userid);
			if ((fp = fopen(buf, "w")) != NULL) {
				fprintf(fp, dellog);
				fclose(fp);
			}
			postfile(buf, "IDScanRecord", genbuf, 2);
			unlink(buf);
			ndel++;
			if (scanreglog != NULL)
				fprintf(scanreglog, "DEL %s\n", urec.userid);
			break;
		case 'Y':
		case 'y':
			prints("以下使用者资料已经更新:\n");
			n = strlen(fdata[5]);
			if (n + strlen(fdata[3]) > 60) {
				if (n > 40)
					fdata[5][n = 40] = '\0';
				fdata[3][60 - n] = '\0';
			}
			strsncpy(udata.realname, fdata[2],
				 sizeof (udata.realname));
			strsncpy(udata.address, fdata[4],
				 sizeof (udata.address));
			sprintf(genbuf, "%s$%s@%s", fdata[3], fdata[5], uid);
			strsncpy(udata.realmail, genbuf,
				 sizeof (udata.realmail));
			urec.userlevel |= PERM_DEFAULT;	// by ylsdd
			updateuserec(&urec, 0);

			saveuserdata(urec.userid, &udata);
			sethomefile(buf, urec.userid, "sucessreg");
			if ((fout = fopen(buf, "w")) != NULL) {
				fprintf(fout, "\n");
				fclose(fout);
			}

			sethomefile(buf, urec.userid, "register");
			if (file_isfile(buf)) {
				sethomefile(genbuf, urec.userid,
					    "register.old");
				rename(buf, genbuf);
			}
			if ((fout = fopen(buf, "w")) != NULL) {
				for (n = 0; field[n] != NULL; n++)
					fprintf(fout, "%s: %s\n", field[n],
						fdata[n]);
				now = time(NULL);
				fprintf(fout, "Date: %s", ctime(&now));
				fprintf(fout, "Approved: %s\n", uid);
				fclose(fout);
			}
			system_mail_file("etc/s_fill", urec.userid,
				  "恭喜您通过身份验证", currentuser->userid);

			system_mail_file("etc/s_fill2", urec.userid,
				  "欢迎加入" MY_BBS_NAME "大家庭", currentuser->userid);
			sethomefile(buf, urec.userid, "mailcheck");
			unlink(buf);
			sprintf(genbuf, "让 %s 通过身分确认.", urec.userid);
			securityreport(genbuf, genbuf);
			npass++;
			break;
		case 'Q':
		case 'q':
			if ((freg = fopen(regfile, "a")) != NULL) {
				for (n = 0; field[n] != NULL; n++)
					fprintf(freg, "%s: %s\n", field[n],
						fdata[n]);
				fprintf(freg, "----\n");
				while (fgets(genbuf, STRLEN, fn) != NULL)
					fputs(genbuf, freg);
				fclose(freg);
			}
			break;
		case 'N':
		case 'n':
			for (n = 0; field[n] != NULL; n++)
				prints("%s: %s\n", finfo[n], fdata[n]);
			printdash(NULL);
			move(9, 0);
			prints
			    ("请选择/输入退回申请表原因, 按 <enter> 取消.\n\n");
			for (n = 0; reason[n] != NULL; n++)
				prints("%d) %s\n", n + 1, reason[n]);
			getdata(12 + n, 0, "退回原因: ", buf, 60, DOECHO, YEA);
			if (buf[0] != '\0') {
				if (buf[0] >= '1' && buf[0] < '1' + n) {
					strcpy(buf, reason[buf[0] - '1']);
				}
				sprintf(genbuf, "<注册失败>%s", buf);
				system_mail_file("etc/f_fill", urec.userid, genbuf, currentuser->userid);
				nreject++;
				break;
			}
			move(10, 0);
			clrtobot();
			prints("取消退回此注册申请表.\n");
			nskip++;
			if (scanreglog != NULL)
				fprintf(scanreglog, "SKIP %s\n", urec.userid);
			break;
			/* run default -- put back to regfile */
		default:
			if ((freg = fopen(regfile, "a")) != NULL) {
				for (n = 0; field[n] != NULL; n++)
					fprintf(freg, "%s: %s\n", field[n],
						fdata[n]);
				fprintf(freg, "----\n");
				fclose(freg);
			}
			nskip++;
			if (scanreglog != NULL)
				fprintf(scanreglog, "SKIP %s\n", urec.userid);
		}
		memset(fdata, 0, sizeof (fdata));
	}
	if (scanreglog != NULL) {
		nresult = npass + nreject + nskip + ndel;
		fprintf(scanreglog, "\n共处理 %3d 份注册单\n\n", nresult);
		fprintf(scanreglog, "\033[32m通过\033[0m %3d个 约占", npass);
		percent = (npass * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fprintf(scanreglog, "\033[33m退回\033[0m %3d个 约占", nreject);
		percent = (nreject * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fprintf(scanreglog, "跳过 %3d个 约占", nskip);
		percent = (nskip * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fprintf(scanreglog, "\033[31m删除\033[0m %3d个 约占", ndel);
		percent = (ndel * 100) / ((nresult <= 0) ? 1 : nresult);
		for (j = percent / 16; j; j--)
			fprintf(scanreglog, "%s", blk[0]);
		fprintf(scanreglog, "%s%d%%\n", blk[8 - (percent % 16) / 2],
			percent);
		fclose(scanreglog);
		time(&dtime);
		t = localtime(&dtime);
		sprintf(genbuf,
			"Telnet 帐号审批记录-%d-%02d-%02d-%02d:%02d:%02d",
			1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		postfile(logname, "IDScanRecord", genbuf, 2);
		unlink(logname);
		tracelog("%s pass %d %d %d %d", currentuser->userid, npass,
			 nreject, nskip, ndel);
	}
	fclose(fn);
	unlink(fname);
	return (0);
}

int
m_register()
{
	FILE *fn;
	char ans[3], *fname;
	int x, y, wid, len;
	char uident[STRLEN];
	char buf[PASSLEN];
	struct userec *lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	getdata(1, 0, "请输入你的密码: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0'
	    || !checkpasswd(currentuser->passwd, currentuser->salt, buf)) {
		prints("\n\n暗号不正确, 不能执行。\n");
		pressreturn();
		clear();
		return -1;
	}
	clear();

	stand_title("设定使用者注册资料");
	for (;;) {
		getdata(1, 0,
			"(0)离开  (1)审查新注册资料  (2)查询使用者注册资料 ? : ",
			ans, 2, DOECHO, YEA);
		if (ans[0] == '0' || ans[0] == '\n' || ans[0] == '\0')
			return 0;
		if (ans[0] == '1' || ans[0] == '2')
			break;

	}
	if (ans[0] == '1') {
		fname = "new_register";
		if ((fn = fopen(fname, "r")) == NULL) {
			prints("\n\n目前并无新注册资料.");
			pressreturn();
		} else {
			y = 3, x = wid = 0;
			while (fgets(genbuf, STRLEN, fn) != NULL && x < 65) {
				if (strncmp(genbuf, "userid: ", 8) == 0) {
					move(y++, x);
					prints(genbuf + 8);
					len = strlen(genbuf + 8);
					if (len > wid)
						wid = len;
					if (y >= t_lines - 2) {
						y = 3;
						x += wid + 2;
					}
				}
			}
			fclose(fn);
			if (askyn("设定资料吗", NA, YEA) == YEA) {
				securityreport("开始Telnet设定使用者注册资料",
					       "开始Telnet设定使用者注册资料");
				scan_register_form(fname);
			}
		}
	} else {
		move(1, 0);
		usercomplete("请输入要查询的代号: ", uident);
		if (uident[0] != '\0') {
			if (!getuser(uident, &lookupuser)) {
				move(2, 0);
				prints("错误的使用者代号...");
			} else {
				sprintf(genbuf, "home/%c/%s/register",
					mytoupper(lookupuser->userid[0]),
					lookupuser->userid);
				if ((fn = fopen(genbuf, "r")) != NULL) {
					sprintf(genbuf, "查询 %s 的注册信息.",
						lookupuser->userid);
					securityreport(genbuf, genbuf);

					prints("\n注册资料如下:\n\n");
					for (x = 1; x <= 15; x++) {
						if (fgets(genbuf, STRLEN, fn))
							prints("%s", genbuf);
						else
							break;
					}
					fclose(fn);
				} else
					prints("\n\n找不到他/她的注册资料!!\n");
				printdash("该用户的其它信息");
				prints("帐号建立日期 : %s",
				       ctime(&lookupuser->firstlogin));
				prints("最近光临日期 : %s",
				       ctime(&lookupuser->lastlogin));
				prints("曾登录的天数 : %d\n",
				       lookupuser->numdays);
				prints("上站总时数   : %d 小时 %d 分钟\n",
				       (int) lookupuser->stay / 3600,
				       (int) (lookupuser->stay / 60) % 60);
			}
		}
		pressanykey();
	}
	clear();
	return 0;
}
extern struct UTMPFILE *utmpshm;
int
m_ordainBM()
{
	return do_ordainBM(NULL, NULL);

}

int
do_ordainBM(const char *userid, const char *abname)
{
	int id, pos, oldbm = 0, i, bm = 0, bigbm, bmpos, minpos, maxpos;
	struct boardheader fh;
	char bname[STRLEN], tmp[256], buf[5][STRLEN];
	char content[1024], title[STRLEN];
	struct userec *lookupuser;
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("任命版主\n");
	clrtoeol();
	move(2, 0);
	if (userid)
		strsncpy(genbuf, userid, sizeof (genbuf));
	else
		usercomplete("输入欲任命的使用者帐号: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf, &lookupuser))) {
		move(4, 0);
		prints("无效的使用者帐号");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	if (abname)
		strsncpy(bname, abname, sizeof (bname));
	else {
		make_blist_full();
		namecomplete("输入该使用者将管理的讨论区名称: ", bname);
		FreeNameList();
	}
	if (*bname == '\0') {
		move(5, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	pos =
	    new_search_record(BOARDS, &fh, sizeof (fh), (void *) cmpbnames,
			      bname);
	if (!pos) {
		move(5, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	oldbm = getbmnum(lookupuser->userid);
	if (oldbm >= 3 && strcmp(lookupuser->userid, "SYSOP")
	    && normal_board(bname)) {
		move(5, 0);
		prints(" %s 已经是三个版的版主了", lookupuser->userid);
		pressanykey();
		clear();
		return -1;
	}
	if (askyn("任命为大版主么? (可以整理精华区)", NA, NA) == YEA) {
		bigbm = 1;
		minpos = 0;
		maxpos = 3;
	} else {
		bigbm = 0;
		minpos = 4;
		maxpos = BMNUM - 1;
	}
	bmpos = -1;
	for (i = 0; i < BMNUM; i++) {
		if (fh.bm[i][0] == 0 && (i >= minpos) && (i <= maxpos)
		    && (bmpos == -1)) {
			bmpos = i;
		}
		if (!strncmp(fh.bm[i], lookupuser->userid, IDLEN)) {
			prints(" %s 已经是该版版主", lookupuser->userid);
			pressanykey();
			clear();
			return -1;
		}
	}
	if (bmpos == -1) {
		prints(" %s 没有空余版主位置", bname);
		pressanykey();
		clear();
		return -1;
	}
	prints("\n你将任命 %s 为 %s 版版主.\n", lookupuser->userid, bname);
	if (askyn("你确定要任命吗?", NA, NA) == NA) {
		prints("取消任命版主");
		pressanykey();
		clear();
		return -1;
	}
	for (i = 0; i < 5; i++)
		buf[i][0] = '\0';
	move(8, 0);
	prints("请输入任命附言(最多五行，按 Enter 结束)");
	for (i = 0; i < 5; i++) {
		getdata(i + 9, 0, ": ", buf[i], STRLEN - 5, DOECHO, YEA);
		if (buf[i][0] == '\0')
			break;
	}

	if (!oldbm) {
		char secu[STRLEN];
		struct userec tmpu;
		memcpy(&tmpu, lookupuser, sizeof (tmpu));
		tmpu.userlevel |= PERM_BOARDS;
		updateuserec(&tmpu, 0);
		sprintf(secu, "版主任命, 给予 %s 的版主权限",
			lookupuser->userid);
		securityreport(secu, secu);
		move(14, 0);
		system_mail_file("etc/bmhelp", lookupuser->userid, "版务操作手册", currentuser->userid);
		prints("%s", secu);
	}
	strncpy(fh.bm[bmpos], lookupuser->userid, IDLEN);
	fh.bm[bmpos][IDLEN] = 0;
	fh.hiretime[bmpos] = time(NULL);
	if (bigbm) {
		anno_title(tmp, &fh);
		get_grp(fh.filename);
		edit_grp(fh.filename, lookgrp, fh.title, tmp);
	}
	bbsinfo.utmpshm->syncbmonline = 1;
	substitute_record(BOARDS, &fh, sizeof (fh), pos);
	if (fh.flag & CLOSECLUB_FLAG) {
		setclubmember(fh.filename, lookupuser->userid, 1);
	}
	reload_boards();
	sprintf(genbuf, "任命 %s 为 %s 讨论区版主", lookupuser->userid,
		fh.filename);
	securityreport(genbuf, genbuf);
	move(15, 0);
	prints("%s", genbuf);
	sprintf(title, "[公告]任命%s 版%s %s ", bname, (!bm) ? "版主" : "版副",
		lookupuser->userid);
	sprintf(content,
		"\n\t\t\t【 公告 】\n\n\t任命 %s 为 %s 版%s！\n",
		lookupuser->userid, bname, (!bm) ? "版主" : "版副");
	for (i = 0; i < 5; i++) {
		if (buf[i][0] == '\0')
			break;
		if (i == 0)
			strcat(content, "\n\n任命附言：\n");
		strcat(content, buf[i]);
		strcat(content, "\n");
	}
	admin_b_report(title, content, &fh);
	mail_buf(content, strlen(content), lookupuser->userid, title, "deliver");
	pressanykey();
	return 0;
}

int
m_retireBM()
{
	return do_retireBM(NULL, NULL);
}

int
do_retireBM(const char *userid, const char *abname)
{
	int id, pos, bmpos, right = 0, oldbm = 0, i;
	int bm = 1;
	struct boardheader fh;
	char buf[5][STRLEN];
	char bname[STRLEN];
	char content[1024], title[256];
	char tmp[256];
	struct userec *lookupuser;
	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return -1;

	clear();
	stand_title("版主离职\n");
	clrtoeol();
	if (userid)
		strsncpy(genbuf, userid, sizeof (genbuf));
	else
		usercomplete("输入欲离任的使用者帐号: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf, &lookupuser))) {
		move(4, 0);
		prints("无效的使用者帐号");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	if (abname)
		strsncpy(bname, abname, sizeof (bname));
	else {
		make_blist_full();
		namecomplete("输入该使用者将管理的讨论区名称: ", bname);
		FreeNameList();
	}
	if (*bname == '\0') {
		move(5, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	pos =
	    new_search_record(BOARDS, &fh, sizeof (fh), (void *) cmpbnames,
			      bname);
	if (!pos) {
		move(5, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	bmpos = -1;
	for (i = 0; i < BMNUM; i++) {
		if (!strcasecmp(fh.bm[i], lookupuser->userid)) {
			bmpos = i;
			if (i < 4)
				bm = 1;
			else
				bm = 0;
		}
	}

	oldbm = getbmnum(lookupuser->userid);
	if (bmpos == -1) {
		move(5, 0);
		prints(" 版主名单中没有%s，如有错误，请通知系统维护。",
		       lookupuser->userid);
		pressanykey();
		clear();
		return -1;
	}
	prints("\n你将取消 %s 的 %s 版%s版主职务.\n",
	       lookupuser->userid, bname, bm ? "大" : "");
	if (askyn("你确定要取消他的该版版主职务吗?", NA, NA) == NA) {
		prints("\n呵呵，你改变心意了？ %s 继续留任 %s 版版主职务！",
		       lookupuser->userid, bname);
		pressanykey();
		clear();
		return -1;
	}
	anno_title(title, &fh);
	fh.bm[bmpos][0] = 0;	//先清理掉, 免的有问题
	fh.hiretime[bmpos] = 0;
	for (i = bmpos; i < (bm ? 4 : BMNUM); i++) {
		if (i == (bm ? 3 : BMNUM - 1)) {	//最后一个BM
			fh.bm[i][0] = 0;
			fh.hiretime[i] = 0;
		} else {
			strcpy(fh.bm[i], fh.bm[i + 1]);
			fh.bm[i][strlen(fh.bm[i + 1])] = 0;
			fh.hiretime[i] = fh.hiretime[i + 1];
		}
	}
	if (bm) {
		anno_title(tmp, &fh);
		get_grp(fh.filename);
		edit_grp(fh.filename, lookgrp, title, tmp);
	}
	bbsinfo.utmpshm->syncbmonline = 1;
	substitute_record(BOARDS, &fh, sizeof (fh), pos);
	reload_boards();
	sprintf(genbuf, "取消 %s 的 %s 讨论区版主职务", lookupuser->userid,
		fh.filename);
	securityreport(genbuf, genbuf);
	move(8, 0);
	prints("%s", genbuf);
	if (!(oldbm - 1)) {
		char secu[STRLEN];
		struct userec tmpu;
		if (!(lookupuser->userlevel & PERM_OBOARDS)
		    && !(lookupuser->userlevel & PERM_SYSOP)) {
			memcpy(&tmpu, lookupuser, sizeof (tmpu));
			tmpu.userlevel &= ~PERM_BOARDS;
			updateuserec(&tmpu, 0);
			sprintf(secu, "版主卸职, 取消 %s 的版主权限",
				lookupuser->userid);
			securityreport(secu, secu);
			move(9, 0);
			prints(secu);
		}
	}
	prints("\n\n");
	if (askyn("需要在相关版面发送通告吗?", YEA, NA) == NA) {
		pressanykey();
		return 0;
	}
	prints("\n");
	if (askyn("正常离任请按 Enter 键确认，撤职惩罚按 N 键", YEA, NA) == YEA)
		right = 1;
	else
		right = 0;
	if (right)
		sprintf(title, "[公告]%s 版%s %s 离任", bname,
			bm ? "大版主" : "版主", lookupuser->userid);
	else
		sprintf(title, "[公告]撤除 %s 版%s %s ", bname,
			bm ? "大版主" : "版主", lookupuser->userid);
	strcpy(currboard, bname);
	if (right) {
		sprintf(content, "\n\t\t\t【 公告 】\n\n"
			"\t经站务组讨论：\n"
			"\t同意 %s 辞去 %s 版的%s职务。\n"
			"\t在此，对他曾经在 %s 版的辛苦劳作表示感谢。\n\n"
			"\t希望今后也能支持本版的工作.",
			lookupuser->userid, bname, bm ? "大版主" : "版主",
			bname);
	} else {
		sprintf(content, "\n\t\t\t【撤职公告】\n\n"
			"\t经站务组讨论决定：\n"
			"\t撤除 %s 版%s %s 的%s职务。\n",
			bname, bm ? "大版主" : "版主", lookupuser->userid,
			bm ? "大版主" : "版主");
	}
	for (i = 0; i < 5; i++)
		buf[i][0] = '\0';
	move(14, 0);
	prints("请输入%s附言(最多五行，按 Enter 结束)",
	       right ? "版主离任" : "版主撤职");
	for (i = 0; i < 5; i++) {
		getdata(i + 15, 0, ": ", buf[i], STRLEN - 5, DOECHO, YEA);
		if (buf[i][0] == '\0')
			break;
		if (i == 0)
			strcat(content,
			       right ? "\n\n离任附言：\n" : "\n\n撤职说明：\n");
		strcat(content, buf[i]);
		strcat(content, "\n");
	}
	admin_b_report(title, content, &fh);
	mail_buf(content, strlen(content), lookupuser->userid, title, "deliver");
	prints("\n执行完毕！");
	pressanykey();
	return 0;
}

int
retireBM(uid, bname)
char *uid;
char *bname;
{
	char tmp[STRLEN];
	char content[1024], title[STRLEN];
	int i, oldbm, id, pos, bmpos = -1, bm = 0;
	struct boardheader fh;
	struct userec *lookupuser;
	if (!(id = getuser(uid, &lookupuser)))
		return -1;
	pos =
	    new_search_record(BOARDS, &fh, sizeof (fh), (void *) cmpbnames,
			      bname);
	if (!pos)
		return -2;
	oldbm = getbmnum(lookupuser->userid);
	for (i = 0; i < BMNUM; i++) {
		if (!strcasecmp(fh.bm[i], lookupuser->userid)) {
			bmpos = i;
			if (i < 4)
				bm = 1;
			else
				bm = 0;
		}
	}
	if (bmpos == -1)
		return -3;
	anno_title(title, &fh);
	fh.bm[bmpos][0] = 0;	//先清理掉, 免的有问题
	fh.hiretime[bmpos] = 0;
	for (i = bmpos; i < (bm ? 4 : BMNUM); i++) {
		if (i == bm ? 3 : BMNUM - 1) {	//最后一个BM
			fh.bm[i][0] = 0;
			fh.hiretime[i] = 0;
		} else {
			strcpy(fh.bm[i], fh.bm[i + 1]);
			fh.hiretime[i] = fh.hiretime[i + 1];
		}
	}
	if (bm) {
		anno_title(tmp, &fh);
		get_grp(fh.filename);
		edit_grp(fh.filename, lookgrp, title, tmp);
	}
	
	bbsinfo.utmpshm->syncbmonline = 1;
	substitute_record(BOARDS, &fh, sizeof (fh), pos);
	reload_boards();

	sprintf(genbuf, "取消 %s 的 %s 讨论区版主职务", lookupuser->userid,
		fh.filename);
	securityreport(genbuf, genbuf);
	if (!(oldbm - 1)) {
		char secu[STRLEN];
		struct userec tmpu;
		if (!(lookupuser->userlevel & PERM_OBOARDS)
		    && !(lookupuser->userlevel & PERM_SYSOP)) {
			memcpy(&tmpu, lookupuser, sizeof (tmpu));
			tmpu.userlevel &= ~PERM_BOARDS;
			updateuserec(&tmpu, 0);
			sprintf(secu, "版主卸职, 取消 %s 的版主权限",
				lookupuser->userid);
			securityreport(secu, secu);
		}
	}
	sprintf(title, "[公告]撤除 %s 版%s %s ", bname,
		bm ? "版主" : "版副", lookupuser->userid);
	strcpy(currboard, bname);
	sprintf(content, "\n\t\t\t【撤职公告】\n\n"
		"\t系统撤职：\n"
		"\t由于ID死亡，撤除 %s 版%s %s 的%s职务。\n",
		bname, bm ? "版主" : "版副", lookupuser->userid,
		bm ? "版主" : "版副");
	deliverreport(title, content);
	if (normal_board(currboard)) {
		strcpy(currboard, "Board");
		deliverreport(title, content);
	}
	return 0;
}

int
retire_allBM(uid)
char *uid;
{
	struct boardheader bh;
	int fd, size;
	size = sizeof (bh);
	if ((fd = open(BOARDS, O_RDONLY, 0)) == -1)
		return -1;
	while (read(fd, &bh, size) > 0)
		retireBM(uid, bh.filename);
	close(fd);
	bbsinfo.utmpshm->syncbmonline = 1;
	return 0;
}

int
m_addpersonal()
{
	FILE *fn;
	char digestpath[80] = MY_BBS_HOME "/0Announce/groups/GROUP_0/Personal_Corpus";
	char personalpath[80], linkpath[80], title[STRLEN];
	char firstchar[2], linkfirstchar[2];
	struct userec *lookupuser;
	int id;
	modify_user_mode(DIGEST);
	if (!check_systempasswd()) {
		return 1;
	}
	clear();
	if (!file_isdir(digestpath)) {
		prints("请先建立个人文集讨论区：Personal_Corpus");
		pressanykey();
		return 1;
	}
	stand_title("创建个人文集");
	clrtoeol();
	move(2, 0);
	usercomplete("请输入使用者代号: ", genbuf);
	if (*genbuf == '\0') {
		clear();
		return 1;
	}
	if (!(id = getuser(genbuf, &lookupuser))) {
		prints("错误的使用者代号");
		clrtoeol();
		pressreturn();
		clear();
		return 1;
	}
	firstchar[0] = mytoupper(lookupuser->userid[0]);
	firstchar[1] = '\0';
	if (!isalpha(lookupuser->userid[0])) {
		getdata(3, 0, "非英文ID，请输入拼音首字母:", linkfirstchar, 2,
				DOECHO, YEA);
		linkfirstchar[0] = toupper(linkfirstchar[0]);
		linkfirstchar[1] = '\0';
	}
	prints("%c", isalpha(lookupuser->userid[0])?firstchar[0]:linkfirstchar[0]);
	sprintf(personalpath, "%s/%c", digestpath, firstchar[0]);
	if (!file_isdir(personalpath)) {
		mkdir(personalpath, 0755);
		sprintf(personalpath, "%s/.Names", personalpath);
		if ((fn = fopen(personalpath, "w")) == NULL) {
			return -1;
		}
		fprintf(fn, "#\n");
		fprintf(fn, "# Title=%s\n", firstchar);
		fprintf(fn, "#\n");
		fclose(fn);
		linkto(digestpath, firstchar, firstchar);
		sprintf(personalpath, "%s/%c", digestpath, firstchar[0]);
	}
	sprintf(personalpath, "%s/%c/%s", digestpath,
		firstchar[0], lookupuser->userid);
	sprintf(linkpath, "%s/%c/%s", digestpath,
		linkfirstchar[0], lookupuser->userid);
	if (file_isdir(personalpath)) {
		prints("该用户的个人文集目录已存在, 按任意键取消..");
		pressanykey();
		return 1;
	}
	if (lookupuser->stay / 60 / 60 < 6) {
		prints("该用户上站时间不够,无法申请个人文集, 按任意键取消..");
		pressanykey();
		return 1;
	}
	move(4, 0);
	if (askyn("确定要为该用户创建一个个人文集吗?", YEA, NA) == NA) {
		prints("你选择取消创建. 按任意键取消...");
		pressanykey();
		return 1;
	}
	mkdir(personalpath, 0755);
	chmod(personalpath, 0755);
	symlink(personalpath, linkpath);
	
	move(7, 0);
	prints("[直接按 ENTER 键, 则标题缺省为: \033[32m%s文集\033[m]",
	       lookupuser->userid);
	getdata(6, 0, "请输入个人文集之标题: ", genbuf, 39, DOECHO, YEA);
	if (genbuf[0] == '\0')
		sprintf(title, "%s文集", lookupuser->userid);
	else
		sprintf(title, "%s文集——%s", lookupuser->userid, genbuf);
	sprintf(title, "%-38.38s(BM: %s _Personal)", title, lookupuser->userid);
	sprintf(linkpath, "%s/%c", digestpath, linkfirstchar[0]);
	sprintf(digestpath, "%s/%c", digestpath, firstchar[0]);
	linkto(digestpath, lookupuser->userid, title);
	if (!isalpha(lookupuser->userid[0])) {
		linkto(linkpath, lookupuser->userid, title);
	}
	sprintf(personalpath, "%s/.Names", personalpath);
	if ((fn = fopen(personalpath, "w")) == NULL) {
		return -1;
	}
	fprintf(fn, "#\n");
	fprintf(fn, "# Title=%s\n", title);
	fprintf(fn, "#\n");
	fclose(fn);
	if (!(lookupuser->userlevel & PERM_SPECIAL8)) {
		char secu[STRLEN];
		struct userec tmpu;
		memcpy(&tmpu, lookupuser, sizeof (tmpu));
		tmpu.userlevel |= PERM_SPECIAL8;
		updateuserec(&tmpu, 0);
		sprintf(secu, "创建个人文集, 给予 %s 文集管理权限",
			lookupuser->userid);
		securityreport(secu, secu);
		move(10, 0);
		prints(secu);

	}
	system_mail_file("etc/s_personal", lookupuser->userid, "个人文集操作手册", currentuser->userid);
	prints("已经创建个人文集, 请按任意键继续...");
	pressanykey();
	return 0;
}
