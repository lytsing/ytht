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
#define EXTERN
#include "bbs.h"
#include "bbstelnet.h"
#include "edit.h"
#include <sys/mman.h>

pid_t childpid;
static int loadkeys(struct one_key *key, char *name);
static int savekeys(struct one_key *key, char *name);
static void keyprint(char *buf, int key);
static int showkeyinfo(struct one_key *akey, int i);
static unsigned int setkeys(struct one_key *key);
static void myexec_cmd(int umode, int pager, const char *cmdfile,
		       const char *param);
static void datapipefd(int fds, int fdn);
static void childreturn(int i);
static void escape_filename(char *fn);
static void bbs_zsendfile(char *filename);

static int
loadkeys(struct one_key *key, char *name)
{
	int i;
	FILE *fp;
	fp = fopen(name, "r");
	if (fp == NULL)
		return 0;
	i = 0;
	while (key[i].fptr != NULL) {
		fread(&(key[i].key), sizeof (int), 1, fp);
		i++;
	}
	fclose(fp);
	return 1;
}

static int
savekeys(struct one_key *key, char *name)
{
	int i;
	FILE *fp;
	fp = fopen(name, "w");
	if (fp == NULL)
		return 0;
	i = 0;
	while (key[i].fptr != NULL) {
		fwrite(&(key[i].key), sizeof (int), 1, fp);
		i++;
	}
	fclose(fp);
	return 1;
}

void
loaduserkeys()
{
	char tempname[STRLEN];
	setuserfile(tempname, "readkey");
	loadkeys(read_comms, tempname);
	setuserfile(tempname, "mailkey");
	loadkeys(mail_comms, tempname);
	setuserfile(tempname, "friendkey");
	loadkeys(friend_list, tempname);
	setuserfile(tempname, "rejectkey");
	loadkeys(reject_list, tempname);
}

int
modify_user_mode(mode)
int mode;
{
	if (uinfo.mode == mode)
		return 0;
	uinfo.mode = mode;
	update_ulist(&uinfo, utmpent);
	return 0;
}

int
showperminfo(pbits, i, use_define)
unsigned int pbits;
int i, use_define;
{
	char buf[STRLEN];

	sprintf(buf, "%c. %-30s %3s", 'A' + i,
		(use_define) ? user_definestr[i] : permstrings[i],
		((pbits >> i) & 1 ? "ON" : "OFF"));
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints(buf);
	return YEA;
}

static void
keyprint(char *buf, int key)
{
	if (isprint(key))
		sprintf(buf, "%c", key);
	else {
		switch (key) {
		case KEY_TAB:
			strcpy(buf, "TAB");
			break;
		case KEY_ESC:
			strcpy(buf, "ESC");
			break;
		case KEY_UP:
			strcpy(buf, "UP");
			break;
		case KEY_DOWN:
			strcpy(buf, "DOWN");
			break;
		case KEY_RIGHT:
			strcpy(buf, "RIGHT");
			break;
		case KEY_LEFT:
			strcpy(buf, "LEFT");
			break;
		case KEY_HOME:
			strcpy(buf, "HOME");
			break;
		case KEY_INS:
			strcpy(buf, "INS");
			break;
		case KEY_DEL:
			strcpy(buf, "DEL");
			break;
		case KEY_END:
			strcpy(buf, "END");
			break;
		case KEY_PGUP:
			strcpy(buf, "PGUP");
			break;
		case KEY_PGDN:
			strcpy(buf, "PGDN");
			break;
		default:
			if (isprint(key | 0140))
				sprintf(buf, "Ctrl+%c", key | 0140);
			else
				sprintf(buf, "%x", key);
		}
	}
}

static int
showkeyinfo(struct one_key *akey, int i)
{
	char buf[STRLEN];
	char buf2[15];
	keyprint(buf2, (akey + i)->key);
	sprintf(buf, "%c. %-26s %6s", '0' + i, (akey + i)->func, buf2);
	move(i + 2 - ((i > 19) ? 20 : 0), 0 + ((i > 19) ? 40 : 0));
	prints(buf);
	return YEA;
}

static unsigned int
setkeys(struct one_key *key)
{
	int i, j, done = NA;
	char choice[3];
	prints("请按下你要的代码来设定键盘，按 Enter 结束.\n");
	i = 0;
	while (key[i].fptr != NULL && i < 40) {
		showkeyinfo(key, i);
		i++;
	}
	while (!done) {
		getdata(t_lines - 1, 0, "选择(ENTER 结束): ", choice, 2, DOECHO,
			YEA);
		*choice = toupper(*choice);
		if (*choice == '\n' || *choice == '\0')
			done = YEA;
		else if (*choice < '0' || *choice > '0' + i - 1)
			bell();
		else {
			j = *choice - '0';
			move(t_lines - 1, 0);
			prints("请定义[\033[35m%s\033[m]的功能键:",
			       key[j].func);
			key[j].key = igetkey();
			showkeyinfo(key, j);
			/* d pbits ^= (1 << i);
			   if((*showfunc)( pbits, i ,YEA)==NA)
			   {
			   pbits ^= (1 << i);
			   } */
		}
	}
	pressreturn();
	return 0;
}

int
x_copykeys()
{
	int i, toc = 0;
	char choice[3];
	FILE *fp;
	char buf[STRLEN];
	char tempname[STRLEN];
	modify_user_mode(USERDEF);
	clear();
	move(0, 0);
	clrtoeol();
	prints("请按下你要的代码来设定键盘类型,按 Enter 结束.\n");
	fp = fopen(MY_BBS_HOME "/etc/keys", "r");
	if (fp == NULL)
		return -1;
	i = 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		sprintf(tempname, "%c. %-26s", 'A' + i, buf);
		move(i + 2 - ((i > 19) ? 20 : 0), 0 + ((i > 19) ? 40 : 0));
		prints(tempname);
		i++;
	}
	getdata(t_lines - 1, 0, "选择(ENTER 结束): ", choice, 2, DOECHO, YEA);
	*choice = toupper(*choice);
	if (*choice == '\n' || *choice == '\0') {
		fclose(fp);
		return 0;
	} else if (*choice < 'A' || *choice > 'A' + i - 1)
		bell();
	else {
		toc = 0;
		i = *choice - 'A';
		if (currentuser->userlevel & PERM_SYSOP) {
			getdata(t_lines - 1, 0,
				"是否把当前你自己的键盘定义设置为系统定义(Y/N):N ",
				choice, 2, DOECHO, YEA);
			if (*choice == 'Y' || *choice == 'y')
				toc = 1;
		}
		if (toc) {
			sprintf(buf, MY_BBS_HOME "/etc/readkey.%d", i);
			savekeys(&read_comms[0], buf);
			sprintf(buf, MY_BBS_HOME "/etc/mailkey.%d", i);
			savekeys(&mail_comms[0], buf);
			sprintf(buf, MY_BBS_HOME "/etc/friendkey.%d", i);
			savekeys(&friend_list[0], buf);
			sprintf(buf, MY_BBS_HOME "/etc/rejectkey.%d", i);
			savekeys(&reject_list[0], buf);
		} else {
			sprintf(buf, MY_BBS_HOME "/etc/readkey.%d", i);
			loadkeys(&read_comms[0], buf);
			setuserfile(buf, "readkey");
			savekeys(&read_comms[0], buf);
			sprintf(buf, MY_BBS_HOME "/etc/mailkey.%d", i);
			loadkeys(&mail_comms[0], buf);
			setuserfile(buf, "mailkey");
			savekeys(&mail_comms[0], buf);
			sprintf(buf, MY_BBS_HOME "/etc/friendkey.%d", i);
			loadkeys(&friend_list[0], buf);
			setuserfile(buf, "friendkey");
			savekeys(&friend_list[0], buf);
			sprintf(buf, MY_BBS_HOME "/etc/rejectkey.%d", i);
			loadkeys(&reject_list[0], buf);
			setuserfile(buf, "rejectkey");
			savekeys(&reject_list[0], buf);
		}
		move(t_lines - 1, 0);
		prints("设置完毕");
	}
	pressreturn();
	return 0;
}

int
x_setkeys()
{
	char tempname[STRLEN];
	modify_user_mode(USERDEF);
	clear();
	move(0, 0);
	clrtoeol();
	setkeys(&read_comms[0]);
	setuserfile(tempname, "readkey");
	savekeys(&read_comms[0], tempname);
	return 0;
}

int
x_setkeys2()
{
	char tempname[STRLEN];
	modify_user_mode(USERDEF);
	clear();
	move(0, 0);
	clrtoeol();
	setkeys(&read_comms[40]);
	setuserfile(tempname, "readkey");
	savekeys(&read_comms[0], tempname);
	return 0;
}

int
x_setkeys3()
{
	char tempname[STRLEN];
	modify_user_mode(USERDEF);
	clear();
	move(0, 0);
	clrtoeol();
	setkeys(&mail_comms[0]);
	setuserfile(tempname, "mailkey");
	savekeys(&mail_comms[0], tempname);
	return 0;
}

int
x_setkeys4()
{
	char tempname[STRLEN];
	modify_user_mode(USERDEF);
	clear();
	move(0, 0);
	clrtoeol();
	setkeys(&friend_list[0]);
	setuserfile(tempname, "friendkey");
	savekeys(&friend_list[0], tempname);
	return 0;
}

int
x_setkeys5()
{
	char tempname[STRLEN];
	modify_user_mode(USERDEF);
	clear();
	move(0, 0);
	clrtoeol();
	setkeys(&reject_list[0]);
	setuserfile(tempname, "rejectdkey");
	savekeys(&reject_list[0], tempname);
	return 0;
}

unsigned int
setperms(pbits, prompt, numbers, showfunc, param)
unsigned int pbits;
char *prompt;
int numbers;
int (*showfunc) (unsigned int, int, int);
int param;
{
	int lastperm = numbers - 1;
	int i, done = NA;
	char choice[3];

	move(4, 0);
	prints("请按下你要的代码来设定%s，按 Enter 结束.\n", prompt);
	move(6, 0);
	clrtobot();
/*    pbits &= (1 << numbers) - 1;*/
	for (i = 0; i <= lastperm; i++) {
		(*showfunc) (pbits, i, param);
	}
	while (!done) {
		getdata(t_lines - 1, 0, "选择(ENTER 结束): ", choice, 2, DOECHO,
			YEA);
		*choice = toupper(*choice);
		if (*choice == '\n' || *choice == '\0')
			done = YEA;
		else if (*choice < 'A' || *choice > 'A' + lastperm)
			bell();
		else {
			i = *choice - 'A';
			pbits ^= (1 << i);
			if ((*showfunc) (pbits, i, param) == NA) {
				pbits ^= (1 << i);
			}
		}
	}
	return (pbits);
}

int
x_level()
{
	int id, oldlevel;
	unsigned int newlevel;
	char content[1024];
	struct userec tmpu;
	struct userec *lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	move(0, 0);
	prints("Change User Priority\n");
	clrtoeol();
	move(1, 0);
	usercomplete("Enter userid to be changed: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf, &lookupuser))) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return -1;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("Set the desired permissions for user '%s'\n", genbuf);
	newlevel =
	    setperms(lookupuser->userlevel, "权限", NUMPERMS, showperminfo, 0);
	move(2, 0);
	if (newlevel == lookupuser->userlevel)
		prints("User '%s' level NOT changed\n", lookupuser->userid);
	else {
		oldlevel = lookupuser->userlevel;
		memcpy(&tmpu, lookupuser, sizeof (tmpu));
		tmpu.userlevel = newlevel;
		{
			char secu[STRLEN];
			sprintf(secu, "修改 %s 的权限", lookupuser->userid);
			permtostr(oldlevel, genbuf);
			sprintf(content, "修改前的权限：%s\n修改后的权限：",
				genbuf);
			permtostr(tmpu.userlevel, genbuf);
			strcat(content, genbuf);
			securityreport(secu, content);
		}
		updateuserec(&tmpu, 0);
		prints("User '%s' level changed\n", lookupuser->userid);
	}
	pressreturn();
	clear();
	return 0;
}

int
x_userdefine()
{
	unsigned int newlevel;
	struct userec tmpu;
	extern int nettyNN;

	modify_user_mode(USERDEF);
	move(1, 0);
	clrtobot();
	move(2, 0);
	newlevel =
	    setperms(currentuser->userdefine, "参数", NUMDEFINES, showperminfo,
		     1);
	move(2, 0);
	if (newlevel == currentuser->userdefine)
		prints("参数没有修改...\n");
	else {
		memcpy(&tmpu, currentuser, sizeof (tmpu));
		tmpu.userdefine = newlevel;
		if ((!convcode && !(newlevel & DEF_USEGB))
		    || (convcode && (newlevel & DEF_USEGB)))
			switch_code();
		updateuserec(&tmpu, usernum);
		uinfo.pager |= FRIEND_PAGER;
		if (!(uinfo.pager & ALL_PAGER)) {
			if (!USERDEFINE(currentuser, DEF_FRIENDCALL))
				uinfo.pager &= ~FRIEND_PAGER;
		}
		uinfo.pager &= ~ALLMSG_PAGER;
		uinfo.pager &= ~FRIENDMSG_PAGER;
		if (USERDEFINE(currentuser, DEF_DELDBLCHAR))
			enabledbchar = 1;
		else
			enabledbchar = 0;
		if (USERDEFINE(currentuser, DEF_FRIENDMSG)) {
			uinfo.pager |= FRIENDMSG_PAGER;
		}
		if (USERDEFINE(currentuser, DEF_ALLMSG)) {
			uinfo.pager |= ALLMSG_PAGER;
			uinfo.pager |= FRIENDMSG_PAGER;
		}
		update_utmp();
		if (USERDEFINE(currentuser, DEF_ACBOARD))
			nettyNN = NNread_init();
		prints("新的参数设定完成...\n\n");
	}
	iscolor = (USERDEFINE(currentuser, DEF_COLOR)) ? 1 : 0;
	pressreturn();
	clear();
	return 0;
}

int
x_cloak()
{
	modify_user_mode(GMENU);
	uinfo.invisible = (uinfo.invisible) ? NA : YEA;
	update_utmp();
	if ((currentuser->userlevel & PERM_BOARDS))
		setbmstatus(1);
	if (!uinfo.in_chat) {
		move(1, 0);
		clrtoeol();
		prints("隐身术 (cloak) 已经%s了!",
		       (uinfo.invisible) ? "启动" : "停止");
		pressreturn();
	}
	return 0;
}

void
x_edits()
{
	short int retv;
	char ans[7], buf[STRLEN];
	char editfile[STRLEN];
	int ch, num, confirm;
	static char *const e_file[] =
	    { "plans", "signatures", "notes", "logout",
		"bansite", "bbsnet",
		NULL
	};
	static char *const explain_file[] =
	    { "个人说明档", "签名档", "自己的备忘录", "离站的画面",
		"禁止本ID登录的IP(每个IP占一行)",
		"自定义穿梭（格式：\033[1;32m地区 BBS名称 IP PORT\033[0m，每个 BBS 一行）",
		NULL
	};

	modify_user_mode(GMENU);
	clear();
	move(1, 0);
	prints("编修个人档案\n\n");
	for (num = 0; e_file[num] != NULL && explain_file[num] != NULL; num++) {
		prints("[\033[1;32m%d\033[m] %s\n", num + 1, explain_file[num]);
	}
	prints("[\033[1;32m%d\033[m] 都不想改\n", num + 1);

	getdata(num + 5, 0, "你要编修哪一项个人档案: ", ans, 2, DOECHO, YEA);
	if (ans[0] - '0' <= 0 || ans[0] - '0' > num || ans[0] == '\n'
	    || ans[0] == '\0')
		return;

	ch = ans[0] - '0' - 1;
	setuserfile(editfile, e_file[ch]);
	move(3, 0);
	clrtobot();
	prints("%s", explain_file[ch]);
	getdata(4, 0, "(E)编辑 (D)删除? [E]: ", ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		confirm = askyn("你确定要删除这个档案", NA, NA);
		if (confirm != 1) {
			move(6, 0);
			prints("取消删除行动\n");
			pressreturn();
			clear();
			return;
		}
		unlink(editfile);
		move(6, 0);
		prints("%s 已删除\n", explain_file[ch]);
		pressreturn();
		clear();
		return;
	}
	modify_user_mode(EDITUFILE);
	retv = vedit(editfile, NA, YEA);
	clear();
	if (retv != REAL_ABORT) {
		prints("%s 更新过\n", explain_file[ch]);
		sprintf(buf, "edit %s", explain_file[ch]);
		if (!strcmp(e_file[ch], "signatures")) {
			set_numofsig();
			prints("系统重新设定以及读入你的签名档...");
		}
	} else
		prints("%s 取消修改\n", explain_file[ch]);
	pressreturn();
}

void
a_edits()
{
	short int retv;
	char ans[7], buf[STRLEN], buf2[STRLEN];
	int ch, num, confirm;
	static char *const e_file[] =
	    { "Welcome", "Welcome2", "issue", "logout", "movie",
		"endline", "../vote/notes",
		"menu.ini", "badname0", "badname", "../.bad_email",
		"../.bansite",
		"../.blockmail",
		"autopost", "junkboards", "untrust",
		"bbsnetA.ini", "bbsnet.ini", "filtertitle",
		"../ftphome/ftp_adm", "badwords", "sbadwords", "pbadwords",
		"../inndlog/newsfeeds.bbs", "spec_site", "secmlist", "bmstand",
		"bmhelp", "s_fill2", "autocross",
		NULL
	};
	static char *const explain_file[] =
	    { "特殊进站公布栏", "进站画面", "进站欢迎档", "离站画面",
		"活动看版", "屏幕底线", "公用备忘录", "menu.ini",
		"不可注册的 ID", "ID 中不能包含的字串", "不可确认之E-Mail",
		"不可上站之位址",
		"拒收E-mail黑名单", "每日自动送信档", "不算POST数的版",
		"不信任IP列表",
		"网络连线A", "网络连线(未使用)", "系统需要过滤的标题",
		"FTP管理员名单", "过滤词汇", "精简过滤词汇", "报警词汇",
		"cn.bbs转信版和新闻组对应", "穿梭IP限制次数", "区长名单",
		"版务考勤标准", "版务上任信件", "注册用户欢迎信件",
		"自动转载文章规则", NULL
	};

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return;
	}
	clear();
	move(0, 0);
	prints("编修系统档案\n\n");
	for (num = 0;
	     USERPERM(currentuser, PERM_SYSOP) ? e_file[num] != NULL
	     && explain_file[num] != NULL : explain_file[num] != "menu.ini";
	     num++) {
		if (num >= 20)
			move(num - 20 + 2, 39);
		prints("[%2d] %s\n", num + 1, explain_file[num]);
	}
	if (num >= 20)
		move(num - 20 + 2, 39);
	prints("[%2d] 都不想改\n", num + 1);

	getdata(t_lines - 1, 0, "你要编修哪一项系统档案: ", ans, 3, DOECHO,
		YEA);
	ch = atoi(ans);
	if (!isdigit(ans[0]) || ch <= 0 || ch > num || ans[0] == '\n'
	    || ans[0] == '\0')
		return;
	ch -= 1;
	sprintf(buf2, "etc/%s", e_file[ch]);
	move(3, 0);
	clrtobot();
	sprintf(buf, "(E)编辑 (D)删除 %s? [E]: ", explain_file[ch]);
	getdata(3, 0, buf, ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		confirm = askyn("你确定要删除这个系统档", NA, NA);
		if (confirm != 1) {
			move(5, 0);
			prints("取消删除行动\n");
			pressreturn();
			clear();
			return;
		}
		{
			char secu[STRLEN];
			sprintf(secu, "删除系统档案：%s", explain_file[ch]);
			securityreport(secu, secu);
		}
		unlink(buf2);
		move(5, 0);
		prints("%s 已删除\n", explain_file[ch]);
		pressreturn();
		clear();
		return;
	}
	modify_user_mode(EDITSFILE);
	retv = vedit(buf2, NA, YEA);
	clear();
	if (retv != REAL_ABORT) {
		prints("%s 更新过", explain_file[ch]);
		{
			char secu[STRLEN];
			sprintf(secu, "修改系统档案：%s", explain_file[ch]);
			securityreport(secu, secu);
		}
	}
	pressreturn();
}

void
x_lockscreen()
{
	char buf[PASSLEN + 1];
	time_t now;

	modify_user_mode(LOCKSCREEN);
	block_msg();
	move(9, 0);
	clrtobot();
	update_endline();
	buf[0] = '\0';
	now = time(0);
	move(9, 0);
	prints
	    ("\n\033[1;37m       _       _____   ___     _   _   ___     ___       __"
	     "\n      ( )     (  _  ) (  _`\\  ( ) ( ) (  _`\\  (  _`\\    |  |"
	     "\n      | |     | ( ) | | ( (_) | |/'/' | (_(_) | | ) |   |  |"
	     "\n      | |  _  | | | | | |  _  | , <   |  _)_  | | | )   |  |"
	     "\n      | |_( ) | (_) | | (_( ) | |\\`\\  | (_( ) | |_) |   |==|"
	     "\n      (____/' (_____) (____/' (_) (_) (____/' (____/'   |__|\033[m\n"
	     "\n\033[1;36m荧幕已在\033[33m %19s\033[36m 时被\033[32m %-12s \033[36m暂时锁住了...\033[m",
	     ctime(&now), currentuser->userid);
	while (*buf == '\0'
	       || !checkpasswd(currentuser->passwd, currentuser->salt, buf)) {
		move(18, 0);
		clrtobot();
		update_endline();
		getdata(19, 0, buf[0] == '\0' ? "请输入你的密码以解锁: " :
			"你输入的密码有误，请重新输入你的密码以解锁: ", buf,
			PASSLEN, NOECHO, YEA);
	}
	unblock_msg();
}

int
heavyload(float maxload)
{
	double cpu_load[3];
	if (maxload == 0)
		maxload = 15;
	get_load(cpu_load);
	if (cpu_load[0] > maxload)
		return 1;
	else
		return 0;
}

static void
myexec_cmd(umode, pager, cmdfile, param)
int umode, pager;
const char *cmdfile, *param;
{
	char param1[256];
	int save_pager;
	pid_t childpid;
	int p[2];
	param1[0] = 0;
	if (param != NULL) {
		char *avoid = "&;!`'\"|?~<>^()[]{}$\n\r\\", *ptr;
		int n = strlen(avoid);
		strsncpy(param1, param, sizeof (param1));
		while (n > 0) {
			n--;
			ptr = strchr(param1, avoid[n]);
			if (ptr != NULL)
				*ptr = 0;
		}
	}

	if (!USERPERM(currentuser, PERM_SYSOP) && heavyload(0)) {
		clear();
		prints("抱歉，目前系统负荷过重，此功能暂时不能执行...");
		pressanykey();
		return;
	}

	if (!file_isfile(cmdfile)) {
		move(2, 0);
		prints("no %s\n", cmdfile);
		pressreturn();
		return;
	}

	save_pager = uinfo.pager;
	if (pager == NA) {
		uinfo.pager = 0;
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, p) < 0)
		return;

	modify_user_mode(umode);
	refresh();
	signal(SIGALRM, SIG_IGN);
	signal(SIGCHLD, SIG_DFL);
	childpid = fork();
	if (childpid == 0) {
		char pidstr[20];
		sprintf(pidstr, "%d", getppid());
		close(p[0]);
		if (p[1] != 0)
			dup2(p[1], 0);
		dup2(0, 1);
		dup2(0, 2);
		if (param1[0]) {
			tracelog("%s exec %s \"%s\" %s %s %d",
				 currentuser->userid, cmdfile, param1,
				 currentuser->userid, uinfo.from, getppid());
			execl(cmdfile, cmdfile, param1, currentuser->userid,
			      uinfo.from, pidstr, NULL);
		} else {
			tracelog("%s exec %s %s %s %d",
				 currentuser->userid, cmdfile,
				 currentuser->userid, uinfo.from, getppid());
			execl(cmdfile, cmdfile, currentuser->userid, uinfo.from,
			      pidstr, NULL);
		}
		exit(0);
	} else if (childpid > 0) {
		close(p[1]);
		datapipefd(0, p[0]);
		close(p[0]);
		while (wait(NULL) != childpid)
			sleep(1);
	} else {
		close(p[0]);
		close(p[1]);
	}
	uinfo.pager = save_pager;
	signal(SIGCHLD, SIG_IGN);
	return;
}

static void
datapipefd(int fds, int fdn)
{
	fd_set rs;
	int retv, max;
	char buf[1024];

	max = 1 + ((fdn > fds) ? fdn : fds);
	FD_ZERO(&rs);
	while (1) {
		FD_SET(fds, &rs);
		FD_SET(fdn, &rs);
		retv = select(max, &rs, NULL, NULL, NULL);
		if (retv < 0) {
			if (errno != EINTR)
				break;
			continue;
		}
		if (FD_ISSET(fds, &rs)) {
#ifdef SSHBBS
			retv = ssh_read(fds, buf, sizeof (buf));
#else
			retv = read(fds, buf, sizeof (buf));
#endif
			if (retv > 0) {
				time(&now_t);
				uinfo.lasttime = now_t;
				if ((unsigned long) now_t -
				    (unsigned long) last_utmp_update > 60)
					update_utmp();
				write(fdn, buf, retv);
			} else if (retv == 0 || (retv < 0 && errno != EINTR))
				break;
			FD_CLR(fds, &rs);
		}
		if (FD_ISSET(fdn, &rs)) {
			retv = read(fdn, buf, sizeof (buf));
			if (retv > 0) {
#ifdef SSHBBS
				ssh_write(fds, buf, retv);
#else
				write(fds, buf, retv);
#endif
			} else if (retv == 0 || (retv < 0 && errno != EINTR))
				break;
			FD_CLR(fdn, &rs);
		}
	}
}

#if 0
/* ppfoong */
void
x_dict()
{
	char buf[STRLEN];
	char *s;
	//int whichdict;

	if (heavyload(0)) {
		clear();
		prints("抱歉，目前系统负荷过重，此功能暂时不能执行...");
		pressanykey();
		return;
	}
	modify_user_mode(DICT);
	clear();
	prints("\n\033[1;32m     _____  __        __   __");
	prints
	    ("\n    |     \\|__|.----.|  |_|__|.-----.-----.---.-.----.--.--.");
	prints
	    ("\n    |  --  |  ||  __||   _|  ||  _  |     |  _  |   _|  |  |");
	prints
	    ("\n    |_____/|__||____||____|__||_____|__|__|___._|__| |___  |");
	prints
	    ("\n                                                     |_____|\033[m");
	prints("\n\n\n欢迎使用本站的字典。");
	prints
	    ("\n本字典主要为\033[1;33m「英汉」\033[m部分, 但亦可作\033[1;33m「汉英」\033[m查询。");
	prints
	    ("\n\n系统将根据您所输入的字串, 自动判断您所要翻查的是英文字还是中文字。");
	prints("\n\n\n请输入您欲翻查的英文字或中文字, 或直接按 <ENTER> 取消。");
	getdata(15, 0, ">", buf, 30, DOECHO, YEA);
	if (buf[0] == '\0') {
		prints("\n您不想查了喔...");
		pressanykey();
		return;
	}
	for (s = buf; *s != '\0'; s++) {
		if (isspace(*s)) {
			prints("\n一次只能查一个字啦, 不能太贪心喔!!");
			pressanykey();
			return;
		}
	}
	myexec_cmd(DICT, YEA, "bin/cdict.sh", buf);
	sprintf(buf, "bbstmpfs/tmp/dict.%s.%d", currentuser->userid, uinfo.pid);
	if (file_isfile(buf)) {
		ansimore(buf, NA);
		if (askyn("要将结果寄回信箱吗", NA, NA) == YEA)
			mail_file(buf, currentuser->userid, "字典查询结果",
				  currentuser->userid);
		unlink(buf);
	}
}
#else
void
x_dict()
{
	clear();
	prints("\n-------stardict-----\n");
	myexec_cmd(DICT, NA, "bin/ptyexec", "bin/sdcv.sh");
	redoscr();
}
#endif

void
x_tt()
{
	myexec_cmd(TT, NA, "bin/tt", NULL);
	redoscr();
}

void
x_worker()
{
	myexec_cmd(WORKER, YEA, "bin/worker", NULL);
	redoscr();
}

void
x_tetris()
{
	myexec_cmd(TETRIS, NA, "bin/tetris", NULL);
	redoscr();
}

void
x_winmine()
{
	myexec_cmd(WINMINE, NA, "bin/winmine", NULL);
	redoscr();
}

void
x_winmine2()
{
	myexec_cmd(WINMINE2, NA, "bin/winmine2", NULL);
	redoscr();
}

void
x_recite()
{
	myexec_cmd(RECITE, NA, "bin/ptyexec", "bin/recite");
	redoscr();
}

void
x_ncce()
{
	myexec_cmd(NCCE, NA, "bin/ptyexec", "bin/ncce");
	redoscr();
}

void
x_chess()
{
	myexec_cmd(CHESS, NA, "bin/chc", NULL);
	redoscr();
}

void
x_qkmj()
{
	myexec_cmd(CHESS, NA, "bin/qkmj", NULL);
	redoscr();
}

void
x_quickcalc()
{
	clear();
	prints("\n-------数字运算, 输入help获得帮助------\n");
	myexec_cmd(QUICKCALC, NA, "bin/ptyexec", "bin/qc");
	redoscr();
}

void
x_freeip()
{
	clear();
	if (heavyload(2.5)) {
		prints("抱歉，目前系统负荷过重，此功能暂时不能执行...");
		pressanykey();
		return;
	}
	myexec_cmd(FREEIP, NA, "bin/ptyexec", "bin/freeip");
	redoscr();
}

static void
childreturn(int i)
{

	int retv;
	while ((retv = waitpid(-1, NULL, WNOHANG | WUNTRACED)) > 0)
		if (childpid > 0 && retv == childpid)
			childpid = 0;
}

int
ent_bnet(char *cmd)
{
	int p[2];
	char fname[256];
	signal(SIGALRM, SIG_IGN);
#ifdef SSHBBS
	move(9, 0);
	clrtobot();
	prints("您目前是使用 ssh 方式连接 %s.\n"
	       "ssh 会对网络数据传输进行加密,保护您的密码和其它私人信息.\n"
	       "您必须知道,网络穿梭是通过本站连接到其它bbs,虽然从您的机器\n"
	       "到本站间的数据传输是加密的,但是从本站到另外的 BBS 站点间的\n"
	       "数据传输并没有加密,您的密码和其它私人信息有被第三方窃听的\n可能.\n",
	       MY_BBS_NAME);
	if (askyn("你确定要使用网络穿梭吗", NA, NA) != 1)
		return -1;
#endif
	if (cmd[0] == 'B') {
		sethomefile(fname, currentuser->userid, "bbsnet");
		if (!file_exist(fname)) {
			move(9, 0);
			clrtobot();
			move(11, 0);
			prints
			    ("\033[1m您尚未编辑自定义穿梭列表，请在主选单 \033[1;32mI-W-6\033[0m\033[1m 处编辑。");
			pressreturn();
			return -1;
		}
	} else if (cmd[0] == 'A') {
		sprintf(fname, "etc/bbsnetA.ini");
	} else {
		sprintf(fname, "etc/bbsnet.ini");
	}

	signal(SIGCHLD, childreturn);
	modify_user_mode(BBSNET);
	do_delay(1);
	refresh();
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, p) < 0)
		return -1;
	if (p[0] <= 2 || p[1] <= 2) {
		int i = p[0] + p[1] + 1;
		dup2(p[0], i);
		dup2(p[1], i + 1);
		close(p[0]);
		close(p[1]);
		p[0] = i;
		p[1] = i + 1;
	}
	inBBSNET = 1;
	childpid = fork();
	if (childpid == 0) {
		close(p[1]);
		if (p[0] != 0)
			dup2(p[0], 0);
		dup2(0, 1);
		dup2(0, 2);
		execl("bin/bbsnet", "bbsnet", fname, "bin/telnet",
		      currentuser->userid, NULL);
		exit(0);
	} else if (childpid > 0) {
		close(p[0]);
		datapipefd(0, p[1]);
		close(p[1]);
	} else {
		close(p[0]);
		close(p[1]);
	}
	signal(SIGCHLD, SIG_IGN);
	inBBSNET = 0;
	redoscr();
	return 0;
}

int
x_denylevel()
{
	int id;
	char ans[7], content[1024];
	int oldlevel;
	struct userec tmpu;
	struct userec *lookupuser;
	char secu[STRLEN];

	modify_user_mode(ADMIN);
	if (!check_systempasswd()) {
		return -1;
	}
	clear();
	move(0, 0);
	prints("更改使用者基本权限\n");
	clrtoeol();
	move(1, 0);
	usercomplete("输入欲更改的使用者帐号: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if (!(id = getuser(genbuf, &lookupuser))) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return -1;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("设定使用者 '%s' 的基本权限 \n\n", genbuf);
	prints("(1) 封禁发表文章权利       (A) 恢复发表文章权利\n");
	prints("(2) 取消基本上站权利       (B) 恢复基本上站权利\n");
	prints("(3) 禁止进入聊天室         (C) 恢复进入聊天室权利\n");
	prints("(4) 禁止呼叫他人聊天       (D) 恢复呼叫他人聊天权利\n");
	prints("(5) 整理个人精华区         (E) 不能整理个人精华区\n");
	prints("(6) 禁止发送信件           (F) 恢复发信权利\n");
	prints("(7) 禁止使用签名档         (G) 恢复使用签名档权利\n");
	getdata(12, 0, "请输入你的处理: ", ans, 3, DOECHO, YEA);
	memcpy(&tmpu, lookupuser, sizeof (tmpu));
	oldlevel = lookupuser->userlevel;
	switch (ans[0]) {
	case '1':
		tmpu.userlevel &= ~PERM_POST;
		break;
	case 'a':
	case 'A':
		tmpu.userlevel |= PERM_POST;
		break;
	case '2':
		tmpu.userlevel &= ~PERM_BASIC;
		break;
	case 'b':
	case 'B':
		tmpu.userlevel |= PERM_BASIC;
		break;
	case '3':
		tmpu.userlevel &= ~PERM_CHAT;
		break;
	case 'c':
	case 'C':
		tmpu.userlevel |= PERM_CHAT;
		break;
	case '4':
		tmpu.userlevel &= ~PERM_PAGE;
		break;
	case 'd':
	case 'D':
		tmpu.userlevel |= PERM_PAGE;
		break;
	case '5':
		tmpu.userlevel |= PERM_SPECIAL8;
		break;
	case 'e':
	case 'E':
		tmpu.userlevel &= ~PERM_SPECIAL8;
		break;
	case '6':
		tmpu.userlevel |= PERM_DENYMAIL;
		break;
	case 'f':
	case 'F':
		tmpu.userlevel &= ~PERM_DENYMAIL;
		break;
	case '7':
		tmpu.userlevel |= PERM_DENYSIG;
		{
			getdata(13, 0, "禁止使用签名档的原因：", genbuf, 40,
				DOECHO, YEA);
			sprintf(content,
				"您被禁止使用签名档，原因是：\n    %s\n\n"
				"(如果是因为签名档图片大小超标，请仔细阅读如下规定：\n"
				"由于系统资源及方便网友浏览考虑，将临时限制图片签名档的大小，具体标准为"
				"签名档中的图片高度不超过100象素，宽度不超过500象素，大小不超过50KB，如果签名"
				"档中有多个图片, 尺寸按照其排列方向累加, 大小按照图片大小的和。"
				"如果超过此标准，将临时限制该ID在全站的post权限，直到该ID保证不再使用该"
				"签名档为止，如果再犯将投入监狱，直到保证不再使用超过规定的签名档。",
				genbuf);
			system_mail_buf(content, strlen(content), tmpu.userid,
					"您被禁止使用签名档",
					currentuser->userid);
		}
		break;
	case 'g':
	case 'G':
		tmpu.userlevel &= ~PERM_DENYSIG;
		break;
	default:
		prints("\n 使用者 '%s' 基本权利没有变更\n", tmpu.userid);
		pressreturn();
		clear();
		return 0;
	}
	sprintf(secu, "修改 %s 的基本权限", tmpu.userid);
	permtostr(oldlevel, genbuf);
	sprintf(content, "修改前的权限：%s\n修改后的权限：", genbuf);
	permtostr(tmpu.userlevel, genbuf);
	strcat(content, genbuf);
	securityreport(secu, content);

	updateuserec(&tmpu, 0);
	prints("\n 使用者 '%s' 基本权限已经更改完毕.\n", tmpu.userid);
	pressreturn();
	clear();
	return 0;
}

static int
finddf(int day, char *checkid)
{
	struct boardheader board1;
	struct fileheader *post1;
	FILE *fpb;
	char dirfile[100];
	time_t fbtime, nowtime, starttime;
	int userlevel, count = 0;
      struct mmapfile mf = { ptr:NULL };
	int start, total, i;
	FILE *fp;
	userlevel = currentuser->userlevel;
	nowtime = time(NULL);
	if (day == 0)
		starttime = 0;
	else
		starttime = nowtime - 86400 * day;

	sprintf(dirfile, MY_BBS_HOME "/bbstmpfs/tmp/checkid.%s.%d", checkid,
		uinfo.pid);
	fp = fopen(dirfile, "w");
	if (fp == NULL)
		return -1;
	if (starttime < 0)
		starttime = 0;
	if (day)
		fprintf(fp, "%20s 最近 %d 天的发文情况.\n\n", checkid, day);
	else
		fprintf(fp, "%20s 的发文情况.\n\n", checkid);
	fpb = fopen(MY_BBS_HOME "/.BOARDS", "r");
	while (fread(&board1, sizeof (board1), 1, fpb)) {
		if (board1.level)
			continue;
		if (board1.flag & CLOSECLUB_FLAG)
			continue;
		sprintf(dirfile, MY_BBS_HOME "/boards/%s/.DIR",
			board1.filename);
		MMAP_TRY {
			if (mmapfile(dirfile, &mf) < 0) {
				MMAP_UNTRY;
				continue;
			}
			total = mf.size / sizeof (struct fileheader);
			if (total == 0) {
				mmapfile(NULL, &mf);
				mf.ptr = NULL;
				MMAP_UNTRY;
				continue;
			}
			start =
			    Search_Bin((struct fileheader *) mf.ptr, starttime,
				       0, total - 1);
			if (start < 0) {
				start = -(start + 1);
			}
			for (i = start; i < total && count <= 1000; i++) {
				post1 =
				    (struct fileheader *) (mf.ptr +
							   i *
							   sizeof (struct
								   fileheader));
				fbtime = post1->filetime;
				if (!strcasecmp(post1->owner, checkid)) {
					count++;
					fprintf(fp, "%20s %4d %s \n",
						board1.filename, count,
						post1->title);
				}
			}
		}
		MMAP_CATCH {
		}
		MMAP_END {
			mmapfile(NULL, &mf);
			mf.ptr = NULL;
		}
		if (count > 1000) {
			fprintf(fp, "好多啊... 下面的不查了\n");
			break;
		}
	}
	fclose(fpb);
	fprintf(fp, "一共找到 %d 篇\n", count);
	fclose(fp);
	return 0;
}

int
s_checkid()
{
	char buf[256];
	char checkuser[20];
	int day, id;
	struct userec *lookupuser;
	modify_user_mode(GMENU);
	clear();
	stand_title("调查ID发文情况\n");
	clrtoeol();
	move(2, 0);
	if ((USERPERM(currentuser, PERM_SYSOP) && heavyload(2.5))
	    || (!USERPERM(currentuser, PERM_SYSOP) && heavyload(1.5))) {
		prints("系统负载过重, 无法执行本指令");
		pressreturn();
		return 1;
	}

	usercomplete("输入欲调查的使用者帐号: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	strcpy(checkuser, genbuf);
	if (!(id = getuser(genbuf, &lookupuser))) {
		move(4, 0);
		prints("无效的使用者帐号");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	getdata(5, 0, "输入天数(0-所有时间): ", buf, 7, DOECHO, YEA);
	day = atoi(buf);
	if ((USERPERM(currentuser, PERM_SYSOP) && heavyload(2.5))
	    || (!USERPERM(currentuser, PERM_SYSOP) && heavyload(1.5))) {
		prints("系统负载过重, 无法执行本指令");
		pressreturn();
		return 1;
	}
	if (finddf(day, checkuser) < 0)
		return 0;
	limit_cpu();
	tracelog("%s finddf %s %d", currentuser->userid, checkuser, day);
	sprintf(buf, MY_BBS_HOME "/bbstmpfs/tmp/checkid.%s.%d",
		checkuser, uinfo.pid);
	mail_file(buf, currentuser->userid, "\"System Report\"",
		  currentuser->userid);
	unlink(buf);
	prints("完毕");
	clrtoeol();
	pressreturn();
	clear();
	return 1;
}

char *
directfile(char *fpath, char *direct, char *filename)
{
	char *t;
	strcpy(fpath, direct);
	if ((t = strrchr(fpath, '/')) == NULL)
		exit(0);
	t++;
	strcpy(t, filename);
	return fpath;
}

static void
escape_filename(char *fn)
{
	static const char invalid[] =
	    { '/', '\\', '!', '&', '|', '*', '?', '`', '\'', '\"', ';', '<',
		'>', ':', '~', '(', ')', '[', ']', '{', '}', '$', '\n', '\r'
	};
	int i, j;

	for (i = 0; i < strlen(fn); i++)
		for (j = 0; j < sizeof (invalid); j++)
			if (fn[i] == invalid[j])
				fn[i] = '_';
}

int
zsend_file(char *from, char *title)
{
	char name[200], name1[200];
	char path[200];
	FILE *fr, *fw;
	char to[200];
	char buf[512], *fn = NULL;
	char attachfile[200];
	char attach_to_send[200];
	int len, isa, base64;

	ansimore("etc/zmodem", 0);
	move(14, 0);
	len = file_size(from);

	prints
	    ("此次传输共 %d bytes, 大约耗时 %d 秒（以 5k/s 计算）", len,
	     len / ZMODEM_RATE);
	move(t_lines - 1, 0);
	clrtoeol();
	strcpy(name, "N");

	getdata(t_lines - 1, 0,
		"您确定要使用Zmodem传输文件么?[y/N]", name, 2, DOECHO, YEA);
	if (toupper(name[0]) != 'Y')
		return FULLUPDATE;
	strncpy(name, title, 76);
	name[80] = '\0';
	escape_filename(name);
	move(t_lines - 2, 0);
	clrtoeol();
	prints("请输入文件名，为空则放弃");
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0, "", name, 78, DOECHO, 0);
	if (name[0] == '\0')
		return FULLUPDATE;
	name[78] = '\0';
	escape_filename(name);
	sprintf(name1, "YTHT-%s-", currboard);
	strcat(name1, name);
	strcpy(name, name1);
	strcat(name1, ".TXT");
	snprintf(path, sizeof (path), PATHZMODEM "/%s.%d", currentuser->userid,
		 uinfo.pid);
	mkdir(path, 0770);
	sprintf(to, "%s/%s", path, name1);
	fr = fopen(from, "r");
	if (fr == NULL)
		return FULLUPDATE;
	fw = fopen(to, "w");
	if (fw == NULL) {
		fclose(fr);
		return FULLUPDATE;
	}
	while (fgets(buf, sizeof (buf), fr) != NULL) {
		base64 = isa = 0;
		if (!strncmp(buf, "begin 644", 10)) {
			isa = 1;
			base64 = 1;
			fn = buf + 10;
		} else if (checkbinaryattach(buf, fr, &len)) {
			isa = 1;
			base64 = 0;
			fn = buf + 18;
		}
		if (isa) {
			sprintf(attachfile, "%s-attach-%s", name, fn);
			if (getattach
			    (fr, buf, attachfile, path, base64, len, 0)) {
				fprintf(fw, "附件%s错误\n", fn);
			} else {
				sprintf(attach_to_send, "%s/%s", path,
					attachfile);
				bbs_zsendfile(attach_to_send);
				fprintf(fw, "附件%s\n", fn);
			}
		} else
			fputs(buf, fw);
	}
	fclose(fw);
	fclose(fr);
	bbs_zsendfile(to);
	rmdir(path);
	return FULLUPDATE;
}

static void
bbs_zsendfile(char *filename)
{
	if (!file_isfile(filename))
		return;
	refresh();
	myexec_cmd(READING, NA, "bin/sz.sh", filename);
	unlink(filename);
}

void
inn_reload()
{
	char ans[4];

	getdata(t_lines - 1, 0, "重读配置吗 (Y/N)? [N]: ", ans, 2, DOECHO, YEA);
	if (ans[0] == 'Y' || ans[0] == 'y') {
		myexec_cmd(ADMIN, NA, "innd/ctlinnbbsd", "reload");
	}
}

void
x_invite()
{
	char toemail[60], toname[40], ans[4];
	char tmpfn[128];
	FILE *fp;
#ifndef ENABLE_INVITATION
	prints("没有开放邀请注册的功能");
	pressreturn();
	return;
#endif
	if ((currentuser->userlevel & PERM_DEFAULT) != PERM_DEFAULT)
		return;
	modify_user_mode(GMENU);
	clear();
	move(0, 0);
	prints("可以发一封 email 给你的好友，邀请他们来" MY_BBS_NAME "\n");
	prints("当你的好友注册后，系统会自动添加你们到对方的好友名单。\n");
	prints("而且你的生命值也会上涨 50 点！（有限时间内有效喔 :) ）。\n");
	getdata(4, 0, "现在邀请朋友来" MY_BBS_NAME "吗(Y/N)？[Y]", ans, 2,
		DOECHO, YEA);
	if (ans[0] == 'N' || ans[0] == 'n')
		return;
	toemail[0] = 0;
	toname[0] = 0;
      EDITAGAIN:
	while (1) {
		getdata(5, 0, "请输入她/他的 email：", toemail, 59, DOECHO, NA);
		if (*toemail == 0)
			return;
		if (trustEmail(toemail))
			break;
		move(6, 0);
		prints("%s 不在认证范围或者已经注册了用户。(输入回车中止邀请)",
		       toemail);
		toemail[0] = 0;
	}
	while (1) {
		getdata(6, 0, "请输入她/他的姓名：", toname, 38, DOECHO, NA);
		if (strlen(toname) >= 4)
			break;
		move(7, 0);
		prints("名字长度要不少于 2 个汉字。");
	}
	move(7, 0);
	clrtoeol();
	prints("按回车开始编辑给 %s 的留言", toemail);
	pressreturn();
	sprintf(tmpfn, "bbstmpfs/tmp/bbs.%d", getpid());
	if (!file_exist(tmpfn) && (fp = fopen(tmpfn, "w"))) {
		fprintf(fp, "快来" MY_BBS_NAME " BBS 申请一个用户");
		fclose(fp);
	}
	vedit(tmpfn, 0, 0);
	clear();
	prints("你的好友的资料为，\n"
	       "名字：\033[33;1m%s\033[m\n信箱：\033[33;1m%s\033[m\n",
	       toname, toemail);
	getdata(4, 0,
		"确定给他/她发送邀请吗？(Y)是，(N)否，(E)修改一下 [Y]：",
		ans, 2, DOECHO, YEA);
	if (ans[0] == 'N' || ans[0] == 'n') {
		unlink(tmpfn);
		return;
	}
	if (ans[0] == 'E' || ans[0] == 'e') {
		goto EDITAGAIN;
	}
	sendInvitation(currentuser->userid, toemail, toname, tmpfn);
	unlink(tmpfn);
	clear();
	tracelog("%s invite %s %s", currentuser->userid, toemail, toname);
	prints("已经给 %s 发出邀请了", toemail);
	pressreturn();
}
