#include "bbslib.h"

#define NHASH 4099
static char *check_multi(char *id, int uid);

int
extandipmask(int ipmask, char *ip1, char *ip2)
{
	struct in_addr addr1, addr2;
	if (!*ip1 || !*ip2 || !strcmp(ip1, ip2))
		return ipmask;
	if (!inet_aton(ip1, &addr1) || !inet_aton(ip2, &addr2))
		return ipmask;
	while (ipmask < 15
	       && (addr1.s_addr << ipmask) != (addr2.s_addr << ipmask))
		ipmask++;
	return ipmask;
}

int
bbslogin_main()
{
	int n, t, infochanged = 0;
	time_t dtime;
	char filename[128], buf[256], id[20], pw[PASSLEN], url[10], *ub =
	    FIRST_PAGE;
	char *ptr;
	char md5pass[MD5LEN];
	struct userec *x, tmpu;
	int ipmask;
	int uid;
	html_header(3);

	if (loginok && !isguest) {
		sprintf(buf, "/" SMAGIC "/?t=%d", (int) now_t);
		redirect(buf);
	}

	strsncpy(id, strtrim(getparm("id")), 13);
	strsncpy(pw, getparm("pw"), PASSLEN);
	strsncpy(url, getparm("url"), 3);
	ipmask = atoi(getparm("ipmask"));
	if (!id[0]) {
		strcpy(id, "guest");
		ipmask = 8;
	}
	if (!strcmp(MY_BBS_ID, "YTHT") && !strcmp(id, "guest")) {
		http_fatal("请输入用户名和密码以登录。");
	}
	if (strcmp(id, "guest")) {
		ipmask = extandipmask(ipmask, getparm("lastip1"), realfromhost);
		ipmask = extandipmask(ipmask, getparm("lastip2"), realfromhost);
	}
	if ((uid = getuser(id, &x)) <= 0) {
		printf("%s<br>", id);
		http_fatal("错误的使用者帐号");
	}
	strcpy(id, x->userid);
	if (strcasecmp(id, "guest")) {
		if (checkbansite(realfromhost)) {
			http_fatal
			    ("对不起, 本站不欢迎来自 [%s] 的登录. <br>若有疑问, 请与SYSOP联系.",
			     realfromhost);
		}
		if (userbansite(x->userid, realfromhost))
			http_fatal("本ID已设置禁止从%s登录", realfromhost);
		if (!checkpasswd(x->passwd, x->salt, pw)) {
			logattempt(x->userid, realfromhost, "WWW", now_t);
			http_fatal
			    ("密码错误，如有疑问请联系站务组，提供注册资料找回密码");
		}
		if (!user_perm(x, PERM_BASIC))
			http_fatal
			    ("由于本帐号名称不符合帐号管理办法，已经被管理员禁止继续上站。<br>请用其他帐号登录在 <font color=red>"
			     DEFAULTBOARD "</font> 版询问.");
		if (file_has_word(MY_BBS_HOME "/etc/prisonor", x->userid)) {
			if (x->inprison == 0) {
				memcpy(&tmpu, x, sizeof (tmpu));
				tmpu.inprison = 1;
				tmpu.dieday = 2;
				updateuserec(&tmpu, 0);
			}
			http_fatal("安心改造，不要胡闹");
		}
		if (x->dieday)
			http_fatal("死了?还要做什么? :)");
		t = x->lastlogin;
		memcpy(&tmpu, x, sizeof (tmpu));
		if (tmpu.salt == 0) {
			tmpu.salt = getsalt_md5();
			genpasswd(md5pass, tmpu.salt, pw);
			memcpy(tmpu.passwd, md5pass, MD5LEN);
			infochanged = 1;
		}
		if (count_uindex(uid) == 0) {
			if (now_t - t > 1800)
				tmpu.numlogins++;
			infochanged = 1;
			tmpu.lastlogin = now_t;
			dtime = t - 4 * 3600;
			t = localtime(&dtime)->tm_mday;
			dtime = now_t - 4 * 3600;
			if (t < localtime(&dtime)->tm_mday
			    && x->numdays < 60000) {
				tmpu.numdays++;
			}
		}
		if (abs(t - now_t) < 5) {
			http_fatal("两次登录间隔过密!");
		}

		if (x->lasthost != from_addr.s_addr) {
			tmpu.lasthost = from_addr.s_addr;
			infochanged = 1;
		}
		if (infochanged)
			updateuserec(&tmpu, 0);
		currentuser = x;
	}
	ptr = getsenv("HTTP_X_FORWARDED_FOR");
	tracelog("%s enter %s www %d %s", x->userid, realfromhost, infochanged,
		 ptr);
	n = 0;
	if (loginok && isguest) {
		u_info->wwwinfo.iskicked = 1;
	}
	if (strcasecmp(id, "guest")) {
		sethomepath(filename, x->userid);
		mkdir(filename, 0755);

		strsncpy(buf, getparm("style"), 3);
		wwwstylenum = -1;
		if (isdigit(buf[0]))
			wwwstylenum = atoi(buf);
		if ((wwwstylenum > NWWWSTYLE || wwwstylenum < 0))
			if (!readuservalue
			    (x->userid, "wwwstyle", buf, sizeof (buf)))
				wwwstylenum = atoi(buf);
		if (wwwstylenum < 0 || wwwstylenum >= NWWWSTYLE)
			wwwstylenum = 1;
		currstyle = &wwwstyle[wwwstylenum];
	} else {
		wwwstylenum = 1;
		currstyle = &wwwstyle[wwwstylenum];

	}
	ub = wwwlogin(x, ipmask);
#ifdef USESESSIONCOOKIE
	{
		extern char sessionCookie[];
		printf
		    ("<script>document.cookie='SESSION=%s; path=/';</script>",
		     urlencode(sessionCookie));
	}
#endif
	if (!strcmp(url, "1")) {
#if 1
		printf
		    ("<script>\n"
		     "function URLencode(sStr) {\n"
		     "return escape(sStr).replace(/\\+/g, '%%2C').replace(/\\\"/g,'%%22').replace(/\\'/g, '%%27');\n"
		     "}\n"
		     "a=window.opener.location.href;\n" "l=a.length;\n"
		     "t=a.indexOf('/" SMAGIC "',1);\n" "t=a.indexOf('/',t+1);\n"
		     //"nu=\"%s\"+\"?t=%ld&b=\"+URLencode(a.substring(t+1,l));\n"
		     "nu=\"%s\"+\"?t=%ld&b=\"+a.substring(t+1,l);\n"
		     "opener.top.location.href=nu;window.close();</script>",
		     ub, now_t);
#else
		printf
		    ("<script>opener.top.location.href='%s?t=%d';window.close();</script>",
		     ub, now_t);
#endif
	} else {
		char buf[256];
		if (strcmp(x->userid, "guest") && shouldbroadcast(uid))
			sprintf(buf, "%s?t=%d&b=ooo", ub, (int) now_t);
		else
			sprintf(buf, "%s?t=%d", ub, (int) now_t);
		redirect(buf);
	}
	http_quit();
	return 0;
}

char *
wwwlogin(struct userec *user, int ipmask)
{
	FILE *fp1;
	int fd;
	int n, uid;
	struct user_info u;
	char *urlbase, fname[80];
	char buf[20];
	uid = getuser(user->userid, NULL);
	fd = open(MY_BBS_HOME "/" ULIST_BASE "." MY_BBS_DOMAIN, O_WRONLY);
	flock(fd, LOCK_EX);

	if ((urlbase = check_multi(user->userid, uid))) {
		flock(fd, LOCK_UN);
		close(fd);
		return urlbase;
	}

	if (strcasecmp(user->userid, "guest") && count_uindex(uid) >= 3) {
		flock(fd, LOCK_UN);
		close(fd);
		http_fatal("您已经登录了三个帐号,不能再登录了");
	}
	bzero(&u, sizeof (struct user_info));
	u.active = 1;
	u.uid = uid;
	u.pid = 1;
	u.mode = LOGIN;
	u.userlevel = user->userlevel;
	u.lasttime = now_t;
	u.curboard = 0;
	if (user_perm(user, PERM_LOGINCLOAK) && (user->flags[0] & CLOAK_FLAG))
		u.invisible = YEA;
	u.pager = 0;
	if (user->userdefine & DEF_FRIENDCALL)
		u.pager |= FRIEND_PAGER;
	if (user->flags[0] & PAGER_FLAG) {
		u.pager |= ALL_PAGER;
		u.pager |= FRIEND_PAGER;
	}
	if (user->userdefine & DEF_FRIENDMSG)
		u.pager |= FRIENDMSG_PAGER;
	if (user->userdefine & DEF_ALLMSG) {
		u.pager |= ALLMSG_PAGER;
		u.pager |= FRIENDMSG_PAGER;
	}
	strsncpy(u.from, fromhost, sizeof (u.from));
	u.fromIP = from_addr.s_addr;
	strsncpy(u.username, user->username, NAMELEN);
	strsncpy(u.userid, user->userid, IDLEN + 1);
	getrandomstr(u.sessionid);
	n = utmp_login(&u);
	if (n > MAXACTIVERUN || n <= 0) {
		flock(fd, LOCK_UN);
		close(fd);
		http_fatal
		    ("抱歉，目前在线用户数已达上限%d，无法登录。请稍后再来。",
		     MAXACTIVERUN);
	}
	flock(fd, LOCK_UN);
	close(fd);
	n--;
	urlbase = makeurlbase(n, uid);
	u_info = &(shm_utmp->uinfo[n]);
	w_info = &(u_info->wwwinfo);
	w_info->login_start_time = now_t;
	w_info->ipmask = ipmask;
	if (strcasecmp(user->userid, "guest")) {
		u_info->unreadmsg = get_unreadcount(user->userid);
		initfriends(u_info);
		sethomefile(fname, user->userid, "clubrights");
		if ((fp1 = fopen(fname, "r")) == NULL) {
			memset(u_info->clubrights, 0, CLUB_SIZE * sizeof (int));
		} else {
			fread(&(u_info->clubrights), sizeof (int), CLUB_SIZE,
			      fp1);
			fclose(fp1);
		}
		if (readuservalue(user->userid, "signature", buf, sizeof (buf))
		    >= 0)
			u_info->signature = atoi(buf);
		w_info->edit_mode = 1;
		set_my_cookie();
	} else {
		u_info->unreadmsg = 0;
		memset(u_info->friend, 0, sizeof (u.friend));
		memset(u_info->clubrights, 0, CLUB_SIZE * sizeof (int));
		w_info->t_lines = 20;
		w_info->att_mode = 0;
		w_info->doc_mode = 1;
	}

	if ((user->userlevel & PERM_BOARDS))
		setbmstatus(user, 1);
	return urlbase;
}

static int
do_check(int uent, int uid)
{
	return (shm_utmp->uinfo[uent].uid == uid
		&& shm_utmp->uinfo[uent].active == 1
		&& shm_utmp->uinfo[uent].pid == 1
		&& now_t - shm_utmp->uinfo[uent].lasttime < 18 * 60
		&& from_addr.s_addr == shm_utmp->uinfo[uent].fromIP);
}

static char *
check_multi(char *id, int uid)
{
	int i, uent;
	int h;
	if (uid <= 0 || uid > MAXUSERS)
		return NULL;
	if (strcasecmp(id, "guest")) {
		//这种算法, wwwlogin必须限制登录窗口数目, 否则
		//上线名单会被轻易冲爆
		for (i = 0; i < 6; i++) {
			uent = uindexshm->user[uid - 1][i] - 1;
			if (uent < 0)
				continue;
			if (do_check(uent, uid))
				return makeurlbase(uent, uid);
		}
		return NULL;
	} else {
		h = utmp_iphash(realfromhost);
		uent = shm_utmp->guesthash_head[h];
		while (uent != 0) {
			if (do_check(uent - 1, uid))
				return makeurlbase(uent - 1, uid);
			uent = shm_utmp->guesthash_next[uent];
		}
	}
	return NULL;
}
