#include "bbslib.h"

char *defines[] = {
	"�������ر�ʱ���ú��Ѻ���",	/* DEF_FRIENDCALL */
	"���������˵�ѶϢ",	/* DEF_ALLMSG */
	"���ܺ��ѵ�ѶϢ",	/* DEF_FRIENDMSG */
	"�յ�ѶϢ��������",	/* DEF_SOUNDMSG */
	"ʹ�ò�ɫ",		/* DEF_COLOR */
	"��ʾ�����",		/* DEF_ACBOARD */
	"��ʾѡ����ѶϢ��",	/* DEF_ENDLINE */
	"�༭ʱ��ʾ״̬��",	/* DEF_EDITMSG */
	"ѶϢ������һ��/����ģʽ",	/* DEF_NOTMSGFRIEND */
	"ѡ������һ��/����ģʽ",	/* DEF_NORMALSCR */
	"������������ New ��ʾ",	/* DEF_NEWPOST */
	"�Ķ������Ƿ�ʹ���ƾ�ѡ��",	/* DEF_CIRCLE */
	"�Ķ������α�ͣ�ڵ�һƪδ��",	/* DEF_FIRSTNEW */
	"��վʱ��ʾ��������",	/* DEF_LOGFRIEND */
	"��վʱ��ʾ����¼",	/* DEF_INNOTE */
	"��վʱ��ʾ����¼",	/* DEF_OUTNOTE */
	"��վʱѯ�ʼĻ�����ѶϢ",	/* DEF_MAILMSG */
	"ʹ���Լ�����վ����",	/* DEF_LOGOUT */
	"���������֯�ĳ�Ա",	/* DEF_SEEWELC1 */
	"������վ֪ͨ",		/* DEF_LOGINFROM */
	"�ۿ����԰�",		/* DEF_NOTEPAD */
	"��Ҫ�ͳ���վ֪ͨ������",	/* DEF_NOLOGINSEND */
	"����ʽ����",		/* DEF_THESIS */
	"�յ�ѶϢ�Ⱥ��Ӧ�����",	/* DEF_MSGGETKEY */
	"��������ɾ��",		/* DEF_DELDBLCHAR */
	"ʹ��GB���Ķ�",		/* DEF_USEGB KCN 99.09.03 */
	"ʹ�ö�̬����",		/* DEF_ANIENDLINE */
	"���η��ʰ�����ʾ���뾫����",	/* DEF_INTOANN */
	"��������ʱ��ʱ����MSG",	/* DEF_POSTNOMSG */
	"��վʱ�ۿ�ͳ����Ϣ",	/* DEF_SEESTATINLOG */
	"���˿������˷�����Ϣ",	/* DEF_FILTERXXX */
	"��ȡվ���ż�",		/* DEF_INTERNETMAIL */
	NULL
};

int
bbsparm_main()
{
	int i, perm = 1, type;
	html_header(1);
	check_msg();
	type = atoi(getparm("type"));
	printf("<body><center>%s -- �޸ĸ��˲��� [ʹ����: %s]<hr>\n", BBSNAME,
	       currentuser->userid);
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ����趨����");
	changemode(USERDEF);
	if (type)
		return read_form();
	printf("<form action=bbsparm?type=1 method=post>\n");
	printf("<table>\n");
	for (i = 0; defines[i]; i++) {
		char *ptr = "";
		if (i % 2 == 0)
			printf("<tr>\n");
		if (currentuser->userdefine & perm)
			ptr = " checked";
		printf
		    ("<td><input type=checkbox name=perm%d%s></td><td>%s</td>",
		     i, ptr, defines[i]);
		perm = perm * 2;
	}
	printf("</table>");
	printf
	    ("<input type=submit value=ȷ���޸�></form><br>���ϲ���������telnet��ʽ�²�������\n");
	printf("</body>");
	http_quit();
	return 0;
}

int
read_form()
{
	int i, perm = 1, def = 0;
	char var[100];
	struct userec tmp;
	for (i = 0; i < 32; i++) {
		sprintf(var, "perm%d", i);
		if (strlen(getparm(var)) == 2)
			def += perm;
		perm = perm * 2;
	}
	memcpy(&tmp, currentuser, sizeof (tmp));
	tmp.userdefine = def;
	updateuserec(&tmp, 0);
	printf("���˲������óɹ�.<br><a href=bbsparm>���ظ��˲�������ѡ��</a>");
	return 0;
}
