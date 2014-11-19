#include "bbslib.h"

int
bbsinfo_main()
{
	int type;
	struct userdata currentdata;
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest)
		http_fatal("����δ��¼");
	changemode(EDITUFILE);
	type = atoi(getparm("type"));
	printf("%s -- �û���������<hr>\n", BBSNAME);
	if (type != 0) {
		check_info();
		http_quit();
	}
	loaduserdata(currentuser->userid, &currentdata);
	if (currentuser->mypic) {
		printf("<table align=right><tr><td><center>");
		printmypic(currentuser->userid);
		printf("<br>��ǰͷ��</center></td></tr></table>");
	}
	printf
	    ("<form action=bbsinfo?type=1 method=post enctype='multipart/form-data'>");
	printf("�����ʺ�: %s<br>\n", currentuser->userid);
	printf
	    ("�����ǳ�: <input type=text name=nick value='%s' size=24 maxlength=%d>%d�����ֻ�%d��Ӣ����ĸ����<br>",
	     void1(nohtml(currentuser->username)), NAMELEN, NAMELEN / 2,
	     NAMELEN);
	printf("�������: %d ƪ<br>\n", currentuser->numposts);
//      printf("�ż�����: %d ��<br>\n", currentuser.nummails);
	printf("��վ����: %d ��<br>\n", currentuser->numlogins);
	printf("��վʱ��: %ld ����<br>\n", currentuser->stay / 60);
	printf
	    ("��ʵ����: <input type=text name=realname value='%s' size=16 maxlength=%d>%d�����ֻ�%d��Ӣ����ĸ����<br>\n",
	     void1(nohtml(currentdata.realname)), NAMELEN, NAMELEN / 2,
	     NAMELEN);
	printf
	    ("��ס��ַ: <input type=text name=address value='%s' size=40 maxlength=%d>%d�����ֻ�%d��Ӣ����ĸ����<br>\n",
	     void1(nohtml(currentdata.address)), STRLEN, STRLEN / 2, STRLEN);
	printf("�ʺŽ���: %s<br>", Ctime(currentuser->firstlogin));
	printf("�������: %s<br>", Ctime(currentuser->lastlogin));
	printf("��Դ��ַ: %s<br>", inet_ntoa(from_addr));
	printf
	    ("�����ʼ�: <input type=text name=email value='%s' size=32 maxlength=%d>%d ��Ӣ����ĸ����<br>\n",
	     void1(nohtml(currentdata.email)), sizeof (currentdata.email) - 1,
	     sizeof (currentdata.email) - 1);
	printf("����ͷ��: <input type=file name=mypic> 12K �ֽ�����<br>");
#if 0
	printf
	    ("��������: <input type=text name=year value=%d size=4 maxlength=4>��",
	     currentuser.birthyear + 1900);
	printf("<input type=text name=month value=%d size=2 maxlength=2>��",
	       currentuser.birthmonth);
	printf("<input type=text name=day value=%d size=2 maxlength=2>��<br>\n",
	       currentuser.birthday);
	printf("�û��Ա�: ");
	printf("��<input type=radio value=M name=gender %s>",
	       currentuser.gender == 'M' ? "checked" : "");
	printf("Ů<input type=radio value=F name=gender %s><br>",
	       currentuser.gender == 'F' ? "checked" : "");
#endif
	printf
	    ("<input type=submit value=ȷ��> <input type=reset value=��ԭ>\n");
	printf("</form>");
	printf("<hr>");
	printf("</body></html>");
	return 0;
}

int
check_info()
{
	int m;
	char buf[256];
	struct userdata currentdata;
	struct userec tmp;
	struct parm_file *parmFile;

	loaduserdata(currentuser->userid, &currentdata);
	memcpy(&tmp, currentuser, sizeof (tmp));
	strsncpy(buf, getparm("nick"), 30);
	for (m = 0; m < strlen(buf); m++)
		if ((buf[m] < 32 && buf[m] > 0) || buf[m] == -1)
			buf[m] = ' ';
	if (strlen(buf) > 1) {
		strcpy(tmp.username, buf);
	} else {
		printf("����: �ǳ�̫��!<br>\n");
	}
	strsncpy(buf, getparm("realname"), 9);
	if (strlen(buf) > 1) {
		strcpy(currentdata.realname, buf);
	} else {
		printf("����: ��ʵ����̫��!<br>\n");
	}
	strsncpy(buf, getparm("address"), 40);
	if (strlen(buf) > 8) {
		strcpy(currentdata.address, buf);
	} else {
		printf("����: ��ס��ַ̫��!<br>\n");
	}
	strsncpy(buf, getparm("email"), 32);
	if (strlen(buf) > 8 && strchr(buf, '@')) {
		strcpy(currentdata.email, buf);
	} else {
		printf("����: email��ַ���Ϸ�!<br>\n");
	}
	if ((parmFile = getparmfile("mypic"))) {
		if (parmFile->len > 12 * 1024) {
			printf("����: ͼƬ�ߴ���� 12K �ֽڣ���ѹ��ͼƬ���������ء�<br>");
		} else {
			char picfile[256];
			int fd;
			sethomefile(picfile, currentuser->userid, "mypic");
			fd = open(picfile, O_CREAT | O_WRONLY, 0660);
			if (fd > 0) {
				write(fd, parmFile->content, parmFile->len);
				close(fd);
			}
			tmp.mypic = 1;
		}
	}
	updateuserec(&tmp, 0);
	saveuserdata(currentuser->userid, &currentdata);
	printf("[%s] ���������޸ĳɹ�.", currentuser->userid);
	return 0;
}
