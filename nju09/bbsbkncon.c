#include "bbslib.h"

int
bbsbkncon_main()
{
	char board[80], bkn[80], dir[256], file[256], filename[256], *ptr;
	struct fileheader x[3];
	int filetime;
	int index[3], ret;
	struct boardmem *brd;
	int num, total;
	changemode(BACKNUMBER);
	getparmboard(board, sizeof(board));
	strsncpy(bkn, getparm("bkn"), 32);
	strsncpy(file, getparm("file"), 32);
	num = atoi(getparm("num")) - 1;
	ptr = bkn;
	while (*ptr) {
		if (*ptr != 'B' && *ptr != '.' && !isdigit(*ptr))
			http_fatal("错误的参数");
		ptr++;
	}
	if (strlen(bkn) < 3)
		http_fatal("错误的参数");
	if ((brd=getboard(board)) == NULL)
		http_fatal("错误的讨论区");
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		http_fatal("错误的参数1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("错误的参数2");
	filetime = atoi(file + 2);
	snprintf(dir, 256, "boards/.backnumbers/%s/%s/.DIR", board, bkn);
	total = file_size(dir) / sizeof (*x);
	if (total <= 0)
		http_fatal("此卷过刊不存在或者为空");
	sprintf(filename, "boards/.backnumbers/%s/%s/%s", board, bkn, file);
	if (*getparm("attachname") == '/') {
		showbinaryattach(filename);
		return 0;
	}
	html_header(1);
	check_msg();
	printf("<body><div class=swidth><center>\n");
	printf("%s -- 文章阅读 [讨论区: %s--过刊]<hr>", BBSNAME, board);
	ret = get_fileheader_records(dir, filetime, num, 0, x, index, 3);
	if (ret == -1) 
		http_fatal("本文不存在或者已被删除");
	if (x[1].owner[0] == '-')
		http_fatal("本文已被删除");
	if (x[1].accessed & FH_HIDE)
		http_fatal("本文已被删除");
	showcon(filename);
	printf("[<a href=bbsfwd?B=%d&file=%s>转寄</a>]", getbnumx(brd), file);
	printf("[<a href=bbsccc?B=%d&file=%s>转贴</a>]", getbnumx(brd), file);
	if (index[0] != NA_INDEX) {
		printf
		    ("[<a href=bbsbkncon?B=%d&bkn=%s&file=%s&num=%d>上一篇</a>]",
		     getbnumx(brd), bkn, fh2fname(&(x[0])), index[0] + 1);
	}
	printf("[<a href=bbsbkndoc?B=%d&bkn=%s>本卷过刊</a>]", getbnumx(brd), bkn);
	if (index[2] != NA_INDEX) {
		printf("[<a href=bbsbkncon?B=%d&bkn=%s&file=%s&num=%d>下一篇</a>]",
			     getbnumx(brd), bkn, fh2fname(&(x[2])), index[2] + 1);
	}
	printf("</center></div></body>\n");
	http_quit();
	return 0;
}
