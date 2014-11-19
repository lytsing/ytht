#include "bbslib.h"

int
bbstfind_main()
{
	char buf[1024], board[80], dir[80];
	struct boardmem *x1;
	struct fileheader *x;
	int i, total = 0, start = 0, numrecords;
	int thread;
	struct mmapfile mf = { ptr:NULL };
	html_header(1);
	check_msg();
	changemode(READING);
	getparmboard(board, sizeof(board));
	thread = atoi(getparm("th"));
	x1 = getboard(board);
	if (x1 == 0)
		http_fatal("�����������");
	sprintf(buf, "bbsman?B=%d&mode=1", getbnumx(x1));
	sprintf(dir, "boards/%s/.DIR", board);
	MMAP_TRY {
		if (mmapfile(dir, &mf) == -1) {
			MMAP_UNTRY;
			http_fatal("�����������");
		}
		if (mf.size == 0) {
			MMAP_UNTRY;
			mmapfile(NULL, &mf);
			http_fatal("��������Ŀǰû������");
		}
		numrecords = mf.size / sizeof (struct fileheader);
		brc_initial(currentuser->userid, board);
		printf("<body><center>%s -- ͬ������� [������: %s (%s)]<br>",
		       BBSNAME, void1(titlestr(x1->header.title)), board);
		if (thread != 0) {
			i = Search_Bin((struct fileheader*)mf.ptr, thread, 0, numrecords - 1);
			if (i < 0)
				i = -(i + 1);
		} else
			i = 0;
		for (; i < numrecords; i++) {
			x =
			    (struct fileheader *) (mf.ptr +
						   i *
						   sizeof (struct fileheader));
			if (thread != x->thread)
				continue;
			if (total == 0) {
				printf
				    ("<a href=bbsdoc?B=%d&start=%d>��������</a> ",
				     getbnumx(x1), (i > 4) ? (i - 4) : 1);
				printf
				    ("<a href=bbstcon?B=%d&start=%d&th=%d>������ȫ��չ��</a> ",
				     getbnumx(x1), i, thread);
				printf
				    ("<table border=1><tr><td>���</td><td>״̬</td><td>����</td>"
				     "<td>����</td><td>���⡡��������&nbsp;</td><td>�Ǽ�</td></tr>\n");
			}
			printf("<tr><td>%d</td><td>%s</td>", i + 1,
			       flag_str2(x->accessed, !brc_un_read(x)));
			printf("<td>%s</td>", userid_str(fh2owner(x)));
			if (!(x->accessed & (FH_MARKED | FH_DIGEST))) {
				char buf2[25];
				sprintf(buf2, "&box%s=on", fh2fname(x));
				if (strlen(buf) < 500)
					strcat(buf, buf2);
			}
			printf("<td>%6.6s</td>", Ctime(x->filetime) + 4);
			printf
			    ("<td><a href=con?B=%d&F=%s&N=%d&st=1&T=%d>%s </a>\n",
			     getbnumx(x1), fh2fname(x), i + 1, feditmark(x),
			     nohtml(x->title));
			if (x->sizebyte)
				printf(" %s", size_str(bytenum(x->sizebyte)));
			printf("</td><td class=%s>%d/%d��</td></tr>\n",
			       x->staravg50 ? "red" : "blk", x->staravg50 / 50,
			       x->hasvoted);
			total++;
			if (total == 1)
				start = i + 1;
		}
	}
	MMAP_CATCH {
		total = 0;
	}
	MMAP_END mmapfile(NULL, &mf);
	printf("</table><hr>\n���ҵ� %d ƪ \n", total);
	printf
	    ("<a href=bbsdoc?B=%d&start=%d>��������</a> ",
	     getbnumx(x1), (start > 5) ? (start - 5) : 1);
	if (total > 0) {
		printf
		    ("<a href=bbstcon?B=%d&start=%d&th=%d>������ȫ��չ��</a> ",
		     getbnumx(x1), start - 1, thread);
		if (has_BM_perm(currentuser, x1))
			printf
			    ("<a onclick='return confirm(\"ȷ��ͬ����ȫ��ɾ��?\")' href=%s>ͬ����ɾ��</a>",
			     buf);}
	http_quit();
	return 0;
}
