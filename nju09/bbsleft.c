#include "bbslib.h"

void
printdiv(int *n, char *str)
{
	printf("<div id=div%da class=r><A href='javascript:changemn(\"%d\");'>",
	       *n, *n);
	printf
	    ("<img border=0 id=img%d style='width:9pt' src=/folder.gif>%s</A></div>\n",
	     *n, str);
	printf("<div id=div%d class=s>\n", (*n)++);
}

void
printsectree(const struct sectree *sec)
{
	int i;
	for (i = 0; i < sec->nsubsec; i++) {
#if 0
		if (sec->subsec[i]->nsubsec)
			continue;
#endif
		printf("&nbsp; <a target=f3 href=boa?secstr=%s>"
		       "%s%c</a><br>\n", sec->subsec[i]->basestr,
		       nohtml(sec->subsec[i]->title),
		       sec->subsec[i]->nsubsec ? '+' : ' ');
	}
}

int
bbsleft_main()
{
	int i;
	int div = 0;
	changemode(MMENU);
	if (0)
		errlog
		    ("HTTP_ACCEPT_LANGUAGE %s; Accept: %s; Accept-Charset: %s; Accept-Encoding: %s",
		     getsenv("HTTP_ACCEPT_LANGUAGE"), getsenv("Accept"),
		     getsenv("Accept-Charset"), getsenv("Accept-Encoding"));
	if (1)
		errlog("HTTP_X_FORWARDED_FOR IP: %s",
		       getsenv("HTTP_X_FORWARDED_FOR IP"));
	html_header(2);
#if 0
	{
		char *ptr;
		char buf[256];
		ptr = getsenv("HTTP_USER_AGENT");
		sprintf(buf, "%-14.14s %.100s", currentuser->userid, ptr);
		addtofile(MY_BBS_HOME "/browser.log", buf);
	}
#endif
	printf
	    ("<body leftmargin=1 topmargin=3 margin-right=0 marginheight=3 marginwidth=1");
	if (strstr(getsenv("HTTP_USER_AGENT"), "Opera"))
		printf(" style=\"height:100%%;width:100%%;\"");
	printf(" onresize=\"doResize();\""
	       " onmouseover=\"doMouseOver();\""
	       " onmouseout=\"doMouseOut(event);\">");
	printf("<nobr>");
	if (!loginok || isguest) {
		printf("<table>\n");
#ifdef HTTPS_DOMAIN
		printf("<form name=l action=https://" HTTPS_DOMAIN
		       "/" SMAGIC "/bbslogin method=post target=_top>");
		printf("<input type=hidden name=usehost value='http%s://%s:%s'>",
		       strcasecmp(getsenv("HTTPS"), "ON") ? "" : "s",
		       getsenv("HTTP_HOST"), getsenv("SERVER_PORT"));
#else
		printf("<form name=l action=bbslogin method=post target=_top>");
#endif
		printf("<tr><td><nobr>"
		       "<input type=hidden name=lastip1 value=''>"
		       "<input type=hidden name=lastip2 value=''>"
		       "帐号<input type=text name=id maxlength=%d size=11><br>"
		       "密码<input type=password name=pw maxlength=%d size=11><br>"
		       "<nobr>范围<select name=ipmask>\n"
		       "<option value=0 selected>单IP</option>\n"
		       "<option value=4>16 IP</option>\n"
		       "<option value=6>64 IP</option>\n"
		       "<option value=8>256 IP</option>\n"
		       "<option value=15>32768 IP</option></select>"
		       "<a href=/ipmask.html target=_blank><font class=red>?</font></a><nobr><br>&nbsp;"
		       "<input type=submit value=登录>&nbsp;"
		       "<input type=submit value=注册 onclick=\"{top.location.href='/"
		       SMAGIC "/bbsemailreg';return false}\">\n"
		       "</nobr></td></tr></form></table>\n", IDLEN,
		       PASSLEN - 1);
	} else {
		char buf[256] = "未注册用户";
		printf
		    ("用户: <a href=bbsqry?userid=%s target=f3>%s</a><br>",
		     currentuser->userid, currentuser->userid);
		if (currentuser->userlevel & PERM_LOGINOK)
			strcpy(buf, cuserexp(countexp(currentuser)));
		if (currentuser->userlevel & PERM_BOARDS)
			strcpy(buf, "版主");
		if (currentuser->userlevel & PERM_XEMPT)
			strcpy(buf, "永久帐号");
		if (currentuser->userlevel & PERM_SYSOP)
			strcpy(buf, "本站站长");
		printf("级别: %s<br>", buf);
		printf("<a href=bbslogout target=_top>注销本次登录</a><br>\n");
	}
	printf("<hr>");
	printf("&nbsp; <a target=f3 href=boa?secstr=?>"
	       MY_BBS_NAME "导读</a><br>\n");
	printf
	    ("&nbsp; <a target=f3 href=bbsshownav?a1=class&a2=all>近日精彩话题</a><br>\n");
	printf("&nbsp; <a target=f3 href=bbstop10>十大热门话题</a><br>\n");
	printf("&nbsp; <a target=f3 href=bbs0an>精华公布栏</a><br>\n");
	if (!strcmp(MY_BBS_ID, "ZSWX")) {
		if (user_perm(currentuser, PERM_POST))
			printf
			    ("&nbsp; <a target=f3 href=blog?U=%s>我的Blog(测试)</a><br>\n",
			     currentuser->userid);
		else
			printf
			    ("&nbsp; <a target=f3 href=blogpage>Blog社区(测试)</a><br>\n");
	}
	if (loginok && !isguest) {
		char *ptr, buf[10];
		struct boardmem *x1;
		int mybrdmode;
		readuservalue(currentuser->userid, "mybrdmode", buf,
			      sizeof (buf));
		mybrdmode = atoi(buf);
		w_info->mybrd_mode = (mybrdmode != 0);
		printdiv(&div, "预定讨论区");
		printf
		    ("&nbsp; <a target=f3 href=bbsboa?secstr=*>预定区总览</a><br>\n");
		readmybrd(currentuser->userid);
		for (i = 0; i < mybrdnum; i++) {
			ptr = mybrd[i];
			if (!mybrdmode) {
				x1 = getboard2(mybrd[i]);
				if (x1)
					ptr = void1(nohtml(x1->header.title));
			}
			printf
			    ("&nbsp; <a target=f3 href=home?B=%d>%s</a><br>\n",
			     getbnum(mybrd[i]), ptr);
		}
		printf
		    ("&nbsp; <a target=f3 href=bbsmybrd?mode=1>预定管理</a><br>\n");
		printf("</div>\n");
	}
	printdiv(&div, "分类讨论区");
	printsectree(&sectree);
	printf("&nbsp; <a target=f3 href=bbsall>所有讨论区</a><br>\n");
	printf("</div>\n");
#if 0
	printf("<div class=r>");
	for (i = 0; i < sectree.nsubsec; i++) {
		const struct sectree *sec = sectree.subsec[i];
		if (!sec->nsubsec)
			continue;
		printf
		    ("--<a target=f3 href=bbsboa?secstr=%s>%s</a><br>\n",
		     sec->basestr, sec->title);
	}
	printf("</div>\n");
#endif
	printdiv(&div, "谈天说地");
	if (loginok && !isguest) {
		printf("&nbsp; <a href=bbsfriend target=f3>在线好友</a><br>\n");
	}
//      printf
//          ("&nbsp;&nbsp;<a href=bbsufind?search=A&limit=20 target=f3>环顾四方</a><br>\n");
	printf("&nbsp; <a href=bbsqry target=f3>查询网友</a><br>\n");
	if (currentuser->userlevel & PERM_PAGE) {
		printf
		    ("&nbsp; <a href=bbssendmsg target=f3>发送讯息</a><br>\n");
		printf
		    ("&nbsp; <a href=bbsmsg target=f3>查看所有讯息</a><br>\n");
	}
	printf("</div>\n");
	if (loginok && !isguest) {
#ifdef HTTPS_DOMAIN
		char str[STRLEN + 10], *ptr;
		char taskfile[256];
#endif
		printdiv(&div, "个人工具箱");
#ifdef HTTPS_DOMAIN
		strsncpy(str, getsenv("SCRIPT_URL"), STRLEN);
		ptr = strrchr(str, '/');
		if (ptr)
			strcpy(ptr, "/bbspwd");
		printf("&nbsp;&nbsp;<a target=f3 href=https://" HTTPS_DOMAIN
		       "%s>修改密码</a><br>", str);
#else
		printf("&nbsp; <a target=f3 href=bbspwd>修改密码</a><br>");
#endif
		printf
		    ("&nbsp; <a target=f3 href=bbsinfo>个人资料和头像</a><br>"
		     "&nbsp; <a target=f3 href=bbsplan>改说明档</a><br>"
		     "&nbsp; <a target=f3 href=bbssig>改签名档</a><br>"
		     "&nbsp; <a target=f3 href=bbsparm>修改个人参数</a><br>"
		     "&nbsp; <a target=f3 href=bbsmywww>WWW个人定制</a><br>"
		     "&nbsp; <a target=f3 href=bbsmyclass>底栏显示的版面</a><br>"
		     "&nbsp; <a target=f3 href=bbsnick>临时改昵称</a><br>"
		     "&nbsp; <a target=f3 href=bbsstat>排名统计</a><br>"
		     "&nbsp; <a target=f3 href=bbsfall>设定好友</a><br>");
#ifdef ENABLE_INVITATION
		if (loginok && !isguest &&
		    (currentuser->userlevel & PERM_DEFAULT) == PERM_DEFAULT)
			printf
			    ("&nbsp;&nbsp;<a target=f3 href=bbsinvite>邀请朋友来%s</a><br>",
			     MY_BBS_NAME);
#endif
		if (currentuser->userlevel & PERM_CLOAK)
			printf("&nbsp;&nbsp;<a target=f3 "
			       "onclick='return confirm(\"确实切换隐身状态吗?\")' "
			       "href=bbscloak>切换隐身</a><br>\n");
		printf("</div>");
		printdiv(&div, "处理信件");
		printf("&nbsp;&nbsp;<a target=f3 href=bbsnewmail>新邮件</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsmail>所有邮件</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsspam>垃圾邮件</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbspstmail>发送邮件</a><br>"
		       "</div>");
	}
	printdiv(&div, "特别服务");
	//printf("&nbsp;&nbsp;<a target=f3 href=bbssechand>二手市场</a><br>\n");
	printf("&nbsp;&nbsp;<a target=f3 href=/wnl.html>万年历</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgincce>科技词典</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/scicalc.html>科学计算器</a><br>\n");
	if (!strcmp(MY_BBS_ID, "TTTAN")) {
		printf
		    ("&nbsp; <a target=f3 href=/unitConverter.html>单位转换</a><br>\n");
	}
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/periodic/periodic.html>元素周期表</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgiman>Linux手册查询</a><br>\n");
	printf("&nbsp;&nbsp;<a href=bbsfind target=f3>文章查询</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgifreeip>IP地址查询</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=bbs0an?path=/groups/GROUP_0/Personal_Corpus>个人文集区</a><br>\n");
//      printf("&nbsp;&nbsp;<a target=f3 href=bbsx?chm=1>下载精华区</a><br>\n");
//      printf
//          ("&nbsp;&nbsp;<a target=f3 href=home/pub/index.html>常用下载</a><br>\n");
	printf("</div>\n");
	printf("<div class=r>");
	if (!strcmp(MY_BBS_ID, "TTTAN")) {
		printf
		    ("&nbsp; <a target=f3 href=/music.xsl>音乐频道</a><br>\n");
	}
	printf("<hr>");
	printf("&nbsp;&nbsp;<form action=bbssearchboard method=post target=f3 style='display: inline;'>"
	     "<input type=text style='width:52pt' name=match id=match maxlength=24 "
	     "size=9 value=选择讨论区 onclick=\"this.select()\">"
	     "</form><br>\n");
	printf("&nbsp;&nbsp;<a href='telnet:%s'>Telnet登录</a><br>\n", BBSHOST);
	printf("&nbsp;&nbsp;<a target=f3 href=home?B=BBSHelp>用户帮助</a>\n");
#if 0
	if (!loginok || isguest)
		printf
		    ("<br>&nbsp;&nbsp;<a href=\"javascript: openreg()\">新用户注册</a>\n");
#endif
	if (loginok && !isguest && !(currentuser->userlevel & PERM_LOGINOK)
	    && !has_fill_form(currentuser->userid))
		printf
		    ("<br>&nbsp;&nbsp;<a target=f3 href=bbsform><font color=red>填写注册单</font></a>\n");
	if (loginok && !isguest && USERPERM(currentuser, PERM_ACCOUNTS))
		printf
		    ("<br>&nbsp;&nbsp;<a href=bbsscanreg target=f3>SCANREG</a>");
	if (loginok && !isguest && USERPERM(currentuser, PERM_SYSOP))
		printf("<br>&nbsp;&nbsp;<a href=kick target=f3>踢www下站</a>");
	//if(loginok && !isguest) printf("<br>&nbsp;&nbsp;<a href='javascript:openchat()'>bbs茶馆</a>");
	printf
	    ("<br>&nbsp;&nbsp;<a href=bbsselstyle target=f3>换个界面看看</a>");
	printf("<table style='width:52pt'>"
	       "<tr><form><td>&nbsp;&nbsp;<input id=hideswitch type=button style='width:52pt' value='隐藏菜单' "
	       "onclick=\"{parent.setCookie('AH',switchAutoHide(),9999999);return false;}\">"
	       "</td></form></tr></table>\n");
	//printf ("<br>&nbsp;&nbsp;<a href='http://bug.ytht.org/' target=_BLANK>报告 Bug</a>");
	if (1 || strcmp(MY_BBS_ID, "YTHT"))
		printf("<br><br><center><img src=/coco.gif>");
	else {
		printf
		    ("<br><center><a href=http://www.cbe-amd.com target=_blank><img src=/cbe-amd.gif border=0></a>");
		printf
		    ("<br><center><a href=http://www.amdc.com.cn/products/cpg/amd64/ target=_blank><img src=/AMD64_logo.gif border=0></a>");
	}
	printf("</div>");
	printf("<script>if(isNS4) arrange();if(isOP)alarrangeO();</script>");
	if (loginok && !isguest) {
		if (USERPERM(currentuser, PERM_LOGINOK)
		    && !USERPERM(currentuser, PERM_POST))
			printf
			    ("<script>alert('您被封禁了全站发表文章的权限, 请参看sysop版公告, 期满后在sysop版申请解封. 如有异议, 可在Arbitration版提出申诉.')</script>\n");
		mails(currentuser->userid, &i);
		if (i > 0)
			printf("<script>alert('您有新信件!')</script>\n");
	}
	// if(loginok&&currentuser.userdefine&DEF_ACBOARD)
	//              printf("<script>window.open('bbsmovie','','left=200,top=200,width=600,height=240');</script>"); 
	//virusalert();
	if (isguest && 0)
		printf
		    ("<script>setTimeout('open(\"regreq\", \"winREGREQ\", \"width=600,height=460\")', 1800000);</script>");
	if (loginok && !isguest) {
		char filename[80];
		sethomepath(filename, currentuser->userid);
		mkdir(filename, 0755);
		sethomefile(filename, currentuser->userid, BADLOGINFILE);
		if (file_exist(filename)) {
			printf("<script>"
			       "window.open('bbsbadlogins', 'badlogins', 'toolbar=0, scrollbars=1, location=0, statusbar=1, menubar=0, resizable=1, width=450, height=300');"
			       "</script>");
		}
	}
	if (!via_proxy && wwwcache->text_accel_port
	    && wwwcache->text_accel_addr.s_addr)
		printf("<script src=http://%s:%d/testdoc.js></script>",
		       inet_ntoa(wwwcache->text_accel_addr),
		       wwwcache->text_accel_port);
	else if (via_proxy)
		w_info->doc_mode = 0;
	printf("<script>setTimeout('if(autoHide==false&&"
	       "parent.getCookie(\"AH\")==\"true\")"
	       "switchAutoHide();',10000);</script>");
	//printf("<script src=/testdoc.js></script>");
	if (!loginok || isguest)
		setlastip();
	printf("</body></html>");
	return 0;
}

/*
 * void
virusalert()
{
	if (file_has_word("virusalert.txt", realfromhost)) {
		printf
		    ("<script>window.open('/virusalert.html','','left=200,top=200,width=250,height=80');</script>");
	}
}
*/
