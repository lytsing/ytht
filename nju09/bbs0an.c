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
		http_fatal("��Ŀ¼������");
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
		http_fatal("Ŀ¼�����ڣ�������һ�����ֲ�����");
	fp = fopen(names, "r");
	if (fp == 0)
		http_fatal("Ŀ¼������");
	if (anc_readtitle(fp, title, sizeof (title))) {
		fclose(fp);
		http_fatal("�����Ŀ¼");
	}
	if (anc_hidetitle(title)) {
		fclose(fp);
		http_fatal("Ŀ¼������");
	}
	if (board[0])
		printboardtop(brd, 6, NULL);
	else
		printf(BBSNAME "--������<br>");
	title[38] = 0;
	printf("<br><font class=f3><b>%s</b></font>",
	       nohtml(void1(strrtrim(title))));
	printhr();
	printf("<table border=1>\n");
	printf
	    ("<tr><td>���</td><td>����</td><td>������</td><td>����</td></tr>");
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
			printf("<td>[����] %s</td>", void1(titlestr(title)));
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
		printf("<br>&lt;&lt; Ŀǰû������ &gt;&gt;\n");
	}
	if (papath[0])
		printf("<br>[<a href=bbs0an?path=%s>������һ��Ŀ¼</a>] ",
		       papath);
	else
		printf
		    ("<br>[<a href='javascript:history.go(-1)'>������һҳ</a>] ");
	if (board[0]) {
		printf("[<a href=bbsdoc?B=%d>��������</a>] ", getbnumx(brd));
#if 0
		printf
		    ("����������[<a href=\"ftp://x." MY_BBS_DOMAIN
		     "/X/%s.tgz\">tgz��ʽ</a>]", board);
		printf
		    ("[<a href=\"ftp://x." MY_BBS_DOMAIN
		     "/X/chm/%s.chm\">chm��ʽ</a>]", board);
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
