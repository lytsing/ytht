#include "bbslib.h"

int
bbslogout_main()
{
	struct userec tmp;
	struct user_info up;
	int st;
	int uid;
	int fd;
	html_header(1);
	printf
	    ("<script>ex=new Date((new Date()).getTime()-1000000).toGMTString();\n"
	     "document.cookie='SESSION='+escape('')+';expires='+ex+';path=/';</script>");
	if (!loginok || isguest) {
		printf("似乎并没有登录啊...");
		redirect(FIRST_PAGE);
		http_quit();
	}
	memcpy(&tmp, currentuser, sizeof (tmp));
	if (now_t > tmp.lastlogin) {
		if (count_uindex(u_info->uid) == 1)
			st = now_t - tmp.lastlogin;
		else
			st = 0;
		//多窗口不重复计算上站时间
		tmp.lastlogout = now_t;
		if (st > 86400) {
			//      errlog("Strange long stay time,%d!, logout, %s", st, currentuser->userid);
		} else {
			tmp.stay += st;
			tracelog("%s exitbbs %d", currentuser->userid, st);
		}
	}
	updateuserec(&tmp, 0);
	uid = u_info->uid;
	brc_uentfinial(u_info);
	fd = open(MY_BBS_HOME "/" ULIST_BASE "." MY_BBS_DOMAIN, O_WRONLY);
	flock(fd, LOCK_EX);
	if (u_info->active) {
		memcpy(&up, u_info, sizeof (struct user_info));
		up.active = NA;
		up.pid = 0;
		up.invisible = YEA;
		up.sockactive = NA;
		up.sockaddr = 0;
		up.destuid = 0;
		up.lasttime = 0;
		utmp_logout(&utmpent, &up);
	}
	close(fd);
	if ((currentuser->userlevel & PERM_BOARDS) && count_uindex(uid) == 0)
		setbmstatus(currentuser, 0);
	redirect(FIRST_PAGE);
	return 0;
}
