#include "bbslib.h"

int
bbsform_main()
{
	int type;
	struct userdata currentdata;
	html_header(1);
	type = atoi(getparm("type"));
	if (!loginok || isguest)
		http_fatal("����δ��¼, �����µ�¼��");
	changemode(NEW);
	printf("%s -- ��дע�ᵥ<hr>\n", BBSNAME);
	check_if_ok();
	if (type == 1) {
		check_submit_form();
		http_quit();
	}
	loaduserdata(currentuser->userid, &currentdata);
	printf
	    ("����, %s, ע�ᵥͨ���󼴿ɻ��ע���û���Ȩ��, ������������������д<br><hr>\n",
	     currentuser->userid);
	printf("<form method=post action=bbsform?type=1>\n");
	printf
	    ("��ʵ����: <input name=realname type=text maxlength=8 size=8 value='%s'><br>\n",
	     void1(nohtml(currentdata.realname)));
	printf
	    ("��ס��ַ: <input name=address type=text maxlength=32 size=32 value='%s'><br>\n",
	     void1(nohtml(currentdata.address)));
	printf
	    ("����绰(��ѡ): <input name=phone type=text maxlength=32 size=32><br>\n");
	printf
	    ("ѧУϵ��/��˾��λ: <input name=dept maxlength=60 size=40><br>\n");
	printf
	    ("У�ѻ�/��ҵѧУ(��ѡ): <input name=assoc maxlength=60 size=42><br><hr><br>\n");
	printf
	    ("<input type=submit value=ע��>  <input type=reset value=ȡ��>  <input type=button value=�鿴���� onclick=\"javascript:{open('/reghelp.html','winreghelp','width=600,height=460,resizeable=yes,scrollbars=yes');}\">\n");
	http_quit();
	return 0;
}

void
check_if_ok()
{
	if (user_perm(currentuser, PERM_LOGINOK))
		http_fatal("���Ѿ�ͨ����վ�������֤, �����ٴ���дע�ᵥ.");
	if (has_fill_form(currentuser->userid))
		http_fatal("Ŀǰվ����δ��������ע�ᵥ�������ĵȴ�.");
}

void
check_submit_form()
{
	FILE *fp;
	char dept[80], phone[80], assoc[80];
	struct userdata currentdata;
	if (strlen(getparm("realname")) < 4)
		http_fatal("��������ʵ����(��������, ����2����)");
	if (strlen(getparm("address")) < 16)
		http_fatal("��ס��ַ��������Ҫ16���ַ�(��8������)");
	if (strlen(getparm("dept")) < 14)
		http_fatal
		    ("������λ��ѧУϵ�������Ƴ�������Ҫ14���ַ�(��7������)");
	loaduserdata(currentuser->userid, &currentdata);
	fp = fopen("new_register", "a");
	if (fp == 0)
		http_fatal("ע���ļ�������֪ͨSYSOP");
	strsncpy(currentdata.realname, getparm("realname"),
		 sizeof (currentdata.realname));
	strsncpy(dept, getparm("dept"), 60);
	strsncpy(currentdata.address, getparm("address"),
		 sizeof (currentdata.address));
	strsncpy(phone, getparm("phone"), 60);
	strsncpy(assoc, getparm("assoc"), 60);
	fprintf(fp, "usernum: %d, %s\n", getuser(currentuser->userid, NULL),
		Ctime(now_t));
	fprintf(fp, "userid: %s\n", currentuser->userid);
	fprintf(fp, "realname: %s\n", currentdata.realname);
	fprintf(fp, "dept: %s\n", dept);
	fprintf(fp, "addr: %s\n", currentdata.address);
	fprintf(fp, "phone: %s\n", phone);
	fprintf(fp, "assoc: %s\n", assoc);
	fprintf(fp, "rereg: 1\n");
	fprintf(fp, "----\n");
	fclose(fp);
	printf
	    ("����ע�ᵥ�ѳɹ��ύ. վ�����������������, ��������������.<br>"
	     "<a href=bbsboa>���" MY_BBS_NAME "</a>");
}
