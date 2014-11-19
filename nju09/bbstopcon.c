#include "bbslib.h"
static int
checkDIR(char *board, char *file)
{
	char buf[80];
	struct mmapfile mf = { ptr:NULL };
	struct fileheader *x;
	int total, retv = -1;
	sprintf(buf, "boards/%s/.DIR", board);
	MMAP_TRY {
		if (mmapfile(buf, &mf) == -1) {
			MMAP_UNTRY;
			return -1;
		}
		total = mf.size / sizeof (struct fileheader);
		retv = Search_Bin((struct fileheader *)mf.ptr, atoi(file + 2), 0, total - 1);
		if (retv >= 0) {
			x = (struct fileheader *) mf.ptr + retv;
			sprintf(buf, "%d", x->edittime - x->filetime);
			parm_add("T", buf);
		}
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	if (retv >= 0)
		return 0;
	return -1;
}

int
bbstopcon_main()
{
	char board[40], file[40], filename[80];
	int i;
	struct boardaux *boardaux;
	struct boardmem *bmem;
	changemode(READING);
	getparmboard(board, sizeof(board));
	strsncpy(file, getparm2("F", "file"), 32);
	if ((bmem = getboard(board)) == NULL)
		http_fatal("错误的讨论区");
	boardaux = getboardaux(getbnum(board));
	if (!boardaux)
		http_fatal("错误的讨论区");
	for (i = 0; i < boardaux->ntopfile; i++) {
		if (!strcmp(file, fh2fname(&boardaux->topfile[i])))
			break;
	}
	if (i >= boardaux->ntopfile)
		http_fatal("错误的参数");
	if (checkDIR(board, file) == 0) {
		bbscon_main();
		return 0;
	}
	sprintf(filename, "boards/%s/%s", board,
		fh2fname(&boardaux->topfile[i]));
	if (*getparm("attachname") == '/') {
		showbinaryattach(filename);
		return 0;
	}
	html_header(1);
	check_msg();
	changemode(READING);
	printf("<body><center>\n");
	printf
	    ("%s -- 提示信息 [讨论区: <a href=\"bbsdoc?B=%d\">%s (%s)</a>]<hr/>",
	     BBSNAME, getbnumx(bmem), void1(titlestr(bmem->header.title)), board);
	showcon(filename);
	printf("[<a href=bbsdoc?B=%d>本讨论区</a>]", getbnumx(bmem));
	printf("</center></body>\n");
	http_quit();
	return 0;
}
