#include "bbslib.h"

int
bbsx_main()
{
	char BoardsFile[256];
	int index, fd, SectNumber, total, start, chm;
	struct boardheader *buffer;
	struct stat st;
	chm = atoi(getparm("chm"));
	printf("Content-type: text/html; charset=%s\r\n\r\n", CHARSET);
	printf("<html>\n");
	check_msg();
	printf("<link rel=stylesheet href=/x.css>\n");
	sprintf(BoardsFile, "%s/.BOARDS", BBSHOME);
	fd = open(BoardsFile, O_RDONLY);
	if (fd == -1)
		http_fatal("error in opening .BOARDS file!");
	fstat(fd, &st);
	total = st.st_size / sizeof (struct boardheader);
	buffer =
	    (struct boardheader *) calloc((size_t) total,
					  sizeof (struct boardheader));
	if (buffer == NULL) {
		close(fd);
		http_fatal("Out of memory!");
	}
	if (read(fd, buffer, (size_t) st.st_size) < (ssize_t) st.st_size) {
		close(fd);
		free(buffer);
		http_fatal("Error readinf .BOARDS file!");
	}
	close(fd);
	printf("<body background=>\n");
	printf
	    ("<center><strong>%s 格式 精华区下载,此格式更新速度大约每%s一次 %s</center></strong>\n",
	     chm ? "chm" : "tgz", chm ? "100年" : "周1/3/5",
	     chm ? "<a href=bbsx?chm=0>tgz格式</a>" :
	     "<a href=bbsx?chm=1>chm格式</a>");
	printf
	    ("<hr><b>下载注意事项:</b> 由于经常有用户用网络蚂蚁等工具开大量进程"
	     "下载精华区, 从而造成系统负载不稳定, 故将精华区下载全部改为用 ftp "
	     "方式下载, 造成的不便请您谅解. " MY_BBS_NAME " ftp 链接方式为,<br>"
	     " &nbsp; &nbsp; 网址: X." MY_BBS_DOMAIN "<br>"
	     " &nbsp; &nbsp; 端口: 21<br>" " &nbsp; &nbsp; 用户: 匿名用户<br>");
	printf("<table width=100%% border=0 cellspacing=0 cellpadding=3>\n");
	for (SectNumber = 0; SectNumber < sectree.nsubsec; SectNumber++) {
		const struct sectree *sec = sectree.subsec[SectNumber];
		printf("<tr>\n");
		printf("<TD width=15%% valign=top align=center><B>%s</B></TD>",
		       sec->title);
		printf("<TD width=85%%>");
		start = 1;
		for (index = 0; index < total; index++) {
			if (sec->basestr[0] != buffer[index].sec1[0]
			    && sec->basestr[0] != buffer[index].sec2[0])
				continue;
			if ((buffer[index].level > 0
			     && (buffer[index].level & PERM_NOZAP) == 0
			     && (buffer[index].level & PERM_POSTMASK) == 0)
			    || (buffer[index].flag & CLOSECLUB_FLAG))
				continue;
			if (!start)
				printf(" - ");
			start = 0;
			printf
			    ("<A HREF=\"ftp://X." MY_BBS_DOMAIN
			     ":21/X/%s%s.%s\">%s</A>", chm ? "chm/" : "",
			     buffer[index].filename, chm ? "chm" : "tgz",
			     buffer[index].title);
		}
		printf("\n</TD></tr>");
	}
	printf("</table>\n");
	free(buffer);
	http_quit();
	return 0;
}
