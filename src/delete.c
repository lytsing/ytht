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
int
d_board()
{
	struct boardheader binfo;
	int bid, ans;
	char bname[STRLEN];
	extern char lookgrp[];

	if (!USERPERM(currentuser, PERM_BLEVELS)) {
		return -1;
	}
	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("ɾ��������");
	make_blist_full();
	move(1, 0);
	namecomplete("������������: ", bname);
	FreeNameList();
	if (bname[0] == '\0')
		return -1;
	bid = getbnum(bname);
	if (get_record(BOARDS, &binfo, sizeof (binfo), bid) == -1) {
		move(2, 0);
		prints("����ȷ��������\n");
		pressreturn();
		clear();
		return -1;
	}
	ans = askyn("��ȷ��Ҫɾ�����������", NA, NA);
	if (ans != 1) {
		move(2, 0);
		prints("ȡ��ɾ���ж�\n");
		pressreturn();
		clear();
		return -1;
	}
	{
		char secu[STRLEN];
		sprintf(secu, "ɾ����������%s", binfo.filename);
		securityreport(secu, secu);
	}
	if (seek_in_file("0Announce/.Search", bname)) {
		move(4, 0);
		if (askyn("�Ƴ�������", NA, NA) == YEA) {
			get_grp(binfo.filename);
			del_grp(lookgrp, binfo.filename, binfo.title);
		}
	}
	if (seek_in_file("etc/junkboards", bname))
		del_from_file("etc/junkboards", bname);
	if (seek_in_file("0Announce/.Search", bname))
		del_from_file("0Announce/.Search", bname);

	if (binfo.filename[0] == '\0')
		return -1;	/* rrr - precaution */
	sprintf(genbuf, "boards/%s", binfo.filename);
	deltree(genbuf);
	sprintf(genbuf, "vote/%s", binfo.filename);
	deltree(genbuf);
	sprintf(genbuf, " << '%s'�� %s ɾ�� >>",
		binfo.filename, currentuser->userid);
	memset(&binfo, 0, sizeof (binfo));
	strsncpy(binfo.title, genbuf, sizeof (binfo.title));
	binfo.level = PERM_SYSOP;
	substitute_record(BOARDS, &binfo, sizeof (binfo), bid);

	reload_boards();
	update_postboards();

	move(4, 0);
	prints("\n���������Ѿ�ɾ��...\n");
	bbsinfo.utmpshm->syncbmonline = 1;
	pressreturn();
	clear();
	return 0;
}

void
offline()
{
	char buf[STRLEN];
	struct userdata currentdata;
	struct userec tmpu;
	loaduserdata(currentuser->userid, &currentdata);
	modify_user_mode(OFFLINE);
	clear();
	if (USERPERM(currentuser, PERM_SYSOP)
	    || USERPERM(currentuser, PERM_BOARDS)
	    || USERPERM(currentuser, PERM_ADMINMENU)
	    || USERPERM(currentuser, PERM_SEEULEVELS)) {
		move(1, 0);
		prints("\n\n������������, ���������ɱ��!!\n");
		pressreturn();
		clear();
		return;
	}
	if (currentuser->stay < 86400) {
		move(1, 0);

		prints("\n\n�Բ���, ����δ���ʸ�ִ�д�����!!\n");
		prints("ֻ����վʱ�䳬��24Сʱ���û�������ɱ.\n");
		pressreturn();
		clear();
		return;
	}

	getdata(1, 0, "�������������: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0'
	    || !checkpasswd(currentuser->passwd, currentuser->salt, buf)) {
		prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
		pressreturn();
		clear();
		return;
	}
	getdata(3, 0, "�������ʲô����? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentdata.realname)) {
		prints("\n\n�ܱ�Ǹ, �Ҳ�����ʶ�㡣\n");
		pressreturn();
		clear();
		return;
	}
	clear();
	move(1, 0);
	prints
	    ("\033[1;5;31m����\033[0;1;31m�� ��ɱ��, ������꽫���������������, Ը����Ϣ");
	prints("\n\n\n\033[1;32m���ѹ��.....\033[m\n\n\n");
	if (askyn("��ȷ��Ҫ�뿪������ͥ", NA, NA) == 1) {
		clear();
		memcpy(&tmpu, currentuser, sizeof (tmpu));
		tmpu.dieday = 7;
		updateuserec(&tmpu, usernum);
		Q_Goodbye();
		return;
	}
}

int
online()
{
	char buf[STRLEN];
	struct tm *nowtime;
	time_t nowtimeins;
	struct userdata currentdata;
	loaduserdata(currentuser->userid, &currentdata);
	modify_user_mode(OFFLINE);
	clear();
	nowtimeins = time(NULL);
	nowtime = localtime(&nowtimeins);
	if (currentuser->dieday != 1) {
		move(1, 0);
		prints("\n\n����δ����!!\n");
		prints("��Ҫ�����˰�!\n");
		prints("������ڻ��� %d ��", currentuser->dieday - 1);
		pressreturn();
		clear();
		return -1;
	}
	getdata(1, 0, "�������������: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0'
	    || !checkpasswd(currentuser->passwd, currentuser->salt, buf)) {
		prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
		pressreturn();
		clear();
		return -2;
	}
	getdata(3, 0, "�������ʲô����? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentdata.realname)) {
		prints("\n\n�ܱ�Ǹ, �Ҳ�����ʶ�㡣\n");
		pressreturn();
		clear();
		return -3;
	}
	clear();
	move(1, 0);
	prints
	    ("\033[1;5;31m����\033[0;1;31m�� ����,���ǻ���,�Ǹ�ֵ�ÿ��ǵ�����");
	prints("\n\n\n\033[1;32m����Ҫ�����.....\033[m\n\n\n");
	if (askyn("��ȷ��Ҫ����Ϊ������?", NA, NA) == 1) {
		struct userec tmpu;
		clear();
		memcpy(&tmpu, currentuser, sizeof (tmpu));
		tmpu.dieday = 0;
		updateuserec(&tmpu, usernum);
		return Q_Goodbye();
	}
	return 0;
}

int
d_user()
{
	int id;
	struct userec *lookupuser;
	char secu[STRLEN];
	char userid[IDLEN+1];

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	stand_title("ɾ��ʹ�����ʺ�");
	move(1, 0);
	usercomplete("��������ɾ����ʹ���ߴ���: ", userid);
	if (*userid == '\0') {
		clear();
		return 0;
	}
	move(1, 0);
	prints("ɾ��ʹ���� '%s'.", userid);
	clrtoeol();
	getdata(2, 0, "(Yes, or No) [N]: ", genbuf, 4, DOECHO, YEA);
	if (genbuf[0] != 'Y' && genbuf[0] != 'y') {	/* if not yes quit */
		move(2, 0);
		if (uinfo.mode != OFFLINE)
			prints("ȡ��ɾ��ʹ����...\n");
		pressreturn();
		clear();
		return 0;
	}
	if (!(id = getuser(userid, &lookupuser))) {
		move(3, 0);
		prints("�����ʹ���ߴ���...");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	if (lookupuser->userid[0] == '\0'
	    || !strcmp(lookupuser->userid, "SYSOP")) {
		prints("�޷�ɾ��!!\n");
		pressreturn();
		clear();
		return 0;
	}
	mcuZero(lookupuser->userid);
	kickoutuserec(lookupuser->userid);
	sprintf(secu, "ɾ��ʹ���ߣ�%s", lookupuser->userid);
	securityreport(secu, secu);
	move(2, 0);
	pressreturn();
	clear();
	return 1;
}

int
kick_user(userinfo)
struct user_info *userinfo;
{
	int id, ind;
	struct user_info uin;
	struct userec *lookupuser;
	char kickuser[40];

	if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE
	    && uinfo.mode != FRIEND) {
		modify_user_mode(ADMIN);
		stand_title("Kick User");
		move(1, 0);
		usercomplete("Enter userid to be kicked: ", kickuser);
		if (*kickuser == '\0') {
			clear();
			return 0;
		}
		if (!(id = getuser(kickuser, &lookupuser))) {
			move(3, 0);
			prints("Invalid User Id");
			clrtoeol();
			pressreturn();
			clear();
			return 0;
		}
		move(1, 0);
		prints("Kick User '%s'.", kickuser);
		clrtoeol();
		getdata(2, 0, "(Yes, or No) [N]: ", genbuf, 4, DOECHO, YEA);
		if (genbuf[0] != 'Y' && genbuf[0] != 'y') {	/* if not yes quit */
			move(2, 0);
			prints("Aborting Kick User\n");
			pressreturn();
			clear();
			return 0;
		}
		ind = search_ulist(&uin, t_cmpuids, id);
	} else {
		uin = *userinfo;
		strcpy(kickuser, uin.userid);
		ind = YEA;
	}

	if (uin.pid != 1
	    && (!ind || !uin.active || uin.pid <= 0
		|| (kill(uin.pid, 0) == -1))) {
		if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE
		    && uinfo.mode != FRIEND) {
			move(3, 0);
			prints("User Has Logged Out");
			clrtoeol();
			pressreturn();
			clear();
		}
		return 0;
	} else if (kill(uin.pid, SIGHUP) < 0) {
		prints("User can't be kicked");
		pressreturn();
		clear();
		return 1;
	}
	tracelog("%s kick %s", currentuser->userid, kickuser);
	move(2, 0);
	if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE
	    && uinfo.mode != FRIEND) {
		prints("User has been Kicked\n");
		pressreturn();
		clear();
	}
	return 1;
}
