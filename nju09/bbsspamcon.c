#include "bbslib.h"
int
bbsspamcon_main()
{
	unsigned long long id;
	int magic;
	char title[60], sender[40];
	char path[256];
	char idbuf[100];
	int ret;
	int feedham;
	struct yspam_ctx *yctx;
	if ((!loginok || isguest) && (!tempuser))
		http_fatal("���ȵ�¼%d", tempuser);
	changemode(RMAIL);
	id = strtoull(getparm("id"), NULL, 10);
	magic = atoi(getparm("magic"));
	feedham = atoi(getparm("feed"));
	
	if (feedham) {
		html_header(1);
		printf("<body><center>\n");
		yctx = yspam_init("127.0.0.1");
		ret =yspam_feed_ham(yctx, currentuser->userid, id, magic);
		yspam_fini(yctx);
		if (ret == 0)
			printf("�ɹ�");
		else
			printf("��ʧ�� %d",ret);
		printf("<br><a href=bbsspam>���������ʼ��б�</a>");
		printf("</center></body>\n");
		http_quit();
	}
	snprintf(path, sizeof(path), MY_BBS_HOME"/maillog/%llu.bbs", id);
	if (*getparm("attachname") == '/') {
		showbinaryattach(path);
		return 0;
	}
	html_header(1);
	yctx = yspam_init("127.0.0.1");
	ret = yspam_getspam(yctx, currentuser->userid, id, magic, title, sender);
	yspam_fini(yctx);
	if (ret < 0) {
		printf("%d", ret);
		http_quit();
		http_fatal("�ڲ�����");	
	}
	printf("<body><center>\n");
	printf("%s -- �����ż� [ʹ����: %s]<hr>\n", BBSNAME, currentuser->userid);
	if (title[0] == 0)
		printf("</center>����: ����<br>");
	else
		printf("</center>����: %s<br>", void1(titlestr(title)));
	printf("������: %s<br>", void1(titlestr(sender)));
	printf("<center>");
	showcon(path);
	sprintf(idbuf, "%llu", id);
	printf("�ⲻ��һ�������ʼ���<a onclick='return confirm(\"�������Ĳ��������ʼ�������׼ȷ�жϽ����������Ǹ���ϵͳ\")' href=bbsspamcon?id=%s&magic=%d&feed=1>�뻹����</a><br>", idbuf, magic);
	printf("<a href=bbsspam>���������ʼ��б�</a>");
	printf("</center></body>\n");
	http_quit();
	return 0;
}
