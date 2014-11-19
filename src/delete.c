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
	stand_title("删除讨论区");
	make_blist_full();
	move(1, 0);
	namecomplete("请输入讨论区: ", bname);
	FreeNameList();
	if (bname[0] == '\0')
		return -1;
	bid = getbnum(bname);
	if (get_record(BOARDS, &binfo, sizeof (binfo), bid) == -1) {
		move(2, 0);
		prints("不正确的讨论区\n");
		pressreturn();
		clear();
		return -1;
	}
	ans = askyn("你确定要删除这个讨论区", NA, NA);
	if (ans != 1) {
		move(2, 0);
		prints("取消删除行动\n");
		pressreturn();
		clear();
		return -1;
	}
	{
		char secu[STRLEN];
		sprintf(secu, "删除讨论区：%s", binfo.filename);
		securityreport(secu, secu);
	}
	if (seek_in_file("0Announce/.Search", bname)) {
		move(4, 0);
		if (askyn("移除精华区", NA, NA) == YEA) {
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
	sprintf(genbuf, " << '%s'被 %s 删除 >>",
		binfo.filename, currentuser->userid);
	memset(&binfo, 0, sizeof (binfo));
	strsncpy(binfo.title, genbuf, sizeof (binfo.title));
	binfo.level = PERM_SYSOP;
	substitute_record(BOARDS, &binfo, sizeof (binfo), bid);

	reload_boards();
	update_postboards();

	move(4, 0);
	prints("\n本讨论区已经删除...\n");
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
		prints("\n\n您有重任在身, 不能随便自杀啦!!\n");
		pressreturn();
		clear();
		return;
	}
	if (currentuser->stay < 86400) {
		move(1, 0);

		prints("\n\n对不起, 您还未够资格执行此命令!!\n");
		prints("只有上站时间超过24小时的用户才能自杀.\n");
		pressreturn();
		clear();
		return;
	}

	getdata(1, 0, "请输入你的密码: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0'
	    || !checkpasswd(currentuser->passwd, currentuser->salt, buf)) {
		prints("\n\n很抱歉, 您输入的密码不正确。\n");
		pressreturn();
		clear();
		return;
	}
	getdata(3, 0, "请问你叫什么名字? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentdata.realname)) {
		prints("\n\n很抱歉, 我并不认识你。\n");
		pressreturn();
		clear();
		return;
	}
	clear();
	move(1, 0);
	prints
	    ("\033[1;5;31m警告\033[0;1;31m： 自杀后, 您的灵魂将升入天国或堕入地狱, 愿您安息");
	prints("\n\n\n\033[1;32m好难过喔.....\033[m\n\n\n");
	if (askyn("你确定要离开这个大家庭", NA, NA) == 1) {
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
		prints("\n\n死期未满啊!!\n");
		prints("你要吓死人啊!\n");
		prints("你的死期还有 %d 天", currentuser->dieday - 1);
		pressreturn();
		clear();
		return -1;
	}
	getdata(1, 0, "请输入你的密码: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0'
	    || !checkpasswd(currentuser->passwd, currentuser->salt, buf)) {
		prints("\n\n很抱歉, 您输入的密码不正确。\n");
		pressreturn();
		clear();
		return -2;
	}
	getdata(3, 0, "请问你叫什么名字? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentdata.realname)) {
		prints("\n\n很抱歉, 我并不认识你。\n");
		pressreturn();
		clear();
		return -3;
	}
	clear();
	move(1, 0);
	prints
	    ("\033[1;5;31m警告\033[0;1;31m： 生存,还是毁灭,是个值得考虑的问题");
	prints("\n\n\n\033[1;32m您可要想清楚.....\033[m\n\n\n");
	if (askyn("你确定要返世为人了吗?", NA, NA) == 1) {
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
	stand_title("删除使用者帐号");
	move(1, 0);
	usercomplete("请输入欲删除的使用者代号: ", userid);
	if (*userid == '\0') {
		clear();
		return 0;
	}
	move(1, 0);
	prints("删除使用者 '%s'.", userid);
	clrtoeol();
	getdata(2, 0, "(Yes, or No) [N]: ", genbuf, 4, DOECHO, YEA);
	if (genbuf[0] != 'Y' && genbuf[0] != 'y') {	/* if not yes quit */
		move(2, 0);
		if (uinfo.mode != OFFLINE)
			prints("取消删除使用者...\n");
		pressreturn();
		clear();
		return 0;
	}
	if (!(id = getuser(userid, &lookupuser))) {
		move(3, 0);
		prints("错误的使用者代号...");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	if (lookupuser->userid[0] == '\0'
	    || !strcmp(lookupuser->userid, "SYSOP")) {
		prints("无法删除!!\n");
		pressreturn();
		clear();
		return 0;
	}
	mcuZero(lookupuser->userid);
	kickoutuserec(lookupuser->userid);
	sprintf(secu, "删除使用者：%s", lookupuser->userid);
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
