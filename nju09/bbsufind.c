#include "bbslib.h"

void
printkick(char *buf, int size, struct user_info *x)
{
	if (!isguest && x->pid == 1
	    &&
	    ((currentuser->userlevel & PERM_SYSOP
	      || !strcmp(currentuser->userid, x->userid))))
		    snprintf(buf, size,
			     "<a onclick='return confirm(\"�����Ҫ�߳����û���?\")' href=kick?k=%s&f=%s>X</a>",
			     x->userid, x->from);
	else
		buf[0] = 0;
}

int
bbsufind_main()
{
	int i, total = 0, total2 = 0;
	int limit;
	struct user_info *x, **user;
	char search;
	char buf[128];
	html_header(1);
	check_msg();
	changemode(LUSERS);
	printf("<body><center>\n");
	printf("%s -- �����û���ѯ [����������: %d��]<hr>\n", BBSNAME,
	       count_online());
	search = toupper(getparm("search")[0]);
	limit = atoi(getparm("limit"));
	if (limit <= 0)
		limit = 0;
	if (search != '*' && (search < 'A' || search > 'Z')) {
		http_fatal("����Ĳ���");
	}
	if (search == '*') {
		if (!(currentuser->userlevel & PERM_SYSOP)) {
			http_fatal("����Ȩʹ�øò���");
		}
		printf("��������ʹ����<br>\n");
	} else {
		printf("��ĸ'%c'��ͷ������ʹ����. \n", search);
		if (limit)
			printf("ǰ %d λ<br>\n", limit);
	}
	user = malloc(sizeof (struct user_info *) * MAXACTIVE);
	if (!user)
		http_fatal("Insufficient memory");
	for (i = 0; i < MAXACTIVE; i++) {
		x = &(shm_utmp->uinfo[i]);
		if (x->active == 0)
			continue;
		if (x->invisible && !USERPERM(currentuser, PERM_SEECLOAK))
			continue;
		if (mytoupper(x->userid[0]) != search && search != '*')
			continue;
		user[total] = x;
		//memcpy(&user[total], x, sizeof (struct user_info));
		total++;
		if (total > limit && limit != 0)
			break;
	}

	printf("<table border=1>\n");
	printf
	    ("<tr><td>���</td><td>��</td><td>ʹ���ߴ���</td><td>ʹ�����ǳ�</td><td>����</td><td>��̬</td><td>����</td></tr>\n");
	qsort(user, total, sizeof (struct user_info *), (void *) cmpuser);
	for (i = 0; i < total; i++) {
		int dt = (now_t - user[i]->lasttime) / 60;
		printf("<tr><td>%d</td>", i + 1);
		printf("<td>%s</td>", isfriend(user[i]->userid) ? "��" : "  ");
		printkick(buf, sizeof (buf), user[i]);
		printf("<td><a href=bbsqry?userid=%s>%s</a> %s</td>",
		       user[i]->userid, user[i]->userid, buf);
		printf("<td><a href=bbsqry?userid=%s>%24.24s</a></td>",
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
		total2++;
	}
	free(user);
	printf("</table>\n");
	printf("��������: %d��", total2);
	printf("<hr>");
//      if (search != '*')
//              printf("[<a href='bbsufind?search=*'>ȫ��</a>] ");
	for (i = 'A'; i <= 'Z'; i++) {
//              if (i == search) {
//                      printf("[%c]", i);
//              } else {
		printf("[<a href=bbsufind?search=%c>%c</a>]", i, i);
//              }
	}
	printf("<br>\n");
	printf("[<a href='javascript:history.go(-1)'>����</a>]");
	printf("</center></body>\n");
	http_quit();
	return 0;
}
