#include "bbslib.h"

#include "self_photo.h"

#ifdef  SELF_PHOTO_VOTE

static int
is_self_photo_vote(char *filename)
{
	if (!strncmp
	    (filename, self_an_vote_path, sizeof (self_an_vote_path) - 1))
		return 1;
	return 0;
}

static void
show_self_photo_vote_item(int type, int vote_type, char *file)
{
	int i;
	printf("<td><form action=selfvote target=_blank>\n"
	       "<input type=hidden name=t value=%d\n />"
	       "<input type=hidden name=v value=%d\n />"
	       "<input type=hidden name=i value=%s\n />", type,
	       vote_type, file);
	printf("<select name=s><option value=0 selected>0</option>\n");
	for (i = 1; i <= 10; i++)
		printf("<option value=%d>%d</option>\n", i, i);
	printf("</select>\n");
	printf("<input type=submit value=%s />", self_an_vote_ctype[vote_type]);
	printf("</form></td><td>&nbsp;</td>");

}

static void
show_self_photo_vote(char *filename)
{
	char *tmp, a;
	int type, i;
	if (strncmp
	    (filename, self_an_vote_path, sizeof (self_an_vote_path) - 1))
		return;
	tmp = filename + sizeof (self_an_vote_path);
	type = atoi(tmp+1);
	for (i = 0; i < 5; i++) {
		if (self_an_vote_type[i] == type)
			break;
	}
	if (5 == i)
		return;
	type = i;
	a = '0' + i;
	printf("<table><tr>");
	for (i = 0; i < SP_NUM; i++) {
		if (!strchr(self_an_vote_limit[i], a))
			continue;
		show_self_photo_vote_item(type, i, getparm("item"));
	}
	printf("</tr></table>");
	return;
}
#endif

int
bbsanc_main()
{
	char *board, *ptr, path[PATHLEN], fn[PATHLEN], buf[PATHLEN],
	    item[PATHLEN], names[PATHLEN], file[80], lastfile[80], title[256] =
	    " ";
	int index = 0, found = 0;
	FILE *fp;
	changemode(DIGEST);
	strsncpy(path, getparm("path"), PATHLEN - 1);
	strsncpy(item, getparm("item"), PATHLEN - 1);
	board = getbfroma(path);
	if (board[0] && !has_read_perm(currentuser, board))
		http_fatal("目录不存在");
	buf[0] = 0;
	if (board[0])
		sprintf(buf, "%s版", board);
	if (strstr(path, ".Search") || strstr(path, ".Names")
	    || strstr(path, "./") || strstr(path, "//") || strstr(path, ".."))
		http_fatal("错误的文件名");
	snprintf(names, PATHLEN, "0Announce%s/.Names", path);
	fp = fopen(names, "r");
	if (fp == 0)
		http_fatal("目录不存在");
	if (anc_readtitle(fp, title, sizeof (title))) {
		fclose(fp);
		http_fatal("错误的目录");
	}
	if (anc_hidetitle(title)) {
		fclose(fp);
		http_fatal("目录不存在");
	}
	while (!anc_readitem(fp, file, sizeof (file), title, sizeof (title))) {
		if (anc_hidetitle(title))
			continue;
		snprintf(fn, sizeof (fn), "%s%s", path, file);
		if (!board[0]) {
			ptr = getbfroma(fn);
			if (*ptr && !has_read_perm(currentuser, ptr))
				continue;
		}
		if (!strcmp(file, item)) {
			found = 1;
			break;
		}
		index++;
		strcpy(lastfile, file);
	}
	if (!found) {
		fclose(fp);
		http_fatal("文件不存在");
	}
	snprintf(fn, PATHLEN, "0Announce%s%s", path, item);
	if (*getparm("attachname") == '/') {
		fclose(fp);
		showbinaryattach(fn);
		return 0;
	}
	html_header(1);
	check_msg();
	printf("<body><div class=swidth><center>%s -- %s精华区文章阅读\n", BBSNAME, board);
	if (strlen(title) > 0)
		printf("<br><font class=f3>%s</font>", void1(title));
	printf("<hr>");
	showcon(fn);
#ifdef SELF_PHOTO_VOTE
	if (loginok && !isguest && is_self_photo_vote(fn))
		show_self_photo_vote(fn);
#endif
	if (index > 0) {
		snprintf(fn, PATHLEN, "0Announce%s%s", path, lastfile);
		if (file_exist(fn)) {
			if (file_isdir(fn)) {
				printf
				    ("[<a href=bbs0an?path=%s%s>上一项</a>] ",
				     path, lastfile);
			} else {
				printf
				    ("[<a href=bbsanc?path=%s&item=%s>上一项</a>] ",
				     path, lastfile);
			}
		}
	}
	printf("[<a href=bbs0an?path=%s>回到目录</a>] ", path);
	while (1) {
		if (anc_readitem
		    (fp, file, sizeof (file), title, sizeof (title)))
			break;
		if (anc_hidetitle(title))
			break;
		snprintf(fn, PATHLEN, "0Announce%s%s", path, file);
		if (!board[0]) {
			ptr = getbfroma(buf);
			if (*ptr && !has_read_perm(currentuser, ptr))
				break;
		}
		if (file_exist(fn)) {
			if (file_isdir(fn)) {
				printf
				    ("[<a href=bbs0an?path=%s%s>下一项</a>] ",
				     path, file);
			} else {
				printf
				    ("[<a href=bbsanc?path=%s&item=%s>下一项</a>] ",
				     path, file);
			}
		}
		break;
	}
	fclose(fp);
	if (board[0])
		printf("[<a href=bbsdoc?B=%d>本讨论区</a>]", getbnum(board));
	printf("</center></div></body>\n");
	http_quit();
	return 0;
}
