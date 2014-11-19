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
	    ("<center><strong>%s ��ʽ ����������,�˸�ʽ�����ٶȴ�Լÿ%sһ�� %s</center></strong>\n",
	     chm ? "chm" : "tgz", chm ? "100��" : "��1/3/5",
	     chm ? "<a href=bbsx?chm=0>tgz��ʽ</a>" :
	     "<a href=bbsx?chm=1>chm��ʽ</a>");
	printf
	    ("<hr><b>����ע������:</b> ���ھ������û����������ϵȹ��߿���������"
	     "���ؾ�����, �Ӷ����ϵͳ���ز��ȶ�, �ʽ�����������ȫ����Ϊ�� ftp "
	     "��ʽ����, ��ɵĲ��������½�. " MY_BBS_NAME " ftp ���ӷ�ʽΪ,<br>"
	     " &nbsp; &nbsp; ��ַ: X." MY_BBS_DOMAIN "<br>"
	     " &nbsp; &nbsp; �˿�: 21<br>" " &nbsp; &nbsp; �û�: �����û�<br>");
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
