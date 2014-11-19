#include "bbslib.h"

int
bbsnotepad_main()
{
	FILE *fp;
	char buf[256];
	html_header(1);
	check_msg();
	changemode(NOTEPAD);
	printf("%s -- ���԰� [����: %6.6s]<hr>\n", BBSNAME, Ctime(now_t) + 4);
	fp = fopen("etc/notepad", "r");
	if (fp == 0) {
		printf("��������԰�Ϊ��");
		http_quit();
	}
	while (1) {
		if (fgets(buf, 255, fp) == 0)
			break;
		hprintf("%s", buf);
	}
	fclose(fp);
	http_quit();
	return 0;
}
