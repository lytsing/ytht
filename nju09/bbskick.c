#include "bbslib.h"
static void
printkickitem(const struct user_info *x)
{
	int dt = (now_t - x->lasttime) / 60;
	if (x->active == 0) {
		return;
	}
	printf("<tr>");
	printf("<td><a href=bbsqry?userid=%s>%s</a> ", x->userid, x->userid);
	printf
	    ("<a onclick='return confirm(\"�����Ҫ�߳����û���?\")' href=kick?k=%s&f=%s>X</a></td>",
	     x->userid, x->from);
	printf("<td><a href=bbsqry?userid=%s>%s</a></td>", x->userid,
	       x->userid);
	printf("<td><font class=c%d>%20.20s</font></td>", x->pid == 1 ? 35 : 32,
	       x->from);
	printf("<td>%s</td>", x->invisible ? "������..." : ModeType(x->mode));
	if (dt == 0) {
		printf("<td></td></tr>\n");
	} else {
		printf("<td>%d</td></tr>\n", dt);
	}
}

static void
printkicklist(const char *usertokick)
{
	struct user_info *x;
	int i, multilognum = 0;
	printf("<hr><table border=1>\n");
	printf
	    ("<tr><td>���</td><td>ʹ���ߴ���</td><td>ʹ�����ǳ�</td><td>��̬</td><td>����</td></tr>\n");
	for (i = 0; i < MAXACTIVE; i++) {
		x = &(shm_utmp->uinfo[i]);
		if (x->active == 0 || x->pid != 1)
			continue;
		if (!(strcasecmp(usertokick, x->userid))) {
			printkickitem(x);
			multilognum++;
		}
	}
	if (!multilognum) {
		printf("</table>");
		printf("û�������������߹,���ô���?<br>");
		printf("<a href=kick> ����һ��? </a>");
		return;
	}
	printf("</table><hr>");
}

int
bbskick_main()
{
	char *tokick;
	char *f;
	int i;
	struct user_info *x;
	html_header(1);
	check_msg();
	if (!loginok || isguest)
		http_fatal("�Ҵҹ��Ͳ��ܽ��д˲���, ���ȵ�¼");
	changemode(LUSERS);
	tokick = getparm("k");
	if (strlen(tokick) == 0) {	//û���û�������k��Ҫ������һ��,�������f
		printf
		    ("<hr><br><center><form action=kick name=userkicker method=post>");
		printf
		    ("������ϣ����ѯ������������û�id  <input type=text name=k ><input type=submit value=�鿴></form><hr>");
	} else {
		if (!(currentuser->userlevel & PERM_SYSOP)
		    && strcmp(currentuser->userid, tokick))
			http_fatal("����Ĳ���");
		f = getparm("f");
		// f����Ϊ0ʱһ����Ϊ�ǴӲ˵����ʵ�
		//���ɰ������id��ѯ����ͬʱ��k��f����������
		if ((strlen(f) == 0) && (currentuser->userlevel & PERM_SYSOP)) {
			printkicklist(tokick);
			return 0;
		} else {
			for (i = 0; i < MAXACTIVE; i++) {
				x = &(shm_utmp->uinfo[i]);
				if (x->active == 0 ||
				    x->pid != 1 || strcmp(x->userid, tokick) ||
				    strcmp(x->from, f))
					continue;
				x->wwwinfo.iskicked = 1;
				printf("�Ѿ��߳��û�");
				return 0;
			}
		}
		printf("�޷��ҵ�Ҫ�߳����û�");
	}
	return 0;
}
