#include "bbslib.h"

static int inform(char *board, char *user);

int
bbsdenydel_main()
{
	int i;
	char board[80], *userid;
	struct boardmem *x;
	html_header(1);
	check_msg();
	if (!loginok)
		http_fatal("����δ��¼, ���ȵ�¼");
	changemode(READING);
	getparmboard(board, sizeof(board));
	if (!(x = getboard(board)))
		http_fatal("�����������");
	if (!has_BM_perm(currentuser, x))
		http_fatal("����Ȩ���б�����");
	loaddenyuser(board);
	userid = getparm("userid");
	if (!USERPERM(currentuser, PERM_SYSOP)
	    && !strcasecmp(userid, currentuser->userid))
		http_fatal("���ܸ��Լ����!");
	for (i = 0; i < denynum; i++) {
		if (!strcasecmp(denyuser[i].id, userid)) {
			denyuser[i].id[0] = 0;
			savedenyuser(board);
			printf("�Ѿ��� %s ���. <br>\n", userid);
			inform(board, userid);
			printf("[<a href=bbsdenyall?B=%d>���ر�������</a>]",
			       getbnumx(x));
			http_quit();
		}
	}
	http_fatal("����û����ڱ���������");
	http_quit();
	return 0;
}

static int
inform(char *board, char *user)
{
	FILE *fp;
	char path[80], title[80], buf[256];
	sprintf(title,  "%s �ָ� %s ��POSTȨ", currentuser->userid, user);
	sprintf(path, "bbstmpfs/tmp/%d.tmp", thispid);
	fp = fopen(path, "w");
	fprintf(fp, "����ƪ���������Զ�����ϵͳ��������\n\n");
	snprintf(buf, sizeof (buf), "%s �ָ� %s �� %s ��POSTȨ\n"
		 "�����%sվ�Ĺ�����,лл!\n\n\n", currentuser->userid, 
		user, board, MY_BBS_NAME);
	fputs(buf, fp);
	fclose(fp);
	securityreport(title, buf);
	post_article(board, title, path, "deliver", "�Զ�����ϵͳ",
		     "�Զ�����ϵͳ", -1, 0, 0, "deliver", -1);
	mail_file(path, user, title, "deliver");
	unlink(path);
	printf("ϵͳ�Ѿ�����֪ͨ��%s.<br>\n", user);
	return 0;
}
