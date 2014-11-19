#include "bbslib.h"
#define ONLINELIMIT 7000

int
bbsfind_main()
{
	char user[32], title3[80], title[80], title2[80];
	int day;
	html_header(1);
	check_msg();
	changemode(READING);
	strsncpy(user, getparm("user"), 13);
	strsncpy(title, getparm("title"), 50);
	strsncpy(title2, getparm("title2"), 50);
	strsncpy(title3, getparm("title3"), 50);
	day = atoi(getparm("day"));
	printf("<body>");
	if (day == 0) {
		printf("%s -- վ�����²�ѯ<hr>\n", BBSNAME);
		printf
		    ("<font color=red>�û���ѯ��ʹ�ø������ϵ� ���ڲ�ѯ ���ܣ��˲�ѯ��Ҫ��������Աʹ��</font>"
		     "<br> Ŀǰϵͳ���� %f��ϵͳ���س��� 1.5 ���������������� %d ʱ�����ܽ��в�ѯ��<br>"
		     "ϵͳ����ͳ��ͼ����������ͳ��ͼ���Ե�<a href=home?B=bbslists>bbslists��</a>�鿴<br>",
		     *system_load(), ONLINELIMIT);
		if (!loginok || isguest)
			printf("<b>����û�е�¼�����ȵ�¼��ʹ�ñ�����</b><br>");
		printf("<form action=bbsfind>\n");
		printf
		    ("��������: <input maxlength=12 size=12 type=text name=user> (���������������)<br>\n");
		printf
		    ("���⺬��: <input maxlength=60 size=20 type=text name=title>");
		printf
		    (" AND <input maxlength=60 size=20 type=text name=title2><br>\n");
		printf
		    ("���ⲻ��: <input maxlength=60 size=20 type=text name=title3><br>\n");
		printf
		    ("�������: <input maxlength=5 size=5 type=text name=day value=7> �����ڵ�����<br><br>\n");
		printf("<input type=submit value=�ύ��ѯ></form>\n");
	} else {
		if (*system_load() >= 1.7 || count_online() > ONLINELIMIT)
			http_fatal
			    ("�û���ѯ��ʹ�ø������ϵ� ���ڲ�ѯ ���ܣ��˲�ѯ��Ҫ��������Աʹ�á�ϵͳ����(%f)����������(%d)����, ������վ�������ٵ�ʱ���ѯ( ϵͳ���س��� 1.5 ���������������� %d ʱ���ܽ��в�ѯ )",
			     *system_load(), count_online(), ONLINELIMIT);
		if (!loginok || isguest)
			http_fatal("���ȵ�¼��ʹ�ñ����ܡ�");
		search(user, title, title2, title3, day * 86400);
	}
	printSoukeForm();
	printf("</body>");
	http_quit();
	return 0;
}

#define NUMX 100
int
search(char *id, char *pat, char *pat2, char *pat3, int dt)
{
	char board[256], dir[256];
	int total, i, sum = 0, nr, j;
	time_t starttime;
	int start;
      struct mmapfile mf = { ptr:NULL };
	struct fileheader *x;
	printf("%s -- վ�����²�ѯ��� <br>\n", BBSNAME);
	printf("����: %s ", id);
	printf("���⺬��: '%s' ", nohtml(pat));
	if (pat2[0])
		printf("�� '%s' ", nohtml(pat2));
	if (pat3[0])
		printf("���� '%s'", nohtml(pat3));
	printf("ʱ��: %d ��<br><hr>\n", dt / 86400);
	starttime = now_t - dt;
	if (starttime < 0)
		starttime = 0;
	if (!search_filter(pat, pat2, pat3)) {
		for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
			strcpy(board, shm_bcache->bcache[i].header.filename);
			if (!has_read_perm_x
			    (currentuser, &(shm_bcache->bcache[i])))
				continue;
			sprintf(dir, "boards/%s/.DIR", board);
			mmapfile(NULL, &mf);
			if (mmapfile(dir, &mf) < 0) {
				continue;
			}
			x = (struct fileheader *) mf.ptr;
			nr = mf.size / sizeof (struct fileheader);
			if (nr == 0)
				continue;
			start =
			    Search_Bin((struct fileheader *) mf.ptr, starttime,
				       0, nr - 1);
			if (start < 0)
				start = -(start + 1);
			for (total = 0, j = start; j < nr; j++) {
				if (abs(now_t - x[j].filetime) > dt)
					continue;
				if (id[0] != 0 && strcasecmp(x[j].owner, id))
					continue;
				if (pat[0]
				    && !strcasestr(x[j].title, pat))
					continue;
				if (pat2[0]
				    && !strcasestr(x[j].title, pat2))
					continue;
				if (pat3[0]
				    && strcasestr(x[j].title, pat3))
					continue;
				if (total == 0)
					printf("<table border=1>\n");
				printf
				    ("<tr><td>%d<td><a href=bbsqry?userid=%s>%s</a>",
				     j + 1, x[j].owner, x[j].owner);
				printf("<td>%6.6s", Ctime(x[j].filetime) + 4);
				printf
				    ("<td><a href=con?B=%s&F=%s&N=%d&T=%d>%s</a>\n",
				     board, fh2fname(&x[j]), j + 1,
				     feditmark(&x[j]), nohtml(x[j].title));
				total++;
				sum++;
				if (sum > 999) {
					printf("</table> ....");
					break;
				}

			}
			if (sum > 999)
				break;
			if (!total)
				continue;
			printf("</table>\n");
			printf
			    ("<br>����%dƪ���� <a href=bbsdoc?B=%d>%s</a><br><br>\n",
			     total, getbnumx(&(shm_bcache->bcache[i])), board);
		}
		mmapfile(NULL, &mf);
	}
	printf("һ���ҵ�%dƪ���·��ϲ�������<br>\n", sum);
	tracelog("%s bbsfind %d", currentuser->userid, sum);
	return sum;
}
