#include "bbslib.h"

int
bbssndmail_main()
{
	char mymaildir[80], userid[80], filename[80], filename2[80], title[80], title2[80],
	    *content;
	char *ptr;
	int i, sig, backup, allfriend, mark = 0, reply = 0, num = 0, use_ubb;
	struct userec *u;
	struct fileheader fh;
	html_header(1);
	strsncpy(userid, getparm("userid"), 40);
	if (!loginok || (isguest && strcmp(userid, "SYSOP")))
		http_fatal("�Ҵҹ��Ͳ���д�ţ����ȵ�¼");
	sprintf(mymaildir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);
	if ((ptr=check_mailperm(currentuser)))
		http_fatal(ptr);
	changemode(SMAIL);
	strsncpy(title, getparm("title"), 50);
	backup = strlen(getparm("backup"));
	allfriend = strlen(getparm("allfriend"));
	reply = atoi(getparm("reply"));
	num = atoi(getparm("num")) - 1;
	use_ubb = strlen(getparm("useubb"));
	if (!strstr(userid, "@") && !allfriend) {
		if (getuser(userid, &u) <= 0)
			http_fatal("������������ʺ� %s", userid);
		strcpy(userid, u->userid);
		if (inoverride(currentuser->userid, userid, "rejects"))
			http_fatal("�޷����Ÿ������");
	}
	if (!USERPERM(currentuser, PERM_LOGINOK) && strcmp(userid, "SYSOP"))
		http_fatal("δͨ��ע���û�ֻ�ܸ�SYSOPд��!");
	for (i = 0; i < strlen(title); i++)
		if (title[i] <= 27 && title[i] >= -1)
			title[i] = ' ';
	sig = atoi(getparm("signature"));
	content = getparm("text");
	if (title[0] == 0)
		strcpy(title, "û����");
	sprintf(filename, "bbstmpfs/tmp/%d.tmp", thispid);
	sprintf(filename2, "bbstmpfs/tmp/%d.tmp2", thispid);
	if (use_ubb)
		ubb2ansi(content, filename2);
	else
		f_write(filename2, content);
	if (insertattachments_byfile(filename, filename2, currentuser->userid) > 0)
		mark |= FH_ATTACHED;
	unlink(filename2);
	if (!allfriend) {
		snprintf(title2, sizeof (title2), "{%s} %s", userid, title);
		post_mail(userid, title, filename, currentuser->userid,
			  currentuser->username, fromhost, sig - 1, mark);
	} else {
		loadfriend(currentuser->userid);
		snprintf(title2, sizeof (title2), "[Ⱥ���ż�] %.60s", title);
		for (i = 0; i < friendnum; i++) {
			if (getuser(fff[i].id, &u) <= 0) {
				u = NULL;
				continue;
			}
			if (inoverride
			    (currentuser->userid, fff[i].id, "rejects"))
				    continue;
			post_mail(fff[i].id, title2, filename,
				  currentuser->userid, currentuser->username,
				  fromhost, sig - 1, mark);
		}
	}
	if (backup)
		post_mail(currentuser->userid, title2, filename,
			  currentuser->userid, currentuser->username, fromhost,
			  sig - 1, mark);
	unlink(filename);
	if (reply > 0) {
		change_dir(mymaildir, &fh, (void *) DIR_do_replied, num + 1, 0, 0, NULL);
	}
	printf("�ż��Ѽĸ�%s.<br>\n", allfriend ? "���к���" : userid);
	if (backup)
		printf("�ż��Ѿ�����.<br>\n");
	printf("<a href='javascript:history.go(-2)'>����</a>");
	http_quit();
	return 0;
}
