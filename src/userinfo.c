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

#include "bbs.h"
#include "bbstelnet.h"

extern time_t login_start_time;
static void getfield(int line, char *info, char *desc, char *buf, int len,
		     int min);

void
permtostr(perm, str)
int perm;
char *str;
{
	int num;
	strcpy(str, "bTCPRp#@XWBA#VS-DOM-F012345678");
	for (num = 0; num < 30; num++)
		if (!(perm & (1 << num)))
			str[num] = '-';
	str[num] = '\0';
}

void
disply_userinfo(u, real)
struct userec *u;
int real;
{
	struct stat st;
	struct userdata data;
	int num, diff;
	int exp;
	struct in_addr in;

	move(real == 1 ? 2 : 3, 0);
	clrtobot();
	loaduserdata(u->userid, &data);
	prints("���Ĵ���     : %s\n", u->userid);
	prints("�����ǳ�     : %s\n", u->username);
	prints("��ʵ����     : %s\n", data.realname);
	prints("��ססַ     : %s\n", data.address);
	prints("�����ʼ����� : %s\n", data.email);
	if (real) {
		prints("��ʵ E-mail  : %s\n", data.realmail);
//        if USERPERM(currentuser, PERM_ADMINMENU)
//           prints("Ident ����   : %s\n", u->ident );
	}
	in.s_addr = u->ip;
	prints("����ָ��     : %s\n", (u->ip == 0) ? "" : inet_ntoa(in));
	prints("�ʺŽ������� : %s", ctime(&u->firstlogin));
	prints("����������� : %s", ctime(&u->lastlogin));
	if (real) {
		in.s_addr = u->lasthost;
		prints("������ٻ��� : %s\n", inet_ntoa(in));
	}
	prints("�ϴ���վʱ�� : %s",
	       u->lastlogout ? ctime(&u->lastlogout) : "����\n");
	prints("��վ����     : %d ��\n", u->numlogins);
	if (real) {
		prints("������Ŀ     : %d\n", u->numposts);
	}
	exp = countexp(u);
	prints("����ֵ       : %d(%s)\n", exp, cuserexp(exp));
	exp = countperf(u);
	prints("����ֵ       : %d(%s)\n", exp, cperf(exp));
	prints("��վ��ʱ��   : %d Сʱ %d ����\n", (int)(u->stay / 3600),
	       (int)((u->stay / 60) % 60));
	sprintf(genbuf, "mail/%c/%s/%s", mytoupper(u->userid[0]), u->userid,
		DOT_DIR);
	if (stat(genbuf, &st) >= 0)
		num = st.st_size / (sizeof (struct fileheader));
	else
		num = 0;
	prints("˽������     : %d ��\n", num);

	if (real) {
		strcpy(genbuf, "bTCPRp#@XWBA#VS-DOM-F012345678");
		for (num = 0; num < 30; num++)
			if (!(u->userlevel & (1 << num)))
				genbuf[num] = '-';
		genbuf[num] = '\0';
		permtostr(u->userlevel, genbuf);
		prints("ʹ����Ȩ��   : %s\n", genbuf);
	} else {
		diff = (time(0) - login_start_time) / 60;
		prints("ͣ���ڼ�     : %d Сʱ %02d ��\n", diff / 60,
		       diff % 60);
		prints("ӫĻ��С     : %dx%d\n", t_lines, t_columns);
	}
	prints("\n");
	if (u->userlevel & PERM_LOGINOK) {
		prints("  ����ע������Ѿ����, ��ӭ���뱾վ.\n");
	} else if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("  ������·, ���Ķ� Announce ������.\n");
	} else {
		prints("  ע����δ�ɹ�, ��ο���վ��վ����˵��.\n");
	}
}

int
uinfo_query(u, real)
struct userec *u;
int real;
{
	struct userec newurec;
	struct userdata data, newdata;
	char ans[3], buf[STRLEN], genbuf[128];
	char src[STRLEN], dst[STRLEN];
	char md5pass[16];
	int i, fail = 0, netty_check = 0;
	struct in_addr in;

	memcpy(&newurec, u, sizeof (struct userec));
	loaduserdata(u->userid, &data);
	memcpy(&newdata, &data, sizeof (struct userdata));
	getdata(t_lines - 1, 0,
		"��ѡ�� (0)���� (1)�޸����� (2)�趨���� ==> [0]", ans, 2,
		DOECHO, YEA);
	clear();

	i = 3;
	move(i++, 0);
	if (ans[0] != '3' || real)
		prints("ʹ���ߴ���: %s\n", u->userid);

	switch (ans[0]) {
	case '1':
		move(1, 0);
		prints("�������޸�,ֱ�Ӱ� <ENTER> ����ʹ�� [] �ڵ����ϡ�\n");

		sprintf(genbuf, "�ǳ� [%s]: ", u->username);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0])
			strncpy(newurec.username, buf, NAMELEN);
		if (!real && buf[0])
			strncpy(uinfo.username, buf, 40);

		sprintf(genbuf, "��ʵ���� [%s]: ", data.realname);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0])
			strsncpy(newdata.realname, buf, sizeof(newdata.realname));

		sprintf(genbuf, "��ס��ַ [%s]: ", data.address);
		getdata(i++, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0])
			strsncpy(newdata.address, buf, sizeof(newdata.address));

		sprintf(genbuf, "�������� [%s]: ", data.email);
		getdata(i++, 0, genbuf, buf, STRLEN - 10, DOECHO, YEA);
		if (buf[0]) {
			strsncpy(newdata.email, buf, sizeof(newdata.email));
		}
		in.s_addr = u->ip;
		sprintf(genbuf, "����ָ�� [%s]: ",
			(u->ip == 0) ? "" : inet_ntoa(in));
		getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
		if (buf[0]) {
			if (inet_aton(buf, &in))
				newurec.ip = in.s_addr;
		}
		if (real) {
			sprintf(genbuf, "��ʵEmail[%s]: ", data.realmail);
			getdata(i++, 0, genbuf, buf, 60, DOECHO, YEA);
			if (buf[0])
				strsncpy(newdata.realmail, buf, sizeof(newdata.realmail));

			sprintf(genbuf, "���ߴ��� [%d]: ", u->numlogins);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newurec.numlogins = atoi(buf);

			sprintf(genbuf, "������Ŀ [%d]: ", u->numposts);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newurec.numposts = atoi(buf);

			sprintf(genbuf, "��վСʱ�� [%ld Сʱ %ld ����]: ",
				(long int) (u->stay / 3600),
				(long int) ((u->stay / 60) % 60));
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newurec.stay = atoi(buf) * 3600;

		}

		break;
	case '2':
		if (!real) {
			getdata(i++, 0, "������ԭ����: ", buf, PASSLEN, NOECHO,
				YEA);
			if (*buf == '\0'
			    || !checkpasswd(u->passwd, u->salt, buf)) {
				prints("\n\n�ܱ�Ǹ, ����������벻��ȷ��\n");
				fail++;
				break;
			}
		}
		getdata(i++, 0, "���趨������: ", buf, PASSLEN, NOECHO, YEA);
		if (buf[0] == '\0') {
			prints("\n\n�����趨ȡ��, ����ʹ�þ�����\n");
			fail++;
			break;
		}
		strsncpy(genbuf, buf, sizeof(genbuf));

		getdata(i++, 0, "����������������: ", buf, PASSLEN, NOECHO,
			YEA);
		if (strncmp(buf, genbuf, PASSLEN)) {
			prints("\n\n������ȷ��ʧ��, �޷��趨�����롣\n");
			fail++;
			break;
		}
		newurec.salt = getsalt_md5();
		genpasswd(md5pass, newurec.salt, buf);
		memcpy(newurec.passwd, md5pass, 16);
		break;
	default:
		clear();
		return 0;
	}
	if (fail != 0) {
		pressreturn();
		clear();
		return 0;
	}
	if (askyn("ȷ��Ҫ�ı���", NA, YEA) == YEA) {
		if (real) {
			char secu[STRLEN];
			sprintf(secu, "�޸� %s �Ļ������ϻ����롣", u->userid);
			securityreport(secu, secu);
		}
#if 0
/* added by netty to automatically send a mail to new user. */
		if ((netty_check == 1)) {
			sprintf(genbuf, "%s", email_domain());
			if (!strstr(newdata.email, genbuf) &&
			    !invalidaddr(newdata.email) &&
			    !invalid_email(newdata.email) &&
			    trustEmail(udata.email)) {
				send_emailcheck(u, &newdata);
			} else {
				if (sysconf_str("EMAILFILE") != NULL) {
					move(t_lines - 5, 0);
					prints
					    ("\n������ĵ����ʼ���ַ ��\033[1;33m%s\033[m��\n",
					     newdata.email);
					prints
					    ("���ǺϷ�֮ UNIX �ʺţ�ϵͳ����Ͷ��ע���ţ������������...\n");
					pressanykey();
				}
			}
		}
		if (netty_check == 1) {
			newurec.userlevel &= ~(PERM_LOGINOK | PERM_PAGE);
			sethomefile(src, newurec.userid, "register");
			sethomefile(dst, newurec.userid, "register.old");
			rename(src, dst);
		}
#endif
		updateuserec(&newurec, 0);
		saveuserdata(newurec.userid, &newdata);
	}
	clear();
	return 0;
}

void
x_info()
{
	modify_user_mode(GMENU);
	if (!strcmp("guest", currentuser->userid)) {
		disply_userinfo(currentuser, 0);
		pressreturn();
		return;
	}
	disply_userinfo(currentuser, 1);
	uinfo_query(currentuser, 0);
}

static void
getfield(line, info, desc, buf, len, min)
int line, len, min;
char *info, *desc, *buf;
{
	char prompt[STRLEN];

	sprintf(genbuf, "  ԭ���趨: %-46.46s \033[1;32m(%s)\033[m",
		(buf[0] == '\0') ? "(δ�趨)" : buf, info);
	move(line, 0);
	prints("%s", genbuf);
	if (min)
		sprintf(prompt, "  %s(����%d������): ", desc, (min + 1) / 2);
	else
		sprintf(prompt, "  %s(��ѡ): ", desc);
	do {
		getdata(line + 1, 0, prompt, genbuf, len, DOECHO, YEA);
		if (genbuf[0] == 0)
			strcpy(genbuf, buf);
	} while (strlen(genbuf) < min);
	strncpy(buf, genbuf, len);
	move(line, 0);
	clrtoeol();
	prints("  %s: %s\n", desc, buf);
	clrtoeol();
}

void
x_fillform()
{
	char rname[NAMELEN], addr[STRLEN];
	char phone[STRLEN], dept[STRLEN], assoc[STRLEN];
	char ans[5], *mesg;
	FILE *fn;
	time_t now;
	int lockfd;
	struct userdata currentdata;

	modify_user_mode(NEW);
	move(3, 0);
	clrtobot();
	if (!strcmp("guest", currentuser->userid)) {
		prints("��Ǹ, ���� new ����һ�����ʺź����������.");
		pressreturn();
		return;
	}
	if (currentuser->userlevel & PERM_LOGINOK) {
		prints("���Ѿ���ɱ�վ��ʹ����ע������, ��ӭ���뱾վ������.");
		pressreturn();
		return;
	}

	if(has_fill_form(currentuser->userid))
	{
		prints("վ����δ��������ע�����뵥, �����ĵȺ�.");
		pressreturn();
		return;
	}
	move(3, 0);
	sprintf(genbuf, "��Ҫ��дע�ᵥ������%s���ͥ�𣿣�email ע�᲻����дע�ᵥ��", MY_BBS_NAME);
	if (askyn(genbuf, YEA, NA) == NA)
		return;
	loaduserdata(currentuser->userid, &currentdata);
	strncpy(rname, currentdata.realname, NAMELEN);
	strncpy(addr, currentdata.address, STRLEN);
	dept[0] = phone[0] = assoc[0] = '\0';
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s ����, ���ʵ��д���µ�����:\n", currentuser->userid);
		getfield(6, "��������", "��ʵ����", rname, NAMELEN, 4);
		getfield(8, "ѧУϵ����˾����", "ѧУϵ��������λ", dept,
			 STRLEN, 14);
		getfield(10, "�������һ����ƺ���", "Ŀǰ��ϸסַ", addr,
			 STRLEN, 16);
		getfield(12, "����������ʱ��", "����绰", phone, STRLEN, 0);
		getfield(14, "У�ѻ���ҵѧУ", "У �� ��", assoc, STRLEN, 0);
/* only for 9#        getfield( 14, "������ID����ʵ����",    "������(������ID����:ID/����)",   assoc, STRLEN );
*/
		mesg = "���������Ƿ���ȷ, �� Q ����ע�� (Y/N/Quit)? [N]: ";
		getdata(t_lines - 1, 0, mesg, ans, 3, DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return;
		if (ans[0] == 'Y' || ans[0] == 'y')
			break;
	}
	strncpy(currentdata.realname, rname, sizeof (currentdata.address));
	strncpy(currentdata.address, addr, sizeof (currentdata.address));
	lockfd = openlockfile(".lock_new_register", O_RDONLY, LOCK_EX);
	if ((fn = fopen("new_register", "a")) != NULL) {
		now = time(NULL);
		fprintf(fn, "usernum: %d, %s", usernum, ctime(&now));
		fprintf(fn, "userid: %s\n", currentuser->userid);
		fprintf(fn, "realname: %s\n", rname);
		fprintf(fn, "dept: %s\n", dept);
		fprintf(fn, "addr: %s\n", addr);
		fprintf(fn, "phone: %s\n", phone);
		fprintf(fn, "assoc: %s\n", assoc);
		fprintf(fn, "rereg: 1\n");
		fprintf(fn, "----\n");
		fclose(fn);
	}
	close(lockfd);
	setuserfile(genbuf, "mailcheck");
	if ((fn = fopen(genbuf, "w")) == NULL) {
		fclose(fn);
		return;
	}
	fprintf(fn, "usernum: %d\n", usernum);
	fclose(fn);
}
