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
#include "common.h"
// modified by yldsd.
// #define BBS_PAGESIZE    (19)
#define BBS_PAGESIZE (t_lines - 4)
#define refreshtime     (30)
extern time_t login_start_time;
extern int can_R_endline;
int (*func_list_show) ();
time_t update_time = 0;
int freshmode = 0;
int toggle1 = 0, toggle2 = 0;
int friendmode = 0;
int usercounter;
int range, page, readplan, num;
int sortmode = 0;
struct user_info **user_record;
struct userec *user_data;

/* add by KCN 1998.11 */
int friendmode1;

static int friend_search(unsigned uid, struct user_info *uentp, int tblsize);
static int UseronlineSearch(int curr_num, int offset);
static int IDSearch(char query[STRLEN], int curr_num, int offset);
static int IPSearch(char query[20], int curr_num, int offset);
static int NickSearch(char query[STRLEN], int curr_num, int offset);
static void print_title(void);
static void update_data(void);
static int print_user_info_title(void);
//static void swap_user_record(int a, int b);
static void change_sortmode(int mode);
static int cmpuinfo(struct user_info **a, struct user_info **b);
static void sort_user_record(int left, int right);
static int fill_userlist(void);
static int cfriendname(struct override *t1, struct override *t2);
static int do_userlist(void);
static int show_userlist(void);
static int deal_key(int ch, int allnum, int pagenum);
static int do_query(int star, int curr);
static char msgchar(struct user_info *uin);
static char pagerchar(int friend, int pager);
static char *idle_str(struct user_info *uent);
static int num_visible_users();

static int
friend_search(uid, uentp, tblsize)
unsigned uid;
struct user_info *uentp;
int tblsize;
{
	int hi, low, mid;
	//int cmp;

	if (uid == 0) {
		return NA;
	}
	hi = tblsize - 1;
	low = 0;
	while (low <= hi) {
		mid = (low + hi) / 2;
		//cmp=uentp->friend[mid] - uid;
		if (uentp->friend[mid] == uid) {
			return YEA;
		}
		if (uentp->friend[mid] > uid)
			hi = mid - 1;
		else
			low = mid + 1;
	}
	return NA;
}

static int
UseronlineSearch(curr_num, offset)
int curr_num;
int offset;
{
	static char method[2], queryID[IDLEN + 2], queryIP[20],
	    queryNick[NAMELEN + 2];
	char ans[STRLEN + 1], pmt[STRLEN];
	strcpy(ans, method);
	sprintf(pmt, "查找方式:(A)ID (B)呢称 (C)IP [%s]:", ans);
	move(t_lines - 1, 0);
	clrtoeol();
	getdata(t_lines - 1, 0, pmt, ans, 2, DOECHO, YEA);
	ans[0] = toupper(ans[0]);
	if (!((ans[0] >= 'A' && ans[0] <= 'C') || ans[0] == '\0'))
		return curr_num;
	if (ans[0] != '\0')
		strcpy(method, ans);
	switch (method[0]) {
	case 'A':
		strcpy(ans, queryID);
		sprintf(pmt, "搜寻%s的ID [%s]: ",
			offset > 0 ? "往后来" : "往先前", ans);
		move(t_lines - 1, 0);
		clrtoeol();
		getdata(t_lines - 1, 0, pmt, ans, IDLEN + 1, DOECHO, YEA);
		if (ans[0] != '\0')
			strcpy(queryID, ans);
		return IDSearch(queryID, curr_num, offset);
	case 'B':
		strcpy(ans, queryNick);
		sprintf(pmt, "搜寻%s的呢称[%s]: ", offset > 0 ? "往后来"
			: "往先前", ans);
		move(t_lines - 1, 0);
		clrtoeol();
		getdata(t_lines - 1, 0, pmt, ans, NAMELEN + 1, DOECHO, YEA);
		if (ans[0] != '\0')
			strcpy(queryNick, ans);
		return NickSearch(queryNick, curr_num, offset);
	case 'C':
		strcpy(ans, queryIP);
		sprintf(pmt, "%s搜寻来自[%s]的ID: ", offset > 0 ? "往后来"
			: "往先前", ans);
		move(t_lines - 1, 0);
		clrtoeol();
		getdata(t_lines - 1, 0, pmt, ans, 17, DOECHO, YEA);
		if (ans[0] != '\0')
			strcpy(queryIP, ans);
		return IPSearch(queryIP, curr_num, offset);
	default:
		return curr_num;
	}
}
static int
IDSearch(query, curr_num, offset)
char query[STRLEN];
int curr_num;
int offset;
{
	int i;
	if (query[0] == '\0')
		return curr_num;
	if (offset > 0) {
		for (i = curr_num + 1; i < range; i++) {
			if (!strncasecmp
			    (user_record[i]->userid, query, strlen(query)))
				return i;
		}
	} else if (offset < 0) {
		for (i = curr_num - 1; i >= 0; i--) {
			if (!strncasecmp
			    (user_record[i]->userid, query, strlen(query)))
				return i;
		}
	}
	return curr_num;
}

static int
IPSearch(query, curr_num, offset)
char query[20];
int curr_num;
int offset;
{
	int i;
	if (query[0] == '\0')
		return curr_num;
	if (offset > 0) {
		for (i = curr_num + 1; i < range; i++) {
			if (!cmpIP(user_record[i]->from, query))
				return i;
		}
	} else if (offset < 0) {
		for (i = curr_num - 1; i >= 0; i--) {
			if (!cmpIP(user_record[i]->from, query))
				return i;
		}
	}
	return curr_num;
}

static int
NickSearch(query, curr_num, offset)
char query[STRLEN];
int curr_num;
int offset;
{
	int i;
	if (query[0] == '\0')
		return curr_num;
	if (offset > 0) {
		for (i = curr_num + 1; i < range; i++) {
			if (!strncmp
			    (user_record[i]->username, query, strlen(query)))
				return i;
		}
	} else if (offset < 0) {
		for (i = curr_num - 1; i >= 0; i--) {
			if (!strncmp
			    (user_record[i]->username, query, strlen(query)))
				return i;
		}
	}
	return curr_num;
}

int
myfriend(uid)
unsigned uid;
{
	return friend_search(uid, &uinfo, uinfo.fnum);
}

int
hisfriend(uentp)
struct user_info *uentp;
{
	if (uentp == NULL)
		return NA;
	return friend_search(uinfo.uid, uentp, uentp->fnum);
}

static void
print_title()
{

	char buf[20];
	switch (sortmode) {
	case 0:
		sprintf(buf, "%s %s",
			(friendmode) ? "[好朋友列表]" : "[使用者列表]",
			"[普通]");
		break;
	case 1:
		sprintf(buf, "%s %s",
			(friendmode) ? "[好朋友列表]" : "[使用者列表]",
			"[字母]");
		break;
	case 2:
		sprintf(buf, "%s %s",
			(friendmode) ? "[好朋友列表]" : "[使用者列表]",
			"[网址]");
		break;
	case 3:
		sprintf(buf, "%s %s",
			(friendmode) ? "[好朋友列表]" : "[使用者列表]",
			"[动态]");
		break;
	}
	docmdtitle(buf,
		   " 聊天[\033[1;32mt\033[m] 寄信[\033[1;32mm\033[m] 送讯息[\033[1;32ms\033[m] 加,减朋友[\033[1;32mo\033[m,\033[1;32md\033[m] 看说明档[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 切换模式 [\033[1;32mc\033[m] 求救[\033[1;32mh\033[m]");
}

static void
update_data()
{
	if (readplan == YEA)
		return;
	if (time(0) >= update_time + refreshtime - 1) {
		freshmode = 1;
	}
	signal(SIGALRM, (void *) update_data);
	alarm(refreshtime);
	return;
}

static int
print_user_info_title()
{
	char title_str[512];
	char *field_2;

	move(2, 0);
	clrtoeol();
	field_2 = "使用者昵称";
	sprintf(title_str,
		"\033[1;44m%s%-12.12s %-22.22s %-16.16s%c %c %-10.10s %5s\033[m\n",
		" 编号  ", "使用者代号", field_2, "来自", 'P',
		/*((USERPERM(currentuser, PERM_SYSOP|PERM_SEECLOAK)) ? 'C' : ' ') */
		'M', "动态", "时:分");
	prints("%s", title_str);
	return 0;
}

static void
change_sortmode(mode)
int mode;
{
	char genbuf[3];
	if (mode) {
		sortmode++;
		if (sortmode > 3)
			sortmode = 0;
	} else {
		getdata(t_lines - 1, 0,
			"用户排序方式(A)普通 (B)字母 (C)网址 (D)动态 [A]: ",
			genbuf, 2, DOECHO, 1);
		if (genbuf[0] == 'B' || genbuf[0] == 'b')
			sortmode = 1;
		else if (genbuf[0] == 'C' || genbuf[0] == 'c')
			sortmode = 2;
		else if (genbuf[0] == 'D' || genbuf[0] == 'd')
			sortmode = 3;
		else
			sortmode = 0;
	}
	update_time = 0;
}

static int
cmpuinfo(struct user_info **a, struct user_info **b)
{
	switch (sortmode) {
	case 2:
		return cmpIP((*a)->from, (*b)->from);
	case 3:
		return (*a)->mode - (*b)->mode;
	default:
		return strcasecmp((*a)->userid, (*b)->userid);
	}
}

static void
sort_user_record(int left, int right)
{
	if (sortmode)
		qsort(&user_record[left], right - left,
		      sizeof (struct user_info *), (void *) cmpuinfo);
}

static int
fill_userlist()
{
	int i, i2, j, uent, testreject, uid;
	int back_sort_mode;
	struct user_info *up;
	int guestuid;

	i2 = 0;
	guestuid = getuser("guest", NULL);
	if (friendmode) {
		for (i = 0; i < uinfo.fnum; i++) {
			if (guestuid > 0 && uinfo.friend[i] == guestuid)
				continue;
			up = NULL;
			uid = uinfo.friend[i];
			testreject = 0;
			if (uid <= 0 || uid > MAXUSERS)
				continue;
			for (j = 0; j < 6; j++) {
				uent = bbsinfo.uindexshm->user[uid - 1][j];
				if (uent <= 0)
					continue;
				up = &(bbsinfo.utmpshm->uinfo[uent - 1]);
				if (!up->active || !up->pid || up->uid != uid)
					continue;
				if (!testreject) {
					if (isreject(up, &uinfo))
						break;
					testreject = 1;
				}
				if (bbsinfo.utmpshm->uinfo[uent - 1].invisible
				    && !USERPERM(currentuser,
						 (PERM_SYSOP | PERM_SEECLOAK)))
					continue;
				user_record[i2] = up;
				i2++;
			}
		}
		back_sort_mode = sortmode;
		if (sortmode == 0)
			sortmode = 1;
		sort_user_record(0, i2);
		sortmode = back_sort_mode;
		range = i2;
	} else {
		for (i = 0; i < MAXACTIVE; i++) {
			if (!bbsinfo.utmpshm->uinfo[i].active
			    || !bbsinfo.utmpshm->uinfo[i].pid
			    || isreject(&(bbsinfo.utmpshm->uinfo[i]), &uinfo)) {
				continue;
			}
			if (!
			    (USERPERM
			     (currentuser, (PERM_SYSOP | PERM_SEECLOAK)))
&& bbsinfo.utmpshm->uinfo[i].invisible) {
				continue;
			}
			user_record[i2] = &(bbsinfo.utmpshm->uinfo[i]);
			i2++;
		}
		sort_user_record(0, i2);
		range = i2;
		limit_cpu();
	}
	return i2 == 0 ? -1 : 1;
}

static int
cfriendname(t1, t2)
struct override *t1;
struct override *t2;
{
	return !strcasecmp(t1->id, t2->id);
}

static int
do_userlist()
{
	int i;
	char user_info_str[STRLEN * 2] /*,pagec */ ;
	int override;
	struct user_info *uentp;
	struct override t1, t2;
	char overridefile[256];

	setuserfile(overridefile, "friends");
	move(3, 0);
	print_user_info_title();

	for (i = 0; i < BBS_PAGESIZE && i + page < range && i + page >= 0; i++) {
		uentp = user_record[i + page];
		override = friendmode;
		if (readplan == YEA) {
			return 0;
		}
		if (uentp == NULL)
			continue;
		if (override && friendmode1) {
			strncpy(t2.id, uentp->userid, IDLEN);
			t2.id[IDLEN] = 0;
			t1.exp[0] = 0;
			search_record(overridefile, &t1, sizeof (t1),
				      (void *) cfriendname, &t2);
			sprintf(user_info_str,
				" %4d%2s%s%-12.12s%s %-22.22s %s%-16.16s%s%c %c %s%-10.10s\033[m %5.5s\n",
				i + 1 + page, (override) ? "□" : "",
				(override) ? "\033[1;32m" : "",
				uentp->userid, (override) ? "\033[m" : "",
				(t1.exp[0] ==
				 0) ? uentp->username : t1.exp,
				(uentp->pid ==
				 1) ? "\033[35m" : ((uentp->isssh ==
						     1) ? "\033[32m" :
						    ""), uentp->from,
				(uentp->pid == 1)
				|| (uentp->isssh == 1) ? "\033[0m" : "",
				pagerchar(hisfriend(uentp),
					  uentp->pager), msgchar(uentp),
				(uentp->invisible ==
				 YEA) ? "\033[1;36m" :
				ModeColor(uentp->mode), ModeType(uentp->mode),
				idle_str(uentp));
		} else
			sprintf(user_info_str,
				" %4d%2s%s%-12.12s%s %-22.22s %s%-16.16s%s%c %c %s%-10.10s\033[m %5.5s\n",
				i + 1 + page, (override) ? "□" : "",
				(override) ? "\033[1;32m" : "",
				uentp->userid, (override) ? "\033[m" : "",
				uentp->username,
				(uentp->pid ==
				 1) ? "\033[35m" : ((uentp->isssh ==
						     1) ? "\033[32m" :
						    ""), uentp->from,
				(uentp->pid == 1)
				|| (uentp->isssh == 1) ? "\033[0m" : "",
				pagerchar(hisfriend(uentp),
					  uentp->pager), msgchar(uentp),
				(uentp->invisible ==
				 YEA) ? "\033[1;36m" :
				ModeColor(uentp->mode), ModeType(uentp->mode),
				idle_str(uentp));
		clrtoeol();
		prints("%s", user_info_str);
	}
	return 0;
}

static int
show_userlist()
{
	now_t = time(NULL);
	if (update_time + refreshtime < now_t) {
		fill_userlist();
		now_t = time(NULL);
		update_time = now_t;
	}
	if (range == 0) {
		move(2, 0);
		prints("没有使用者（朋友）在列表中...\n");
		clrtobot();
		if (friendmode) {
			move(BBS_PAGESIZE + 3, 0);
			if (askyn("是否转换成使用者模式", YEA, NA) == YEA) {
				page = -1;
				friendmode = NA;
				modify_user_mode(LUSERS);
				fill_userlist();
				return 1;
			}
		} else
			pressanykey();
		return -1;
	}
	do_userlist();
	clrtobot();
	return 1;
}

static int
deal_key(ch, allnum, pagenum)
char ch;
int allnum, pagenum;
{
	char buf[STRLEN];
	char tempuser[20];
	static int msgflag;
	extern int friendflag;
	char desc[5];

	if (msgflag == YEA) {
		show_message(NULL);
		msgflag = NA;
	}
	switch (ch) {
/* add by zhoulin 98.11*/
	case 'w':
	case 'W':
/*                        if (!friendmode) 
                             return 0;*/
		friendmode1 = ~friendmode1 & 1;
		break;
	case 'f':
	case 'F':
/*                        if(strcmp(currentuser->userid,user_record[allnum]->userid))
                                return 0;*/
		buf[0] = 0;	//add by ylsdd
		getdata(BBS_PAGESIZE + 3, 0, "变换昵称: ", buf, NAMELEN, DOECHO,
			NA);
		if (buf[0] != '\0') {
			strcpy(uinfo.username, buf);
		}
		break;
	case 'k':
	case 'K':
		if (!USERPERM(currentuser, PERM_SYSOP)
		    && strcmp(currentuser->userid, user_record[allnum]->userid))
			return 1;
		sprintf(buf, "你要把 %s 踢出站外吗",
			user_record[allnum]->userid);
		strcpy(tempuser, user_record[allnum]->userid);
		move(BBS_PAGESIZE + 3, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (!strcmp(user_record[allnum]->userid, tempuser)
		    && kick_user(user_record[allnum]) == 1) {
			sprintf(buf, "%s 已被踢出站外",
				user_record[allnum]->userid);
		} else {
			sprintf(buf, "%s 无法踢出站外",
				user_record[allnum]->userid);
		}
		msgflag = YEA;
		break;
	case 'h':
	case 'H':
		show_help("help/userlisthelp");
		break;
	case 't':
	case 'T':
		if (!USERPERM(currentuser, PERM_PAGE))
			return 1;
		if (strcmp(currentuser->userid, user_record[allnum]->userid))
			ttt_talk(user_record[allnum]);
		else
			return 1;
		break;
	case 'm':
	case 'M':
		if (!USERPERM(currentuser, PERM_POST))
			return 1;
		m_send(user_record[allnum]->userid);
		break;
	case 'c':
	case 'C':
		if (friendmode)
			friendmode = NA;
		else
			friendmode = YEA;
		update_time = 0;
		break;
	case 's':
	case 'S':
		if (!USERPERM(currentuser, PERM_PAGE))
			return 1;
		if (!canmsg(user_record[allnum])) {
			sprintf(buf, "%s 已经关闭讯息呼叫器",
				user_record[allnum]->userid);
			msgflag = YEA;
			break;
		}
		do_sendmsg(user_record[allnum]->userid, user_record[allnum],
			   NULL, 2, user_record[allnum]->pid);
		break;
	case 'o':
	case 'O':
	case 'r':
	case 'R':
		if (ch == 'o' || ch == 'O') {
			friendflag = YEA;
			strcpy(desc, "好友");
		} else {
			friendflag = NA;
			strcpy(desc, "坏人");
		}
		if (!strcmp("guest", currentuser->userid))
			return 0;
		sprintf(buf, "确定要把 %s 加入%s名单吗",
			user_record[allnum]->userid, desc);
		move(BBS_PAGESIZE + 3, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (addtooverride(user_record[allnum]->userid)
		    == -1) {
			sprintf(buf, "%s 已在%s名单",
				user_record[allnum]->userid, desc);
		} else {
			sprintf(buf, "%s 列入%s名单",
				user_record[allnum]->userid, desc);
		}
		msgflag = YEA;
		break;
	case 'd':
	case 'D':
		sprintf(buf, "确定要把 %s 从好友名单删除吗",
			user_record[allnum]->userid);
		move(BBS_PAGESIZE + 3, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (deleteoverride(user_record[allnum]->userid, "friends")
		    == -1) {
			sprintf(buf, "%s 本来就不在朋友名单中",
				user_record[allnum]->userid);
		} else {
			sprintf(buf, "%s 已从朋友名单移除",
				user_record[allnum]->userid);
		}
		msgflag = YEA;
		break;
	case 'a':
	case 'A':
		change_sortmode(0);
		break;
	case KEY_TAB:
		change_sortmode(1);
		break;
	case '/':		//down search ID
		num = UseronlineSearch(num, 1);
		break;
	case '?':		//up search ID
		num = UseronlineSearch(num, -1);
		break;
/*        case 'Y':
	        if (USERPERM(currentuser, PERM_CLOAK))
			x_cloak();
		break;								
*/
	default:
		return 0;
	}
	if (friendmode)
		modify_user_mode(FRIEND);
	else
		modify_user_mode(LUSERS);
	if (readplan == NA) {
		print_title();
		clrtobot();
		if (show_userlist() == -1)
			return -1;
		update_endline();
		if (msgflag == YEA) {
			show_message(buf);
			msgflag = NA;
		}
	}
	return 1;
}

void
setlistrange(i)
int i;
{
	range = i;
}

static int
do_query(star, curr)
int star, curr;
{
	if (user_record[curr] != NULL) {
		clear();
		t_query(user_record[curr]->userid);
		move(t_lines - 1, 0);
		prints
		    ("\033[0;1;37;44m聊天[\033[1;32mt\033[37m] 寄信[\033[1;32mm\033[37m] 送讯息[\033[1;32ms\033[37m] 加,减朋友[\033[1;32mo\033[37m,\033[1;32md\033[37m] 选择使用者[\033[1;32m↑\033[37m,\033[1;32m↓\033[37m] 切换模式 [\033[1;32mc\033[37m] 求救[\033[1;32mh\033[37m]\033[m");
	}
	return 0;
}

int
t_friends()
{
	char buf[STRLEN];
	if (!USERPERM(currentuser, PERM_BASIC))
		return DONOTHING;
	user_record = malloc(sizeof (struct user_info *) * MAXACTIVE);

	modify_user_mode(FRIEND);
	friendmode = YEA;
	friendmode1 = 0;
	setuserfile(buf, "friends");
	if (!file_isfile(buf)) {
		move(1, 0);
		clrtobot();
		prints("你尚未利用 Info -> Override 设定好友名单，所以...\n");
		range = 0;
	} else {
		num_alcounter();
		range = count_friends;
	}
	if (range == 0) {
		move(2, 0);
		clrtobot();
		prints("目前无好友上线\n");
		move(BBS_PAGESIZE + 3, 0);
		if (askyn("是否转换成使用者模式", YEA, NA) == YEA) {
			range = num_active_users();
			page = -1;
			friendmode = NA;
			modify_user_mode(LUSERS);
			update_time = 0;
			choose(YEA, 0, print_title, deal_key, show_userlist,
			       do_query);
			clear();
			free(user_record);
			user_record = NULL;
			return FULLUPDATE;
		}
	} else {
		update_time = 0;
		choose(YEA, 0, print_title, deal_key, show_userlist, do_query);
	}
	clear();
	friendmode = NA;
	free(user_record);
	user_record = NULL;
	return FULLUPDATE;
}

int
t_users()
{
	user_record = malloc(sizeof (struct user_info *) * MAXACTIVE);
	friendmode = NA;
	modify_user_mode(LUSERS);
	range = num_visible_users();
	if (range == 0) {
		move(3, 0);
		clrtobot();
		prints("目前无使用者上线\n");
	}
	update_time = 0;
	choose(YEA, 0, print_title, deal_key, show_userlist, do_query);
	clear();
	free(user_record);
	user_record = NULL;
	return 0;
}

int
choose(update, defaultn, title_show, key_deal, list_show, read)
int update;
int defaultn;
void (*title_show) ();
int (*key_deal) (int, int, int);
int (*list_show) ();
int (*read) (int, int);
{
	int ch, number, deal;

	readplan = NA;
	(*title_show) ();
	func_list_show = list_show;
	signal(SIGALRM, SIG_IGN);
	if (update == 1)
		update_data();
	page = -1;
	number = 0;
	num = defaultn;
	while (1) {
		if (num <= 0)
			num = 0;
		if (num >= range)
			num = range - 1;
		if (page < 0 || freshmode == 1) {
			freshmode = 0;
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			move(3, 0);
			clrtobot();
			if ((*list_show) () == -1)
				return -1;
			update_endline();
		}
		if (num < page || num >= page + BBS_PAGESIZE) {
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			if ((*list_show) () == -1)
				return -1;
			update_endline();
			continue;
		}
		if (readplan == YEA) {
			if ((*read) (page, num) == -1)
				return num;
		} else {
			move(3 + num - page, 0);
			prints(">");
			move(3 + num - page, 0);
		}
		can_R_endline = 1;
		ch = egetch();
		can_R_endline = 0;
		if (readplan == NA)
			move(3 + num - page, 0);
		prints(" ");
		if (ch == 'q' || ch == 'e' || ch == KEY_LEFT || ch == EOF) {
			if (readplan == YEA) {
				readplan = NA;
				move(1, 0);
				clrtobot();
				if ((*list_show) () == -1)
					return -1;
				(*title_show) ();
				update_endline();
				continue;
			}
			break;
		}
		deal = (*key_deal) (ch, num, page);
		if (range == 0)
			break;
		if (deal == 1) {
			(*title_show) ();
			continue;
		} else if (deal == -1)
			break;
		switch (ch) {
		case 'P':
		case 'b':
		case Ctrl('B'):
		case KEY_PGUP:
			if (num == 0)
				num = range - 1;
			else
				num -= BBS_PAGESIZE;
			break;
		case ' ':
			if (readplan == YEA) {
				if (++num >= range)
					num = 0;
				break;
			}
		case 'N':
		case Ctrl('F'):
		case KEY_PGDN:
			if (num == range - 1)
				num = 0;
			else
				num += BBS_PAGESIZE;
			break;
		case 'p':
		case 'l':
		case KEY_UP:
			if (num-- <= 0)
				num = range - 1;
			break;
		case 'n':
		case 'j':
		case KEY_DOWN:
			if (++num >= range)
				num = 0;
			break;
		case '$':
		case KEY_END:
			num = range - 1;
			break;
		case KEY_HOME:
			num = 0;
			break;
		case '\n':
		case '\r':
			if (number > 0) {
				num = number - 1;
				break;
			}
			/* fall through */
		case KEY_RIGHT:
			{
				if (readplan == YEA) {
					if (++num >= range)
						num = 0;
				} else
					readplan = YEA;
				break;
			}
		default:
			;
		}
		if (ch >= '0' && ch <= '9') {
			number = number * 10 + (ch - '0');
			ch = '\0';
		} else {
			number = 0;
		}
	}
	signal(SIGALRM, SIG_IGN);
	return -1;
}

static char
msgchar(uin)
struct user_info *uin;
{
	if (isreject(uin, &uinfo))
		return '*';
	if ((uin->pager & ALLMSG_PAGER))
		return ' ';
	if (hisfriend(uin)) {
		if ((uin->pager & FRIENDMSG_PAGER))
			return 'O';
		else
			return '#';
	}
	return '*';
}

static char
pagerchar(friend, pager)
int friend, pager;
{
	if (pager & ALL_PAGER)
		return ' ';
	if ((friend)) {
		if (pager & FRIEND_PAGER)
			return 'O';
		else
			return '#';
	}
	return '*';
}

static char *
idle_str(uent)
struct user_info *uent;
{
	static char hh_mm_ss[32];
	time_t diff;
	int limit, hh, mm, temppid;

	if (uent == NULL) {
		strcpy(hh_mm_ss, "不详");
		return hh_mm_ss;
	}
	if ((temppid = uent->pid) <= 0) {
		strcpy(hh_mm_ss, "不详");
		return hh_mm_ss;
	}

	diff = now_t - uent->lasttime;

#ifdef DOTIMEOUT
	/* the 60 * 60 * 24 * 5 is to prevent fault /dev mount from
	   kicking out all users */

	if (uent->ext_idle)
		limit = IDLE_TIMEOUT * 3;
	else
		limit = IDLE_TIMEOUT;

	if ((diff > limit) && (diff < 86400 * 5))
		/* kill( uent->pid, SIGHUP ); */
		kill(temppid, SIGHUP);	/*by ylsdd, so do to avoid kill(0 or -1, SIGXXX), */
#endif

	hh = diff / 3600;
	mm = (diff / 60) % 60;

	if (hh > 0)
		sprintf(hh_mm_ss, "%d:%02d", hh, mm);
	else if (mm > 0)
		sprintf(hh_mm_ss, "%d", mm);
	else
		sprintf(hh_mm_ss, "   ");

	return hh_mm_ss;
}

static int
num_visible_users()
{
	return bbsinfo.utmpshm->activeuser;
}
