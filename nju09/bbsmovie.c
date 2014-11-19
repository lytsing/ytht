#include "bbslib.h"

int
bbsmovie_main()
{
	FILE *fp;
	char buf[120][256], buf2[80], *ptr;
	int total = 0, num, i;
	html_header(1);
	//num=atoi(getparm("num"));
	fp = fopen("etc/movie", "r");
	if (fp == 0)
		http_fatal("��վ�������δ����");
	while (total < 120) {
		if (fgets(buf[total], 255, fp) == 0)
			break;
		total++;
	}
	fclose(fp);
	ptr = getparm("num");
	if (!isdigit(ptr[0])) {
		num = rand() % (total / 5);
	} else
		num = atoi(ptr);
	if (num > total / 5 - 1)
		num = total / 5 - 1;
	if (num < 0)
		num = 0;
	if (total >= 5) {
		sprintf(buf2, "bbsmovie?num=%d", (num + 1) % (total / 5));
		refreshto(buf2, 59);
	}
	printf("<center>%s -- ����� [��%d/%dҳ]<hr>\n", BBSNAME, num + 1,
	       total / 5);
	printf("<table><tr><td>");
	printf
	    ("<font color=red>���������������������������� һ �� �� Ϳ �� �� �� ���������������������������</font>\n");
	for (i = num * 5; i <= num * 5 + 4; i++)
		fhhprintf(stdout, "%s", void1(buf[i]));
	printf
	    ("<font color=red>������������������������������������������������������������������������������</font>\n");
	printf("</table><hr>\n");
	printf("<div align=right>");
	printf("(59���Զ�ˢ��) ");
	for (i = 0; i < total / 5; i++) {
		if (i != num) {
			printf("<a href=bbsmovie?num=%d>[%d]</a>", i, i + 1);
		} else {
			printf("[%d]", i + 1);
		}
	}
	printf("</div>");
}
