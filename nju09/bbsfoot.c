#include "bbslib.h"

void
showmyclass()
{
	struct brcinfo *brcinfo;
	struct boardmem *bx;
	char *ptr = "";
	brcinfo = brc_readinfo(currentuser->userid);
	if ((bx = getboard(brcinfo->myclass))) {
		brc_initial(currentuser->userid, NULL);
		if (!brc_board_read(bx->header.filename, bx->lastpost))
			ptr = "<font color=red>";
		printf
		    ("<a href=home?B=%d target=f3 title='可以通过左侧工具栏--个人工具箱设置此处的版面'>%s%s%s</a> ",
		     getbnum(brcinfo->myclass), ptr,
		     brcinfo->myclasstitle[0] ? nohtml(brcinfo->
						       myclasstitle) :
		     nohtml(bx->header.title), ptr[0] ? "</font>" : "");
	} else {
		printf
		    ("<a href=bbsmyclass target=f3 title='点击此处设定自己的同学录'>我的班级</a> ");
	}
}

int
bbsfoot_main()
{
	int dt = 0, mail_total = 0, mail_unread = 0, lasttime = 0;
	char *id = "guest";
	static int r = 0;
	html_header(2);
	printf("<script>function t(){return (new Date()).valueOf();}</script>");
	printf("</head><body topmargin=1 marginheight=1 class=foot style='overflow:hidden;'>\n");
	if (loginok) {
		id = currentuser->userid;
		dt = abs(now_t - w_info->login_start_time) / 60;
	}
	printf("时间[%16.16s] ", Ctime(now_t));
	printf("在线[%d] ", count_online());
	if (loginok && !isguest)
		printf("好友[<a href=bbsfriend target=f3>%d</a>] ",
		       countFriend());
	printf("帐号[<a href=bbsqry?userid=%s target=f3>%s</a>] ", id, id);
	if (loginok && !isguest) {
		int thistime;
		lasttime = atoi(getparm("lt"));
		thistime = mails_time(id);
		if (thistime <= lasttime) {
			mail_total = atoi(getparm("mt"));
			mail_unread = atoi(getparm("mu"));
		} else {
			mail_total = mails(id, &mail_unread);
			lasttime = thistime;
		}
		if (mail_unread == 0) {
			printf("信箱[<a href=bbsmail target=f3>%d封</a>] ",
			       mail_total);
		} else {
			printf
			    ("信箱[<a href=bbsmail target=f3>%d(<font color=red>新信%d</font>)</a>] ",
			     mail_total, mail_unread);
		}
		showmyclass();
	}
	printf("停留[%d小时%d分] ", dt / 60, dt % 60);
	printf
	    ("<script>function refresha(){window.location.reload(false);}</script>");
	printf("<a href='javascript:refresha()'>刷新</a>");
	printf("<script>setTimeout('self.location.replace("
	       "\"bbsfoot?lt=%d&mt=%d&mu=%d&sn='+t()+'\")', %d);</script>",
	       lasttime, mail_total, mail_unread, 400000 + r * 1000);
	r = (r + dt + now_t) % 30;
	printf("</body></html>");
	return 0;
}

int
mails_time(char *id)
{
	char path[80];
	if (!loginok || isguest)
		return 0;
	sprintf(path, "mail/%c/%s/.DIR", mytoupper(id[0]), id);
	return file_time(path);
}

int
mails(char *id, int *unread)
{
	struct fileheader *x;
	char path[80];
	int total = 0, i;
      struct mmapfile mf = { ptr:NULL };
	*unread = 0;
	if (!loginok || isguest)
		return 0;
	setmailfile(path, id, ".DIR");
	MMAP_TRY {
		if (mmapfile(path, &mf) < 0) {
			MMAP_UNTRY;
			MMAP_RETURN(0);
		}
		total = mf.size / sizeof (struct fileheader);
		x = (struct fileheader *) mf.ptr;
		for (i = 0; i < total; i++) {
			if (!(x->accessed & FH_READ))
				(*unread)++;
			x++;
		}
	}
	MMAP_CATCH {
		total = 0;
		*unread = 0;
	}
	MMAP_END mmapfile(NULL, &mf);
	return total;

}

int
countFriend()
{
	int guestuid, uid, uent, testreject, total = 0;
	int i, j;
	struct user_info *up;
	if (!loginok || isguest)
		return 0;
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
			//memcpy(&user[total], up, sizeof (struct user_info));
			total++;
		}
	}
	return total;
}
