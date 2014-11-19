#include "bbslib.h"

int
bbsgdoc_main()
{
	FILE *fp;
	char board[80], buf[128];
	struct boardmem *x1;
	struct fileheader x;
	int i, start, total;
	html_header(1);
	check_msg();
	changemode(READING);
	getparmboard(board, sizeof(board));
	x1 = getboard(board);
	if (x1 == 0)
		nosuchboard(board, "bbsgdoc");
	updateinboard(x1);
	strcpy(board, x1->header.filename);
	sprintf(buf, "boards/%s/.DIGEST", board);
	fp = fopen(buf, "r");
	if (fp == 0)
		http_fatal("错误的讨论区目录");
	total = file_size(buf) / sizeof (struct fileheader);
	start = getdocstart(total, w_info->t_lines);
	printf("<body>");
	printf("<nobr><center>\n");
	sprintf(buf, "文章%d篇", total);
	printboardtop(x1, 4, buf);
	if (total <= 0) {
		fclose(fp);
		http_fatal("本讨论区文摘目前没有文章");
	}
	printf("<a href=bbspst?B=%d>发表文章</a> ", getbnumx(x1));
	sprintf(buf, "bbsgdoc?B=%d", getbnumx(x1));
	bbsdoc_helper(buf, start, total, w_info->t_lines);
	printhr();
	printf("<table cellSpacing=0 cellPadding=2 border=0>\n");
	printf
	    ("<tr class=docbgcolor><td>序号</td><td>状态</td><td>作者</td><td>日期</td><td>标题</td><td>星级</td><td>评价</td></tr>\n");
	fseek(fp, (start - 1) * sizeof (struct fileheader), SEEK_SET);
	for (i = 0; i < w_info->t_lines; i++) {
		if (fread(&x, sizeof (x), 1, fp) <= 0)
			break;
		printf("<tr><td>%d</td><td>%s</td><td>%s</td>",
		       start + i, flag_str(x.accessed), userid_str(x.owner));
		printf("<td>%12.12s</td>", Ctime(x.filetime) + 4);
		printf
		    ("<td><a href=bbsgcon?B=%d&file=%s&num=%d>%s%s</a></td><td>%d</td><td>%d</td></tr>\n",
		     getbnumx(x1), fh2fname(&x), start + i - 1, strncmp(x.title,
								 "Re: ",
								 4) ? "● " :
		     "", void1(titlestr(x.title)), x.staravg50 / 50,
		     x.hasvoted);
	}
	printf("</table>");
	printhr();
	printf("<a href=bbspst?B=%d>发表文章</a> ", getbnumx(x1));
	sprintf(buf, "bbsgdoc?B=%d", getbnumx(x1));
	bbsdoc_helper(buf, start, total, w_info->t_lines);
	fclose(fp);
	printdocform("bbsgdoc", getbnumx(x1));
	printf("</body>");
	http_quit();
	return 0;
}
