#include "bbslib.h"

int
bbsdenyall_main()
{
	int i;
	char board[80];
	struct boardmem *brd;
	html_header(1);
	if (!loginok)
		http_fatal("����δ��¼, ���ȵ�¼");
	changemode(READING);
	getparmboard(board, sizeof(board));
	if (!(brd = getboard(board)))
		http_fatal("�����������");
	if (!has_BM_perm(currentuser, brd))
		http_fatal("����Ȩ���б�����");
	loaddenyuser(board);
	printf("<center>\n");
	printf("%s -- �����û����� [������: %s]<hr><br>\n", BBSNAME, board);
	printf("���湲�� %d �˱���<br>", denynum);
	printf
	    ("<table border=1><tr><td>���<td>�û��ʺ�<td>����ԭ��<td>ԭ���������<td>����\n");
	for (i = 0; i < denynum; i++) {
		printf("<tr><td>%d", i + 1);
		printf("<td><a href=bbsqry?userid=%s>%s</a>", denyuser[i].id,
		       denyuser[i].id);
		printf("<td>%s\n", nohtml(denyuser[i].exp));
		printf("<td>%s\n", Ctime(denyuser[i].free_time) + 4);
		printf
		    ("<td>[<a onclick='return confirm(\"ȷʵ�����?\")' href=bbsdenydel?B=%d&userid=%s>���</a>]",
		     getbnumx(brd), denyuser[i].id);
	}
	printf("</table><hr>\n");
	printf
	    ("[<a href=bbsdenyadd?B=%d>�趨�µĲ���POST�û�</a>]</center>\n",
	     getbnumx(brd));
	printf
	    ("<p><center><a href=bbstdoc?B=%d>���ذ���</a></center>", getbnumx(brd));
	http_quit();
	return 0;
}
