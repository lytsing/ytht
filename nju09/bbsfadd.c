#include "bbslib.h"

int
bbsfadd_main()
{
	FILE *fp;
	char path[80], userid[80], exp[80];
	struct userec *x;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("����δ��¼�����ȵ�¼");
	changemode(GMENU);
	sethomefile(path, currentuser->userid, "friends");
	printf("<body><center>%s -- �������� [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	strsncpy(userid, getparm("userid"), 13);
	strsncpy(exp, getparm("exp"), 32);
	loadfriend(currentuser->userid);
	if (userid[0] == 0 || exp[0] == 0) {
		if (userid[0])
			printf("<font color=red>���������˵��</font>");
		printf("<form action=bbsfadd>\n");
		printf
		    ("������������ĺ����ʺ�: <input type=text name=userid value='%s'><br>\n",
		     userid);
		printf
		    ("�������������ѵ�˵��: <input type=text name=exp value=''>\n");
		printf("<br><input type=submit value=ȷ��></form>\n");
		http_quit();
	}
	if (getuser(userid, &x) <= 0) 
		http_fatal("�����ʹ�����ʺ�");
	if (friendnum >= MAXFRIENDS - 1)
		http_fatal("���ĺ��������Ѵﵽ����, ��������µĺ���");
	if (isfriend(userid))
		http_fatal("�����Ѿ�����ĺ�����������");
	strsncpy(fff[friendnum].id, x->userid, sizeof(fff[friendnum].id));
	strsncpy(fff[friendnum].exp, exp, sizeof(fff[friendnum].exp));
	friendnum++;
	fp = fopen(path, "w");
	fwrite(fff, sizeof (struct override), friendnum, fp);
	fclose(fp);
	initfriends(u_info);
	printf("[%s]�Ѽ������ĺ�������.<br>\n <a href=bbsfall>���غ�������</a>",
	       userid);
	http_quit();
	return 0;
}
