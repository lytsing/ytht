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
#define DENY 1
#define UNDENY 2
#define CHANGEDENY 3

static int addtodeny(char *uident, char *msg, int ischange, int isglobal,
		     int isanony, int *dday, int *limit);
static int deldeny(char *uident, int isglobal, int isanony, int ischange);
static int delclubmember(char *uident);
static int deny_notice(int action, char *user, int isglobal, int isanony,
		       char *msgbuf, int day, int limit);
static int setclubnum(int bnum, char *user, int mode);
static void setinprison();
static void setexecuteuser();

const static char *denyClass[] = {"����", "վ��", "�������ܹ�", "����", NULL};

static int
setclubnum(int bnum, char *user, int mode)
{
	FILE *fp;
	char fn[STRLEN];
	int clubrights[CLUB_SIZE];
	struct userec *lookupuser;
	if (bnum == 0)
		return 0;
	if (bbsinfo.bcache[bnum - 1].header.clubnum == 0)	//��������ȫ��closeclub
		return 1;
	if (bbsinfo.bcache[bnum - 1].header.clubnum >
	    CLUB_SIZE * sizeof (int) * 8) {
		errlog("clubnum too large..");
		return 0;
	}
	if (!getuser(user, &lookupuser))
		return 0;
	sethomefile(fn, user, "clubrights");
	if ((fp = fopen(fn, "r")) != NULL) {
		fread(&(clubrights), sizeof (int), CLUB_SIZE, fp);
		fclose(fp);
	} else
		memset(&(clubrights), 0, CLUB_SIZE * sizeof (int));
	if (mode == 0)
		clubrights[bbsinfo.bcache[bnum - 1].header.clubnum /
			   (8 * sizeof (int))] |=
		    (1 << bbsinfo.bcache[bnum - 1].header.clubnum %
		     (8 * sizeof (int)));
	else
		clubrights[bbsinfo.bcache[bnum - 1].header.clubnum /
			   (8 * sizeof (int))] &=
		    ~(1 << bbsinfo.bcache[bnum - 1].header.clubnum %
		      (8 * sizeof (int)));
	sethomefile(fn, user, "clubrights");
	if ((fp = fopen(fn, "w")) != NULL) {
		fwrite(&(clubrights), sizeof (int), CLUB_SIZE, fp);
		fclose(fp);
	}
	return 0;
}

static int
addtodeny(uident, msg, ischange, isglobal, isanony, dday, limit)
char *uident;
char *msg;
int ischange, isglobal, isanony;
int *dday;
int *limit;
{
	char buf[50], strtosave[256];
	char buf2[50];
	int day;
	time_t nowtime;
	char ans;
	int seek;
	struct boardmem *bp;

	if (isglobal)
		strcpy(genbuf, "deny_users");
	else if (isanony)
		setbfile(genbuf, currboard, "deny_anony");
	else
		setbfile(genbuf, currboard, "deny_users");
	seek = seek_in_file(genbuf, uident);
	if ((ischange && !seek) || (!ischange && seek)) {
		move(2, 0);
		prints("�����ID����!");
		pressreturn();
		return -1;
	}
	buf[0] = 0;
	move(2, 0);
	prints("�������%s", (isanony) ? "Anonymous" : uident);
	while (strlen(buf) < 4)
		getdata(3, 0, "����˵��(��������): ", buf, 40, DOECHO, YEA);

	do {
		getdata(4, 0, "��������(0-�ֶ����): ", buf2, 4, DOECHO, YEA);
		day = atoi(buf2);
		if (day < 0)
			continue;
		if (!(currentuser->userlevel & PERM_SYSOP)
		    && (!day || day > 14)) {
			move(4, 0);
			prints("����Ȩ��,����Ҫ,����ϵվ��!");
			pressreturn();
		} else
			break;
	} while (1);
	bp = getbcache(currboard);
	if (!bp)
		return -1;
	if (USERPERM(currentuser, PERM_BLEVELS)
		|| (USERPERM(currentuser, PERM_SPECIAL4)
		&& issecm((bp->header).sec1, currentuser->userid))) {
		// limit =  0 ��ʾ��ͨ���
		//       =  1 ��ʾվ��ʹ�����Ʒ�ʽ
		//	 =  2 ��ʾ�������ܹ�ʹ�����Ʒ�ʽ
		//	 =  3 ��ʾ����ʹ�����Ʒ�ʽ
		move(5, 0);
		if (askyn("�Ƿ�ʹ����ʱ���Ʒ�ʽ", YEA, NA) == YEA) {
			if (USERPERM(currentuser, PERM_SYSOP))
				*limit = 1;
			else if (USERPERM(currentuser, PERM_OBOARDS))
				*limit = 2;
			else if (USERPERM(currentuser, PERM_SPECIAL4))
				*limit = 3;
		}
	}
	//add by lepton for deny bm's right
	nowtime = time(NULL);
	*dday = day;
	if (day) {
		struct tm *tmtime;
		time_t undenytime = nowtime + (day + 1) * 86400;
		tmtime = gmtime(&undenytime);
		sprintf(strtosave, "%-12s %-40s %2d��%2d�ս� \033[%ldm", uident,
			buf, tmtime->tm_mon + 1, tmtime->tm_mday,
			(long int) (undenytime - 86400));
		sprintf(msg,
			"����ԭ��: %s\n��������: %d\n�������: %d��%d��\n"
			"������Ա: %s  [%s]\n"
			"���취: ��ϵ������ǰ��������ϵͳ�Զ����\n"
			"�������飬���������Ա�������Arbitration��Ͷ��\n",
			buf, day, tmtime->tm_mon + 1, tmtime->tm_mday,
			currentuser->userid, denyClass[*limit]);
	} else {
		sprintf(strtosave, "%-12s %-35s �ֶ����", uident, buf);
		sprintf(msg,
			"����ԭ��: %s\n�������: [�ֶ�]\n"
			"������Ա: %s  [վ��]\n���취: ��д�Ÿ� SYSOP ��ϵվ��������\n"
			"�������飬���������Ա�������Arbitration��Ͷ��\n",
			buf, currentuser->userid);
	}
	if (ischange)
		ans = askone(6, 0, "���Ҫ�ı�ô?[y/N]: ", "YN", 'N');
	else
		ans = askone(6, 0, "���Ҫ��ô?[y/N]: ", "YN", 'N');
	if (ans != 'Y')
		return -1;
	if (ischange)
		deldeny(uident, isglobal, 0, 1);
	if (isglobal)
		strcpy(genbuf, "deny_users");
	else if (isanony)
		setbfile(genbuf, currboard, "deny_anony");
	else
		setbfile(genbuf, currboard, "deny_users");
	return addtofile(genbuf, strtosave);
}

static int
deldeny(uident, isglobal, isanony, ischange)
char *uident;
int isglobal;
int isanony;
int ischange;
{
	char fn[STRLEN];

	if (isglobal)
		strcpy(fn, "deny_users");
	else if (isanony)
		setbfile(fn, currboard, "deny_anony");
	else
		setbfile(fn, currboard, "deny_users");
	if (!USERPERM(currentuser, PERM_SYSOP)
	    && !strcmp(uident, currentuser->userid) && !ischange)	/*���ܸ��Լ���� */
		return 0;
	return del_from_file(fn, uident);
}

int
is_inprison(char *uident)
{
	return seek_in_file("etc/prisonor", uident);
}

static void
setinprison()
{
	char ans;
	char uident[STRLEN];
	char buf[50], buf2[8];
	int day, count;
	long pos;
	char repbuf[STRLEN], msgbuf[256], tmpbuf[256];
	struct tm *tmtime;
	time_t nowtime = time(NULL), daytime;
	struct userec *lookupuser;
	while (1) {
		clear();
		prints("�趨����������Ա����\n");
		count = listfilecontent("etc/prisonor");
		FreeNameList();
		if (count)
			ans = askone(1, 0,
				     "(A)���� (D)ɾ�� (E)�뿪 [E]: ", "ADE",
				     'E');
		else
			ans = askone(1, 0, "(A)���� (E)�뿪 [E]: ", "AE", 'E');

		if (ans == 'A') {
			usercomplete("���ӷ��̵�ʹ����: ", uident);
			if (uident[0] == 0)
				continue;
			if (!getuser(uident, &lookupuser)) {
				prints("�����ʹ�����ʺţ�\n");
				pressreturn();
				continue;
			}
			strcpy(uident, lookupuser->userid);
			pos = is_inprison(uident);
			if (pos) {
				prints("%s �Ѿ��ڷ����ˣ�\n", uident);
				pressreturn();
				continue;
			}
			buf[0] = 0;
			while (strlen(buf) < 4)
				getdata(3, 0, "����˵��(��������): ", buf, 28,
					DOECHO, YEA);
			buf[28] = 0;
			while (1) {
				getdata(4, 0, "��������(0-�ֶ�����): ", buf2, 4,
					DOECHO, YEA);
				day = atoi(buf2);
				if ((day < 0)
				    || ((day == 0) && (buf2[0] != '0')))
					continue;
				break;
			}

			ans = askone(5, 0, "���Ҫִ��ô?[y/N]: ", "YN", 'N');
			if (ans == 'Y') {
				//day+1����ʾ��Ҫ,ʵ��Ҫ������. denyͬ.
				daytime = nowtime + (day + 1) * 86400;
				tmtime = gmtime(&daytime);
				if (day > 0)
					sprintf(tmpbuf, "%-28s��%2d��%2d�ս�",
						buf, tmtime->tm_mon + 1,
						tmtime->tm_mday);
				else
					sprintf(tmpbuf, "%-28s���ֶ�����", buf);
				sprintf(msgbuf, "%-12s %-40s \033[%ldm", uident,
					tmpbuf,
					day >
					0 ? (long int) (daytime - 86400) : 0L);
				if (addtofile("etc/prisonor", msgbuf) < 0) {
					prints("ִ��ʧ�ܣ�����ϵϵͳά����\n");
					pressreturn();
					continue;
				}
				if (day > 0)
					sprintf(repbuf,
						"���� %s �������� %d ��",
						uident, day);
				else
					sprintf(repbuf, "���� %s ��������",
						uident);
				if (day > 0)
					sprintf(msgbuf,
						"վ�� %s �� %s �ؽ��������� %d ��\n\n��������: %2d��%2d��\n����˵��: %s\n",
						currentuser->userid, uident,
						day, tmtime->tm_mon + 1,
						tmtime->tm_mday, buf);
				else
					sprintf(msgbuf,
						"վ�� %s �� %s �ؽ�����\n\n��������: �ֶ�����\n����˵��: %s\n",
						currentuser->userid, uident,
						buf);
				securityreport(repbuf, msgbuf);
				deliverreport(repbuf, msgbuf);
				mail_buf(msgbuf, strlen(msgbuf), uident, repbuf, "deliver");
				prints("ִ�гɹ���");
				pressreturn();
			}
		} else if ((ans == 'D') && count) {
			usercomplete("�ͷŷ��̵�ʹ����: ", uident);
			if (uident[0] == 0)
				continue;
			if (!getuser(uident, &lookupuser)) {
				prints("�����ʹ�����ʺţ�\n");
				pressreturn();
				continue;
			}
			strcpy(uident, lookupuser->userid);
			pos = is_inprison(uident);
			if (!pos) {
				prints("%s û���ڷ��̣�\n", uident);
				pressreturn();
				continue;
			}
			ans = askone(3, 0, "���Ҫִ��ô?[y/N]: ", "YN", 'N');
			if (ans == 'Y') {
				int ret =
				    del_from_file(MY_BBS_HOME "/etc/prisonor",
						  uident);
				sprintf(repbuf, "%s �� %s �Ӽ������ͷ�",
					currentuser->userid, uident);
				sprintf(msgbuf,
					"վ�� %s �� %s �Ӽ������ͷ�\n�����վ��Ĺ�����\n",
					currentuser->userid, uident);
				securityreport(repbuf, msgbuf);
				mail_buf(msgbuf, strlen(msgbuf), uident, repbuf, "deliver");
				if (ret > 0)
					prints("ִ�гɹ���");
				else
					prints("ִ��ʧ�ܣ�");
				pressreturn();
			}
		} else
			break;
	}
}

int
is_todel(char *uident)
{
	return seek_in_file("etc/tobeexecuted", uident);
}

static void
setexecuteuser()
{
	char ans;
	char uident[STRLEN];
	char buf[50], buf2[8];
	int day, count;
	long pos;
	char repbuf[STRLEN], msgbuf[256], tmpbuf[256];
	struct tm *tmtime;
	time_t nowtime = time(NULL), daytime;
	struct userec *lookupuser;
	int fd_lock;
	int ret;
	while (1) {
		clear();
		prints("�趨���嵵��Ա����\n");
		count = listfilecontent("etc/tobeexecuted");
		FreeNameList();
		if (count)
			ans = askone(1, 0,
				     "(A)���� (D)ɾ�� (E)�뿪 [E]: ", "ADE",
				     'E');
		else
			ans = askone(1, 0, "(A)���� (E)�뿪 [E]: ", "AE", 'E');

		if (ans == 'A') {
			usercomplete("���Ӵ��嵵��ʹ����: ", uident);
			if (uident[0] == 0)
				continue;
			if (!getuser(uident, &lookupuser)) {
				prints("�����ʹ�����ʺţ�\n");
				pressreturn();
				continue;
			}
			strcpy(uident, lookupuser->userid);
			pos = is_todel(uident);
			if (pos) {
				prints("%s �Ѿ����б����ˣ�\n", uident);
				pressreturn();
				continue;
			}
			buf[0] = 0;
			while (strlen(buf) < 4)
				getdata(3, 0, "����˵��(��������): ", buf, 28,
					DOECHO, YEA);
			buf[28] = 0;
			while (1) {
				getdata(4, 0, "��������(Ĭ��1��): ", buf2, 4,
					DOECHO, YEA);
				day = atoi(buf2);
				if ((day == 0) && (buf2[0] != '0'))
					day = 1;
				else if (day <= 0)
					continue;
				break;
			}
			ans = askone(5, 0, "���Ҫִ��ô?[y/N]: ", "YN", 'N');
			if (ans == 'Y') {
				//day+1����ʾ��Ҫ,ʵ��Ҫ������. denyͬ.
				daytime = nowtime + (day + 1) * 86400;
				tmtime = gmtime(&daytime);
				sprintf(tmpbuf, "%-28s��%2d��%2d��ִ��",
						buf, tmtime->tm_mon + 1,
						tmtime->tm_mday);
				sprintf(msgbuf, "%-12s %-42s \033[%ldm", uident,
					tmpbuf,
					day >
					0 ? (long int) (daytime - 86400) : 0L);
				// lock
				if ((fd_lock = open("etc/tobeexecuted.lock", O_RDWR | O_CREAT, 0660)) == -1) {
					prints("����ʧ�ܣ�\n");
					return;
				}
				flock(fd_lock, LOCK_EX);
				if (addtofile("etc/tobeexecuted", msgbuf) < 0) {
					prints("ִ��ʧ�ܣ�����ϵϵͳά����\n");
					pressreturn();
					continue;
				}
				flock(fd_lock, LOCK_UN);
				close(fd_lock);
				sprintf(repbuf,
						"%d ���ɾ���˺� %s",
						day, uident);
				sprintf(msgbuf,
						"ɾ���˺� %s���������߱��ݸ������ϡ�\n\n%d ���ִ��\nִ������: %2d��%2d��\nɾ��ԭ��: %s\n",
						uident, day, tmtime->tm_mon + 1,
						tmtime->tm_mday, buf);
				securityreport(repbuf, msgbuf);
				deliverreport(repbuf, msgbuf);
				mail_buf(msgbuf, strlen(msgbuf), uident, repbuf, "deliver");
				prints("ִ�гɹ���");
				pressreturn();
			}
		} else if ((ans == 'D') && count) {
			usercomplete("ȡ�����嵵��ʹ����: ", uident);
			if (uident[0] == 0)
				continue;
			if (!getuser(uident, &lookupuser)) {
				prints("�����ʹ�����ʺţ�\n");
				pressreturn();
				continue;
			}
			strcpy(uident, lookupuser->userid);
			pos = is_todel(uident);
			if (!pos) {
				prints("%s û�����б��У�\n", uident);
				pressreturn();
				continue;
			}
			ans = askone(3, 0, "���Ҫִ��ô?[y/N]: ", "YN", 'N');
			if (ans == 'Y') {
				// lock
				if ((fd_lock = open("etc/tobeexecuted.lock", O_RDWR | O_CREAT, 0660)) == -1) {
					prints("����ʧ�ܣ�\n");
					return;
				}
				flock(fd_lock, LOCK_EX);
				ret =
				    del_from_file(MY_BBS_HOME "/etc/tobeexecuted",
						  uident);
				flock(fd_lock, LOCK_UN);
				close(fd_lock);
				sprintf(repbuf, "%s �� %s ���嵵���������",
					currentuser->userid, uident);
				sprintf(msgbuf,
					"վ�� %s �� %s ���嵵���������\n�����վ��Ĺ�����\n",
					currentuser->userid, uident);
				securityreport(repbuf, msgbuf);
				mail_buf(msgbuf, strlen(msgbuf), uident, repbuf, "deliver");
				if (ret > 0)
					prints("ִ�гɹ���");
				else
					prints("ִ��ʧ�ܣ�");
				pressreturn();
			}
		} else
			break;
	}
}

int
deny_user()
{
	char uident[STRLEN];
	char ans;
	char msgbuf[512];
	int count, isglobal = 0;
	int day, limit = 0;

	if (!IScurrBM) {
		return DONOTHING;
	}
	if (!strcmp(currboard, "Penalty")) {
		if (!USERPERM(currentuser, PERM_SYSOP)) {
			isglobal = 1;
			limit = 2;
		} else {
			ans = askone(0, 0, "�趨ʲô������(A) ȫվ��� "
					"(B) ���� (C) �嵵 (E)�뿪 [E]:", "ABCE", 'E');
			if (ans == 'E')
				return FULLUPDATE;
			if (ans == 'A') {
				isglobal = 1;
				limit = 1;
			} else if (ans == 'B') {
				setinprison();
				return FULLUPDATE;
			} else if (ans == 'C') {
				setexecuteuser();
				return FULLUPDATE;
			}
		}
	}
	if (isglobal)
		strcpy(genbuf, "deny_users");
	else
		setbfile(genbuf, currboard, "deny_users");
//      ansimore(genbuf, YEA);
	while (1) {
		clear();
		prints("�趨�޷� Post ������\n");
		if (isglobal)
			strcpy(genbuf, "deny_users");
		else
			setbfile(genbuf, currboard, "deny_users");
		count = listfilecontent(genbuf);
		if (count)
			ans = askone(1, 0, "(A)���� (D)ɾ�� (C)�ı� "
				"(E)�뿪 [E]: ", "ADCE", 'E');
		else
			ans = askone(1, 0, "(A)���� (E)�뿪 "
				"[E]: ", "AE", 'E');
		if (ans == 'A') {
			move(1, 0);
			if (isglobal)
				usercomplete("�����޷� POST ��ʹ����: ",
					     uident);
			else {
				int canpost = 0;
				while (!canpost) {
					move(1, 0);
					clrtoeol();
					usercomplete("�����޷� POST ��ʹ���ߣ�",
						     uident);
					if (*uident == '\0')
						break;
					canpost = posttest(uident, currboard);
				}
			}
			if (*uident != '\0') {
				if (addtodeny
				    (uident, msgbuf, 0, isglobal, 0,
				     &day, &limit) == 1) {
					deny_notice(DENY, uident, isglobal, 0,
						    msgbuf, day, limit);
					tracelog("%s deny %s %s",
						 currentuser->userid, currboard,
						 uident);
				}
			}
		} else if (ans == 'C') {
			move(1, 0);
			usercomplete("�ı�˭�ķ��ʱ���˵��: ", uident);
			if (*uident != '\0') {
				if (addtodeny
				    (uident, msgbuf, 1, isglobal, 0,
				     &day, &limit) == 1) {
					deny_notice(CHANGEDENY, uident,
						    isglobal, 0, msgbuf, day,
						    limit);
				}
			}
		} else if ((ans == 'D') && count) {
			move(1, 0);
			namecomplete("ɾ���޷� POST ��ʹ����: ", uident);
			move(1, 0);
			clrtoeol();
			if (uident[0] != '\0') {
				if (deldeny(uident, isglobal, 0, 0)) {
					deny_notice(UNDENY, uident, isglobal, 0,
						    msgbuf, 0, -1);
				}
			}
		} else {
			break;
		}
	}
	FreeNameList();
	clear();
	return FULLUPDATE;
}

int
setclubmember(board, uid, action)
char *board;
char *uid;
int action;
{
	char fn[STRLEN];
	int i;

	setbfile(fn, board, "club_users");
	if (action == 1) {
		if ((i = getbnum(board)) == 0)
			return -1;
		setclubnum(i, uid, 0);
		return addtofile(fn, uid);
	} else {
		if ((i = getbnum(board)) == 0)
			return -1;
		setclubnum(i, uid, 1);
		return del_from_file(fn, uid);
	}
}

int
addclubdenysysop(uident)
char *uident;
{
	int id, i, seek;
	char fn[STRLEN];
	struct userec *lookupuser;

	if (!(id = getuser(uident, &lookupuser))) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return -1;
	}
	if ((i = getbnum(currboard)) == 0)
		return -1;
	setbfile(genbuf, currboard, "club_deny_sysop");
	
	seek = seek_in_file(genbuf, uident);
	if (seek) {
		move(2, 0);
		prints("��ͱ������乬�ˣ��������Ļ�վ�����������������أ�");
		pressreturn();
		return -1;
	}

	if (askone(4, 0, "���Ҫ�����[y/N]", "YN", 'N') != 'Y')
		return -1;

	setbfile(fn, currboard, "club_deny_sysop");
	return addtofile(fn, uident);
}

int
addclubmember(uident)
char *uident;
{
	int id;
	int i;
	char fn[STRLEN];
	int seek;
	struct userec *lookupuser;

	if (!(id = getuser(uident, &lookupuser))) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return -1;
	}
	if ((i = getbnum(currboard)) == 0)
		return -1;
	setbfile(genbuf, currboard, "club_users");
	if (bbsinfo.bcache[i - 1].header.flag & CLOSECLUB_FLAG
	    && bbsinfo.bcache[i - 1].header.flag & CLUBLEVEL_FLAG) {
		if (clubmember_num(currboard) > 48) {
			move(2, 0);
			prints("��ȫclose���ֲ����48����Ա");
			pressreturn();
			return -1;
		}
	}

	seek = seek_in_file(genbuf, uident);
	if (seek) {
		move(2, 0);
		prints("�����ID �Ѿ�����!");
		pressreturn();
		return -1;
	}

	if (askone(4, 0, "���Ҫ���ô?[y/N]: ", "YN", 'N') != 'Y')
		return -1;
	setbfile(fn, currboard, "club_users");
	setclubnum(i, uident, 0);
	return addtofile(fn, uident);
}

int
delclubdenysysop(uident)
char *uident;
{
	char fn[STRLEN];
	int i;

	if ((i = getbnum(currboard)) == 0)
		return DONOTHING;
	
	setbfile(fn, currboard, "club_deny_sysop");
	if (!seek_in_file(fn, uident)) {
		move(3, 0);
		prints("�ۣ�ʧ���˿ڣ�");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}

	return del_from_file(fn, uident);
}

static int
delclubmember(uident)
char *uident;
{
	char fn[STRLEN];
	int i;
	if ((i = getbnum(currboard)) == 0)
		return DONOTHING;
	setbfile(fn, currboard, "club_users");
	if (!seek_in_file(fn, uident)) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	setclubnum(i, uident, 1);
	return del_from_file(fn, uident);
}

int
clubmember()
{
	char uident[STRLEN];
	char ans, repbuf[STRLEN], cmd_msg[STRLEN], cmd_key[STRLEN];
	int count1, count2, i;

	if (!(IScurrBM)) {
		return DONOTHING;
	}
	if ((i = getbnum(currboard)) == 0)
		return DONOTHING;
	if (!(bbsinfo.bcache[i - 1].header.flag & CLUB_FLAG))
		return DONOTHING;
	while (1) {
		setbfile(genbuf, currboard, "club_deny_sysop");
		count2 = listfilecontent(genbuf);
		clear();
		prints("�趨���ֲ�����\n");
		setbfile(genbuf, currboard, "club_users");
		count1 = listfilecontent(genbuf);
		
		strcpy(cmd_msg, "(A) ���� ");
		strcpy(cmd_key, "A");
		if (count1)	{
			sprintf(cmd_msg, "%s(D)ɾ�� ", cmd_msg);
			sprintf(cmd_key, "%sD", cmd_key);
		}
		if (bbsinfo.bcache[i - 1].header.flag & CLOSECLUB_FLAG &&
				bbsinfo.bcache[i - 1].header.flag & CLUBLEVEL_FLAG) {
			sprintf(cmd_msg, "%s(X)���Ӻ����� ", cmd_msg);
			sprintf(cmd_key, "%sX", cmd_key);
			if (count2) {
				sprintf(cmd_msg, "%s(Z)ɾ�������� ", cmd_msg);
				sprintf(cmd_key, "%sZ", cmd_key);
			}
		}
		if (count1) {
			sprintf(cmd_msg, "%s(M)���ֲ�Ⱥ���ż� ", cmd_msg);
			sprintf(cmd_key, "%sM", cmd_key);
		}
		sprintf(cmd_msg, "%s(E)�뿪 [E]��", cmd_msg);
		sprintf(cmd_key, "%sE", cmd_key);
		
		ans = askone(1, 0, cmd_msg, cmd_key, 'E');
		if (ans == 'A') {
			clear();
			setbfile(genbuf, currboard, "club_users");
			listfilecontent(genbuf);
			move(1, 0);
			usercomplete("���Ӿ��ֲ���Ա: ", uident);
			if (*uident != '\0') {
				if (addclubmember(uident) == 1) {
					sprintf(repbuf,
						"%s��%s����%s���ֲ�Ȩ��",
						uident, currentuser->userid,
						currboard);
					securityreport(repbuf, repbuf);
					deliverreport(repbuf, repbuf);
					system_mail_buf(repbuf, strlen(repbuf), uident, repbuf, currentuser->userid);
				}
			}
		} else if (ans == 'X') {
			clear();
			setbfile(genbuf, currboard, "club_deny_sysop");
			listfilecontent(genbuf);
			move(1, 0);
			usercomplete("����վ�������������SYSOP���Է�ֹ���з�SYSOPվ��͵������", uident);
			if (*uident != '\0') {
				if (addclubdenysysop(uident) == 1) {
					sprintf(repbuf,
							"%s��%s����%s���ֲ�������",
							currentuser->userid, uident, currboard);
					securityreport(repbuf, repbuf);
					deliverreport(repbuf, repbuf);
					system_mail_buf(repbuf, strlen(repbuf), uident, repbuf, currentuser->userid);
				}
			}
		} else if ((ans == 'D') && count1) {
			clear();
			setbfile(genbuf, currboard, "club_users");
			listfilecontent(genbuf);
			move(1, 0);
			namecomplete("ɾ�����ֲ�ʹ����: ", uident);
			move(1, 0);
			clrtoeol();
			if (uident[0] != '\0') {
				sprintf(genbuf, "���Ҫȡ�� %s �ľ��ֲ�Ȩ��ô:",
					uident);
				if (askyn(genbuf, YEA, NA))
					if (delclubmember(uident)) {
						sprintf(repbuf,
							"%s��%sȡ��%s���ֲ� Ȩ��",
							uident,
							currentuser->userid,
							currboard);
						securityreport(repbuf, repbuf);
						deliverreport(repbuf, repbuf);
						system_mail_buf(repbuf, strlen(repbuf), uident,
							 repbuf, currentuser->userid);
					}
			}
		} else if ((ans == 'Z') && count2) {
			clear();
			setbfile(genbuf, currboard, "club_deny_sysop");
			listfilecontent(genbuf);
			move(1, 0);
			namecomplete("ɾ���������е�վ��", uident);
			clrtoeol();
			if (uident[0] != '\0') {
				sprintf(genbuf, "��ķ� %s ����͵����", uident);
				if (askyn(genbuf, NA, YEA))
					if (delclubdenysysop(uident)) {
						sprintf(repbuf,
								"%s��%s���ֲ���������ɾ��%s",
								currentuser->userid, currboard, uident);
						securityreport(repbuf, repbuf);
						deliverreport(repbuf, repbuf);
						system_mail_buf(repbuf, strlen(repbuf), uident,
								repbuf, currentuser->userid);
					}
			}
			clrtoeol();
			
		} else if ((ans == 'M') && count1) {
			club_send();
		} else {
			break;
		}
	}
	FreeNameList();
	clear();
	return FULLUPDATE;
}

int
deny_from_article(ent, fileinfo, direct)
int ent;
struct fileheader *fileinfo;
char *direct;
{
	char msgbuf[512];
	char user[STRLEN];
	int seek, canpost, isanony;
	int day, limit = 0;
	if (!IScurrBM) {
		return DONOTHING;
	}
	if (!strcmp(fh2owner(fileinfo), "Anonymous")) {	/* ���������� */
		isanony = 1;
		setbfile(genbuf, currboard, "deny_anony");
		strcpy(user, fh2realauthor(fileinfo));
	} else {
		isanony = 0;
		setbfile(genbuf, currboard, "deny_users");
		strcpy(user, fileinfo->owner);
	}
	seek = seek_in_file(genbuf, user);
	if (seek) {		/* ��� */
		move(2, 0);
		if (askone(4, 0, "���Ҫ���ô?[y/N]: ", "YN", 'N') == 'N')
			return -1;
		if (deldeny(user, 0, isanony, 0) == 1)
			deny_notice(UNDENY, user, 0, isanony, msgbuf, 0, limit);

	} else {		/* ������� */
		canpost = posttest(user, currboard);
		if ((canpost)
		    && (addtodeny(user, msgbuf, 0, 0, isanony, &day, &limit) ==
			1)) {
			deny_notice(DENY, user, 0, isanony, msgbuf, day, limit);
			tracelog("%s deny %s %s", currentuser->userid,
				 currboard, user);
		}
	}
	return 0;
}

static int
deny_notice(action, user, isglobal, isanony, msgbuf, day, limit)
char *user, *msgbuf;
int action, isglobal, isanony;
int day;
int limit;
{
	char repbuf[STRLEN];
	char tmpbuf[256], tmpbuf2[256];
	int i;
	char repuser[IDLEN + 1];
	if (isanony)
		strcpy(repuser, "Anonymous");
	else
		strcpy(repuser, user);
	switch (action) {
	case DENY:
		if (isglobal) {
			sprintf(repbuf,
				"%s �� %s ȡ����ȫվ��POSTȨ", user, currentuser->userid);
			securityreport(repbuf, msgbuf);
			deliverreport(repbuf, msgbuf);
			mail_buf(msgbuf, strlen(msgbuf), user, repbuf, "deliver");
		} else if (limit >=1 && limit <= 3) {
			for (i = 10; msgbuf[i]; i++)
				if (msgbuf[i + 1] == '\n')
					msgbuf[i + 1] = 0;
			/*�����ȡ������� */
			strncpy(tmpbuf, msgbuf + 10, 256);
			tmpbuf[255] = 0;
			strncpy(tmpbuf2, msgbuf + i + 1, 256);
			tmpbuf2[255] = 0;
			sprintf(repbuf,
				"%s �� %s ��ʱ������%s��POSTȨ",
				repuser, currentuser->userid, currboard);
			sprintf(msgbuf,
				"%s %s ��Ϊ %s �� \033[1;32m%s\033[0m  ����\n"
				"�뱾�������%s����Ϊ����������׼����ȷ��\n"
				"�ָ���POSTȨ���߸�����ȷ�������\n\n%s",
				denyClass[limit], currentuser->userid, repuser, tmpbuf,
				repuser, tmpbuf2);
			securityreport(repbuf, msgbuf);
			deliverreport(repbuf, msgbuf);
			if (!hideboard(currboard))
				penaltyreport(repbuf, msgbuf);
			mail_buf(msgbuf, strlen(msgbuf), user, repbuf, "deliver");
		}
		/*old action */
		else {
			sprintf(repbuf, "%s �� %s ȡ����%s��POSTȨ", 
					repuser, currentuser->userid, currboard);
			securityreport(repbuf, msgbuf);
			deliverreport(repbuf, msgbuf);
			if (!hideboard(currboard))
				penaltyreport(repbuf, msgbuf);
			mail_buf(msgbuf, strlen(msgbuf), user, repbuf, "deliver");
		}
		break;
	case CHANGEDENY:
		sprintf(repbuf, "%s �ı�� %s ��ʱ���˵��",
			currentuser->userid, user);
		securityreport(repbuf, msgbuf);
		deliverreport(repbuf, msgbuf);
		if (!hideboard(currboard))
			penaltyreport(repbuf, msgbuf);
		mail_buf(msgbuf, strlen(msgbuf), user, repbuf, "deliver");
		break;
	case UNDENY:
		sprintf(repbuf, "%s �ָ� %s ��POSTȨ",
			currentuser->userid, repuser);
		sprintf(genbuf, "%s �ָ� %s ��%s��POSTȨ",
			currentuser->userid, repuser, isglobal ? "ȫվ" : currboard);
		snprintf(msgbuf, 256, "%s\n" "�����%sվ�Ĺ�����,лл!\n", genbuf, MY_BBS_NAME);
		securityreport(repbuf, genbuf);
		deliverreport(repbuf, msgbuf);
		mail_buf(msgbuf, strlen(msgbuf), user, repbuf, "deliver");
		break;
	}
	return 0;
}

int
clubmember_num(char *bname)
{
	FILE *fp;
	int id = 0;
	char buf[256];
	setbfile(buf, bname, "club_users");
	fp = fopen(buf, "r");
	if (fp == NULL)
		return 0;
	while (fgets(buf, 256, fp)) {
		if (!isspace(buf[0]))
			id++;
	}
	fclose(fp);
	return id;
}
