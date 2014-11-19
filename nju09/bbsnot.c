#include "bbslib.h"

int
bbsnot_main()
{
	char board[80], filename[80];
	struct boardmem *x;
	html_header(1);
	check_msg();
	changemode(READING);
	getparmboard(board, sizeof (board));
	printf("<body><center>");
	if (!(x = getboard(board)))
		http_fatal("����İ���");
	printboardtop(x, 1, "");
	printf("<div class=wline3></div>");
	sprintf(filename, "vote/%s/notes", board);
	if (file_exist(filename)) {
		printf("<div class=swidth>");
		showcon(filename);
		printf("</div>");
	} else
		printf("<br>�����������ޡ�����¼����\n");
	printf("<br>[<a href=bbsdoc?B=%d>��������</a>] ", getbnumx(x));
	if (has_BM_perm(currentuser, x))
		printf("[<a href=bbsmnote?B=%d>�༭����¼</a>]", getbnumx(x));
	printf("</center>\n");
	http_quit();
	return 0;
}
