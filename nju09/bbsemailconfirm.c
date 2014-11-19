#include "bbslib.h"

int
doConfirm(struct userec *urec, char *email, int invited)
{
	char fnMailCheck[STRLEN], fnRegister[STRLEN], buf[STRLEN];
	struct userec tmpu = *urec;
	FILE *fp;
	tmpu.userlevel |= PERM_DEFAULT;
	updateuserec(&tmpu, 0);
	sethomefile(buf, urec->userid, "register.old");
	sethomefile(fnRegister, urec->userid, "register");
	rename(fnRegister, buf);
	sethomefile(fnMailCheck, urec->userid, "mailcheck");
	rename(fnMailCheck, fnRegister);
	mail_file("etc/s_fill", urec->userid, "恭禧您通过身份验证", "SYSOP");
	mail_file("etc/s_fill2", urec->userid, "欢迎加入" MY_BBS_NAME "大家庭",
		  "SYSOP");
	sprintf(buf, "%s 通过身分确认(%s).", urec->userid,
		invited ? "邀请" : "email");
	securityreport(buf, buf);
	fp = fopen(USEDEMAIL, "a");
	if (fp) {
		fprintf(fp, "%s %s\n", email, urec->userid);
		fclose(fp);
	}
	return 0;
}

int
bbsemailconfirm_main()
{
	char userid[IDLEN + 1], randStr[80], savedRandStr[80], passwd[80],
	    email[256];
	struct userec *urec;
	char emailCheckFile[80], iregpass[5];
	char *ub;
	html_header(1);
	printf("<body>");
	printf("<nobr><center>%s -- 新用户注册确认<hr>\n", BBSNAME);
	strsncpy(userid, getparm("userid"), sizeof (userid));
	strsncpy(randStr, getparm("randstr"), sizeof (randStr));
	strsncpy(passwd, getparm("pass"), sizeof (passwd));
	strsncpy(iregpass, getparm("regpass"), sizeof (iregpass));
	if (getuser(userid, &urec) <= 0) {
		printf("用户 <b>%s</b> 并不存在，如果是较早以前注册的，"
		       "请<a href=bbsemailreg>重新注册</a>。", userid);
		return 0;
	}
	strcpy(userid, urec->userid);
	if ((urec->userlevel & PERM_DEFAULT) == PERM_DEFAULT) {
		printf("用户 <b>%s</b> 已经通过验证，"
		       "请到<a href=/>首页</a>登录进站。", userid);
		return 0;
	}
	sethomefile(emailCheckFile, userid, "mailcheck");
	if (readstrvalue(emailCheckFile, "email", email, sizeof (email)) < 0) {
		printf("用户 <b>%s</b> 并没有使用 email 注册，"
		       "或者使用 email 注册之后填写了注册单，使用手工注册",
		       userid);
		return 0;
	}
	if (!trustEmail(email)) {
		printf("用户 <b>%s</b> 所提供的 email 可能已经被别的"
		       "用户所使用过，不能再次用于 email 验证。"
		       "请<a href=/>登录</a>本站并填写注册单，"
		       "以手工完成验证", userid);
		return 0;
	}
	if (readstrvalue
	    (emailCheckFile, "randstr", savedRandStr,
	     sizeof (savedRandStr)) < 0) {
		printf("系统并没有存储相应验证码。");
		return 0;
	}
#if 0
	if (checkbansite(realfromhost)) {
		http_fatal
		    ("对不起, 本站不欢迎来自 [%s] 的登录. <br>若有疑问, 请与SYSOP联系.",
		     realfromhost);
	}
#endif
	//check password...
	if (!checkpasswd(urec->passwd, urec->salt, passwd)) {
		//logattempt(urec->userid, realfromhost, "WWW", now_t);
		if (passwd[0])
			printf("<font color=red>用户密码不正确</font>");
		goto PRINTFORM;
	}
	//check regpass
	switch (checkRegPass(iregpass, urec->userid)) {
	case -1:
		http_fatal("验证码错误");
	case -2:
		http_fatal("验证码过期，发呆太久了吧");
	default:
		break;
	}

	if (strcmp(randStr, savedRandStr)) {
		printf
		    ("所提供的 email 验证码不正确，请使用最后一封确认信中的连接。");
		return 0;
	}
	//授予权限
	doConfirm(urec, email, 0);
	//登录
	ub = wwwlogin(urec, 0);
	//print....
	printf("恭喜您通过了身份验证！现在<a href=%s?t=%d>进入%s</a>", ub,
	       now_t, MY_BBS_NAME);
	printf("</body></html>");
	return 0;

      PRINTFORM:
	printf("<form name=regform method=post action=bbsemailconfirm>\n");
	printf("<table border=1>\n");
	printf("<input type=hidden name=userid value='%s'>", userid);
	printf("<input type=hidden name=randStr value='%s'>", randStr);
	printf("<tr><td align=right>用户：</td><td align=left>%s</td></tr>",
	       userid);
	printf
	    ("<tr><td align=right>Email 验证码：</td><td align=left>%s</td></tr>",
	     randStr);
	printf("<tr><td align=right>密码：</td><td align=left>"
	       "<input type=password name=pass size=%d maxlength=%d>\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td>&nbsp;</td><td align=left>"
	       "<img src=\"/cgi-bin/regimage?userid=%s\">", userid);
	printf("<tr><td align=right>请输入上面的数字：</td><td align=left>"
	       "<input name=regpass size=10 maxlength=10>\n");
	printf("</table><br><hr>\n");
	printf("<input type=submit value='确认'>");
	printf("</form></center>");
	http_quit();
	return 0;
}
