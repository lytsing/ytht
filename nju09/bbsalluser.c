#include "bbslib.h"

int
bbsalluser_main()
{
	FILE *fp;
	struct userec x;
	int pos[MAXUSERS], total = 0;
	int p, i, start;
	html_header(1);
	check_msg();
	changemode(LAUSERS);
	for (i = 0; i < MAXUSERS; i++) {
		if (shm_ucache->userid[i][0] > 32) {
			pos[total] = i;
			total++;
		}
	}
	printf("<body><center>%s -- ����ʹ�����б� [�û�����: %d]<hr>\n",
	       BBSNAME, total);
	start = atoi(getparm("start"));
	if (start > total || start < 0)
		start = 0;
	printf("<table border=1>\n");
	printf
	    ("<tr><td>���</td><td>����</td><td>ʹ���ߴ���</td><td>�ǳ�</td><td>��վ����</td><td>������</td><td>�������ʱ��</td></tr>\n");
	fp = fopen(PASSFILE, "r");
	if (NULL == fp)
		return -1;
	for (i = 0; i < 20; i++) {
		if (start + i > total - 1)
			break;
		p = pos[start + i];
		fseek(fp, sizeof (struct userec) * p, SEEK_SET);
		if (fread(&x, sizeof (x), 1, fp) <= 0)
			break;
		printf
		    ("<tr><td>%d</td><td>%s</td><td><a href=bbsqry?userid=%s>%s</a></td><td>%s</td><td>%d</td><td>%d</td><td>%s</td></tr>\n",
		     start + i + 1,
		     isfriend(x.userid) ? "<font color=green>��</font>" : " ",
		     x.userid, x.userid, nohtml(x.username), x.numlogins,
		     x.numposts, Ctime(x.lastlogin) + 4);
	}
	fclose(fp);
	printf("</table><hr>\n");
	if (start > 0)
		printf("[<a href=bbsalluser?start=0>��һҳ</a>] ");
	if (start > 0)
		printf("[<a href=bbsalluser?start=%d>��һҳ</a>] ",
		       start - 20 < 0 ? 0 : start - 20);
	if (start < total - 19)
		printf("[<a href=bbsalluser?start=%d>��һҳ</a>]", start + 20);
	if (start < total - 19)
		printf("[<a href=bbsalluser?start=%d>���һҳ</a>]\n",
		       total - 19);
	printf("<form action=bbsalluser>\n");
	printf("<input type=submit value=��ת��>");
	printf("��<input name=start type=text size=5>��ʹ����\n");
	printf("</form></body></html>\n");
	return 0;
}
