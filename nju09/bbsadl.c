#include "bbslib.h"

int
bbsadl_main()
{
	int i, no = 0;
	char brd[100], buf[256];
	html_header(1);
	check_msg();
	printf("<nobr><center>%s -- ���������ط���<hr>\n", BBSNAME);
	printf("<table>\n");
	printf("<tr><td>���<td>����<td>��С(�ֽ�)<td>����ʱ��\n");
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		strsncpy(brd, shm_bcache->bcache[i].header.filename, 60);
		sprintf(buf, "www/an/%s.tgz", brd);
		if (!file_exist(buf))
			continue;
		no++;
		printf
		    ("<tr><td>%d<td><a href=/an/%s.tgz>%s.tgz</a><td>%ld<td>%s\n",
		     no, brd, brd, file_size(buf), Ctime(file_time(buf)) + 4);
	}
	printf("</table>");
	return 0;
}
