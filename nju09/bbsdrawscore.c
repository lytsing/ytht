#include "bbslib.h"

int
bbsdrawscore_main()
{
	char board[10][32], parm[] = "B1";
	struct boardmem *x1;
	int i, boardcount;
	int drawrelative = 0, drawlog = 0;
	changemode(READING);
	check_msg();
	html_header(1);

	if (!strcasecmp("on", getparm("relative")))
		drawrelative = 1;
	if (!strcasecmp("on", getparm("logarithm")))
		drawlog = 1;

	strsncpy(board[0], strtrim(getparm2("B0", "board")), 32);
	x1 = getboard2(board[0]);
	if (!x1)
		nosuchboard(board[0], "bbsdrawscore");
	printf("<applet width=750 height=550 code='JPlot.class' "
	       "archive='/JPlot.jar,/jcommon-0.9.0.jar,/jfreechart-0.9.15.jar'>");
	printf("<param name='title' value='%s BBS'>", MY_BBS_ID);
	if (drawrelative)
		printf("<param name=relative value='1'>");
	if (drawlog)
		printf("<param name=log value='1'>");
	printparam(0, board[0]);
	for (i = boardcount = 1; i < 10; i++) {
		parm[1] = '0' + i;
		strsncpy(board[boardcount], strtrim(getparm(parm)), 32);
		if (!board[i][0])
			continue;
		x1 = getboard2(board[boardcount]);
		if (!x1)
			continue;
		printparam(boardcount, board[boardcount]);
		boardcount++;
	}
	printf("</applet>");
	printf("<br>添加或者去掉版面<form action=bbsdrawscore>");
	for (i = 0; i < boardcount; i++)
		printf(" <input maxlength=32 name='B%d' value='%s'>", i,
		       board[i]);
	for (; i < 10; i++)
		printf(" <input maxlength=32 name='B%d'>", i);
	printf("<br><input type=checkbox name=relative %s>用相对值画图 &nbsp; ",
	       drawrelative ? "checked" : "");
	printf("<input type=checkbox name=logarithm %s>用对数坐标画图",
	       drawlog ? "checked" : "");
	printf("<br><input type=submit value='确定'></form>");
	printf("</body></html>");
	return 0;
}

int
printparam(int n, char *board)
{
	FILE *fp;
	char buf[80];
	int yy, mm, dd, score, count = 0;
	int daycount = 0;
	int dayscore[7];
	int ascore;
	int i;
	float total = 0;
	bzero(dayscore, sizeof(dayscore));
	if (board)
		snprintf(buf, sizeof (buf), "boards/%s/boardscore", board);
	else
		snprintf(buf, sizeof (buf), "boards/boardscore");
	printf("<param name='name%d' value='%s'>", n, board);
	printf("<param name='value%d' value='", n);
	fp = fopen(buf, "r");
	if (!fp)
		goto END;
	while (fgets(buf, sizeof (buf), fp)) {
		if (4 != sscanf(buf, "%d%d%d%d", &yy, &mm, &dd, &score))
			continue;
		dayscore[daycount] = score;
		if (daycount < 7)
			daycount++;
		else
			daycount = 0;
		count++;
		if (count < 7)
			continue;
		ascore = 0;
		for (i = 0; i < 7; i++)
			ascore += dayscore[i];
		printf("%d:%d:%d:%d ", yy, mm, dd, ascore);
		total += ascore;
	}
	count -= 6;
	fclose(fp);
      END:
	printf("'>");
	if (!count)
		count = 1;
	printf("<param name='avg%d' value='%f'>", n, total / count);
	return 0;
}
