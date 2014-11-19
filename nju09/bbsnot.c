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
		http_fatal("错误的版面");
	printboardtop(x, 1, "");
	printf("<div class=wline3></div>");
	sprintf(filename, "vote/%s/notes", board);
	if (file_exist(filename)) {
		printf("<div class=swidth>");
		showcon(filename);
		printf("</div>");
	} else
		printf("<br>本讨论区尚无「备忘录」。\n");
	printf("<br>[<a href=bbsdoc?B=%d>本讨论区</a>] ", getbnumx(x));
	if (has_BM_perm(currentuser, x))
		printf("[<a href=bbsmnote?B=%d>编辑备忘录</a>]", getbnumx(x));
	printf("</center>\n");
	http_quit();
	return 0;
}
