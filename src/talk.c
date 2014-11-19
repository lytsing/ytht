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
    Copyright (C) 1999	KCN,Zhou lin,kcn@cic.tsinghua.edu.cn
    
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
#define M_INT 8			/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in talk/chat */

int talkidletime = 0;
int ulistpage;
int friendflag = 1;
//#ifdef TALK_LOG
int talkrec = -1;
char partner[IDLEN + 1];
//#endif

struct talk_win {
	int curcol, curln;
	int sline, eline;
};

int nowmovie;

static char *const refuse[] = {
	"抱歉，我现在想专心看 Board。    ", "我今天很累，不想跟别人聊天。    ",
	"我现在有事，等一下再 Call 你。  ", "我马上要离开了，下次再聊吧。    ",
	"请你不要再 Page，我不想跟你聊。 ", "请先写一封自我介绍给我，好吗？  ",
	"对不起，我现在在等人。          ", "请不要吵我，好吗？..... :)      ",
	NULL
};

char save_page_requestor[STRLEN];

static char canpage(int friend, int pager);
static int show_user_plan(char *userid);
static int cmpfnames(char *userid, struct override *uv);
static int cmpunums(int unum, struct user_info *up);
static int cmpmsgnum(int unum, struct user_info *up);
static int setpagerequest(int mode);
static void do_talk_nextline(struct talk_win *twin);
static void do_talk_char(struct talk_win *twin, int ch);
static void talkflush(void);
static void moveto(int mode, struct talk_win *twin);
static int do_talk(int fd);
static int override_title(void);
static char *override_doentry(int ent, struct override *fh, char buf[512]);
static int override_edit(int ent, struct override *fh, char *direc);
static int override_dele(int ent, struct override *fh, char *direct);
static int friend_edit(int ent, struct override *fh, char *direct);
static int friend_add(int ent, struct override *fh, char *direct);
static int friend_dele(int ent, struct override *fh, char *direct);
static int friend_mail(int ent, struct override *fh, char *direct);
static int friend_query(int ent, struct override *fh, char *direct);
static int friend_help(void);
static int reject_edit(int ent, struct override *fh, char *direct);
static int reject_add(int ent, struct override *fh, char *direct);
static int reject_dele(int ent, struct override *fh, char *direct);
static int reject_query(int ent, struct override *fh, char *direct);
static int reject_help(void);
static int cmpfuid(unsigned *a, unsigned *b);
static void do_log(char *msg, int who);
static char *Cdate(time_t * clock);
//static void creat_list(void);

struct one_key friend_list[] = {
	{'r', friend_query, "查询好友"},
	{'m', friend_mail, "给好友写信"},
	{'M', friend_mail, "给好友写信"},
	{'a', friend_add, "添加好友"},
	{'A', friend_add, "添加好友"},
	{'d', friend_dele, "删除好友"},
	{'D', friend_dele, "删除好友"},
	{'E', friend_edit, "编辑好友"},
	{'h', friend_help, "查看帮助"},
	{'H', friend_help, "查看帮助"},
	{'\0', NULL, ""}
};

struct one_key reject_list[] = {
	{'r', reject_query, "查询坏人"},
	{'a', reject_add, "添加坏人"},
	{'A', reject_add, "增加坏人"},
	{'d', reject_dele, "删除坏人"},
	{'D', reject_dele, "删除坏人"},
	{'E', reject_edit, "编辑坏人"},
	{'h', reject_help, "查看帮助"},
	{'H', reject_help, "查看帮助"},
	{'\0', NULL, ""}
};

static char
canpage(friend, pager)
int friend, pager;
{
	if ((pager & ALL_PAGER)
	    || USERPERM(currentuser, (PERM_SYSOP | PERM_FORCEPAGE)))
		return YEA;
	if ((pager & FRIEND_PAGER)) {
		if (friend)
			return YEA;
	}
	return NA;
}

int
t_pager()
{

	if (uinfo.pager & ALL_PAGER) {
		uinfo.pager &= ~ALL_PAGER;
		if (USERDEFINE(currentuser, DEF_FRIENDCALL))
			uinfo.pager |= FRIEND_PAGER;
		else
			uinfo.pager &= ~FRIEND_PAGER;
	} else {
		uinfo.pager |= ALL_PAGER;
		uinfo.pager |= FRIEND_PAGER;
	}

	if (!uinfo.in_chat && uinfo.mode != TALK) {
		move(1, 0);
		prints("您的呼叫器 (pager) 已经\033[1m%s\033[m了!",
		       (uinfo.pager & ALL_PAGER) ? "打开" : "关闭");
		pressreturn();
	}
	update_utmp();
	return 0;
}

/*Add by SmallPig*/
/*此函数只负责列印说明档，并不管清除或定位的问题。*/
static int
show_user_plan(userid)
char *userid;
{
	int i, foo;
	char pfile[STRLEN];

	sethomefile(pfile, userid, "plans");
	if (!file_exist(pfile)) {
		prints("\033[1;36m没有个人说明档\033[m\n");
		return NA;
	}
	prints("\033[1;36m个人说明档如下：\033[m\n");
	getyx(&i, &foo);
	ansimore2(pfile, 0, i, t_lines - i);
	return YEA;
}

/* Modified By Excellent*/
int
t_query(q_id)
char q_id[IDLEN + 2];
{
	char uident[STRLEN];
	int tuid = 0;
	int exp, perf;		/*Add by SmallPig */
	char qry_mail_dir[STRLEN];
	struct in_addr in;
	struct userec *lookupuser;

	if ((uinfo.mode != LUSERS && uinfo.mode != LAUSERS
	     && uinfo.mode != FRIEND && uinfo.mode != READING
	     && uinfo.mode != MAIL && uinfo.mode != RMAIL && uinfo.mode != GMENU
	     && uinfo.mode != BACKNUMBER && uinfo.mode != DO1984)
	    || q_id == NULL) {
		modify_user_mode(QUERY);
		move(1, 0);
		clrtobot();
		prints("查询谁:\n<输入使用者代号, 按空白键可列出符合字串>\n");
		move(1, 8);
		usercomplete(NULL, uident);
		if (uident[0] == '\0') {
			return 0;
		}
	} else {
		if (*q_id == '\0')
			return 0;
		if (strchr(q_id, ' '))
			strtok(q_id, " ");
		strncpy(uident, q_id, sizeof (uident));
		uident[sizeof (uident) - 1] = '\0';
	}
	if (!(tuid = getuser(uident, &lookupuser))) {
		move(2, 0);
		clrtoeol();
		prints("\033[1m不正确的使用者代号\033[m\n");
		pressanykey();
		return -1;
	}
	uinfo.destuid = tuid;
	update_utmp();

	move(1, 0);
	clrtobot();
	sprintf(qry_mail_dir, "mail/%c/%s/%s", mytoupper(lookupuser->userid[0]),
		lookupuser->userid, DOT_DIR);

	exp = countexp(lookupuser);
	perf = countperf(lookupuser);
	move(6, 0);
	show_user_plan(lookupuser->userid);
	move(1, 0);
	prints
	    ("\033[m\033[1m%s \033[m(\033[1m%s\033[m) 共上站 \033[1m%d\033[m 次，发表过 \033[1m%d\033[m 篇文章",
	     lookupuser->userid, lookupuser->username, lookupuser->numlogins,
	     lookupuser->numposts);
	strcpy(genbuf, Ctime(lookupuser->lastlogin));
	if (is_inprison(lookupuser->userid)) {
		strcpy(genbuf, Ctime(lookupuser->lastlogin));
		prints("\n在监狱服刑，上次放风时间[\033[1m%s\033[m]\n", genbuf);
		if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS
		    && uinfo.mode != FRIEND && uinfo.mode != GMENU)
			pressanykey();
		uinfo.destuid = 0;
		return 0;

	}

	if (lookupuser->dieday) {
		prints
		    ("\n已经离开了人世,呜呜...\n还有 [\033[1m%d\033[m] 天就要转世投胎了\n",
		     countlife(lookupuser));
		if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS
		    && uinfo.mode != FRIEND && uinfo.mode != GMENU)
			pressanykey();
		uinfo.destuid = 0;
		return 0;
	}
	in.s_addr = lookupuser->lasthost & 0x0000ffff;
	prints("\n上次在 [\033[1m%s\033[m] 从 [\033[1m%s\033[m] 到本站一游。\n",
	       genbuf, (lookupuser->lasthost == 0 ? "(不详)" : inet_ntoa(in)));
	strcpy(genbuf,
	       (lookupuser->lastlogout >=
		lookupuser->lastlogin) ? Ctime(lookupuser->lastlogout) :
	       "因在线上或不正常断线不详");
	prints
	    ("离站时间：[\033[1m%s\033[m] 信箱：[\033[1;5m%2s\033[m]，生命力：[\033[1m%d\033[m]。\n",
	     genbuf, (check_query_mail(qry_mail_dir) == 1) ? "信" : "  ",
	     countlife(lookupuser));
	if (lookupuser->userlevel & PERM_BOARDS) {
		prints("担任版务：");
		apply_boards(bm_printboard, lookupuser->userid);
		prints("\n");
	}
	t_search_ulist(tuid);
	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS
	    && uinfo.mode != FRIEND && uinfo.mode != GMENU) {
		int ch, oldmode;
		char buf[STRLEN];
		oldmode = uinfo.mode;
		move(t_lines - 1, 0);
		prints
		    ("\033[1;37m寄信[\033[1;32mm\033[1;37m] 送讯息[\033[1;32ms\033[1;37m] 加,减好友[\033[1;32mo\033[1;37m,\033[1;32md\033[1;37m] 其它键继续");
		clrtoeol();
		resetcolor();
		ch = igetkey();
		switch (toupper(ch)) {
		case 'S':
			if (strcmp(uident, "guest")
			    && !USERPERM(currentuser, PERM_PAGE))
				break;
			do_sendmsg(uident, 0, NULL, 2, 0);
			break;
		case 'M':
			if (!USERPERM(currentuser, PERM_POST))
				break;
			m_send(uident);
			break;
		case 'O':
			if (!strcmp("guest", currentuser->userid))
				break;
			friendflag = 1;
			if (addtooverride(uident) == -1)
				sprintf(buf, "%s 已在好友名单", uident);
			else
				sprintf(buf, "%s 列入好友名单", uident);
			move(t_lines - 1, 0);
			clrtoeol();
			prints("%s", buf);
			refresh();
			sleep(1);
			break;
		case 'D':
			if (!strcmp("guest", currentuser->userid))
				break;
			sprintf(buf, "确定要把 %s 从好友名单删除吗 (Y/N) [N]: ",
				uident);
			move(t_lines - 1, 0);
			clrtoeol();
			getdata(t_lines - 1, 0, buf, genbuf, 4, DOECHO, YEA);
			move(t_lines - 1, 0);
			clrtoeol();
			if (genbuf[0] != 'Y' && genbuf[0] != 'y')
				break;
			if (deleteoverride(uident, "friends") == -1)
				sprintf(buf, "%s 本来就不在好友名单中", uident);
			else
				sprintf(buf, "%s 已从好友名单移除", uident);
			move(t_lines - 1, 0);
			clrtoeol();
			prints("%s", buf);
			refresh();
			sleep(1);
			break;
		}
		uinfo.mode = oldmode;
	}
	uinfo.destuid = 0;
	return 0;
}

int
bm_printboard(struct boardmem *bmem, void *param)
{
	char *who = (char *) param;
	if (chk_BM_id(who, &bmem->header) && hasreadperm(&bmem->header))
		prints("%s ", bmem->header.filename);
	return 0;
}

void
num_alcounter()
{
	static int last_time = 0;
	int i, t = time(NULL);
	if (abs(t - last_time) < 20) {
		count_users = bbsinfo.utmpshm->activeuser;
		return;
	}
	last_time = t;
	count_friends = 0;
	for (i = 0; i < uinfo.fnum; i++)
		count_friends += query_uindex(uinfo.friend[i], 1) ? 1 : 0;
	count_users = num_active_users();
}

int
num_active_users()
{
	return bbsinfo.utmpshm->activeuser;
}

static int
cmpfnames(userid, uv)
char *userid;
struct override *uv;
{
	return !strcmp(userid, uv->id);
}

int
t_cmpuids(uid, up)
int uid;
struct user_info *up;
{
	return (up->active && uid == up->uid);
}

int
t_talk()
{
	int netty_talk;

#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	netty_talk = ttt_talk(NULL);
	clear();
	return (netty_talk);
}

int
ttt_talk(userinfo)
struct user_info *userinfo;
{
	char uident[STRLEN];
	char reason[STRLEN];
	int tuid, ucount, unum, tmp;
	int savemode;

	struct user_info uin;

	move(1, 0);
	clrtobot();
	if (uinfo.invisible) {
		move(2, 0);
		prints("抱歉, 此功能在隐身状态下不能执行...\n");
		pressreturn();
		return 0;
	}
	if (userinfo == NULL) {
		move(2, 0);
		prints("<输入使用者代号>\n");
		move(1, 0);
		clrtoeol();
		prints("跟谁聊天: ");
//		creat_list();
		usercomplete(NULL, uident);
		if (uident[0] == '\0') {
			clear();
			return 0;
		}
		if (!(tuid = getuser(uident, NULL)) || tuid == usernum) {
		      wrongid:
			move(2, 0);
			prints("错误代号\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
		ucount = count_logins(&uin, t_cmpuids, tuid, 0);
		move(3, 0);
		prints("目前 %s 的 %d logins 如下: \n", uident, ucount);
		clrtobot();
		if (ucount > 1) {
		      list:move(5, 0);
			prints("(0) 算了算了，不聊了。\n");
			ucount = count_logins(&uin, t_cmpuids, tuid, 0);
			count_logins(&uin, t_cmpuids, tuid, 1);
			clrtobot();
			tmp = ucount + 8;
			getdata(tmp, 0, "请选一个你看的比较顺眼的 [0]: ",
				genbuf, 4, DOECHO, YEA);
			unum = atoi(genbuf);
			if (unum == 0) {
				clear();
				return 0;
			}
			if (unum > ucount || unum < 0) {
				move(tmp, 0);
				prints("笨笨！你选错了啦！\n");
				clrtobot();
				pressreturn();
				goto list;
			}
			if (!search_ulistn(&uin, t_cmpuids, tuid, unum))
				goto wrongid;
		} else if (!search_ulist(&uin, t_cmpuids, tuid))
			goto wrongid;
	} else {
		uin = *userinfo;
		tuid = uin.uid;
		strcpy(uident, uin.userid);
		move(1, 0);
		clrtoeol();
		prints("跟谁聊天: %s", uin.userid);
	}
	/* youzi : check guest */
	if (!strcmp(uin.userid, "guest")
	    && !USERPERM(currentuser, PERM_FORCEPAGE))
		return -1;

	/*  check if pager on/off       --gtv */
	if (!canpage(hisfriend(&uin), uin.pager)) {
		move(2, 0);
		prints("对方呼叫器已关闭.\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	}
	if (uin.mode == SYSINFO || uin.mode == IRCCHAT || uin.mode == BBSNET ||
	    uin.mode == DICT || uin.mode == ADMIN || uin.mode == ARCHIE
	    || uin.mode == LOCKSCREEN || uin.mode == GAME || uin.mode == WWW
	    || uin.mode == HYTELNET || uin.mode == PAGE) {
		move(2, 0);
		prints("目前无法呼叫.\n");
		clrtobot();
		pressreturn();
		return -1;
	}
	if (!uin.active || uin.pid <= 0 || uin.pid == 1
	    || (kill(uin.pid, 0) == -1)) {
		move(2, 0);
		if (uin.active && uin.pid == 1)
			prints("对方是WWW上线，无法呼叫.\n");
		else
			prints("对方已离开\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	} else {
		int sock, msgsock, length;
		struct sockaddr_in server;
		char c, answer[2] = "";

		move(3, 0);
		clrtobot();
		show_user_plan(uident);
		getdata(2, 0,
			"想找对方谈天请按'y'(Y/N)[N]:", answer, 4, DOECHO, YEA);
		if (*answer != 'y') {
			clear();
			return 0;
		}
		tracelog("%s talk %s", currentuser->userid, uident);

/* modified end. */

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			perror("socket err\n");
			return -1;
		}

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = 0;
		if (bind(sock, (struct sockaddr *) &server, sizeof (server)) <
		    0) {
			close(sock);
			perror("bind err");
			return -1;
		}
		length = sizeof (server);
		if (getsockname(sock, (struct sockaddr *) &server, &length) < 0) {
			close(sock);
			perror("socket name err");
			return -1;
		}
		uinfo.sockactive = YEA;
		uinfo.sockaddr = server.sin_port;
		uinfo.destuid = tuid;
		savemode = uinfo.mode;
		modify_user_mode(PAGE);

/* modified end */

//#ifdef TALK_LOG
		strcpy(partner, uin.userid);
//#endif

		kill(uin.pid, SIGUSR1);
		clear();
		prints("     呼叫 %s 中...\n输入 Ctrl-D 结束\n", uident);

		listen(sock, 1);
		add_io(sock, 20);
		while (YEA) {
			int ch;
			ch = igetkey();
			if (ch == I_TIMEOUT) {
				move(0, 0);
				add_io(0, 0);
				add_io(sock, 20);
				prints("再次呼叫.\n");
				bell();
				if (kill(uin.pid, SIGUSR1) == -1) {
					move(0, 0);
					prints("对方已离线\n");
					pressreturn();
					/*Add by SmallPig 2 lines */
					uinfo.sockactive = NA;
					uinfo.destuid = 0;
					add_io(0, 0);
					close(sock);
					return -1;
				}
				continue;
			}
			if (ch == I_OTHERDATA)
				break;
			if (ch == '\004') {
				add_io(0, 0);
				close(sock);
				uinfo.sockactive = NA;
				uinfo.destuid = 0;
				clear();
				return 0;
			}
		}
		add_io(0, 0);
		msgsock = accept(sock, (struct sockaddr *) 0, (int *) 0);
		close(sock);
		if (msgsock == -1) {
			perror("accept");
			return -1;
		}
		uinfo.sockactive = NA;
/*      uinfo.destuid = 0 ;*/
		read(msgsock, &c, sizeof (c));

		clear();

		switch (c) {
		case 'y':
		case 'Y':
			sprintf(save_page_requestor, "%s (%s)", uin.userid,
				uin.username);
			do_talk(msgsock);
			break;
		case 'a':
		case 'A':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[0]);
			pressreturn();
			break;
		case 'b':
		case 'B':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[1]);
			pressreturn();
			break;
		case 'c':
		case 'C':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[2]);
			pressreturn();
			break;
		case 'd':
		case 'D':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[3]);
			pressreturn();
			break;
		case 'e':
		case 'E':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[4]);
			pressreturn();
			break;
		case 'f':
		case 'F':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[5]);
			pressreturn();
			break;
		case 'g':
		case 'G':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[6]);
			pressreturn();
			break;
		case 'n':
		case 'N':
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       refuse[7]);
			pressreturn();
			break;
		case 'm':
		case 'M':
			read(msgsock, reason, sizeof (reason));
			prints("%s (%s)说：%s\n", uin.userid, uin.username,
			       reason);
			pressreturn();
		default:
			sprintf(save_page_requestor, "%s (%s)", uin.userid,
				uin.username);
//#ifdef TALK_LOG
			strcpy(partner, uin.userid);
//#endif

			do_talk(msgsock);
			break;
		}
		close(msgsock);
		clear();
		uinfo.destuid = 0;
	}
	modify_user_mode(savemode);
	occurredSIGUSR2 = 1;
	return 0;
}

struct user_info ui;
char page_requestor[STRLEN];
char page_requestorid[STRLEN];

static int
cmpunums(unum, up)
int unum;
struct user_info *up;
{
	if (!up->active)
		return 0;
	return (unum == up->destuid);
}

static int
cmpmsgnum(unum, up)
int unum;
struct user_info *up;
{
	if (!up->active)
		return 0;
	return (unum == up->destuid && up->sockactive == 2);
}

static int
setpagerequest(mode)
int mode;
{
	int tuid;
	if (mode == 0)
		tuid = search_ulist(&ui, cmpunums, usernum);
	else
		tuid = search_ulist(&ui, cmpmsgnum, usernum);
	if (tuid == 0)
		return 1;
	if (!ui.sockactive)
		return 1;
	uinfo.destuid = ui.uid;
	sprintf(page_requestor, "%s (%s)", ui.userid, ui.username);
	strcpy(page_requestorid, ui.userid);
	return 0;
}

int
servicepage(line, mesg)
int line;
char *mesg;
{
	static time_t last_check;
	time_t now;
	char buf[STRLEN];
	int tuid = search_ulist(&ui, cmpunums, usernum);

	if (tuid == 0 || !ui.sockactive)
		talkrequest = NA;
	if (!talkrequest) {
		if (page_requestor[0]) {
			switch (uinfo.mode) {
			case TALK:
				move(line, 0);
				printdash(mesg);
				break;
			default:	/* a chat mode */
				sprintf(buf, "** %s 已停止呼叫.",
					page_requestor);
				printchatline(buf);
			}
			memset(page_requestor, 0, STRLEN);
			last_check = 0;
		}
		return NA;
	} else {
		now = time(0);
		if (now - last_check > P_INT) {
			last_check = now;
			if (!page_requestor[0]
			    && setpagerequest(0 /*For Talk */ ))
				return NA;
			else
				switch (uinfo.mode) {
				case TALK:
					move(line, 0);
					sprintf(buf, "** %s 正在呼叫你",
						page_requestor);
					printdash(buf);
					break;
				default:	/* chat */
					sprintf(buf, "** %s 正在呼叫你",
						page_requestor);
					printchatline(buf);
				}
		}
	}
	return YEA;
}

int
talkreply()
{
	int a;
	struct hostent *h;
	char buf[512];
	char reason[51];
	char hostname[STRLEN];
	struct sockaddr_in sin;
	char inbuf[STRLEN * 2];
	struct userec *lookupuser;

	struct user_info uip;
	int tuid;

	talkrequest = NA;
	if (setpagerequest(0 /*For Talk */ ))
		return 0;
#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	clear();

	if (!(tuid = getuser(page_requestorid, &lookupuser)))
		return 0;
	who_callme(&uip, t_cmpuids, tuid, uinfo.uid);
	uinfo.destuid = uip.uid;
	getuser(uip.userid, &lookupuser);

	move(5, 0);
	clrtobot();
	show_user_plan(page_requestorid);
	move(1, 0);
	prints("(A)【%s】(B)【%s】\n", refuse[0], refuse[1]);
	prints("(C)【%s】(D)【%s】\n", refuse[2], refuse[3]);
	prints("(E)【%s】(F)【%s】\n", refuse[4], refuse[5]);
	prints("(G)【%s】(N)【%s】\n", refuse[6], refuse[7]);
	prints("(M)【留言给 %-13s            】\n", page_requestorid);

	sprintf(inbuf, "你想跟 %s %s吗？请选择(Y/N/A/B/C/D)[Y] ",
		page_requestor, "聊聊天");

	strcpy(save_page_requestor, page_requestor);
//#ifdef TALK_LOG
	strcpy(partner, page_requestorid);
//#endif

	memset(page_requestor, 0, sizeof (page_requestor));
	memset(page_requestorid, 0, sizeof (page_requestorid));
	getdata(0, 0, inbuf, buf, 2, DOECHO, YEA);
	gethostname(hostname, STRLEN);
	if (!(h = gethostbyname(hostname))) {
		perror("gethostbyname");
		return -1;
	}
	memset(&sin, 0, sizeof (sin));
	sin.sin_family = h->h_addrtype;
	memcpy(&sin.sin_addr, h->h_addr, h->h_length);
	sin.sin_port = ui.sockaddr;
	a = socket(sin.sin_family, SOCK_STREAM, 0);
	if ((connect(a, (struct sockaddr *) &sin, sizeof (sin)))) {
		close(a);
		perror("connect err");
		return -1;
	}
	if (buf[0] != 'A' && buf[0] != 'a' && buf[0] != 'B' && buf[0] != 'b'
	    && buf[0] != 'C' && buf[0] != 'c' && buf[0] != 'D' && buf[0] != 'd'
	    && buf[0] != 'e' && buf[0] != 'E' && buf[0] != 'f' && buf[0] != 'F'
	    && buf[0] != 'g' && buf[0] != 'G' && buf[0] != 'n' && buf[0] != 'N'
	    && buf[0] != 'm' && buf[0] != 'M')
		buf[0] = 'y';
	if (buf[0] == 'M' || buf[0] == 'm') {
		move(1, 0);
		clrtobot();
		getdata(1, 0, "留话：", reason, 50, DOECHO, YEA);
	}
	write(a, buf, 1);
	if (buf[0] == 'M' || buf[0] == 'm')
		write(a, reason, sizeof (reason));
	if (buf[0] != 'y') {
		close(a);
		clear();
		return 0;
	}
	clear();
	do_talk(a);
	close(a);
	clear();
	return 0;
}

static void
do_talk_nextline(twin)
struct talk_win *twin;
{

	twin->curln = twin->curln + 1;
	if (twin->curln > twin->eline)
		twin->curln = twin->sline;
	if (twin->curln != twin->eline) {
		move(twin->curln + 1, 0);
		clrtoeol();
	}
	move(twin->curln, 0);
	clrtoeol();
	twin->curcol = 0;
}

static void
do_talk_char(twin, ch)
struct talk_win *twin;
int ch;
{

	if (isprint2(ch)) {
		if (twin->curcol < 79) {
			move(twin->curln, (twin->curcol)++);
			prints("%c", ch);
			return;
		}
		do_talk_nextline(twin);
		twin->curcol++;
		prints("%c", ch);
		return;
	}
	switch (ch) {
	case Ctrl('H'):
	case '\177':
		if (twin->curcol == 0) {
			return;
		}
		(twin->curcol)--;
		move(twin->curln, twin->curcol);
		prints(" ");
		move(twin->curln, twin->curcol);
		return;
	case Ctrl('M'):
	case Ctrl('J'):
		do_talk_nextline(twin);
		return;
	case Ctrl('G'):
		bell();
		return;
	default:
		break;
	}
	return;
}

char talkobuf[80];
int talkobuflen;
int talkflushfd;

static void
talkflush()
{
	if (talkobuflen)
		write(talkflushfd, talkobuf, talkobuflen);
	talkobuflen = 0;
}

static void
moveto(mode, twin)
int mode;
struct talk_win *twin;
{
	if (mode == 1)
		twin->curln--;
	if (mode == 2)
		twin->curln++;
	if (mode == 3)
		twin->curcol++;
	if (mode == 4)
		twin->curcol--;
	if (twin->curcol < 0) {
		twin->curln--;
		twin->curcol = 0;
	} else if (twin->curcol > 79) {
		twin->curln++;
		twin->curcol = 0;
	}
	if (twin->curln < twin->sline) {
		twin->curln = twin->eline;
	}
	if (twin->curln > twin->eline) {
		twin->curln = twin->sline;
	}
	move(twin->curln, twin->curcol);
}

void
endmsg(int sig)
{
	int x, y;
	int tmpansi;
	tmpansi = showansi;
	showansi = 1;
	talkidletime += 60;
	if (uinfo.in_chat == YEA)
		return;
	if (!canProcessSIGALRM && sig) {
		occurredSIGALRM = 1;
		handlerSIGALRM = endmsg;
		return;
	}
	if (talkidletime >= IDLE_TIMEOUT)
		kill(getpid(), SIGHUP);
	getyx(&x, &y);
	update_endline();
	signal(SIGALRM, (void *) endmsg);
	move(x, y);
	alarm(60);
	showansi = tmpansi;
	return;
}

static int
do_talk(fd)
int fd;
{
	struct talk_win mywin, itswin;
	char mid_line[256];
	int page_pending = NA;
	int i, i2;
	int previous_mode;
//#ifdef TALK_LOG
	char mywords[80], itswords[80], talkbuf[80];
	int mlen = 0, ilen = 0;
	time_t now;
	char ans[3];
	mywords[0] = itswords[0] = '\0';
//#endif

	signal(SIGALRM, SIG_IGN);
	endmsg(0);
	previous_mode = uinfo.mode;
	modify_user_mode(TALK);
	sprintf(mid_line, " %s (%s) 和 %s 正在畅谈中",
		currentuser->userid, currentuser->username,
		save_page_requestor);

	memset(&mywin, 0, sizeof (mywin));
	memset(&itswin, 0, sizeof (itswin));
	i = (t_lines - 1) / 2;
	mywin.eline = i - 1;
	itswin.curln = itswin.sline = i + 1;
	itswin.eline = t_lines - 2;
	move(i, 0);
	printdash(mid_line);
	move(0, 0);

	talkobuflen = 0;
	talkflushfd = fd;
	add_io(fd, 0);
	add_flush(talkflush);

	while (YEA) {
		int ch;
		if (talkrequest)
			page_pending = YEA;
		if (page_pending)
			page_pending = servicepage((t_lines - 1) / 2, mid_line);
		ch = igetkey();
		talkidletime = 0;
		if (ch == '\033') {
			igetkey();
			igetkey();
			continue;
		}
		if (ch == I_OTHERDATA) {
			char data[80];
			int datac;
			register int i;

			datac = read(fd, data, 80);
			if (datac <= 0)
				break;
			for (i = 0; i < datac; i++) {
				if (data[i] >= 1 && data[i] <= 4) {
					moveto(data[i] - '\0', &itswin);
					continue;
				}
//#ifdef TALK_LOG
				/*
				 * Sonny.990514 add an robust and fix some
				 * logic problem
				 */
				/*
				 * Sonny.990606 change to different algorithm
				 * and fix the
				 */
				/* existing do_log() overflow problem       */
				else if (isprint2(data[i])) {
					if (ilen >= 80) {
						itswords[79] = '\0';
						(void) do_log(itswords, 2);
						ilen = 0;
					} else {
						itswords[ilen] = data[i];
						ilen++;
					}
				} else if ((data[i] == Ctrl('H')
					    || data[i] == '\177') && !ilen) {
					itswords[ilen--] = '\0';
				} else if (data[i] == Ctrl('M')
					   || data[i] == '\r'
					   || data[i] == '\n') {
					itswords[ilen] = '\0';
					(void) do_log(itswords, 2);
					ilen = 0;
				}
//#endif

				do_talk_char(&itswin, data[i]);
			}
		} else {
			if (ch == Ctrl('D') || ch == Ctrl('C'))
				break;
			if (isprint2(ch) || ch == Ctrl('H') || ch == '\177'
			    || ch == Ctrl('G') || ch == Ctrl('M')) {
				talkobuf[talkobuflen++] = ch;
				if (talkobuflen == 80)
					talkflush();
//#ifdef TALK_LOG
				if (mlen < 80) {
					if ((ch == Ctrl('H') || ch == '\177')
					    && mlen != 0) {
						mywords[mlen--] = '\0';
					} else {
						mywords[mlen] = ch;
						mlen++;
					}
				} else if (mlen >= 80) {
					mywords[79] = '\0';
					(void) do_log(mywords, 1);
					mlen = 0;
				}
//#endif

				do_talk_char(&mywin, ch);
			} else if (ch == '\n') {
//#ifdef TALK_LOG
				if (mywords[0] != '\0') {
					mywords[mlen++] = '\0';
					(void) do_log(mywords, 1);
					mlen = 0;
				}
//#endif
				talkobuf[talkobuflen++] = '\r';
				talkflush();
				do_talk_char(&mywin, '\r');
			} else if (ch >= KEY_UP && ch <= KEY_LEFT) {
				moveto(ch - KEY_UP + 1, &mywin);
				talkobuf[talkobuflen++] = ch - KEY_UP + 1;
				if (talkobuflen == 80)
					talkflush();
			} else if (ch == Ctrl('E')) {
				for (i2 = 0; i2 <= 10; i2++) {
					talkobuf[talkobuflen++] = '\r';
					talkflush();
					do_talk_char(&mywin, '\r');
				}
			} else if (ch == Ctrl('P')
				   && USERPERM(currentuser, PERM_BASIC)) {
				t_pager();
				update_utmp();
				update_endline();
			}
		}
	}
	add_io(0, 0);
	talkflush();
	signal(SIGALRM, SIG_IGN);
	add_flush(NULL);
	modify_user_mode(previous_mode);
//#ifdef TALK_LOG
	/* edwardc.990106 聊天纪录 */
	mywords[mlen] = '\0';
	itswords[ilen] = '\0';
	if (mywords[0] != '\0')
		do_log(mywords, 1);
	if (itswords[0] != '\0')
		do_log(itswords, 2);

	now = time(0);
	sprintf(talkbuf, "\n\033[1;34m通话结束, 时间: %s \033[m\n",
		Cdate(&now));
	write(talkrec, talkbuf, strlen(talkbuf));
	close(talkrec);

	sethomefile(genbuf, currentuser->userid, "talklog");
	if (!file_isfile(genbuf))
		return 0;

	getdata(23, 0, "是否寄回聊天纪录 [Y/n]: ", ans, 2, DOECHO, YEA);

	switch (ans[0]) {
	case 'n':
	case 'N':
		break;
	default:
		sethomefile(talkbuf, currentuser->userid, "talklog");
		sprintf(mywords, "跟 %s 的聊天记录 [%s]", partner,
			Cdate(&now) + 4);
		{
			char temp[STRLEN];
			strncpy(temp, save_title, STRLEN);
			system_mail_file(talkbuf, currentuser->userid, mywords, currentuser->userid);
			strncpy(save_title, temp, STRLEN);
		}
	}
	sethomefile(talkbuf, currentuser->userid, "talklog");
	unlink(talkbuf);
//#endif
	return 0;
}

/*Add by SmallPig*/
int
seek_in_file(filename, seekstr)
char filename[STRLEN], seekstr[STRLEN];
{
	FILE *fp;
	char buf[STRLEN];
	char *namep;

	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, STRLEN, fp) != NULL) {
		namep = (char *) strtok(buf, ": \n\r\t");
		if (namep != NULL && strcasecmp(namep, seekstr) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

int
listfilecontent(fname)
char *fname;
{
	FILE *fp;
	int y = 3, cnt = 0;
	char u_buf[20], line[STRLEN], *nick;
	int display;

	move(y, 0);
	CreateNameList();
	if ((fp = fopen(fname, "r")) == NULL) {
		prints("(none)\n");
		return 0;
	}
	display = 1;
	while (fgets(genbuf, STRLEN, fp) != NULL) {
		if (y >= t_lines - 1) {
			if (askyn("是否继续观看下一屏?", 1, 1)) {
				move(3, 0);
				clrtobot();
				y = 3;
			} else {
				y = 3;
				display = 0;
			}
		}
		if (strtok(genbuf, " \n\r\t") == NULL)
			continue;
		strncpy(u_buf, genbuf, 20);
		u_buf[19] = '\0';
		AddNameList(u_buf);
		if (!display) {
			cnt++;
			continue;
		}
		nick = (char *) strtok(NULL, "\n\r\t");
		if (nick != NULL) {
			while (*nick == ' ')
				nick++;
			if (*nick == '\0')
				nick = NULL;
		}
		if (nick == NULL) {
			strcpy(line, u_buf);
		} else {
			sprintf(line, "%-12s %s", u_buf, nick);
		}
		if (strlen(line) > 78)
			line[78] = '\0';
		prints("%s", line);
		cnt++;
		y++;
		move(y, 0);
	}
	fclose(fp);
	if (cnt == 0)
		prints("(none)\n");
	return cnt;
}

int
addtofile(filename, str)
char filename[STRLEN], str[256];
{
	FILE *fp;
	int rc;

	if ((fp = fopen(filename, "a")) == NULL)
		return -1;
	flock(fileno(fp), LOCK_EX);
	rc = fprintf(fp, "%s\n", str);
	fflush(fp);
	flock(fileno(fp), LOCK_UN);
	fclose(fp);
	return (rc == EOF ? -1 : 1);
}

int
addtooverride(uident)
char *uident;
{
	struct override tmp;
	int n;
	char buf[STRLEN];
	char desc[5];

	memset(&tmp, 0, sizeof (tmp));
	if (friendflag) {
		setuserfile(buf, "friends");
		n = MAXFRIENDS;
		strcpy(desc, "好友");
	} else {
		setuserfile(buf, "rejects");
		n = MAXREJECTS;
		strcpy(desc, "坏人");
	}
	if (get_num_records(buf, sizeof (struct override)) >= n) {
		move(t_lines - 2, 0);
		clrtoeol();
		prints("抱歉，本站目前仅可以设定 %d 个%s, 请按任何件继续...", n,
		       desc);
		igetkey();
		move(t_lines - 2, 0);
		clrtoeol();
		return -1;
	} else {
		if (friendflag) {
			if (myfriend(getuser(uident, NULL))) {
				prints("%s 已在好友名单", uident);
				//show_message(buf);
				return -1;
			}
		} else
		    if (search_record
			(buf, &tmp, sizeof (tmp), (void *) cmpfnames,
			 uident) > 0) {
			prints("%s 已在坏人名单", uident);
			//show_message(buf);
			return -1;
		}
	}
	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS
	    && uinfo.mode != FRIEND)
		n = 2;
	else
		n = t_lines - 2;

	strcpy(tmp.id, uident);
	move(n, 0);
	clrtoeol();
	sprintf(genbuf, "请输入给%s【%s】的说明: ", desc, tmp.id);
	getdata(n, 0, genbuf, tmp.exp, 40, DOECHO, YEA);

	n = append_record(buf, &tmp, sizeof (struct override));
	if (n != -1)
		(friendflag) ? getfriendstr() : getrejectstr();
	else
		errlog("append override error");
	return n;
}

int
del_from_file(filename, str)
char filename[STRLEN], str[STRLEN];
{
	FILE *fp, *nfp;
	int deleted = NA, len;
	char fnnew[STRLEN];

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	sprintf(fnnew, "%s.%d", filename, getpid());
	if ((nfp = fopen(fnnew, "w")) == NULL) {
		fclose(fp);
		return -1;
	}
	len = strlen(str);
	while (fgets(genbuf, STRLEN, fp) != NULL) {
		if ((strncmp(genbuf, str, len) == 0)
		    && (genbuf[len] <= 32 || strchr(": \n\r\t", genbuf[len])))
			deleted = YEA;
		else		/*if (*genbuf > ' ') */
			fputs(genbuf, nfp);
	}
	fclose(fp);
	fclose(nfp);
	if (!deleted)
		return -1;
	return (rename(fnnew, filename) + 1);
}

int
deleteoverride(uident, filename)
char *uident;
char *filename;
{
	int deleted;
	struct override fh;
	char buf[STRLEN];

	setuserfile(buf, filename);
	deleted =
	    search_record(buf, &fh, sizeof (fh), (void *) cmpfnames, uident);
	if (deleted > 0) {
		if (delete_record(buf, sizeof (fh), deleted) != -1) {
			(friendflag) ? getfriendstr() : getrejectstr();
		} else {
			deleted = -1;
			errlog("delete override error");
		}
	}
	return (deleted > 0) ? 1 : -1;
}

static int
override_title()
{
	char desc[5];

	if (chkmail())
		strcpy(genbuf, "[您有信件]");
	else
		strcpy(genbuf, MY_BBS_NAME);
	if (friendflag) {
		showtitle("[编辑好友名单]", genbuf);
		strcpy(desc, "好友");
	} else {
		showtitle("[编辑坏人名单]", genbuf);
		strcpy(desc, "坏人");
	}
	prints
	    (" [\033[1;32m←\033[m,\033[1;32me\033[m] 离开 [\033[1;32mh\033[m] 求助 [\033[1;32m→\033[m,\033[1;32mRtn\033[m] %s说明档 [\033[1;32m↑\033[m,\033[1;32m↓\033[m] 选择 [\033[1;32ma\033[m] 增加%s [\033[1;32md\033[m] 删除%s\n",
	     desc, desc, desc);
	prints
	    ("\033[1;44m 编号  %s代号      %s说明                                                   \033[m\n",
	     desc, desc);
	return 0;
}

static char *
override_doentry(ent, fh, buf)
int ent;
struct override *fh;
char buf[512];
{
	sprintf(buf, " %4d  %-12.12s  %s", ent, fh->id, fh->exp);
	return buf;
}

static int
override_edit(ent, fh, direc)
int ent;
struct override *fh;
char *direc;
{
	struct override nh;
	char buf[STRLEN / 2];
	int pos;

	pos =
	    search_record(direc, &nh, sizeof (nh), (void *) cmpfnames, fh->id);
	move(t_lines - 2, 0);
	clrtoeol();
	if (pos > 0) {
		sprintf(buf, "请输入 %s 的新%s说明: ", fh->id,
			(friendflag) ? "好友" : "坏人");
		getdata(t_lines - 2, 0, buf, nh.exp, 40, DOECHO, NA);
	}
	if (substitute_record(direc, &nh, sizeof (nh), pos) < 0)
		errlog("Override files subs err");
	move(t_lines - 2, 0);
	clrtoeol();
	return NEWDIRECT;
}

int
override_add()
{
	char uident[13];

	clear();
	move(1, 0);
	usercomplete("请输入要增加的代号: ", uident);
	if (uident[0] != '\0') {
		if (getuser(uident, NULL) <= 0) {
			move(2, 0);
			prints("错误的使用者代号...");
			pressanykey();
			return FULLUPDATE;
		} else if (addtooverride(uident) == 0)
			prints("\n把 %s 加入%s名单中...", uident,
			       (friendflag) ? "好友" : "坏人");
	}
	pressanykey();
	return FULLUPDATE;
}

static int
override_dele(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	char buf[STRLEN];
	char desc[5];
	char fname[10];
	int deleted = NA;

	if (friendflag) {
		strcpy(desc, "好友");
		strcpy(fname, "friends");
	} else {
		strcpy(desc, "坏人");
		strcpy(fname, "rejects");
	}
	saveline(t_lines - 2, 0, NULL);
	move(t_lines - 2, 0);
	sprintf(buf, "是否把【%s】从%s名单中去除", fh->id, desc);
	if (askyn(buf, NA, NA) == YEA) {
		move(t_lines - 2, 0);
		clrtoeol();
		if (deleteoverride(fh->id, fname) == 1) {
			prints("已从%s名单中移除【%s】,按任何键继续...", desc,
			       fh->id);
			deleted = YEA;
		} else
			prints("找不到【%s】,按任何键继续...", fh->id);
	} else {
		move(t_lines - 2, 0);
		clrtoeol();
		prints("取消删除%s...", desc);
	}
	igetkey();
	move(t_lines - 2, 0);
	clrtoeol();
	saveline(t_lines - 2, 1, NULL);
	return (deleted) ? PARTUPDATE : DONOTHING;
}

static int
friend_edit(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	friendflag = YEA;
	return override_edit(ent, fh, direct);
}

static int
friend_add(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	friendflag = YEA;
	return override_add();
}

static int
friend_dele(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	friendflag = YEA;
	return override_dele(ent, fh, direct);
}

static int
friend_mail(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	if (!USERPERM(currentuser, PERM_POST))
		return DONOTHING;
	m_send(fh->id);
	return FULLUPDATE;
}

static int
friend_query(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	int ch;

	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints
	    ("\033[0;1;44;31m[读取好友说明档]\033[33m 寄信给好友 m │ 结束 Q,← │上一位 ↑│下一位 <Space>,↓      \033[m");
	ch = egetch();
	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case 'm':
	case 'M':
		m_send(fh->id);
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

static int
friend_help()
{
	show_help("help/friendshelp");
	return FULLUPDATE;
}

static int
reject_edit(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	friendflag = NA;
	return override_edit(ent, fh, direct);
}

static int
reject_add(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	friendflag = NA;
	return override_add();
}

static int
reject_dele(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	friendflag = NA;
	return override_dele(ent, fh, direct);
}

static int
reject_query(ent, fh, direct)
int ent;
struct override *fh;
char *direct;
{
	int ch;

	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints
	    ("\033[0;1;44;31m[读取坏人说明档]\033[33m 结束 Q,← │上一位 ↑│下一位 <Space>,↓                      \033[m");
	ch = egetch();
	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

static int
reject_help()
{
	show_help("help/rejectshelp");
	return FULLUPDATE;
}

void
t_friend()
{
	char buf[STRLEN];

	friendflag = YEA;
	setuserfile(buf, "friends");
	i_read(GMENU, buf, override_title, (void *) override_doentry,
	       friend_list, sizeof (struct override));
	clear();
	return;
}

void
t_reject()
{
	char buf[STRLEN];

	friendflag = NA;
	setuserfile(buf, "rejects");
	i_read(GMENU, buf, override_title, (void *) override_doentry,
	       reject_list, sizeof (struct override));
	clear();
	return;
}

struct user_info *
t_search(sid, pid, invisible_check)
char *sid;
int pid;
int invisible_check;
{
	int i;
	struct user_info *cur, *tmp = NULL;

	for (i = 0; i < MAXACTIVE; i++) {
		cur = &(bbsinfo.utmpshm->uinfo[i]);
		if (!cur->active || !cur->pid)
			continue;
		if (!strcasecmp(cur->userid, sid)) {
			if ((pid == 0))
				return (isreject(cur, &uinfo)
					|| (invisible_check
					    && (cur->invisible
						&& !USERPERM(currentuser,
							     (PERM_SEECLOAK |
							      PERM_SYSOP))))) ?
				    NULL : cur;
			tmp = cur;
			if (pid == cur->pid)
				break;
		}
	}
	/* by gluon
	   if(tmp != NULL)
	   {
	   if (tmp->invisible && !USERPERM(currentuser, PERM_SEECLOAK|PERM_SYSOP))
	   return NULL;
	   }
	 */
	return isreject(cur, &uinfo) ? NULL : tmp;
}

static int
cmpfuid(a, b)
unsigned *a, *b;
{
	return *a - *b;
}

int
getfriendstr()
{
	int i;
	struct override *tmp;

	memset(uinfo.friend, 0, sizeof (uinfo.friend));
	setuserfile(genbuf, "friends");
	uinfo.fnum = get_num_records(genbuf, sizeof (struct override));
	if (uinfo.fnum <= 0)
		return -1;
	uinfo.fnum = (uinfo.fnum >= MAXFRIENDS) ? MAXFRIENDS : uinfo.fnum;
	tmp = (struct override *) calloc(sizeof (struct override), uinfo.fnum);
	get_records(genbuf, tmp, sizeof (struct override), 1, uinfo.fnum);
	for (i = 0; i < uinfo.fnum; i++) {
		uinfo.friend[i] = getuser(tmp[i].id, NULL);
		if (uinfo.friend[i] == 0)
			deleteoverride(tmp[i].id, "friends");
		/* 顺便删除已不存在帐号的好友 */
	}
	free(tmp);
	qsort(&uinfo.friend, uinfo.fnum, sizeof (uinfo.friend[0]),
	      (void *) cmpfuid);
	update_ulist(&uinfo, utmpent);
	return 0;
}

int
getrejectstr()
{
	int nr, i;
	struct override *tmp;

	memset(uinfo.reject, 0, sizeof (uinfo.reject));
	setuserfile(genbuf, "rejects");
	nr = get_num_records(genbuf, sizeof (struct override));
	if (nr <= 0)
		return -1;
	nr = (nr >= MAXREJECTS) ? MAXREJECTS : nr;
	tmp = (struct override *) calloc(sizeof (struct override), nr);
	get_records(genbuf, tmp, sizeof (struct override), 1, nr);
	for (i = 0; i < nr; i++) {
		uinfo.reject[i] = getuser(tmp[i].id, NULL);
		if (uinfo.reject[i] == 0)
			deleteoverride(tmp[i].id, "rejects");
	}
	free(tmp);
	return 0;
}

int
wait_friend()
{
	FILE *fp;
	int tuid;
	char buf[STRLEN];
	char uid[13];

	modify_user_mode(WFRIEND);
	clear();
	move(1, 0);
	usercomplete("请输入使用者代号以加入系统的寻人名册: ", uid);
	if (uid[0] == '\0') {
		clear();
		return 0;
	}
	if (!(tuid = getuser(uid, NULL))) {
		move(2, 0);
		prints("\033[1m不正确的使用者代号\033[m\n");
		pressanykey();
		clear();
		return -1;
	}
	sprintf(buf, "你确定要把 \033[1m%s\033[m 加入系统寻人名单中", uid);
	move(2, 0);
	if (askyn(buf, YEA, NA) == NA) {
		clear();
		return -1;
	}
	if ((fp = fopen("friendbook", "a")) == NULL) {
		prints("系统的寻人名册无法开启，请通知站长...\n");
		pressanykey();
		return -1;
	}
	sprintf(buf, "%d@%s", tuid, currentuser->userid);
	if (!seek_in_file("friendbook", buf))
		fprintf(fp, "%s\n", buf);
	fclose(fp);
	move(3, 0);
	prints
	    ("已经帮你加入寻人名册中，\033[1m%s\033[m 上站系统一定会通知你...\n",
	     uid);
	pressanykey();
	clear();
	return 0;
}

//#ifdef TALK_LOG
/* edwardc.990106 分别为两位聊天的人作纪录 */
/* -=> 自己说的话 */
/* --> 对方说的话 */

static void
do_log(char *msg, int who)
{
/* Sonny.990514 试著抓 overflow 的问题... */
/* Sonny.990606 overflow 问题解决. buf[100] 是正确的. 参考 man sprintf() */
	time_t now;
	char buf[128];
	now = time(0);
	msg[79] = 0;
	if (msg[strlen(msg) - 1] == '\n')
		msg[strlen(msg) - 1] = 0;
	if (strlen(msg) < 1 || msg[0] == '\r' || msg[0] == '\n')
		return;

	/* 只帮自己做 */
	sethomefile(buf, currentuser->userid, "talklog");

	if (!file_isfile(buf) || talkrec == -1) {
		talkrec = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0644);
		buf[127] = 0;
		snprintf(buf, 127,
			 "\033[1;32m与 %s 的情话绵绵, 日期: %s \033[m\n",
			 save_page_requestor, Cdate(&now));
		write(talkrec, buf, strlen(buf));
		sprintf(buf,
			"\t颜色分别代表: \033[1;33m%s\033[m \033[1;36m%s\033[m \n\n",
			currentuser->userid, partner);
		write(talkrec, buf, strlen(buf));
	}
	if (who == 1) {		/* 自己说的话 */
		sprintf(buf, "\033[1;33m-=> %s \033[m\n", msg);
		write(talkrec, buf, strlen(buf));
	} else if (who == 2) {	/* 别人说的话 */
		sprintf(buf, "\033[1;36m--> %s \033[m\n", msg);
		write(talkrec, buf, strlen(buf));
	}
}

//#endif
static char *
Cdate(clock)
time_t *clock;
{
	static char foo[22];
	struct tm *mytm = localtime(clock);

	strftime(foo, 22, "%m/%d/%Y %T %a", mytm);
	return (foo);
}
