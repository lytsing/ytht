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
#include "regular.h"

typedef void (*power_dofunc) (int, struct fileheader *, char *);
static void sure_markdel(int ent, struct fileheader *fileinfo, char *direct);
static void sure_unmarkdel(int ent, struct fileheader *fileinfo, char *direct);
static void power_dir(int ent, struct fileheader *fileinfo, char *direct);
static int power_range(char *filename, int id1, int id2, char *select,
		       power_dofunc function, int *shoot);
static int titlehas(char *buf);
static int idis(char *buf);
static int idhas(char *buf);
static int checkmark(char *buf);
static int counttextlt(char *num);
static int checktext(char *query);
static int checkattr(char *buf);
static int checkstar(char *buf);
static int checkevas(char *buf);

static void
sure_markdel(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	if (fileinfo->owner[0] == '-')
		return;
	change_dir(direct, fileinfo,
		   (void *) DIR_do_suremarkdel, ent, digestmode, 0, NULL);
	return;
}

static void
sure_unmarkdel(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	if (fileinfo->owner[0] == '-')
		return;
	change_dir(direct, fileinfo,
		   (void *) DIR_do_sureunmarkdel, ent, digestmode, 0, NULL);
	return;
}

static struct fileheader *select_cur;
static void
power_dir(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	extern char currdirect[STRLEN];
//For hide the article is deleted by who, ugly code....
	if (!strncmp(fileinfo->title + 33, "- ", 2))
		fileinfo->title[33] = 0;
	append_record(currdirect, fileinfo, sizeof (struct fileheader));
}

int
mkpowersymlink(char *direct)
{
	char *ptr, buf[256];
	ptr = strrchr(direct, '/');
	if (!ptr || strncmp(ptr + 1, ".POWER.", 7))
		return -1;
	sprintf(buf, MY_BBS_HOME "/bbstmpfs/tmp/POWER.%s.%d",
		currentuser->userid, uinfo.pid);
	close(creat(buf, 0600));
	symlink(buf, direct);
	return 0;
}

void
rmpower()
{
	rmpowersymlink(NULL);
}

int
rmpowersymlink(char *direct)
{
	char *ptr, buf[256];
	sprintf(buf, MY_BBS_HOME "/bbstmpfs/tmp/POWER.%s.%d",
		currentuser->userid, uinfo.pid);
	unlink(buf);
	if (!direct)
		return 0;
	ptr = strrchr(direct, '/');
	if (!ptr || strncmp(ptr + 1, ".POWER.", 7))
		return -1;
	unlink(direct);
	return 0;
}

static int
power_range(filename, id1, id2, select, function, shoot)
char *filename;
int id1, id2;
char *select;
power_dofunc function;
int *shoot;
{
	struct fileheader *buf;
	int fd, bufsize, i, n, ret;
	struct stat st;
	*shoot = 0;
	if ((fd = open(filename, O_RDONLY)) == -1) {
		return -1;
	}
	fstat(fd, &st);
	if (-1 == id2)
		id2 = st.st_size / sizeof (struct fileheader);
	bufsize = sizeof (struct fileheader) * (id2 + 1 - id1);
	buf = malloc(bufsize);
	if (buf == NULL) {
		close(fd);
		return -4;
	}

	lseek(fd, (id1 - 1) * sizeof (struct fileheader), SEEK_SET);
	n = read(fd, buf, bufsize);
	close(fd);
	for (i = id1; i < id1 + n / sizeof (struct fileheader); i++) {
		select_cur = buf + (i - id1);
		ret = checkf(select);
		if (ret > 0) {
			(*function) (i, select_cur, filename);
			(*shoot)++;
		} else if (ret < 0) {
			free(buf);
			return -ret;
		}
	}
	free(buf);
	return 0;
}

int
power_action(filename, id1, id2, select, action)
char *filename;
int id1, id2;
char *select;
int action;
{
	ExtStru myextstru[] = {
		{titlehas, Pair("����", "��")},
		{idis, Pair("����", "��")},
		{idhas, Pair("����", "��")},
		{counttextlt, Pair("��ˮ����", "����")},
		{checkmark, Pair("���", "��")},
		{checktext, Pair("����", "��")},
		{checkattr, Pair("����", "��")},
		{checkstar, Pair("�Ƽ��Ǽ�", "����")},
		{checkevas, Pair("�Ƽ�����", "����")},
		{NULL, NULL, NULL}
	};
	power_dofunc function;
	int shoot;
	int ret;
	extern char currdirect[STRLEN];
	extstru = myextstru;
	switch (action) {
	case 1:
		function = sure_markdel;
		break;
	case 2:
		function = sure_unmarkdel;
		break;
	case 9:
		digestmode = 3;
		setbdir(currdirect, currboard, digestmode);
		unlink(currdirect);
		mkpowersymlink(currdirect);
		function = power_dir;
		break;
	case 0:
	default:
		return FULLUPDATE;
	}
	ret = power_range(filename, id1, id2, select, function, &shoot);
	tracelog("%s select %s %d %d", currentuser->userid, currboard, id1,
		 id2);
	if (ret < 0) {
		prints("�޷�ִ�г�������:%d,����ϵϵͳά��.\n", ret);
		pressreturn();
		if (action == 9) {
			digestmode = NA;
			setbdir(currdirect, currboard, digestmode);
		}
		return FULLUPDATE;
	} else if (ret > 0) {
		prints
		    ("���������﷨����,�ӵ�%d���ַ����Ҿ;��ò��Ծ�,һ�������������ַ���!",
		     ret);
		if (action == 9) {
			digestmode = NA;
			setbdir(currdirect, currboard, digestmode);
		}
		pressreturn();
		return FULLUPDATE;
	}

	limit_cpu();
	if (action == 9)
		return NEWDIRECT;
	fixkeep(filename, (id1 <= 0) ? 1 : id1, (id2 <= 0) ? 1 : id2);

	prints("�������,��%dƪ������������\n", shoot);
	pressreturn();
	return FULLUPDATE;
}

static int
titlehas(char *buf)
{
	if (strstr(select_cur->title, buf))
		return 1;
	else
		return 0;
}

static int
idis(char *buf)
{
	if (strcasecmp(select_cur->owner, buf))
		return 0;
	else
		return 1;
}

static int
idhas(char *buf)
{
	if (strcasestr(select_cur->owner, buf))
		return 1;
	else
		return 0;
}

static int
checkmark(char *buf)
{
	if (!strcasecmp(buf, "m")) {
		if (select_cur->accessed & FH_MARKED)
			return 1;
		else
			return 0;
	} else if (!strcasecmp(buf, "g")) {
		if (select_cur->accessed & FH_DIGEST)
			return 1;
		else
			return 0;
	} else if (!strcasecmp(buf, "@")) {
		if (select_cur->accessed & FH_ATTACHED)
			return 1;
		else
			return 0;
	} else
		return 0;
}

static int
counttextlt(char *num)
{
	int n, size;
	if (select_cur->sizebyte)
		size = bytenum(select_cur->sizebyte);
	else {
		char buf[256];
		snprintf(buf, sizeof (buf), "boards/%s/%s",
			 currboard, fh2fname(select_cur));
		size = eff_size(buf);
	}
	n = atoi(num);
	if (size < n)
		return 1;
	else
		return 0;
}

static int
checktext(char *query)
{
	char buf[256];
	snprintf(buf, sizeof (buf), "boards/%s/%s",
		 currboard, fh2fname(select_cur));
	return searchpattern(buf, query);
}

static int
checkattr(char *buf)
{
	if (!strcmp(buf, "δ��")) {
		if (UNREAD(select_cur, &brc))
			return 1;
		else
			return 0;
	} else if (!strcmp(buf, "ԭ��")) {
		if (strncmp(select_cur->title, "Re: ", 4))
			return 1;
		else
			return 0;
	} else
		return 0;
}

static int
checkstar(char *buf)
{
	if (select_cur->staravg50 >= atoi(buf) * 50)
		return 1;
	else
		return 0;
}

static int
checkevas(char *buf)
{
	if (select_cur->hasvoted >= atoi(buf))
		return 1;
	else
		return 0;
}

int
power_select(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char num[8];
	static char select[STRLEN];
	int inum1, inum2, answer;
	char dir[STRLEN];
	if (uinfo.mode != READING || digestmode != NA)
		return DONOTHING;
	snprintf(dir, STRLEN, "%s", direct);
	clear();
	prints("                  ��ǿ����ѡ��\n\n");
	prints("��ѡ�������Χ\n");
	getdata(3, 0, "��ƪ���±��: ", num, 7, DOECHO, YEA);
	inum1 = atoi(num);
	if (inum1 <= 0) {
		prints("������\n");
		pressreturn();
		return FULLUPDATE;
	}
	getdata(4, 0, "ĩƪ���±��: ", num, 7, DOECHO, YEA);
	inum2 = atoi(num);
	if (inum2 - inum1 <= 1) {
		prints("������\n");
		pressreturn();
		return FULLUPDATE;
	}
	move(6, 0);
	prints("����:\n"
	       "   ����һ: ��auto����������\n"
	       "   ��������������: ������ auto\n"
	       "   ���Ӷ�: ��autoд�Ĺ�ˮ����\n"
	       "   ��������������: ������ auto �� ��ˮ�������� 40\n"
	       "   ������: ��autoд�Ĺ�ˮ����,���ұ����Ϊm��\n"
	       "   ��������������: ������ auto �� ��ˮ�������� 40 �� ��Ǻ� m\n"
	       "   ������: �����б������һ����Ϳ����ytht������\n"
	       "   ��������������: ���⺬ һ����Ϳ �� ���⺬ ytht\n"
	       "   ������: �����в���autoҲ����deliverд������\n"
	       "   ��������������: �� (������ auto �� ������ deliver)\n"
	       "   (����)��������������: ���߲��� auto �� ���߲��� deliver\n"
	       "   ������: �������и���������\n"
	       "   ��������������: ��Ǻ� @\n"
	       "   ������: �������Ƽ��Ǽ���3�����ϵ�����\n"
	       "   ��������������: �Ƽ��Ǽ����� 3\n"
	       "   ���Ӱ�: �������Ƽ�������10�����ϵ�����\n"
	       "   ��������������: �Ƽ��������� 10\n");

	getdata(5, 0, "��������������: ", select, 60, DOECHO, NA);
	getdata(6, 0,
		"��������ϣ���Ĳ���: (0)ȡ�� (1)���ɾ�� (2)ȥ�����ɾ�� (9)�Ķ�:",
		num, 2, DOECHO, YEA);
	answer = atoi(num);
	if (answer > 0 && answer < 9 && !IScurrBM) {
		clrtoeol();
		prints("�����ǰ���\n");
		pressreturn();
		return FULLUPDATE;
	}
	return power_action(dir, inum1, inum2, select, answer);
}
