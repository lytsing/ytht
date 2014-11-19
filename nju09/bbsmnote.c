#include "bbslib.h"

int
bbsmnote_main()
{
	FILE *fp;
	char path[256], buf[10000], board[256];
	struct boardmem *x;
	int n = 0;
	html_header(1);
	check_msg();
	printf("<center>\n");
	if (!loginok || isguest)
		http_fatal("匆匆过客，请先登录");
	changemode(EDIT);
	getparmboard(board, sizeof(board));
	x = getboard(board);
	if (!has_BM_perm(currentuser, x))
		http_fatal("你无权进行本操作");
	sprintf(path, "vote/%s/notes", board);
	if (!strcasecmp(getparm("type"), "update"))
		save_note(path);
	printf("%s -- 编辑备忘录 [讨论区: %s]<hr>\n", BBSNAME, board);
	printf
	    ("<form name=form1 method=post action=bbsmnote?type=update&B=%d>\n",
	     getbnumx(x));
	fp = fopen(path, "r");
	if (fp) {
		n = fread(buf, 1, 9999, fp);
		fclose(fp);
	}
	printf("<table border=1><tr><td>");
	printubb("form1", "text");
	printf
	    ("<textarea  onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}'  onkeypress='if(event.keyCode==10) return document.form1.submit()' name=text rows=20 cols=80 wrap=virtual>\n");
	if (n > 0)
		ansi2ubb(buf, n, stdout);
	printf("</textarea></table>\n");
	printf("<input type=submit value=存盘> ");
	printf("<input type=reset value=复原>\n");
	printf("<hr>\n");
	http_quit();
	return 0;
}

void
save_note(char *path)
{
	char *buf = getparm("text");
    int use_ubb = strlen(getparm("useubb"));
	
    if (use_ubb)
        ubb2ansi(buf, path);
    else
        f_write(path, buf);
	printf("备忘录修改成功。<br>\n");
	printf("<a href='javascript:history.go(-2)'>返回</a>");
	http_quit();
}
