#include "bbslib.h"
int
bbsgcon_main()
{
	char board[80], dir[80], file[80], filename[80], *ptr;
	int num;
	struct fileheader x[3];
	int index[3];
	int ret, filetime;
	struct boardmem *brd;
	changemode(READING);
	getparmboard(board, sizeof (board));
	strsncpy(file, getparm2("F", "file"), 32);
	num = atoi(getparm("num")) - 1;
	if ((brd = getboard(board)) == NULL)
		http_fatal("错误的讨论区");
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		http_fatal("错误的参数1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("错误的参数2");
	sprintf(dir, "boards/%s/.DIGEST", board);
	filetime = atoi(file + 2);
	sprintf(filename, "boards/%s/%s", board, file);
	ret = get_fileheader_records(dir, filetime, num, 0, x, index, 3);
	if (ret == -1)
		http_fatal("文章不存在或已被删除");
	num = index[1];
	if (*getparm("attachname") == '/') {
		showbinaryattach(filename);
		return 0;
	}
	html_header(1);
	check_msg();
	changemode(READING);
	printf("<body><div class=swidth><center>\n");
	printWhere(brd, "阅读文摘");
	printf("<div class=hright><a href=gdoc?B=%d&S=%d>返回文摘列表</a>"
	       "</div><div class=wline3></div><br>", getbnumx(brd), num - 5);
	showcon(filename);
	if (index[0] != NA_INDEX) {
		printf("[<a href=bbsgcon?B=%d&file=%s&num=%d>上一篇</a>]",
		       getbnumx(brd), fh2fname(&(x[0])), index[0] + 1);
	}
	printf("[<a href=bbsgdoc?B=%d&S=%d>本讨论区</a>]", getbnumx(brd),
	       num - 5);
	if (index[2] != NA_INDEX) {
		printf("[<a href=bbsgcon?B=%d&file=%s&num=%d>下一篇</a>]",
		       getbnumx(brd), fh2fname(&(x[2])), index[2] + 1);
	}
	ptr = x[1].title;
	if (!strncmp(ptr, "Re: ", 4))
		ptr += 4;
	printf("[<a href='bbstfind?B=%d&th=%d'>同主题阅读</a>]\n",
	       getbnumx(brd), x[1].thread);
	printf("</center></div></body>\n");
	http_quit();
	return 0;
}
