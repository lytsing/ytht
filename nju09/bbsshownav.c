#include "bbslib.h"
#define MAXNAVCOUNT 120
int
bbsshownav_main()
{
	char *secstr;
	secstr = getparm("secstr");
	html_header(1);
	changemode(SELECT);
	printf("<style type=text/css>A {color: #0000f0}</style>");
	printf("<body bgcolor=#FFFFFF>");
	printf("<center>\n");
	printf("近日精彩话题<hr>");
	if (shownavpart(1, secstr))
		printf("错误的参数!");
	printf("<hr>");
	printf("</body>");
	http_quit();
	return 0;
}

int
shownavpart(int mode, const char *secstr)
{
	static char lines[MAXNAVCOUNT][100];
	static int count;
	static time_t loadtime;
	FILE *fp;
	char buf[200];
	int i;
	if (file_time("wwwtmp/navpart.txt") >= loadtime) {
		fp = fopen("wwwtmp/navpart.txt", "r");
		if (fp == NULL)
			return -1;
		count = 0;
		while (count < MAXNAVCOUNT && fgets(buf, sizeof (buf), fp) != NULL) {
			buf[sizeof (lines[0]) - 1] = 0;
			memcpy(lines[count], buf, sizeof (lines[0]));
			count++;
		}
		fclose(fp);
		loadtime = now_t;
	}

	if (mode) {
		printf("<table><tr><td>名次</td><td>评分</td><td>人数</td><td>讨论区</td><td>作者</td><td>标题</td></tr>");
		for (i = 0; i < count; i++) {
			printf("<tr><td>%d</td>", i+1);
			memcpy(buf, lines[i], sizeof (lines[0]));
			shownavpartline(buf, mode);
		}
		printf("</table>");
	} else {
		printf("<table>");
		for (i = now_t % 5; i < count; i += 5) {
			memcpy(buf, lines[i], sizeof (lines[0]));
			shownavpartline(buf, mode);
		}
		printf("</table>");
	}
	return 0;
}

void
shownavpartline(char *buf, int mode)
{
	char *numstr, *board, *author, *title, *boardstr, *ptr;
	int star, thread;
	struct boardmem *x1;
	star = atof(buf) + 0.5;
	numstr = strchr(buf, ' ');
	if (numstr == NULL)
		return;
	*(numstr++) = 0;
	board = strchr(numstr, ' ');
	if (board == NULL)
		return;
	*(board++) = 0;
	author = strchr(board, ' ');
	if (author == NULL)
		return;
	*(author++) = 0;
	ptr = strchr(author, ' ');
	if (ptr == NULL)
		return;
	*(ptr++) = 0;
	thread = atoi(ptr);
	title = strchr(ptr, ' ');
	if (title == NULL)
		return;
	*(title++) = 0;
	x1 = getboard(board);
	if (x1 == 0)
		boardstr = board;
	else {
		boardstr = x1->header.title;
	}
	if (mode == 0) {
		printf("<tr><td><li /><a href='bbstcon?B=%d&th=%s'>",
		       getbnumx(x1), ptr);
		if (!strncmp(title, "[转载] ", 7) && strlen(title) > 20)
			title += 7;
		if (strlen(title) > 45)
			title[45] = 0;
		printf("%s</a>", void1(titlestr(title)));
		printf("<font class=f1>");
		printf("&lt;<a href='home?B=%d' class=blk>%s</a>&gt;",
		       getbnumx(x1), boardstr);
		printf("</font></td></tr>");
	} else {
		printf
		    ("<td>%d</td><td>%s</td><td><a href='home?B=%d'>%s</a></td>"
		     "<td>", star, numstr, getbnumx(x1), boardstr);
		printf("%s</td><td><a href='bbstcon?B=%d&th=%d'>%s</a>"
		       "</td></tr>", userid_str(author), getbnumx(x1), thread,
		       void1(titlestr(title)));
	}
}
