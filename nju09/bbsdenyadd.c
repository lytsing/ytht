#include "bbslib.h"

static int show_form(char *board);
static int inform(char *board, char *user, char *exp, int dt);

int
bbsdenyadd_main()
{
	int i;
	char exp[80], board[80], *userid;
	int dt;
	struct userec *x;
	struct boardmem *x1;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("����δ��¼, ���ȵ�¼");
	changemode(READING);
	getparmboard(board, sizeof(board));
	strsncpy(exp, getparm("exp"), 30);
	dt = atoi(getparm("dt"));
	if (!(x1 = getboard(board)))
		http_fatal("�����������");
	if (!has_BM_perm(currentuser, x1))
		http_fatal("����Ȩ���б�����");
	loaddenyuser(board);
	userid = getparm("userid");
	if (userid[0] == 0)
		return show_form(board);
	if (getuser(userid, &x) <= 0)
		http_fatal("�����ʹ�����ʺ�");
	if (!has_post_perm(x, x1))
		http_fatal("����˱�����û��postȨ");
	strcpy(userid, x->userid);
	if (!(currentuser->userlevel & PERM_SYSOP) && (dt > 14))
		http_fatal("���ʱ�����14��,������Ȩ��,����Ҫ,����ϵվ��");
	if (dt < 1 || dt > 99)
		http_fatal("�����뱻������(1-99)");
	if (exp[0] == 0)
		http_fatal("���������ԭ��");
	for (i = 0; i < denynum; i++)
		if (!strcasecmp(denyuser[i].id, userid))
			http_fatal("���û��Ѿ�����");
	if (denynum > 40)
		http_fatal("̫���˱�����");
	strsncpy(denyuser[denynum].id, userid, 13);
	strsncpy(denyuser[denynum].exp, exp, 30);
	denyuser[denynum].free_time = now_t + dt * 86400;
	denynum++;
	savedenyuser(board);
	printf("��� %s �ɹ�<br>\n", userid);
	tracelog("%s deny %s %s", currentuser->userid, board, userid);
	inform(board, userid, exp, dt);
	printf("[<a href=bbsdenyall?B=%d>���ر����ʺ�����</a>]", getbnumx(x1));
	http_quit();
	return 0;
}

static int
show_form(char *board)
{
	printf("<center>%s -- ������� [������: %s]<hr>\n", BBSNAME, board);
	printf
	    ("<form action=bbsdenyadd><input type=hidden name=board value='%s'>",
	     board);
	printf
	    ("���ʹ����<input name=userid size=12> ����POSTȨ <input name=dt size=2> ��, ԭ��<input name=exp size=20>\n");
	printf("<input type=submit value=ȷ��></form>");
	return 0;
}

static int
inform(char *board, char *user, char *exp, int dt)
{
	FILE *fp;
	char path[80], title[80], buf[512];
	struct tm *tmtime;
	time_t daytime = now_t + (dt + 1) * 86400;
	tmtime = gmtime(&daytime);
	sprintf(title, "%s �� %s ȡ����%s��POSTȨ", user, currentuser->userid, board);
	sprintf(path, "bbstmpfs/tmp/%d.tmp", thispid);
	fp = fopen(path, "w");
	fprintf(fp, "����ƪ���������Զ�����ϵͳ��������\n\n");
	snprintf(buf, sizeof (buf),
		"����ԭ��: %s\n"
		"��������: %d\n"
		"�������: %d��%d��\n"
		"������Ա: %s\n"
		"���취: ��ϵ������ǰ������ϵͳ�Զ����\n"
		"�������飬���������Ա�������Arbitration��Ͷ��\n\n\n",
		 exp, dt, tmtime->tm_mon + 1, tmtime->tm_mday,
		currentuser->userid);
	fputs(buf, fp);
	fclose(fp);
	securityreport(title, buf);
	post_article(board, title, path, "deliver", "�Զ�����ϵͳ",
		     "�Զ�����ϵͳ", -1, 0, 0, "deliver", -1);
	if (!hideboard(board))
		post_article("Penalty", title, path, "deliver", "�Զ�����ϵͳ",
			     "�Զ�����ϵͳ", -1, 0, 0, "deliver", -1);
	mail_file(path, user, title, "deliver");
	unlink(path);
	printf("ϵͳ�Ѿ�����֪ͨ��%s.<br>\n", user);
	return 0;
}
