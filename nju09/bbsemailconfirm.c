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
	mail_file("etc/s_fill", urec->userid, "������ͨ�������֤", "SYSOP");
	mail_file("etc/s_fill2", urec->userid, "��ӭ����" MY_BBS_NAME "���ͥ",
		  "SYSOP");
	sprintf(buf, "%s ͨ�����ȷ��(%s).", urec->userid,
		invited ? "����" : "email");
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
	printf("<nobr><center>%s -- ���û�ע��ȷ��<hr>\n", BBSNAME);
	strsncpy(userid, getparm("userid"), sizeof (userid));
	strsncpy(randStr, getparm("randstr"), sizeof (randStr));
	strsncpy(passwd, getparm("pass"), sizeof (passwd));
	strsncpy(iregpass, getparm("regpass"), sizeof (iregpass));
	if (getuser(userid, &urec) <= 0) {
		printf("�û� <b>%s</b> �������ڣ�����ǽ�����ǰע��ģ�"
		       "��<a href=bbsemailreg>����ע��</a>��", userid);
		return 0;
	}
	strcpy(userid, urec->userid);
	if ((urec->userlevel & PERM_DEFAULT) == PERM_DEFAULT) {
		printf("�û� <b>%s</b> �Ѿ�ͨ����֤��"
		       "�뵽<a href=/>��ҳ</a>��¼��վ��", userid);
		return 0;
	}
	sethomefile(emailCheckFile, userid, "mailcheck");
	if (readstrvalue(emailCheckFile, "email", email, sizeof (email)) < 0) {
		printf("�û� <b>%s</b> ��û��ʹ�� email ע�ᣬ"
		       "����ʹ�� email ע��֮����д��ע�ᵥ��ʹ���ֹ�ע��",
		       userid);
		return 0;
	}
	if (!trustEmail(email)) {
		printf("�û� <b>%s</b> ���ṩ�� email �����Ѿ������"
		       "�û���ʹ�ù��������ٴ����� email ��֤��"
		       "��<a href=/>��¼</a>��վ����дע�ᵥ��"
		       "���ֹ������֤", userid);
		return 0;
	}
	if (readstrvalue
	    (emailCheckFile, "randstr", savedRandStr,
	     sizeof (savedRandStr)) < 0) {
		printf("ϵͳ��û�д洢��Ӧ��֤�롣");
		return 0;
	}
#if 0
	if (checkbansite(realfromhost)) {
		http_fatal
		    ("�Բ���, ��վ����ӭ���� [%s] �ĵ�¼. <br>��������, ����SYSOP��ϵ.",
		     realfromhost);
	}
#endif
	//check password...
	if (!checkpasswd(urec->passwd, urec->salt, passwd)) {
		//logattempt(urec->userid, realfromhost, "WWW", now_t);
		if (passwd[0])
			printf("<font color=red>�û����벻��ȷ</font>");
		goto PRINTFORM;
	}
	//check regpass
	switch (checkRegPass(iregpass, urec->userid)) {
	case -1:
		http_fatal("��֤�����");
	case -2:
		http_fatal("��֤����ڣ�����̫���˰�");
	default:
		break;
	}

	if (strcmp(randStr, savedRandStr)) {
		printf
		    ("���ṩ�� email ��֤�벻��ȷ����ʹ�����һ��ȷ�����е����ӡ�");
		return 0;
	}
	//����Ȩ��
	doConfirm(urec, email, 0);
	//��¼
	ub = wwwlogin(urec, 0);
	//print....
	printf("��ϲ��ͨ���������֤������<a href=%s?t=%d>����%s</a>", ub,
	       now_t, MY_BBS_NAME);
	printf("</body></html>");
	return 0;

      PRINTFORM:
	printf("<form name=regform method=post action=bbsemailconfirm>\n");
	printf("<table border=1>\n");
	printf("<input type=hidden name=userid value='%s'>", userid);
	printf("<input type=hidden name=randStr value='%s'>", randStr);
	printf("<tr><td align=right>�û���</td><td align=left>%s</td></tr>",
	       userid);
	printf
	    ("<tr><td align=right>Email ��֤�룺</td><td align=left>%s</td></tr>",
	     randStr);
	printf("<tr><td align=right>���룺</td><td align=left>"
	       "<input type=password name=pass size=%d maxlength=%d>\n",
	       PASSLEN - 1, PASSLEN - 1);
	printf("<tr><td>&nbsp;</td><td align=left>"
	       "<img src=\"/cgi-bin/regimage?userid=%s\">", userid);
	printf("<tr><td align=right>��������������֣�</td><td align=left>"
	       "<input name=regpass size=10 maxlength=10>\n");
	printf("</table><br><hr>\n");
	printf("<input type=submit value='ȷ��'>");
	printf("</form></center>");
	http_quit();
	return 0;
}
