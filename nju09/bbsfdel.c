#include "bbslib.h"

int
bbsfdel_main()
{
	FILE *fp;
	int i, total = 0;
	char path[80], userid[80];
	struct override f[200];
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("����δ��¼�����ȵ�¼");
	changemode(GMENU);
	sethomefile(path, currentuser->userid, "friends");
	printf("<center>%s -- �������� [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	strsncpy(userid, getparm("userid"), 13);
	if (userid[0] == 0) {
		printf("<form action=bbsfdel>\n");
		printf("��������ɾ���ĺ����ʺ�: <input type=text><br>\n");
		printf("<input type=submit>\n");
		printf("</form>");
		http_quit();
	}
	loadfriend(currentuser->userid);
	if (friendnum <= 0)
		http_fatal("��û���趨�κκ���");
	if (!isfriend(userid))
		http_fatal("���˱����Ͳ�����ĺ���������");
	for (i = 0; i < friendnum; i++) {
		if (strcasecmp(fff[i].id, userid)) {
			memcpy(&f[total], &fff[i], sizeof (struct override));
			total++;
		}
	}
	fp = fopen(path, "w");
	fwrite(f, sizeof (struct override), total, fp);
	fclose(fp);
	initfriends(u_info);
	printf
	    ("[%s]�Ѵ����ĺ���������ɾ��.<br>\n <a href=bbsfall>���غ�������</a>",
	     userid);
	http_quit();
	return 0;
}
