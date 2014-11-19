#include "bbslib.h"

int
bbsfriend_main()
{
	int i, j, testreject, total = 0, uent, uid;
	struct user_info *user[MAXFRIENDS];
	struct user_info *up;
	int guestuid;
	html_header(1);
	if (!loginok || isguest)
		http_fatal("匆匆过客无法查看好友, 请先登录");
	check_msg();
	changemode(FRIEND);
	printf("<body><center>\n");
	printf("%s -- 在线好友列表 [使用者: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	guestuid = getuser("guest", NULL);
	for (i = 0; i < u_info->fnum; i++) {
		if (u_info->friend[i] == guestuid)
			continue;
		up = NULL;
		uid = u_info->friend[i];
		testreject = 0;
		if (uid <= 0 || uid > MAXUSERS)
			continue;
		for (j = 0; j < 6; j++) {
			uent = uindexshm->user[uid - 1][j];
			if (uent <= 0)
				continue;
			up = &(shm_utmp->uinfo[uent - 1]);
			if (!up->active || !up->pid || up->uid != uid)
				continue;
			if (!testreject) {
				if (isreject(up, u_info))
					break;
				testreject = 1;
			}
			if (shm_utmp->uinfo[uent - 1].invisible
			    && !USERPERM(currentuser,
					 (PERM_SYSOP | PERM_SEECLOAK)))
				    continue;
			user[total] = up;
			//memcpy(&user[total], up, sizeof (struct user_info));
			total++;
		}
	}
	printf("<table border=1>\n");
	printf
	    ("<tr><td>序号</td><td>友</td><td>使用者代号</td><td>使用者昵称</td><td>来自</td><td>动态</td><td>发呆</td></tr>\n");
	qsort(user, total, sizeof (struct user_info *), (void *) cmpuser);
	for (i = 0; i < total; i++) {
		int dt = (now_t - user[i]->lasttime) / 60;
		printf("<tr><td>%d</td>", i + 1);
		printf("<td>%s</td>", "√");
		printf("<td><a href=bbsqry?userid=%s>%s</a></td>",
		       user[i]->userid, user[i]->userid);
		printf("<td><a href=bbsqry?userid=%s>%24.24s</a></td>",
		       user[i]->userid, nohtml(void1(user[i]->username)));
		printf("<td><font class=c%d>%20.20s</font></td>",
		       user[i]->pid == 1 ? 35 : 32, user[i]->from);
		printf("<td>%s</td>",
		       user[i]->
		       invisible ? "隐身中..." : ModeType(user[i]->mode));
		if (dt == 0) {
			printf("<td></td></tr>\n");
		} else {
			printf("<td>%d</td></tr>\n", dt);
		}
	}
	printf("</table>\n");
	if (total == 0)
		printf("目前没有好友在线");
	printf("<hr>");
	printf("[<a href=bbsfall>全部好友名册</a>]");
	printf("</center></body>\n");
	http_quit();
	return 0;
}
