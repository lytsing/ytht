#include "ythtbbs.h"
int
qnyjzx(char *id)
{
	if (!strcmp(id, "qnyjzxA") || !strcmp(id, "qnyjzxB")
	    || !strcmp(id, "qnyjzxC")) {
		return 1;
	}
	return 0;
}

int
politics(char *board)
{
	if (!strcasecmp(board, "triangle") || !strcasecmp(board, "news")
	    || !strcasecmp(board, "auto_news"))
		return 1;
	return 0;
}
