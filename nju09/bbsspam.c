#include "bbslib.h"
int
bbsspam_main()
{
	int num;
	struct spamheader *sh = NULL;
	struct spamheader *sh1;
	uint16_t spamnum;
	struct yspam_ctx *yctx;
	unsigned long long mid;
	char midbuf[100];
	int ret;

	if ((!loginok || isguest) && (!tempuser)) 
		http_fatal("����δ��¼, ���ȵ�¼");
	html_header(1);
	changemode(RMAIL);
	printf("<body><center>\n");
	printf("%s -- �����ż��б� [ʹ����: %s]\n", BBSNAME, currentuser->userid);
	yctx = yspam_init("127.0.0.1");
	ret = yspam_getallspam(yctx, currentuser->userid, &sh, &spamnum);
	yspam_fini(yctx);
	if (ret < 0 && ret != -YSPAM_ERR_NOSUCHMAIL) {
		if (sh)
			free(sh);
		http_fatal("�ڲ����� 1");
	}
	printf("���� %d �������ʼ�", spamnum);
	printf("<table>");
	printf
	    ("<tr><td>���</td><td>������</td><td>����</td><td>�ż�����</td></tr>\n");
	sh1 = sh;
	for (num = 1; num <= spamnum; num++) {
		mid = ((unsigned long long)ntohl(sh1->mailid_high) << 32) + ntohl(sh1->mailid_low);
		sprintf(midbuf, "%llu", mid);
		printf("<tr><td>%d</td><td>%s</td><td>%12.12s</td><td><a href=bbsspamcon?id=%s&magic=%d>", num, void1(titlestr(sh1->sender)), Ctime(ntohl(sh1->time)) + 4, midbuf, ntohl(sh1->magic));
		if (sh1->title[0] == 0)
			printf("����");
		else
			printf("%s", void1(titlestr(sh1->title)));
		printf("</a></td></tr>");
		sh1++;
	}
	if (sh)
		free(sh);
	printf("</table>");
	http_quit();
	return 0;
}
