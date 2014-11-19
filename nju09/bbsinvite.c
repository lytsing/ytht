#include "bbslib.h"

static int
printInviteForm()
{
	printf("<div class=line></div><div class=content0 align=left>"
	       "<b>邀请自己的朋友到" MY_BBS_NAME " BBS 来</b><br>"
	       "填写如下的表格，系统会自动发送一封邀请信给你的朋友，"
	       "如果他/她收到了信，并且进行注册，系统会自动把该用户加入到你的好友名单里。"
	       "同时你的生命值也会增加 50 点！<br>&nbsp;"
	       "<form action=bbsinvite?doit=1 method=post>"
	       "<table><tr><td align=right><nobr>他/她的 email：</nobr></td>"
	       "<td align=left><input type=text name=ivemail maxlength=55 value=''></td></tr>"
	       "<tr><td align=right>他/她的名字：</td>"
	       "<td align=left><input type=text name=ivname value=''></td></tr>"
	       "<tr><td align=right>邀请留言：</td>"
	       "<td align=left><textarea name=note cols=55 rows=5 wrap=virtual>"
	       "快来" MY_BBS_NAME " BBS 建立个用户吧"
	       "</textarea></td></tr>"
	       "</table><input type=submit value='确定'></form>"
	       "</div><div class=line></div>");
	return 0;
}

int
bbsinvite_main()
{
	char ivemail[60], ivname[40], *note;
	char tmpfn[256];
	int retv;
	html_header(1);
	check_msg();
	printf("<body><div class=swidth>");
	if (!loginok || isguest
	    || (currentuser->userlevel & PERM_DEFAULT) != PERM_DEFAULT)
		http_fatal("匆匆过客无法发送邀请, 请先登录");
	changemode(MMENU);
	if (atoi(getparm("doit")) == 0) {
		printInviteForm();
		http_quit();
		return 0;
	}

	strsncpy(ivemail, getparm("ivemail"), sizeof (ivemail));
	strsncpy(ivname, getparm("ivname"), sizeof (ivname));
	note = getparm("note");
	if (!*ivemail || !*ivname)
		http_fatal("被邀请人的 email 和名字都需要填写");
	if (!trustEmail(ivemail))
		http_fatal("该 email 无效，或者其使用者已经在本站注册");
	sprintf(tmpfn, "bbstmpfs/tmp/bbsinvite.%d", (int) getpid());
	f_write(tmpfn, note);
	retv = sendInvitation(currentuser->userid, ivemail, ivname, tmpfn);
	unlink(tmpfn);
	if (retv < 0) {
		http_fatal("发送 email 失败，代码 %d", retv);
	}
	tracelog("%s invite %s %s", currentuser->userid, ivemail, ivname);

	printf("<br>成功给 %s 发送邀请！<br>", ivname);
	printf("<a href=bbsinvite>再发送几个邀请</a>");
	printf("</div></body></html>");
	return 0;
}

