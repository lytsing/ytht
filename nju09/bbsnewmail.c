#include "bbslib.h"

int
bbsnewmail_main()
{
	FILE *fp;
	struct fileheader x;
	int total = 0, total2 = 0;
	char dir[80];
	if (!loginok || isguest)
		http_fatal("����δ��¼, ���ȵ�¼");
	sprintf(dir, "mail/%c/%s/.DIR", mytoupper(currentuser->userid[0]),
		currentuser->userid);

	if (cache_header(file_time(dir), 1))
		return 0;
	html_header(1);
	check_msg();
	changemode(RMAIL);
	printf("<body><center>\n");
	printf
	    ("%s -- ���ʼ��б� [ʹ����: %s] [��������: %dk, ���ÿռ�: %dk]<hr>\n",
	     BBSNAME, currentuser->userid, max_mailsize(currentuser), get_mailsize(currentuser));
	fp = fopen(dir, "r");
	if (fp == 0)
		http_fatal("Ŀǰ��������û���κ��ż�");
	printf("<table border=1>\n");
	printf
	    ("<tr><td>���</td><td>״̬</td><td>������</td><td>����</td><td>�ż�����</td></tr>\n");
	while (1) {
		if (fread(&x, sizeof (x), 1, fp) <= 0)
			break;
		total++;
		if (x.accessed & FH_READ)
			continue;
		printf("<tr><td>%d</td><td>N</td>", total);
		printf("<td>%s</td>", userid_str(fh2owner(&x)));
		printf("<td>%6.6s</td>", Ctime(x.filetime) + 4);
		printf("<td><a href=bbsmailcon?file=%s&num=%d>", fh2fname(&x),
		       total);
		if (strncmp("Re: ", x.title, 4))
			printf("�� ");
		hprintf("%42.42s", void1(x.title));
		printf(" </a></td></tr>\n");
		total2++;
	}
	fclose(fp);
	printf("</table><hr>\n");
	printf("�������乲��%d���ż�, ��������%d��.", total, total2);
	printf("</center></body>");
	http_quit();
	return 0;
}
