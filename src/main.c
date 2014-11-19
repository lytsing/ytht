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
#ifdef LINUX
#include <mcheck.h>
#endif

int ERROR_READ_SYSTEM_FILE = NA;
int RMSG = YEA;
int have_msg_unread = 0;
int nettyNN = 0;
int count_friends = 0, count_users = 0;
int die;
int iscolor = 1;
int listmode;
int numofsig = 0;
jmp_buf byebye;
char temp_sessionid[10];

int talkrequest = NA;
int enter_uflags;

struct user_info uinfo;

char fromhost[30];		//Mask might be applied on fromhost,
			//use realfromhost for the actual IP.
char realfromhost[30];
unsigned int realfromIP;

char ULIST[STRLEN];
int utmpent = -1;
time_t login_start_time;
int showansi = 1;
int convcode = 0;
int runtest;
int runssh = 0;
struct bbsinfo bbsinfo;

static void u_enter(void);
static void setflags(int mask, int value);
static void u_exit(void);
static void talk_request(int signum);
static void multi_user_check(void);
#ifndef SSHBBS
static int simplepasswd(char *str, int check);
#endif
static void reaper(void);
static void system_init(int argc, char **argv);
static int getuptime(void);
static void login_query(void);
static void notepad_init(void);
static void user_login(void);
static int chk_friend_book(void);
static char *boardmargin(void);
static void R_endline(int signum);
static void tlog_recover(void);

static void
u_enter()
{
	struct userec tmpu;
	int utmpfd;
	char buf[20];
	inprison = is_inprison(currentuser->userid);
	if (inprison) {
		memcpy(&tmpu, currentuser, sizeof (struct userec));
		tmpu.userdefine &=
		    ~(DEF_ANIENDLINE | DEF_ACBOARD | DEF_COLOR | DEF_ENDLINE);
		tmpu.dieday = 2;	//随便一个非负值
		tmpu.inprison = 1;
		updateuserec(&tmpu, usernum);
	} else {
		if (currentuser->inprison == 1) {	//应该是个刚放出来的人
			memcpy(&tmpu, currentuser, sizeof (struct userec));
			tmpu.dieday = 0;
			updateuserec(&tmpu, usernum);
		} else if (currentuser->dieday > 0) {
			memcpy(&tmpu, currentuser, sizeof (struct userec));
			tmpu.userdefine &=
			    ~(DEF_ANIENDLINE | DEF_ACBOARD | DEF_COLOR |
			      DEF_ENDLINE);
			updateuserec(&tmpu, usernum);
		}
	}
	if (currentuser->dieday)
		die = 1;
	else
		die = 0;
	enter_uflags = currentuser->flags[0];
	listmode = 0;
	digestmode = NA;
	memset(&uinfo, 0, sizeof (uinfo));

	uinfo.active = YEA;
	uinfo.pid = getpid();
	if ((USERPERM(currentuser, PERM_LOGINCLOAK)
	     && (currentuser->flags[0] & CLOAK_FLAG)) || currentuser->dieday
	    || inprison)
		uinfo.invisible = YEA;
	uinfo.mode = LOGIN;
	uinfo.pager = 0;
	if (USERDEFINE(currentuser, DEF_DELDBLCHAR))
		enabledbchar = 1;
	else
		enabledbchar = 0;
	if (USERDEFINE(currentuser, DEF_FRIENDCALL)) {
		uinfo.pager |= FRIEND_PAGER;
	}
	if (currentuser->flags[0] & PAGER_FLAG) {
		uinfo.pager |= ALL_PAGER;
		uinfo.pager |= FRIEND_PAGER;
	}
	if (USERDEFINE(currentuser, DEF_FRIENDMSG)) {
		uinfo.pager |= FRIENDMSG_PAGER;
	}
	if (USERDEFINE(currentuser, DEF_ALLMSG)) {
		uinfo.pager |= ALLMSG_PAGER;
		uinfo.pager |= FRIENDMSG_PAGER;
	}
	uinfo.uid = usernum;
	uinfo.userlevel = currentuser->userlevel;
	strsncpy(uinfo.from, fromhost, sizeof (uinfo.from));
	uinfo.fromIP = realfromIP;
	uinfo.lasttime = time(0);
	if (runssh)
		uinfo.isssh = 1;
	iscolor = (USERDEFINE(currentuser, DEF_COLOR)) ? 1 : 0;
	strsncpy(uinfo.userid, currentuser->userid, sizeof (uinfo.userid));
	strsncpy(uinfo.username, currentuser->username,
		 sizeof (uinfo.username));
	getrandomstr(uinfo.sessionid);
	if (USERPERM(currentuser, PERM_EXT_IDLE))
		uinfo.ext_idle = YEA;
	uinfo.curboard = 0;
	if (strcasecmp(currentuser->userid, "guest")) {
		get_mailsize(currentuser);
		getfriendstr();
		getrejectstr();
		if (readuservalue
		    (currentuser->userid, "signature", buf, sizeof (buf)) >= 0)
			uinfo.signature = atoi(buf);
		else
			uinfo.signature = 0;
	} else {
		memset(uinfo.friend, 0, sizeof (uinfo.friend));
		memset(uinfo.reject, 0, sizeof (uinfo.reject));
		uinfo.signature = 0;
	}
	utmpfd = open(".UTMP." MY_BBS_DOMAIN, O_RDWR | O_CREAT, 0600);
	if (utmpfd < 0) {
		errlog("utmp lock error");
		exit(-1);
	}
	flock(utmpfd, LOCK_EX);
	utmpent = utmp_login(&uinfo);
	if (utmpent < 0) {
		errlog("Fault: No utmpent slot for %s\n", uinfo.userid);
		exit(-1);
	}
	flock(utmpfd, LOCK_UN);
	close(utmpfd);
	get_temp_sessionid(temp_sessionid);
#ifdef LINUX
#ifdef SSHBBS
	setproctitle("sshbbsd %s %s", realfromhost, currentuser->userid);
#else
	if (runtest)
		setproctitle("bbstest d %s %s", realfromhost,
			     currentuser->userid);
	else
		setproctitle("bbs d %s %s", realfromhost, currentuser->userid);
#endif
#endif
}

static void
setflags(mask, value)
int mask, value;
{
	struct userec tmpu;
	if (((currentuser->flags[0] & mask) && 1) != value) {
		memcpy(&tmpu, currentuser, sizeof (struct userec));
		if (value)
			tmpu.flags[0] |= mask;
		else
			tmpu.flags[0] &= ~mask;
		updateuserec(&tmpu, usernum);
	}
}

static void
u_exit()
{
	int utmpfd;
	signal(SIGHUP, SIG_DFL);
	signal(SIGALRM, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGUSR2, SIG_IGN);
	setflags(PAGER_FLAG, (uinfo.pager & ALL_PAGER));
	if (USERPERM(currentuser, PERM_LOGINCLOAK))
		setflags(CLOAK_FLAG, uinfo.invisible);
	uinfo.active = NA;
	uinfo.pid = 0;
	uinfo.invisible = YEA;
	uinfo.sockactive = NA;
	uinfo.sockaddr = 0;
	uinfo.destuid = 0;
	uinfo.lasttime = 0;
	if (utmpent < 0) {
		errlog("reenter u_exit");
		return;
	}
	utmpfd = open(".UTMP." MY_BBS_DOMAIN, O_RDWR | O_CREAT, 0600);
	if (utmpfd < 0) {
		errlog("utmp lock error");
		return;
	}
	flock(utmpfd, LOCK_EX);
	utmp_logout(&utmpent, &uinfo);
	flock(utmpfd, LOCK_UN);
	close(utmpfd);
}

int
cmpuids(uid, up)
char *uid;
struct userec *up;
{
	return !strncasecmp(uid, up->userid, sizeof (up->userid));
}

int started = 0;

static void
talk_request(int signum)
{
	signal(SIGUSR1, talk_request);
	talkrequest = YEA;
	bell();
	bell();
	bell();
	sleep(1);
	bell();
	bell();
	bell();
	bell();
	bell();
	return;
}

void
do_abort_bbs()
{
	time_t stay;
	if (!started)
		return;

	started = 0;
	if (uinfo.mode == POSTING || uinfo.mode == SMAIL || uinfo.mode == EDIT
	    || uinfo.mode == EDITUFILE || uinfo.mode == EDITSFILE
	    || uinfo.mode == EDITANN)
		keep_fail_post();
	stay = time(0) - login_start_time;
	tracelog("%s drop %ld", currentuser->userid, stay);
	if ((currentuser->userlevel & PERM_BOARDS)
	    && (count_uindex(usernum) == 1))
		setbmstatus(0);
	u_exit();
}

void
abort_bbs()
{
	do_abort_bbs();
	exit(0);
}

static void
multi_user_check()
{
	struct user_info uin, *puin;
	char buffer[STRLEN];
	int logins, allowedlogin = 2;

	if (USERPERM(currentuser, PERM_MULTILOG))
		allowedlogin = 3;

	if (USERPERM(currentuser, PERM_ARBITRATE))
		allowedlogin = 3;

	if (!strcmp("guest", currentuser->userid) ||
	    (!strcmp(MY_BBS_ID, "TTTAN")
	     && !strcmp(currentuser->userid, "peaches"))) {
		if (heavyload(0)) {
			prints
			    ("\033[1;33m抱歉, 目前系统负荷过重, 请勿重覆 Login。\033[m\n");
			refresh();
			exit(1);
		}
		return;
	}
	logins = count_uindex_telnet(usernum);
	if (!logins)
		return;
	puin = query_uindex(usernum, 0);
	if (!puin)
		return;
	uin = *puin;
	if (!uin.active || uin.pid <= 1 || (kill(uin.pid, 0) == -1))
		return;

	getdata(t_lines - 1, 0,
		"\033[1;37m您想删除重复的 login 吗 (Y/N)? [N]\033[m", buffer, 4,
		DOECHO, YEA);
	if (toupper(buffer[0]) != 'Y') {
		logins = count_uindex_telnet(usernum);
		if (logins >= allowedlogin
		    || (currentuser->dieday && logins >= 2)) {
			scroll();
			scroll();
			move(t_lines - 2, 0);
			prints
			    ("\033[1;33m很抱歉, 您已 Telnet Login 相同帐号%d次, "
			     "为确保他人上站权益,\n 此连线将被取消。\033[m\n",
			     logins);
			refresh();
			exit(1);
		}
		return;
	}
	//因为问答之间可以发生很多事情, 重新读取
	puin = query_uindex(usernum, 0);
	if (!puin)
		return;
	uin = *puin;
	if (!uin.active || uin.pid <= 1 || (kill(uin.pid, 0) == -1))
		return;

	kill(uin.pid, SIGHUP);
	tracelog("%s kick %s multi-login", currentuser->userid,
		 currentuser->userid);
}

#ifndef SSHBBS
static int
simplepasswd(str, check)
char *str;
int check;
{
	char ch;

	while ((ch = *str++) != '\0') {
		if (check == 1) {
			if (!(ch >= 'a' && ch <= 'z'))
				return 0;
		} else if (!(ch >= '0' && ch <= '9'))
			return 0;
	}
	return 1;
}
#endif

static void
reaper()
{
	while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0) ;
	signal(SIGCHLD, (void *) reaper);
}

#ifdef HAVE_MALLOPT
#include <malloc.h>
#endif
static void
system_init(argc, argv)
int argc;
char **argv;
{
	struct sigaction act;
	struct in_addr in;
#ifdef HAVE_MALLOPT
	mallopt(M_MMAP_THRESHOLD, 10000);
#endif
	login_start_time = time(0);
	sprintf(ULIST, "%s.%s", ULIST_BASE, MY_BBS_DOMAIN);

	if (argc >= 3) {
		strsncpy(realfromhost, argv[2], sizeof (realfromhost));
	} else {
		realfromhost[0] = '\0';
	}
	inet_aton(realfromhost, &in);
	realfromIP = in.s_addr;
	in.s_addr &= 0x0000ffff;
	strsncpy(fromhost, inet_ntoa(in), sizeof (fromhost));

#ifndef lint
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
#ifdef DOTIMEOUT
	init_alarm();
	uinfo.mode = LOGIN;
	alarm(LOGIN_TIMEOUT);
#else
	signal(SIGALRM, SIG_IGN);
#endif
	signal(SIGTERM, SIG_IGN);
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
#endif
	signal(SIGHUP, (void *) abort_bbs);
	//signal(SIGTTOU,count_msg) ;
	signal(SIGTTOU, R_endline);
	signal(SIGUSR1, talk_request);
	signal(SIGUSR2, (void *) r_msg);
	signal(SIGCHLD, (void *) reaper);

	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	act.sa_handler = (void *) r_msg;
	sigaction(SIGUSR2, &act, NULL);
}

static int
getuptime()
{
	FILE *fp;
	int n;
	fp = fopen("/proc/uptime", "r");
	if (fp == NULL)
		return 0;
	fscanf(fp, "%d", &n);
	fclose(fp);
	return n;
}

static void
login_query()
{
#ifndef SSHBBS
	char uid[IDLEN + 2], passbuf[PASSLEN];
	int attempts;
	char md5pass[16];
	struct userec tmpu;
#endif
	int n;
	char buf[STRLEN];
/*-----------------New Century-----
time_t timenow;
int dis;
char str[40], str1[100];
timenow=time(NULL);
strcpy(str,ctime(&timenow));
str[24]='\0';
dis=946656000-timenow;
if(dis>=0) sprintf(str1,"现在是 %s , 距2000年还有%d秒\n", str, dis);
else sprintf(str1,"现在是 %s, 新世纪已经开始了%d秒\n",str,-dis);
---------------------------------*/

	if (bbsinfo.utmpshm->activeuser >= MAXACTIVERUN) {
		ansimore("etc/loginfull", NA);
		refresh();
		sleep(1);
		exit(1);
	}
	fill_shmfile(5, "etc/endline", getBBSKey(ENDLINE_SHM));
	//currentuser->userdefine |= DEF_COLOR;
	ansimore2("etc/issue", NA, 0, 20);

	move(t_lines - 4, 0);
	n = getuptime();
	prints("\033[1;32m欢迎光临\033[1;33m %s\033[32m ", MY_BBS_NAME);
	prints
	    ("目前上站人数 [\033[36m%d/%d\033[32m] WWW匿名[\033[36m%d\033[32m] ",
	     bbsinfo.utmpshm->activeuser, MAXACTIVERUN,
	     bbsinfo.utmpshm->wwwguestnum);
	prints("总用户[\033[36m%d\033[32m]\n", usersum());
	prints("系统持续运行 [\033[36m%d天%d小时%d分钟\033[32m] ",
	       n / (3600 * 24), n % (3600 * 24) / 3600, n % 3600 / 60);
	prints("最高人数记录 [\033[36m%d\033[32m] ", bbsinfo.utmpshm->maxuser);
	prints("本日最高人数 [\033[36m%d\033[32m]\n",
	       bbsinfo.utmpshm->maxtoday);
	prints("\033[m试用请输入 '\033[1;36mguest\033[m', "
	       "注册请输入 '\033[1;31mnew\033[m', "
	       "add '.' after YourID to login for BIG5\n");
#ifndef SSHBBS
	attempts = 0;
	while (1) {
		if (attempts++ >= LOGINATTEMPTS) {
			ansimore("etc/goodbye", NA);
			refresh();
			exit(1);
		}
		move(t_lines - 1, 0);
		clrtoeol();
		getdata(t_lines - 1, 0, "请输入帐号: ",
			uid, IDLEN + 1, DOECHO, YEA);
		scroll();
		{
			int l = strlen(uid);
			if (l > 0 && uid[l - 1] == '.') {
				uid[l - 1] = 0;
				if (!convcode)
					switch_code();
			}
		}
		/* ppfoong */
		if ((strcasecmp(uid, "guest") == 0 || !strcmp(uid, "peaches"))
		    && (MAXACTIVE - bbsinfo.utmpshm->activeuser < 10)) {
			ansimore("etc/loginfull", NA);
			refresh();
			exit(1);
		}

		if (strcmp(uid, "new") == 0) {
//#ifdef LOGINASNEW
#if 1
			//      memset(currentuser, 0, sizeof (currentuser));
			new_register();
			ansimore("etc/firstlogin", YEA);
			break;
#else
			prints
			    ("\033[1;37m本系统目前无法以 \033[36mnew\033[37m 注册, 请用\033[36m guest\033[37m 进入...\033[m\n");
			scroll();
#endif
		} else if (*uid == '\0'
			   || !(usernum = getuser(uid, &currentuser))) {
			move(t_lines - 1, 0);
			clrtoeol();
			prints
			    ("\033[1;31m错误的使用者帐号...注册新帐号输入 new\033[0m\n");
			scroll();
		} else if (strcasecmp(uid, "guest") == 0) {
			//currentuser->userlevel = 0;
			break;
		} else if (!strcmp(MY_BBS_ID, "TTTAN")
			   && !strcmp(uid, "peaches")) {
			strcpy(fromhost, "999.999.999.999");
			break;
		} else if (userbansite(currentuser->userid, realfromhost)) {
			prints("\033[1;31m用户%s已经禁止从%s尝试登录\033[0m\n",
			       currentuser->userid, realfromhost);
			scroll();
		} else {
			if (!convcode)
				convcode =
				    !(currentuser->userdefine & DEF_USEGB);
			move(t_lines - 1, 0);
			clrtoeol();
			getdata(t_lines - 1, 0, "请输入密码: ",
				passbuf, PASSLEN, NOECHO, YEA);
			scroll();
			if (!checkpasswd
			    (currentuser->passwd, currentuser->salt, passbuf)) {
				logattempt(currentuser->userid, realfromhost,
					   "", now_t);
				move(t_lines - 1, 0);
				clrtoeol();
				prints("\033[1;31m密码输入错误...\033[m\n");
				scroll();
			} else {
				if (currentuser->salt == 0) {
					memcpy(&tmpu, currentuser,
					       sizeof (struct userec));
					tmpu.salt = getsalt_md5();
					genpasswd(md5pass, tmpu.salt, passbuf);
					memcpy(tmpu.passwd, md5pass, 16);
					updateuserec(&tmpu, usernum);
				}
				if (!USERPERM(currentuser, PERM_BASIC)) {
					move(t_lines - 2, 0);
					clrtoeol();
					prints
					    ("\033[1;32m由于本帐号名称不符合帐号管理办法，已经被管理员禁止继续上站。\033[m\n");
					move(t_lines - 1, 0);
					clrtoeol();
					prints
					    ("\033[1;32m请用其他帐号登录在 \033[1;36m"
					     DEFAULTBOARD
					     "\033[1;32m 版面查询原因 \033[0m\n");
					refresh();
					sleep(5);
					exit(1);
				}
				if (simplepasswd(passbuf, 1)
				    || simplepasswd(passbuf, 2)
				    || strstr(passbuf, currentuser->userid)) {
					move(t_lines - 1, 0);
					clrtoeol();
					prints
					    ("\033[1;33m* 密码过于简单, 请选择一个以上的特殊字元.\033[m\n");
					scroll();
					move(t_lines - 1, 0);
					clrtoeol();
					getdata(t_lines - 1, 0,
						"-按 <ENTER> 继续", genbuf, 5,
						NOECHO, YEA);
					scroll();
				}
				bzero(passbuf, PASSLEN - 1);
				break;
			}
		}
	}
#else
	if (userbansite(currentuser->userid, realfromhost)) {
		move(t_lines - 1, 0);
		clrtoeol();
		prints("\033[1;31m用户%s已经禁止从%s尝试登录\033[0m\n",
		       currentuser->userid, realfromhost);
		exit(0);
	}
	move(t_lines - 1, 0);
	clrtoeol();
	prints("%s 欢迎您使用ssh方式访问 %s 按任意键继续",
	       currentuser->userid, MY_BBS_NAME);
	*genbuf = egetch();
#endif
	multi_user_check();
	sprintf(buf, "home/%c/%s/%s.deadve", mytoupper(currentuser->userid[0]),
		currentuser->userid, currentuser->userid);
	if (file_isfile(buf)) {
		if (strcasecmp("guest", currentuser->userid))
			system_mail_file(buf, currentuser->userid,
					 "不正常断线所保留的部份...",
					 currentuser->userid);
		unlink(buf);
	}
	sethomepath(genbuf, currentuser->userid);
	mkdir(genbuf, 0775);
}

static void
notepad_init()
{
	FILE *fp;
	char notetitle[STRLEN];
	char tmp[STRLEN * 2];
	char *fname, *bname, *ntitle;
	long int maxsec;
	int fd;
	time_t ftime;
	struct tm *tm;
	struct utimbuf utm;
	maxsec = 24 * 60 * 60;
	ftime = file_time("etc/checknotepad");
	if (ftime == 0) {
		fd = open("etc/checknotepad", O_RDWR | O_CREAT | O_EXCL, 0600);
		close(fd);
	}
	if (now_t - ftime < maxsec)
		return;

	move(t_lines - 1, 0);
	prints("对不起，系统自动发信，请稍候.....");
	refresh();

	//Touch "etc/checknotepad". This should be after the refresh().
	if ((fd = open("etc/checknotepad", O_RDONLY)) < 0)
		return;
	flock(fd, LOCK_EX);
	ftime = file_time("etc/checknotepad");
	if (now_t - ftime < maxsec) {
		close(fd);
		return;
	}
	tm = localtime(&now_t);
	tm->tm_sec = 0;
	tm->tm_min = 0;
	tm->tm_hour = 4;
	utm.actime = mktime(tm);
	utm.modtime = utm.actime;
	utime("etc/checknotepad", &utm);
	close(fd);

	if ((fp = fopen("etc/autopost", "r")) == NULL)
		return;
	while (fgets(tmp, STRLEN, fp) != NULL) {
		fname = strtok(tmp, " \n\t:@");
		bname = strtok(NULL, " \n\t:@");
		ntitle = strtok(NULL, " \n\t:@");
		if (fname == NULL || bname == NULL || ntitle == NULL)
			continue;
		sprintf(notetitle, "[%.10s] %s", ctime(&now_t), ntitle);
		if (file_isfile(fname)) {
			postfile(fname, bname, notetitle, 1);
			tracelog("system post %s %s", bname, ntitle);
		}
	}
	fclose(fp);
	sprintf(notetitle, "[%.10s] 留言版记录", ctime(&now_t));
	if (file_isfile("etc/notepad")) {
		postfile("etc/notepad", "notepad", notetitle, 1);
		unlink("etc/notepad");
	}
}

static void
user_login()
{
	char fname[STRLEN];
	time_t dtime;
	int day;
	int info_changed = 0;
	time_t now;
	struct userec tmpu;
	now = time(NULL);
	tracelog("%s enter %s", currentuser->userid, realfromhost);
	u_enter();
	started = 1;
	if (!currentuser->dieday && USERDEFINE(currentuser, DEF_SEESTATINLOG)) {
#ifdef USE_NOTEPAD
		notepad_init();
		if (strcmp(currentuser->userid, "guest")
		    || USERDEFINE(currentuser, DEF_NOTEPAD))
			shownotepad();
#endif

		ansimore("0Announce/bbslist/countusr", 1);
		if (file_exist("Welcome2"))
			ansimore2("Welcome2", 1, 0, 24);
		if (USERDEFINE(currentuser, DEF_FILTERXXX))
			ansimore("etc/dayf", 1);
		else
			ansimore("etc/posts/day", 1);
		if (file_exist("etc/posts/newsday"))
			ansimore("etc/posts/newsday", 1);
		ansimore("etc/posts/good10", 0);
		move(t_lines - 2, 0);
		prints
		    ("\033[1;36m☆ 这是您第 \033[33m%d\033[36m 次拜访本站。\n",
		     currentuser->numlogins + 1);
		prints("☆ 上次连线时间为 \033[33m%s\033[m",
		       Ctime(currentuser->lastlogin));
		igetkey();
	}
	setuserfile(fname, BADLOGINFILE);
	if (ansimore(fname, NA) != -1) {
		char ans[3];
		getdata(t_lines - 1, 0,
			"如何处理以上密码输入错误记录 (m) 邮回信箱 (r) 清除 (c) 继续 [c]: ",
			ans, 2, DOECHO, YEA);
		if (ans[0] == 'm' || ans[0] == 'M') {
			char title[STRLEN];
			sprintf(title, "[%12.12s] 密码输入错误记录",
				ctime(&now) + 4);
			system_mail_file(fname, currentuser->userid, title,
					 currentuser->userid);
			have_msg_unread = 0;
			unlink(fname);
		} else if (ans[0] == 'r' || ans[0] == 'R') {
			unlink(fname);
		}
	}
	if (shouldbroadcast(usernum) && ansimore(BROADCAST_FILE, NA) != -1) {
		char ans[4];
		while (1) {
			getdata(t_lines - 1, 0,
				"阅读完毕请输入大写的 IC 离开: ", ans, 3,
				DOECHO, YEA);
			if (!strcmp(ans, "IC"))
				break;
		}
	}
	memcpy(&tmpu, currentuser, sizeof (tmpu));
	if (currentuser->lasthost != realfromIP
	    && strcmp(currentuser->userid, "guest")
	    && strcmp(currentuser->userid, "peaches")) {
		info_changed = 1;
		tmpu.lasthost = realfromIP;
	}
	if (USERPERM(currentuser, PERM_LOGINOK)) {
		FILE *fp;
		setuserfile(fname, "clubrights");
		if ((fp = fopen(fname, "r")) == NULL) {
			memset(&(uinfo.clubrights), 0,
			       CLUB_SIZE * sizeof (int));
		} else {
			fread(&(uinfo.clubrights), sizeof (int), CLUB_SIZE, fp);
			fclose(fp);
		}
	}
	ISdelrq = clubtest("deleterequest");
#ifdef REG_EXPIRED
	/* ppfoong - 每 REG_EXPIRED 天重新进行身份确认 */
	if (USERPERM(currentuser, PERM_LOGINOK) &&
	    strcmp(currentuser->userid, "SYSOP") &&
	    strcmp(currentuser->userid, "guest")) {
		struct stat st;
		int expired = 0;
		setuserfile(fname, "mailcheck");
		if (stat(fname, &st) == -1
		    || now - st.st_mtime >= REG_EXPIRED * 86400) {
			setuserfile(fname, "register");
			if (stat(fname, &st) == 0) {
				if (now - st.st_mtime >= REG_EXPIRED * 86400) {
					setuserfile(fname, "register.old");
					if (stat(fname, &st) == -1
					    || now - st.st_mtime >=
					    REG_EXPIRED * 86400)
						expired = 1;
					else
						expired = 0;
				}
			} else
				expired = 1;	/*  漏网之鱼??  */
		}
		if (expired) {
			strcpy(currentuser->email, "");
			strcpy(currentuser->address, "");
			tmpu.userlevel &= ~(PERM_LOGINOK | PERM_PAGE);
			info_changed = 1;
			mail_file("etc/expired", currentuser->userid,
				  "更新个人资料说明。", 1);
			setuserfile(fname, "sucessreg");
			unlink(fname);
		}
	}
#endif
	if (strcmp(currentuser->userid, "SYSOP") == 0) {
		info_changed = 1;
		tmpu.userlevel = ~0;	/* SYSOP gets all permission bits */
	}
	if (currentuser->firstlogin == 0) {
		info_changed = 1;
		tmpu.firstlogin = login_start_time - 7 * 86400;
	}
	if (count_uindex(usernum) == 1) {
		dtime = time(NULL) - 4 * 3600;
		day = localtime(&dtime)->tm_mday;
		dtime = currentuser->lastlogin - 4 * 3600;
		if (day > localtime(&dtime)->tm_mday) {
			if (currentuser->numdays < 60000)
				tmpu.numdays++;
			if (currentuser->dieday > 1)
				tmpu.dieday--;
		}
		if (now - currentuser->lastlogin > 1800
		    || currentuser->numlogins <= 1) {
			tmpu.numlogins++;
		}
		tmpu.lastlogin = time(NULL);
		info_changed = 1;
	} else if (currentuser->numlogins <= 1) {
		tmpu.numlogins++;
		info_changed = 1;
	}
	if (info_changed)
		updateuserec(&tmpu, usernum);
	check_register_info();
}

void
set_numofsig()
{
	int sigln;
	char signame[STRLEN];
	setuserfile(signame, "signatures");
	sigln = countln(signame);
	numofsig = sigln / MAXSIGLINES;
	if ((sigln % MAXSIGLINES) != 0)
		numofsig += 1;
}

static int
chk_friend_book()
{
	FILE *fp;
	int idnum, n = 0;
	char buf[STRLEN], *ptr;
	if ((fp = fopen("friendbook", "r")) == NULL)
		return n;
	move(5, 0);
	prints("\033[1m系统寻人名册列表:\033[m\n\n");
	while (fgets(buf, sizeof (buf), fp) != NULL) {
		char uid[14];
		char msg[STRLEN];
		struct user_info *uin;
		ptr = strstr(buf, "@");
		if (ptr == NULL)
			continue;
		ptr++;
		strcpy(uid, ptr);
		ptr = strstr(uid, "\n");
		*ptr = '\0';
		idnum = atoi(buf);
		if (idnum != usernum || idnum <= 0)
			continue;
		uin = t_search(uid, NA, 1);
		sprintf(msg, "%s 已经上站。", currentuser->userid);
		if (!uinfo.invisible && uin != NULL
		    && !USERDEFINE(currentuser, DEF_NOLOGINSEND)
		    && do_sendmsg(uin->userid, uin, msg, 2, uin->pid) == 1) {
			prints
			    ("\033[1m%s\033[m 找你，系统已经告诉他你上站的消息。\n",
			     uid);
		} else
			prints
			    ("\033[1m%s\033[m 找你，系统无法联络到他，请你跟他联络。\n",
			     uid);
		n++;
		del_from_file("friendbook", buf);
		if (n > 15) {
			pressanykey();
			move(7, 0);
			clrtobot();
		}
	}
	fclose(fp);
	return n;
}

#ifndef SSHBBS
int
main(argc, argv)
int argc;
char *argv[];
#else
int
bbs_entry(argc, argv)
int argc;
char *argv[];
#endif
{
	char fname[STRLEN];
#ifdef LINUX
#ifndef SSHBBS
	int fd;
	initproctitle(argc, argv);
#endif
#endif
	umask(0007);
	time(&now_t);
	signal(SIGALRM, SIG_DFL);
	alarm(LOGIN_TIMEOUT);
	term_init();
	initscr();
	srand(time(NULL) + getpid());
	load_sysconf();
	conv_init();
	if (argc < 2 || ((*argv[1] != 'h') && (*argv[1] != 'e')
			 && (*argv[1] != 'd'))) {
		prints("You cannot execute this program directly.\n");
		refresh();
		exit(-1);
	}
	if (!strstr(argv[0], "bbstest")) {
		runtest = 0;
	} else {
		runtest = 1;
	}
#ifdef SSHBBS
	runssh = 1;
#endif
#ifndef SSHBBS
#ifdef LINUX
	if (runtest == 0) {
		fd = open(MY_BBS_HOME "/mtrace.lock", O_WRONLY | O_CREAT, 0644);
		if (fd != -1) {
			if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
				sprintf(fname,
					MY_BBS_HOME "/mtrace/mtrace_bbs.%d.%d",
					(int) now_t, getpid());
				setenv("MALLOC_TRACE", fname, 1);
				mtrace();
			} else {
				close(fd);
			}
		}
	}
#endif
#endif
	if (*argv[1] == 'e')
		convcode = 1;
	system_init(argc, argv);
	if (setjmp(byebye)) {
		abort_bbs();
	}
	set_cpu_limit(MAXACTIVERUN / (2 * 2));	//第一个2表示两cpu,第二个是为了给点余量
	init_tty();
#ifndef SSHBBS
	if (initbbsinfo(&bbsinfo) < 0) {
		prints("please init shm first\n");
		refresh();
		exit(0);
	}
	if (uhash_uptime() == 0) {
		prints("please init uhash first\n");
		refresh();
		exit(-1);
	}
#endif
	if (argc == 4) {
		prints("pid without username\n");
		refresh();
		exit(-1);
	}
	login_query();
	user_login();
	login_start_time = time(0);
	currboardstarttime = login_start_time;
	m_init();
	RMSG = NA;
	clear();
//#ifdef TALK_LOG
	tlog_recover();		/* 990713.edwardc for talk_log recover */
//#endif
	if (strcmp(currentuser->userid, "guest")) {
		if (USERPERM(currentuser, PERM_ACCOUNTS)
		    && file_isfile("new_register")) {
			prints
			    ("\033[1;33m有新使用者正在等您通过注册资料。\033[m");
			pressanykey();
			clear();
		}
		if (chk_friend_book())
			pressanykey();
		move(7, 0);
		clrtobot();
		set_numofsig();
		if (USERDEFINE(currentuser, DEF_INNOTE)) {
			setuserfile(fname, "notes");
			if (file_isfile(fname))
				ansimore(fname, YEA);
		}
	}
	nettyNN = NNread_init();
	b_closepolls();
	if ((currentuser->userlevel & PERM_BOARDS)) {
		setbmstatus(1);
	}
	num_alcounter();
	if (count_friends > 0 && USERDEFINE(currentuser, DEF_LOGFRIEND))
		t_friends();
	loaduserkeys();
	if ((!(currentuser->userlevel & PERM_LOGINOK))
	    && strcmp("guest", currentuser->userid)
	    && strcmp("SYSOP", currentuser->userid)) {
		char buf[256];
		sethomefile(buf, currentuser->userid, "mailcheck");
		if (!file_exist(buf))
			x_fillform();
	}
	board_sorttype = currentuser->flags[0] & BRDSORT_MASK;
	if (strcmp(currentuser->userid, "guest")) {
		occurredSIGUSR2 = 1;
	}
	//清理power_select残余
	atexit(rmpower);

	while (1) {
		if (inprison)
			domenu("PRISONMENU");
		else if (currentuser->dieday)
			domenu("DIEMENU");
		else {
			if (USERDEFINE(currentuser, DEF_NORMALSCR))
				domenu("TOPMENU");
			else
				domenu("TOPMENU2");
		}
		Goodbye();
	}
}

int refscreen = NA;
int
egetch()
{
	int rval;
	check_calltime();
	if (talkrequest) {
		talkreply();
		refscreen = YEA;
		return -1;
	}
/*    if (ntalkrequest) {
        ntalkreply() ;
        refscreen = YEA ;
        return -1 ;
    } */
	while (1) {
		rval = igetkey();
		if (talkrequest) {
			talkreply();
			refscreen = YEA;
			return -1;
		}
/*        if(ntalkrequest) {
            ntalkreply() ;
            refscreen = YEA ;
            return -1 ;
        } */
		if (rval != Ctrl('L'))
			break;
		redoscr();
	}
	refscreen = NA;
	return rval;
}

static char *
boardmargin()
{
	static char buf[STRLEN];
	if (selboard)
		sprintf(buf, "讨论区 [%s]", currboard);
	else {
		strcpy(currboard, DEFAULTBOARD);
		if (getbnum(currboard)) {
			selboard = 1;
			sprintf(buf, "讨论区 [%s]", currboard);
		} else
			sprintf(buf, "目前并没有设定讨论区");
	}
	return buf;
}

int endlineoffset = 0;
/*Add by SmallPig*/
void
update_endline()
{
	char buf[STRLEN];
	static int linetype = 0;
	int y, x;
	getyx(&y, &x);
	if (!USERDEFINE(currentuser, DEF_ENDLINE)) {
		move(t_lines - 1 + endlineoffset, 0);
		clrtoeol();
		goto END;
	}

	move(t_lines - 1 + endlineoffset, 0);
	clrtoeol();
	if (currentuser->userdefine & DEF_ANIENDLINE) {
		linetype = !linetype;
		if (linetype) {
			if (show_endline())
				goto END;
		}
	}
	sprintf(buf, "[\033[36m%.12s\033[33m]", currentuser->userid);
	num_alcounter();
	prints
	    ("\033[1;44;33m时间:[\033[36m%16s\033[33m] 在线/朋友:[\033[36m%4d\033[33m/\033[1;36m%3d\033[33m] 状态:[\033[36m%c%c%c%c%c%c\033[33m] 使用者:%-s\033[m",
	     ctime(&now_t), count_users, count_friends,
	     (uinfo.pager & ALL_PAGER) ? 'P' : 'p',
	     (uinfo.pager & FRIEND_PAGER) ? 'O' : 'o',
	     (uinfo.pager & ALLMSG_PAGER) ? 'M' : 'm',
	     (uinfo.pager & FRIENDMSG_PAGER) ? 'F' : 'f',
	     (USERDEFINE(currentuser, DEF_MSGGETKEY)) ? 'X' : 'x',
	     (uinfo.invisible == 1) ? 'C' : 'c', buf);
      END:
	move(y, x);
	return;
}

/*ReWrite by SmallPig*/
void
showtitle(title, mid)
char *title, *mid;
{
	char buf[STRLEN], *note;
	int spc1, spc2;
	note = boardmargin();
	spc1 = 39 - num_noans_chr(title) - strlen(mid) / 2;
	spc2 = 40 - strlen(note) - strlen(mid) + strlen(mid) / 2;	//直接减strlen(mid)/2有整除问题
	if (spc1 < 2) {
		spc2 -= 2 - spc1;	//ecnegrevid: 版面标题行太宽显示不好
		spc1 = 2;
	}
	if (spc2 < 2) {
		note[strlen(note) - (2 - spc2)] = 0;
		spc2 = 2;
	}
	move(0, 0);
	clrtoeol();
	sprintf(buf, "%*s", spc1, "");
	if (!strcmp(mid, MY_BBS_NAME))
		prints("\033[1;44;33m%s%s\033[37m%s\033[1;44m", title, buf,
		       mid);
	else if (mid[0] == '[')
		prints("\033[1;44;33m%s%s\033[5;36m%s\033[m\033[1;44m", title,
		       buf, mid);
	else
		prints("\033[1;44;33m%s%s\033[36m%s", title, buf, mid);
	sprintf(buf, "%*s", spc2, "");
	prints("%s\033[33m%s\033[m\n", buf, note);
	//update_endline(); //从来都是画了然后清掉,何必画之 ylsdd
	move(1, 0);
}

void
docmdtitle(title, prompt)
char *title, *prompt;
{
	char *middoc;
	if (chkmail()) {
		if (!strncmp("[讨论区列表]", title, 12))
			middoc = "[您有信件,请按 w 查看信件]";
		else
			middoc = "[您有信件]";
	} else
		middoc = MY_BBS_NAME;
	showtitle(title, middoc);
	move(1, 0);
	clrtoeol();
	prints("%s", prompt);
	clrtoeol();
}

static void
R_endline(int signum)
{
	signal(SIGTTOU, R_endline);
	now_t = time(NULL);
	if (!can_R_endline)
		return;
	if (uinfo.mode != READBRD
	    && uinfo.mode != READNEW
	    && uinfo.mode != SELECT
	    && uinfo.mode != LUSERS
	    && uinfo.mode != FRIEND
	    && uinfo.mode != READING
	    && uinfo.mode != RMAIL && uinfo.mode != DIGEST)
		return;
/*---------
    if (uinfo.mode != READBRD
        &&uinfo.mode != READNEW 
        &&uinfo.mode != SELECT
        &&uinfo.mode != LUSERS
        &&uinfo.mode != FRIEND
        &&!(uinfo.mode==READING&&can_R_endline)
        &&!(uinfo.mode==RMAIL&&can_R_endline)
        &&!(uinfo.mode==DIGEST&&can_R_endline) )
        return;
----------*/
	update_endline();
}

//#ifdef TALK_LOG
static void
tlog_recover()
{
	char buf[256];
	sprintf(buf, "home/%c/%s/talk_log",
		toupper(currentuser->userid[0]), currentuser->userid);
	if (strcasecmp(currentuser->userid, "guest") == 0 || !file_isfile(buf))
		return;
	clear();
	strcpy(genbuf, "");
	getdata(0, 0,
		"\033[1;32m您有一个不正常断线所留下来的聊天记录, 您要 .. (M) 寄回信箱 (Q) 算了？[Q]：\033[m",
		genbuf, 2, DOECHO, YEA);
	if (genbuf[0] == 'M' || genbuf[0] == 'm') {
		system_mail_file(buf, currentuser->userid, "聊天记录",
				 currentuser->userid);
	}
	unlink(buf);
}

//#endif

/* youzi quick goodbye */
int
Q_Goodbye()
{
	extern int started;
	time_t stay;
	char fname[STRLEN], notename[STRLEN];
	int logouts, mylogout = NA;
	struct userec tmpu;
	clear();
	prints("\n\n\n\n");
	if (USERDEFINE(currentuser, DEF_OUTNOTE)) {
		setuserfile(notename, "notes");
		if (file_isfile(notename))
			ansimore(notename, YEA);
	}
	if (USERDEFINE(currentuser, DEF_LOGOUT)) {
		setuserfile(fname, "logout");
		if (file_isfile(fname))
			mylogout = YEA;
	}
	if (mylogout) {
		logouts = countlogouts(fname);
		if (logouts >= 1) {
			user_display(fname, (logouts == 1) ? 1 :
				     (currentuser->numlogins % (logouts)) + 1,
				     YEA);
		}
	} else {
		if (fill_shmfile(2, "etc/logout", getBBSKey(GOODBYE_SHM))) {
			show_goodbyeshm();
		}
	}
	pressreturn();

	stay = now_t - login_start_time;
	if (started) {
		tracelog("%s exitbbs %ld", currentuser->userid, stay);
		if ((currentuser->userlevel & PERM_BOARDS)
		    && (count_uindex(usernum) == 1))
			setbmstatus(0);
	}
	memcpy(&tmpu, currentuser, sizeof (tmpu));
	//多窗口不重复计算上站时间. 检查now_t防止时钟变化导致负的stay
	if (count_uindex(usernum) == 1 && now_t > tmpu.lastlogin)
		stay = now_t - tmpu.lastlogin;
	else
		stay = 0;
	tmpu.stay += stay;
	tmpu.lastlogout = now_t;
	tmpu.flags[0] &= ~BRDSORT_MASK;
	tmpu.flags[0] |= board_sorttype;
	updateuserec(&tmpu, usernum);
	if (strcmp(currentuser->userid, "guest") && count_uindex(usernum) == 0) {
		FILE *fp;
		char buf[STRLEN], *ptr;
		if ((fp = fopen("friendbook", "r")) != NULL) {
			while (fgets(buf, sizeof (buf), fp) != NULL) {
				char uid[14];
				ptr = strstr(buf, "@");
				if (ptr == NULL) {
					del_from_file("friendbook", buf);
					continue;
				}
				ptr++;
				strcpy(uid, ptr);
				ptr = strstr(uid, "\n");
				*ptr = '\0';
				if (!strcmp(uid, currentuser->userid))
					del_from_file("friendbook", buf);
			}
			fclose(fp);
		}
	}
	if (started)
		u_exit();
	started = 0;
	exit(0);
}
