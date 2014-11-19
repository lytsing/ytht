#include "bbslib.h"

int
bbsmdoc_main()
{
	FILE *fp;
	char board[80], dir[80];
	struct boardmem *x1;
	struct fileheader x;
	int i, start, total;
	char bmbuf[(IDLEN + 1) * 4];
	if (!loginok || isguest)
		http_fatal("���ȵ�¼");
	changemode(READING);
	getparmboard(board, sizeof(board));
	x1 = getboard(board);
	if (x1 == 0) {
		html_header(1);
		nosuchboard(board, "bbsmdoc");
	}
	updateinboard(x1);
	strcpy(board, x1->header.filename);
	if (!has_BM_perm(currentuser, x1))
		http_fatal("��û��Ȩ�޷��ʱ�ҳ");
	sprintf(dir, "boards/%s/.DIR", board);
	if (cache_header(file_time(dir), 10))
		return 0;
	html_header(1);
	printf("</head><body>");
	check_msg();
	fp = fopen(dir, "r");
	if (fp == 0)
		http_fatal("�����������Ŀ¼");
	total = file_size(dir) / sizeof (struct fileheader);
	start = atoi(getparm("start"));
	if (strlen(getparm("start")) == 0 || start > total - 20)
		start = total - 20;
	if (start < 0)
		start = 0;
	printf("<nobr><center>\n");
	printf("%s -- [������: %s] ����[%s] ������[%d]<hr>\n",
	       BBSNAME, board, userid_str(bm2str(bmbuf, &(x1->header))), total);
	if (total <= 0) {
		fclose(fp);
		http_fatal("��������Ŀǰû������");
	}
	printf("<form name=form1 method=post action=bbsman>\n");
	printf("<table>\n");
	printf("<tr><td>���<td>����<td>״̬<td>����<td>����<td>����\n");
	fseek(fp, start * sizeof (struct fileheader), SEEK_SET);
	for (i = 0; i < 20; i++) {
		char filename[80];
		if (fread(&x, sizeof (x), 1, fp) <= 0)
			break;
		sprintf(filename, "boards/%s/%s", board, fh2fname(&x));
		printf("<tr><td>%d", start + i + 1);
		printf
		    ("<td><input style='height:18px' name=box%s type=checkbox>",
		     fh2fname(&x));
		printf("<td>%s<td>%s", flag_str_bm(x.accessed),
		       userid_str(fh2owner(&x)));
		printf("<td>%12.12s", Ctime(x.filetime) + 4);
		x.title[40] = 0;
		printf("<td><a href=con?B=%d&F=%s&N=%d&T=%d>%s%s </a>\n",
		       getbnumx(x1), fh2fname(&x), start + i + 1, feditmark(&x),
		       strncmp(x.title, "Re: ", 4) ? "�� " : "",
		       void1(titlestr(x.title)));
	}
	fclose(fp);
	printf("</table>\n");
	printf("<input type=hidden name=mode value=''>\n");
	printf("<input type=hidden name=board value='%s'>\n", board);
	printf
	    ("<input type=button value=��ɾ����� onclick="
	     "'if(confirm(\"��ʹ��telnet��ʽ��������ɾ�������������ɾ����ǣ������Ҫ�ӱ����?\")) {document.form1.mode.value=1; document.form1.submit();}'>&nbsp;&nbsp;\n");
	printf
	    ("<input type=button value=��M onclick='document.form1.mode.value=2; document.form1.submit();'>\n");
	printf
	    ("<input type=button value=��G onclick='document.form1.mode.value=3; document.form1.submit();'>\n");
	printf
	    ("<input type=button value=����Re onclick='document.form1.mode.value=4; document.form1.submit();'>\n");
	printf
	    ("<input type=button value=���MG onclick='document.form1.mode.value=5; document.form1.submit();'>\n");
	printf("</form>\n");
	if (start > 0) {
		printf("<a href=bbsmdoc?B=%d&start=%d>��һҳ</a> ",
		       getbnumx(x1), start < 20 ? 0 : start - 20);
	}
	if (start < total - 20) {
		printf("<a href=bbsmdoc?B=%d&start=%d>��һҳ</a> ",
		       getbnumx(x1), start + 20);
	}
	printf("<a href=bbsdoc?B=%d>һ��ģʽ</a> ", getbnumx(x1));
	printf("<a href=bbsdenyall?B=%d>��������</a> ", getbnumx(x1));
	printf("<a href=bbsmnote?B=%d>�༭����¼</a> ", getbnumx(x1));
	printdocform("bbsmdoc", getbnumx(x1));
	http_quit();
	return 0;
}
