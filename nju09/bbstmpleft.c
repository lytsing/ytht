#include "bbslib.h"

int
bbstmpleft_main()
{
	int i;
	int div = 0;
	changemode(MMENU);
	if (0)
		errlog("%s-%s-%s-%s", getsenv("HTTP_ACCEPT_LANGUAGE"),
		       getsenv("Accept"), getsenv("Accept-Charset"),
		       getsenv("Accept-Encoding"));
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
	printf("<script src=" BBSLEFTJS "></script>\n"
	       "<body leftmargin=1 topmargin=3 MARGINHEIGHT=3 MARGINWIDTH=1"
	       " onMouseOver='doMouseOver()' onMouseEnter='doMouseOver()' onMouseOut='doMouseOut()'>\n<nobr>");
	if (!loginok || isguest) {
		printf("<table width=124>\n");
#ifdef HTTPS_DOMAIN
		printf("<form name=l action=https://" HTTPS_DOMAIN "/" SMAGIC
		       "/bbslogin method=post target=_top>");
		printf("<input type=hidden name=usehost value='http%s://%s:%s'>",
		       strcasecmp(getsenv("HTTPS"), "ON")?"":"s",
		       getsenv("HTTP_HOST"), getsenv("SERVER_PORT"));
#else
		printf("<form name=l action=bbslogin method=post target=_top>");
#endif
		printf("<tr><td>"
		       "<input type=hidden name=lastip1 value=''>"
		       "<input type=hidden name=lastip2 value=''>"
		       "�ʺ�<input type=text name=id maxlength=%d size=11><br>"
		       "����<input type=password name=pw maxlength=%d size=11><br>"
		       "��Χ<select name=ipmask>\n"
		       "<option value=0 selected>��IP</option>\n"
		       "<option value=1>2 IP</option>\n"
		       "<option value=2>4 IP</option>\n"
		       "<option value=3>8 IP</option>\n"
		       "<option value=4>16 IP</option>\n"
		       "<option value=5>32 IP</option>\n"
		       "<option value=6>64 IP</option>\n"
		       "<option value=7>128 IP</option>\n"
		       "<option value=8>256 IP</option>\n"
		       "<option value=15>32768 IP</option></select>"
		       "<a href=/ipmask.html target=_blank class=red>?</a>\n<br>&nbsp;"
		       "<input type=submit value=��¼>&nbsp;"
		       "<input type=submit value=ע�� onclick=\"{top.location.href='/"
		       SMAGIC "/bbsreg';return false}\">\n"
		       "</td></tr></form></table>\n", IDLEN, PASSLEN - 1);
	} else {
		char buf[256] = "δע���û�";
		printf("�û�: <a href=bbsqry?userid=%s target=f3>%s</a><br>",
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

	printf("&nbsp;&nbsp;<a target=f3 href=boa?secstr=?>" MY_BBS_ID
	       "����</a><br>\n");
//      printf("&nbsp;&nbsp;<a target=f3 href=bbs0an>����������</a><br>\n");
#if 0
	if (loginok && !isguest) {
		char *ptr, buf[10];
		struct boardmem *x1;
		int mybrdmode;
		readuservalue(currentuser->userid, "mybrdmode", buf,
			      sizeof (buf));
		mybrdmode = atoi(buf);
		w_info->mybrd_mode = (mybrdmode != 0);
		printdiv(&div, "Ԥ��������");
		readmybrd(currentuser->userid);
		for (i = 0; i < mybrdnum; i++) {
			ptr = mybrd[i];
			if (!mybrdmode) {
				x1 = getboard2(mybrd[i]);
				if (x1)
					ptr = void1(nohtml(x1->header.title));
			}
			printf
			    ("&nbsp;&nbsp;<a target=f3 href=home?B=%d>%s</a><br>\n",
			     getbnum(mybrd[i]), ptr);
		}
		printf
		    ("&nbsp;&nbsp;<a target=f3 href=bbsboa?secstr=*>Ԥ��������</a><br>\n");
		printf
		    ("&nbsp;&nbsp;<a target=f3 href=bbsmybrd?mode=1>Ԥ������</a><br>\n");
		printf("</div>\n");
	}
	printdiv(&div, "����������");
	printsectree(&sectree);
	printf("</div>\n");
#endif
#if 0
	printf("<div class=r>");
	for (i = 0; i < sectree.nsubsec; i++) {
		const struct sectree *sec = sectree.subsec[i];
		if (!sec->nsubsec)
			continue;
		printf("--<a target=f3 href=bbsboa?secstr=%s>%s</a><br>\n",
		       sec->basestr, sec->title);
	}
	printf("</div>\n");
#endif
	printdiv(&div, "̸��˵��");
	if (loginok && !isguest) {
		printf
		    ("&nbsp;&nbsp;<a href=bbsfriend target=f3>���ߺ���</a><br>\n");
	}
//      printf
//          ("&nbsp;&nbsp;<a href=bbsufind?search=A&limit=20 target=f3>�����ķ�</a><br>\n");
	printf("&nbsp;&nbsp;<a href=bbsqry target=f3>��ѯ����</a><br>\n");
	if (currentuser->userlevel & PERM_PAGE) {
		printf
		    ("&nbsp;&nbsp;<a href=bbssendmsg target=f3>����ѶϢ</a><br>\n");
		printf
		    ("&nbsp;&nbsp;<a href=bbsmsg target=f3>�鿴����ѶϢ</a><br>\n");
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
		printf("&nbsp;&nbsp;<a target=f3 href=bbspwd>�޸�����</a><br>");
#endif
		printf("&nbsp;&nbsp;<a target=f3 href=bbsinfo>��������</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsplan>��˵����</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbssig>��ǩ����</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsparm>�޸ĸ��˲���</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsmywww>WWW���˶���</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsmyclass>������ʾ�İ���</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsnick>��ʱ���ǳ�</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsstat>����ͳ��</a><br>"
		       "&nbsp;&nbsp;<a target=f3 href=bbsfall>�趨����</a><br>");
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
#ifdef HTTPS_DOMAIN
		sprintf(taskfile, "dllist/%s.task", currentuser->userid);
		strsncpy(str, getsenv("SCRIPT_URL"), STRLEN);
		ptr = strrchr(str, '/');
		if (ptr)
			strcpy(ptr, "/bbsdlprepare");
		if (file_exist(taskfile))
			printf("&nbsp;&nbsp;<a target=f3 href=https://"
			       HTTPS_DOMAIN "%s>�����ļ�����</a><br>", str);
#endif
	}
	printdiv(&div, "�ر����");
	//printf("&nbsp;&nbsp;<a target=f3 href=bbssechand>�����г�</a><br>\n");
	printf("&nbsp;&nbsp;<a target=f3 href=/wnl.html>������</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgincce>�Ƽ��ʵ�</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/scicalc.html>��ѧ������</a><br>\n");
	printf
	    ("&nbsp;&nbsp;<a target=f3 href=/periodic/periodic.html>Ԫ�����ڱ�</a><br>\n");
	/*printf
	   ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgiman>Linux�ֲ��ѯ</a><br>\n");
	   printf("&nbsp;&nbsp;<a href=bbsfind target=f3>���²�ѯ</a><br>\n");
	   printf
	   ("&nbsp;&nbsp;<a target=f3 href=/cgi-bin/cgifreeip>IP��ַ��ѯ</a><br>\n");
	   printf("&nbsp;&nbsp;<a target=f3 href=bbsx?chm=1>���ؾ�����</a><br>\n");
	 */
//      printf
//          ("&nbsp;&nbsp;<a target=f3 href=home/pub/index.html>��������</a><br>\n");
	printf("</div>\n");

	printf("<div class=r>");
	printf("<hr>");
	printf("<table width=124>");
	if (0)
		printf
		    ("<tr><form action=bbssearchboard method=post target=f3><td>&nbsp;&nbsp;"
		     "<input type=text style='width:90px' name=match maxlength=24 "
		     "size=9 value=ѡ�������� onclick=\"this.select()\"></td></form></tr>");
	printf
	    ("<tr><form><td>&nbsp;&nbsp;<input type=button style='width:90px' value='���ز˵�' "
	     "onclick=\"{if(switchAutoHide()==true) {this.value='ֹͣ�Զ�����';top.document.getElementById('fs0').cols='14,*';}"
	     "else this.value='���ز˵�';return false;}\">"
	     "</td></form></tr></table>\n");
	printf("&nbsp;&nbsp;<a target=f3 href=home?B=BBSHelp>�û�����</a>\n");
	//printf("<br>&nbsp;&nbsp;<a href=kick target=f3>��www��վ</a>");
	printf
	    ("<br>&nbsp;&nbsp;<a href=bbsselstyle target=f3>�������濴��</a>");
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
	//printf("<script src=/testdoc.js></script>");
	if (!loginok || isguest)
		setlastip();
	printf("</body></html>");
	return 0;
}
