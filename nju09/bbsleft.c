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
		       "�ʺ�<input type=text name=id maxlength=%d size=11><br>"
		       "����<input type=password name=pw maxlength=%d size=11><br>"
		       "<nobr>��Χ<select name=ipmask>\n"
		       "<option value=0 selected>��IP</option>\n"
		       "<option value=4>16 IP</option>\n"
		       "<option value=6>64 IP</option>\n"
		       "<option value=8>256 IP</option>\n"
		       "<option value=15>32768 IP</option></select>"
		       "<a href=/ipmask.html target=_blank><font class=red>?</font></a><nobr><br>&nbsp;"
		       "<input type=submit value=��¼>&nbsp;"
		       "<input type=submit value=ע�� onclick=\"{top.location.href='/"
		       SMAGIC "/bbsemailreg';return false}\">\n"
		       "</nobr></td></tr></form></table>\n", IDLEN,
		       PASSLEN - 1);
	} else {
		char buf[256] = "δע���û�";
		printf
		    ("�û�: <a href=bbsqry?userid=%s target=f3>%s</a><br>",
		     currentuser->userid, currentuser->userid);
		if (currentuser->userlevel & PERM_LOGINOK)
			strcpy(buf, cuserexp(countexp(currentuser)));
		if (currentuser->userlevel & PERM_BOARDS)
			strcpy(buf, "����");
		if (currentuser->userlevel & PERM_XEMPT)
			strcpy(buf, "�����ʺ�");
		if (currentuser->userlevel & PERM_SYSOP)
			strcpy(buf, "��վվ��");
		printf("����: %s<br>", buf);
		printf("<a href=bbslogout target=_top>ע�����ε�¼</a><br>\n");
	}
	printf("<hr>");
	printf("&nbsp; <a target=f3 href=boa?secstr=?>"
	       MY_BBS_NAME "����</a><br>\n");
	printf
	    ("&nbsp; <a target=f3 href=bbsshownav?a1=class&a2=all>���վ��ʻ���</a><br>\n");
	printf("&nbsp; <a target=f3 href=bbstop10>ʮ�����Ż���</a><br>\n");
	printf("&nbsp; <a target=f3 href=bbs0an>����������</a><br>\n");
	if (!strcmp(MY_BBS_ID, "ZSWX")) {
		if (user_perm(currentuser, PERM_POST))
			printf
			    ("&nbsp; <a target=f3 href=blog?U=%s>�ҵ�Blog(����)</a><br>\n",
			     currentuser->userid);
		else
			printf
			    ("&nbsp; <a target=f3 href=blogpage>Blog����(����)</a><br>\n");
	}
	if (loginok && !isguest) {
		char *ptr, buf[10];
		struct boardmem *x1;
		int mybrdmode;
		readuservalue(currentuser->userid, "mybrdmode", buf,
			      sizeof (buf));
		mybrdmode = atoi(buf);
		w_info->mybrd_mode = (mybrdmode != 0);
		printdiv(&div, "Ԥ��������");
		printf
		    ("&nbsp; <a target=f3 href=bbsboa?secstr=*>Ԥ��������</a><br>\n");
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
		    ("&nbsp; <a target=f3 href=bbsmybrd?mode=1>Ԥ������</a><br>\n");
		printf("</div>\n");
	}
	printdiv(&div, "����������");
	printsectree(&sectree);
	printf("&nbsp; <a target=f3 href=bbsall>����������</a><br>\n");
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
	printdiv(&div, "̸��˵��");
	if (loginok && !isguest) {
		printf("&nbsp; <a href=bbsfriend target=f3>���ߺ���</a><br>\n");
	}
//      printf
//          ("&nbsp;&nbsp;<a href=bbsufind?search=A&limit=20 target=f3>�����ķ�</a><br>\n");
	printf("&nbsp; <a href=bbsqry target=f3>��ѯ����</a><br>\n");
	if (currentuser->userlevel & PERM_PAGE) {
		printf
		    ("&nbsp; <a href=bbssendmsg target=f3>����ѶϢ</a><br>\n");
		printf
		    ("&nbsp; <a href=bbsmsg target=f3>�鿴����ѶϢ</a><br>\n");
	}
	printf("</div>\n");
	if (loginok && !isguest) {
#ifdef HTTPS_DOMAIN
		char str[STRLEN + 10], *ptr;
		char taskfile[256];
#endif
		printdiv(&div, "���˹�����");
#ifdef HTTPS_DOMAIN
		strsncpy(str, getsenv("SCRIPT_URL"), STRLEN);
		ptr = strrchr(str, '/');
		if (ptr)
			strcpy(ptr, "/bbspwd");
		printf("&nbsp;&nbsp;<a target=f3 href=https://" HTTPS_DOMAIN
		       "%s>�޸�����</a><br>", str);
#else
		printf("&nbsp; <a target=f3 href=bbspwd>�޸�����</a><br>");
#endif
		printf
		    ("&nbsp; <a target=f3 href=bbsinfo>�������Ϻ�ͷ��</a><br>"
		     "&nbsp; <a target=f3 href=bbsplan>��˵����</a><br>"
		     "&nbsp; <a target=f3 href=bbssig>��ǩ����</a><br>"
		     "&nbsp; <a target=f3 href=bbsparm>�޸ĸ��˲���</a><br>"
		     "&nbsp; <a target=f3 href=bbsmywww>WWW���˶���</a><br>"
		     "&nbsp; <a target=f3 href=bbsmyclass>������ʾ�İ���</a><br>"
		     "&nbsp; <a target=f3 href=bbsnick>��ʱ���ǳ�</a><br>"
		     "&nbsp; <a target=f3 href=bbsstat>����ͳ��</a><br>"
		     "&nbsp; <a target=f3 href=bbsfall>�趨����</a><br>");
#ifdef ENABLE_INVITATION
		if (loginok && !isguest &&
		    (currentuser->userlevel & PERM_DEFAULT) == PERM_DEFAULT)
			printf
			    ("&nbsp;&nbsp;<a target=f3 href=bbsinvite>����������%s</a><br>",
			     MY_BBS_NAME);
#endif
		if (currentuser->userlevel & PERM_CLOAK)
			printf("&nbsp;&nbsp;<a target=f3 "
			       "onclick='return confirm(\"ȷʵ�л�����״̬��?\")' "
			       "href=bbscloak>�л�����</a><br>\n");
		printf("</div>");
		printdiv(&div, "�����ż�");
		printf("&nbsp;&nbsp;<a target=f3 href=bbsnewmail>���ʼ�</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsmail>�����ʼ�</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsspam>�����ʼ�</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbspstmail>�����ʼ�</a><br>"
		       "</div>");
	}
	printdiv(&div, "�ر����");
	//printf("&nbsp;&nbsp;<a target=f3 href=bbssechand>�����г�</a><br>\n");
	printf("&nbsp;&nbsp;<a target=f3 href=/wnl.html>������</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgincce>�Ƽ��ʵ�</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/scicalc.html>��ѧ������</a><br>\n");
	if (!strcmp(MY_BBS_ID, "TTTAN")) {
		printf
		    ("&nbsp; <a target=f3 href=/unitConverter.html>��λת��</a><br>\n");
	}
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/periodic/periodic.html>Ԫ�����ڱ�</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgiman>Linux�ֲ��ѯ</a><br>\n");
	printf("&nbsp;&nbsp;<a href=bbsfind target=f3>���²�ѯ</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgifreeip>IP��ַ��ѯ</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=bbs0an?path=/groups/GROUP_0/Personal_Corpus>�����ļ���</a><br>\n");
//      printf("&nbsp;&nbsp;<a target=f3 href=bbsx?chm=1>���ؾ�����</a><br>\n");
//      printf
//          ("&nbsp;&nbsp;<a target=f3 href=home/pub/index.html>��������</a><br>\n");
	printf("</div>\n");
	printf("<div class=r>");
	if (!strcmp(MY_BBS_ID, "TTTAN")) {
		printf
		    ("&nbsp; <a target=f3 href=/music.xsl>����Ƶ��</a><br>\n");
	}
	printf("<hr>");
	printf("&nbsp;&nbsp;<form action=bbssearchboard method=post target=f3 style='display: inline;'>"
	     "<input type=text style='width:52pt' name=match id=match maxlength=24 "
	     "size=9 value=ѡ�������� onclick=\"this.select()\">"
	     "</form><br>\n");
	printf("&nbsp;&nbsp;<a href='telnet:%s'>Telnet��¼</a><br>\n", BBSHOST);
	printf("&nbsp;&nbsp;<a target=f3 href=home?B=BBSHelp>�û�����</a>\n");
#if 0
	if (!loginok || isguest)
		printf
		    ("<br>&nbsp;&nbsp;<a href=\"javascript: openreg()\">���û�ע��</a>\n");
#endif
	if (loginok && !isguest && !(currentuser->userlevel & PERM_LOGINOK)
	    && !has_fill_form(currentuser->userid))
		printf
		    ("<br>&nbsp;&nbsp;<a target=f3 href=bbsform><font color=red>��дע�ᵥ</font></a>\n");
	if (loginok && !isguest && USERPERM(currentuser, PERM_ACCOUNTS))
		printf
		    ("<br>&nbsp;&nbsp;<a href=bbsscanreg target=f3>SCANREG</a>");
	if (loginok && !isguest && USERPERM(currentuser, PERM_SYSOP))
		printf("<br>&nbsp;&nbsp;<a href=kick target=f3>��www��վ</a>");
	//if(loginok && !isguest) printf("<br>&nbsp;&nbsp;<a href='javascript:openchat()'>bbs���</a>");
	printf
	    ("<br>&nbsp;&nbsp;<a href=bbsselstyle target=f3>�������濴��</a>");
	printf("<table style='width:52pt'>"
	       "<tr><form><td>&nbsp;&nbsp;<input id=hideswitch type=button style='width:52pt' value='���ز˵�' "
	       "onclick=\"{parent.setCookie('AH',switchAutoHide(),9999999);return false;}\">"
	       "</td></form></tr></table>\n");
	//printf ("<br>&nbsp;&nbsp;<a href='http://bug.ytht.org/' target=_BLANK>���� Bug</a>");
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
			    ("<script>alert('���������ȫվ�������µ�Ȩ��, ��ο�sysop�湫��, ��������sysop��������. ��������, ����Arbitration���������.')</script>\n");
		mails(currentuser->userid, &i);
		if (i > 0)
			printf("<script>alert('�������ż�!')</script>\n");
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
