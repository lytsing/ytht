#include "bbslib.h"

int
anc_readtitle(FILE * fp, char *title, int size)
{
	char buf[512];
	while (fgets(buf, sizeof (buf), fp)) {
		if (!strncmp(buf, "# Title=", 8)) {
			strsncpy(title, buf + 8, size);
			return 0;
		}
	}
	return -1;
}

int
anc_readitem(FILE * fp, char *path, int sizepath, char *name, int sizename)
{
	char buf[512];
	int hasname = 0;
	while (fgets(buf, sizeof (buf), fp)) {
		if (!strncmp(buf, "Name=", 5)) {
			strsncpy(name, buf + 5, sizename);
			hasname = 1;
		}
		if (!hasname)
			continue;
		if (strncmp(buf, "Path=~", 6))
			continue;
		strsncpy(path, strtrim(buf + 6), sizepath);
		hasname = 2;
		break;
	}
	if (hasname != 2)
		return -1;
	return 0;
}

int
anc_hidetitle(char *title)
{
	if (!(currentuser->userlevel & PERM_SYSOP) &&
	    (strstr(title, "(BM: SYSOPS)") != NULL
	     || strstr(title, "(BM: SECRET)") != NULL
	     || strstr(title, "(BM: BMS)") != NULL))
		return 1;
	if (strstr(title, "<HIDE>") != NULL)
		return 1;
	return 0;
}

int
bbs0an_main()
{
	FILE *fp;
	int index = 0, len;
	//int visit[2];
	char *ptr, papath[PATHLEN], path[PATHLEN], names[PATHLEN],
	    file[80], buf[PATHLEN], title[256] = " ";
	char board[40];
	struct boardmem *brd;
	html_header(1);
	changemode(DIGEST);
	check_msg();
	printf("</head><body topmargin=0><center>\n");
	strsncpy(path, getparm("path"), PATHLEN - 1);
	len = strlen(path);
	if (len && path[len - 1] == '/')
		path[len - 1] = 0;
	if (strstr(path, "..") || strstr(path, "./") || strstr(path, "//"))
		http_fatal("此目录不存在");
	snprintf(names, PATHLEN, "0Announce%s/.Names", path);
	strcpy(papath, path);
	ptr = strrchr(papath, '/');
	if (ptr != NULL) {
		if (ptr != papath || !strcmp(papath, "/"))
			*ptr = '\0';
		else
			ptr[1] = 0;
	}
	strsncpy(board, getbfroma(path), 32);
	brd = getboard2(board);
	if (board[0] && (!brd || !has_read_perm_x(currentuser, brd)))
		http_fatal("目录不存在，或者是一个俱乐部版面");
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
	if (board[0])
		printboardtop(brd, 6, NULL);
	else
		printf(BBSNAME "--精华区<br>");
	title[38] = 0;
	printf("<br><font class=f3><b>%s</b></font>",
	       nohtml(void1(strrtrim(title))));
	printhr();
	printf("<table border=1>\n");
	printf
	    ("<tr><td>编号</td><td>标题</td><td>整理人</td><td>日期</td></tr>");
	while (!anc_readitem(fp, file, sizeof (file), title, sizeof (title))) {
		char *id;
		if (anc_hidetitle(title))
			continue;
		snprintf(buf, sizeof (buf), "%s%s", path, file);
		ptr = getbfroma(buf);
		if (*ptr && !has_read_perm(currentuser, ptr))
			continue;
		index++;
		if (strlen(title) <= 39)
			id = "";
		else {
			title[38] = 0;
			id = title + 39;
			if ((ptr = strchr(id, ':')))
				id = ptr + 1;
			ptr = strchr(id, ')');
			if (ptr)
				ptr[0] = 0;
		}
		printf("<tr><td>%d</td>", index);
		snprintf(buf, sizeof (buf), "0Announce%s%s", path, file);
		if (!file_exist(buf)) {
			printf("<td>[错误] %s</td>", void1(titlestr(title)));
		} else if (file_isdir(buf)) {
			printf
			    ("<td><img src=/dir.gif> <a href=bbs0an?path=%s%s>%s</a></td>",
			     path, file, void1(titlestr(title)));
		} else {
			printf
			    ("<td><img src=/text.gif> <a href=bbsanc?path=%s&item=%s>%s</a></td>",
			     path, file, void1(titlestr(title)));
		}
		if (id[0])
			printf("<td>%s</td>", userid_str(id));
		else
			printf("<td> </td>");
		printf("<td>%6.6s %s</td></tr>", Ctime(file_time(buf)) + 4,
		       Ctime(file_time(buf)) + 20);

	}
	fclose(fp);
	printf("</table>");
	if (index <= 0) {
		printf("<br>&lt;&lt; 目前没有文章 &gt;&gt;\n");
	}
	if (papath[0])
		printf("<br>[<a href=bbs0an?path=%s>返回上一层目录</a>] ",
		       papath);
	else
		printf
		    ("<br>[<a href='javascript:history.go(-1)'>返回上一页</a>] ");
	if (board[0]) {
		printf("[<a href=bbsdoc?B=%d>本讨论区</a>] ", getbnumx(brd));
#if 0
		printf
		    ("精华区下载[<a href=\"ftp://x." MY_BBS_DOMAIN
		     "/X/%s.tgz\">tgz格式</a>]", board);
		printf
		    ("[<a href=\"ftp://x." MY_BBS_DOMAIN
		     "/X/chm/%s.chm\">chm格式</a>]", board);
#endif
	}
	printSoukeForm();
	printf("</body>");
	http_quit();
	return 0;
}

int
getvisit(int n[2], const char *path)
{
	char fn[PATH_MAX + 1];
	int fd;
	n[0] = 0;
	n[1] = 0;
	if (snprintf(fn, PATH_MAX + 1, "0Announce%s/.logvisit", path) >
	    PATH_MAX)
		return -1;
	fd = open(fn, O_RDONLY | O_CREAT, 0660);
	if (fd < 0)
		return -1;
	if (read(fd, n, sizeof (int) * 2) <= 0) {
		n[0] = 0;
		n[1] = 0;
	}
	close(fd);
	return 0;
}
