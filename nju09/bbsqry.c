#include "bbslib.h"

int
bbsqry_main()
{
	FILE *fp;
	char userid[14], filename[80], buf[512];
	struct userec *x;
	int tmp2;
	struct in_addr in;
	html_header(1);
	check_msg();
	changemode(QUERY);
	strsncpy(userid, getparm("U"), 13);
	if (!userid[0])
		strsncpy(userid, getparm("userid"), 13);
	printf("<body><center>");
	printf("%s -- ��ѯ����<hr>\n", BBSNAME);
	if (userid[0] == 0) {
		printf("<form name=qry action=bbsqry>\n");
		printf
		    ("�������û���: <input name=userid maxlength=12 size=12>\n");
		printf("<input type=submit value=��ѯ�û�>\n");
		printf("</form><hr>\n");
		printf("<script>document.qry.userid.focus();</script>");
		http_quit();
	}
	if (getuser(userid, &x) <= 0) {
		printf("�����ܣ��϶������ô��ˣ�����û���˰�");
		printf("<p><a href=javascript:history.go(-1)>���ٷ���</a>");
		http_quit();
	}
	printf("</center><pre>\n");
	if (x->mypic) {
		printf("<table align=left><tr><td><center>");
		printmypic(x->userid);
		printf("</center></td></tr></table>");
	}
	printf("<b><font size=+1>%s</font></b> (<font class=gre>%s</font>) "
	       "����վ <font class=gre>%d</font> �Σ�"
	       "�������� <font class=gre>%d</font> ƪ\n",
	       x->userid, x->username, x->numlogins, x->numposts);
//      show_special(x->userid);

	in.s_addr = x->lasthost & 0x0000ffff;
	printf
	    ("�ϴ��� <font color=green>%s</font> �� <font color=green>%s</font> ����վһ�Ρ�<br>",
	     Ctime(x->lastlogin), inet_ntoa(in));
	mails(userid, &tmp2);
	printf("���䣺[<font color=green>%s</font>]��", tmp2 ? "��" : "  ");
	if (!strcasecmp(x->userid, currentuser->userid)) {
		printf
		    ("����ֵ��[<font color=green>%d</font>](<font color=olive>%s</font>) ",
		     countexp(x), cuserexp(countexp(x)));
		printf
		    ("����ֵ��[<font color=green>%d</font>](<font color=olive>%s</font>) ",
		     countperf(x), cperf(countperf(x)));
	}
	if (x->dieday) {
		printf
		    ("<br>�Ѿ��뿪������,����...<br>���� [<b>%d</b>] ���Ҫת��Ͷ̥��<br>",
		     countlife(x));
	} else {
		printf("��������[<font color=red>%d</font>]��<br>",
		       countlife(x));
		if (x->userlevel & PERM_BOARDS) {
			int i;
			printf("���ΰ���");
			for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++)
				bm_printboard(&shm_bcache->bcache[i],
					      x->userid);
			printf("<br>");
		}
		if (!show_onlinestate(userid)) {
			printf
			    ("Ŀǰ����վ��, �ϴ���վʱ�� [<font color=blue>%s</font>]\n\n",
			     (x->lastlogout >=
			      x->lastlogin) ? Ctime(x->
						    lastlogout) :
			     "�������ϻ��������߲���");
		}
	}
	printf("\n");
	printf("</pre><table width=100%%><tr><td>");
	sethomefile(filename, x->userid, "plans");
	fp = fopen(filename, "r");
	sprintf(filename, "00%s-plan", x->userid);
	fdisplay_attach(NULL, NULL, NULL, NULL);
	if (fp) {
		while (1) {
			if (fgets(buf, 256, fp) == 0)
				break;
			if (!strncmp(buf, "begin 644 ", 10)) {
				errlog("old attach %s", filename);
				fdisplay_attach(stdout, fp, buf, filename);
				continue;
			}
			fhhprintf(stdout, "%s", buf);
		}
		fclose(fp);
	} else {
		printf("<font color=teal>û�и���˵����</font><br>");
	}

	printf("</td></tr></table>");
	printf
	    ("<br><br><a href=bbspstmail?userid=%s&title=û����>[�������]</a> ",
	     x->userid);
	printf("<a href=bbssendmsg?destid=%s>[����ѶϢ]</a> ", x->userid);
	printf("<a href=bbsfadd?userid=%s>[�������]</a> ", x->userid);
	printf("<a href=bbsfdel?userid=%s>[ɾ������]</a> ", x->userid);
	if (x->userlevel & PERM_SPECIAL8) {
		printf
		    ("<a href=bbs0an?path=/groups/GROUP_0/Personal_Corpus/%c/%s>[�����ļ�]</a> ",
		     mytoupper(x->userid[0]), x->userid);
	}
	sethomefile(filename, x->userid, "B/config");
	if (file_isfile(filename)) {
		printf
		    ("<a href=blog?U=%s>[Blog]</a>", x->userid);
	}
	printf("<hr>");
	printf("<center><form name=qry action=bbsqry>\n");
	printf("�������û���: <input name=userid maxlength=12 size=12>\n");
	printf("<input type=submit value=��ѯ�û�>\n");
	printf("</form><hr>\n");
	printf("</body>\n");
	http_quit();
	return 0;
}

void
show_special(char *id2)
{
	FILE *fp;
	char id1[80], name[80];
	fp = fopen("etc/sysops", "r");
	if (fp == 0)
		return;
	while (1) {
		id1[0] = 0;
		name[0] = 0;
		if (fscanf(FCGI_ToFILE(fp), "%s %s", id1, name) <= 0)
			break;
		if (!strcmp(id1, id2))
			printf
			    (" <font color=red>��</font><font color=olive>%s</font><font color=red>��</font>",
			     name);
	}
	fclose(fp);
}

int
bm_printboard(struct boardmem *bmem, char *who)
{
	if (chk_BM_id(who, &bmem->header) && has_view_perm_x(currentuser, bmem)) {
		printf("<a href=bbsdoc?B=%d>", getbnumx(bmem));
		printf("%s", bmem->header.filename);
		printf("</a> ");
	}
	return 0;
}

int
show_onlinestate(char *userid)
{
	int uid, i, uent, num = 0;
	struct user_info *uentp;
	uid = getuser(userid, NULL);
	if (uid <= 0 || uid > MAXUSERS)
		return 0;
	for (i = 0; i < 6; i++) {
		uent = uindexshm->user[uid - 1][i];
		if (uent <= 0 || uent > MAXACTIVE)
			continue;
		uentp = &(shm_utmp->uinfo[uent - 1]);
		if (!uentp->active || !uentp->pid || uentp->uid != uid)
			continue;
		if (uentp->invisible && !USERPERM(currentuser, PERM_SEECLOAK))
			continue;
		num++;
		if (num == 1)
			printf("Ŀǰ��վ��, ״̬����:\n");
		if (uentp->invisible)
			printf("<font color=olive>C</font>");
		printf("<font color=%s>%s</font> ",
		       uentp->pid == 1 ? "magenta" : "blue",
		       ModeType(uentp->mode));
		if (num % 5 == 0)
			printf("<br>");
	}
	return num;
}
