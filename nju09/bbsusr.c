#include "bbslib.h"

int
bbsusr_main()
{
	int i, start, total = 0;
	struct user_info *x, **user;
	char buf[128];
	html_header(1);
	check_msg();
	changemode(LUSERS);
	printf("<body><center>\n");
	printf("%s -- �����û��б� [Ŀǰ����: %d��]<hr>\n", BBSNAME,
	       count_online());
	user = malloc(sizeof (struct user_info *) * MAXACTIVE);
	if (!user)
		http_fatal("Insufficient memory");
	for (i = 0; i < MAXACTIVE; i++) {
		x = &(shm_utmp->uinfo[i]);
		if (x->active == 0)
			continue;
		if (x->invisible && !HAS_PERM(PERM_SEECLOAK))
			continue;
		user[total] = x;
		//memcpy(&user[total], x, sizeof (struct user_info));
		total++;
	}
	start = atoi(getparm("start"));
	printf("<table border=1>\n");
	printf
	    ("<tr><td>���</td><td>��</td><td>ʹ���ߴ���</td><td>ʹ�����ǳ�</td><td>����</td><td>��̬</td><td>����</td></tr>\n");
	qsort(user, total, sizeof (struct user_info *), (void *) cmpuser);
	if (start > total - 5)
		start = total - 5;
	if (start < 0)
		start = 0;
	for (i = start; i < start + w_info->t_lines && i < total; i++) {
		int dt = (now_t - user[i]->lasttime) / 60;
		printf("<tr><td>%d</td>", i + 1);
		printf("<td>%s</td>", isfriend(user[i]->userid) ? "��" : "  ");
		printkick(buf, sizeof (buf), user[i]);
		printf("<td><a href=bbsqry?userid=%s>%s</a> %s</td>",
		       user[i]->userid, user[i]->userid, buf);
		printf("<td><a href=bbsqry?userid=%s>%24.24s </a>",
		       user[i]->userid, nohtml(void1(user[i]->username)));
		printf("<td><font class=c%d>%20.20s</font></td>",
		       user[i]->pid == 1 ? 35 : 32, user[i]->from);
		printf("<td>%s</td>",
		       user[i]->
		       invisible ? "������..." : ModeType(user[i]->mode));
		if (dt == 0) {
			printf("<td></td></tr>\n");
		} else {
			printf("<td>%d</td></tr>\n", dt);
		}
	}
	free(user);
	printf("</table>\n");
	printf("<hr>");
	if (currentuser.userlevel & PERM_SYSOP)
		printf("[<a href='bbsufind?search=*'>ȫ��</a>] ");
	for (i = 'A'; i <= 'Z'; i++)
		printf("[<a href=bbsufind?search=%c>%c</a>]", i, i);
	printf("<br>\n");
	printf("[<a href=bbsfriend>���ߺ���</a>] ");
	if (start > 0)
		printf("[<a href=bbsusr?start=%d>��һҳ</a>]", start - 20);
	if (start < total - w_info->t_lines)
		printf("[<a href=bbsusr?start=%d>��һҳ</a>]",
		       start + w_info->t_lines);
	printf("<br><form action=bbsusr>\n");
	printf("<input type=submit value=��ת����> ");
	printf("<input type=input size=4 name=start> ��ʹ����</form>");
	printf("</center></body>\n");
	http_quit();
	return 0;
}
