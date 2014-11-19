#include "bbslib.h"

int
bbsclear_main()
{
	char board[80], start[80], buf[256];
	html_header(1);
	check_msg();
	getparmboard(board, sizeof(board));
	strsncpy(start, getparm("S"), 32);
	if (!start[0])
		strsncpy(start, getparm("start"), 32);
	if (!getboard(board))
		http_fatal("´íÎóµÄÌÖÂÛÇø");
	changemode(READNEW);
	brc_initial(currentuser->userid, board);
	brc_clear();
	brc_update(currentuser->userid);
	sprintf(buf, "doc?B=%d&S=%s", getbnum(board), start);
	refreshto(buf, 0);
	http_quit();
	return 0;
}
