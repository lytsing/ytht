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
			http_fatal("����Ĳ���");
		ptr++;
	}
	if (strlen(bkn) < 3)
		http_fatal("����Ĳ���");
	if ((brd=getboard(board)) == NULL)
		http_fatal("�����������");
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		http_fatal("����Ĳ���1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("����Ĳ���2");
	filetime = atoi(file + 2);
	snprintf(dir, 256, "boards/.backnumbers/%s/%s/.DIR", board, bkn);
	total = file_size(dir) / sizeof (*x);
	if (total <= 0)
		http_fatal("�˾���������ڻ���Ϊ��");
	sprintf(filename, "boards/.backnumbers/%s/%s/%s", board, bkn, file);
	if (*getparm("attachname") == '/') {
		showbinaryattach(filename);
		return 0;
	}
	html_header(1);
	check_msg();
	printf("<body><div class=swidth><center>\n");
	printf("%s -- �����Ķ� [������: %s--����]<hr>", BBSNAME, board);
	ret = get_fileheader_records(dir, filetime, num, 0, x, index, 3);
	if (ret == -1) 
		http_fatal("���Ĳ����ڻ����ѱ�ɾ��");
	if (x[1].owner[0] == '-')
		http_fatal("�����ѱ�ɾ��");
	if (x[1].accessed & FH_HIDE)
		http_fatal("�����ѱ�ɾ��");
	showcon(filename);
	printf("[<a href=bbsfwd?B=%d&file=%s>ת��</a>]", getbnumx(brd), file);
	printf("[<a href=bbsccc?B=%d&file=%s>ת��</a>]", getbnumx(brd), file);
	if (index[0] != NA_INDEX) {
		printf
		    ("[<a href=bbsbkncon?B=%d&bkn=%s&file=%s&num=%d>��һƪ</a>]",
		     getbnumx(brd), bkn, fh2fname(&(x[0])), index[0] + 1);
	}
	printf("[<a href=bbsbkndoc?B=%d&bkn=%s>�������</a>]", getbnumx(brd), bkn);
	if (index[2] != NA_INDEX) {
		printf("[<a href=bbsbkncon?B=%d&bkn=%s&file=%s&num=%d>��һƪ</a>]",
			     getbnumx(brd), bkn, fh2fname(&(x[2])), index[2] + 1);
	}
	printf("</center></div></body>\n");
	http_quit();
	return 0;
}
