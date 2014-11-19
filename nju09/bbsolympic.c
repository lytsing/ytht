#include "bbslib.h"

int
bbsolympic_main()
{
	html_header(1);
	printf("</head><body>");
	changemode(SELECT);
	showolympic();
	return 0;
}

int
showolympic()
{
	FILE *fp;
	char *fname = "wwwtmp/olympic.html";
	char buf[1024];
	fp = fopen(fname, "rt");
	if (!fp)
		http_fatal("无法打开文件");
	while (fgets(buf, sizeof (buf), fp)) {
		if (buf[0] != '#') {
			fputs(buf, stdout);
			continue;
		}
		if (!strncmp(buf, "#showfile ", 10)) {
			char *ptr;
			ptr = strchr(buf, '\n');
			if (ptr)
				*ptr = 0;
			showfile(buf + 10);
		} else if (!strncmp(buf, "#showfirstanc ", 14)) {
			showfirstanc(buf + 14);
		} else if (!strncmp(buf, "#showlastanc ", 13)) {
			showlastanc(buf + 13);
		} else if (!strncmp(buf, "#shownewpost ", 13)) {
			shownewpost(buf + 13);
		} else if (!strncmp(buf, "#shownewmark ", 13)) {
			shownewmark(buf + 13);
		} else if (!strncmp(buf, "#showoneboardscript ", 20)) {
			showoneboardscript(buf + 20);
		} else
			fputs(buf, stdout);
	}
	fclose(fp);
	return 0;
}

int
showlastanc(char *buf)
{
	char ancpath[256], file[256], title[80];
	char files[5][256], titles[5][80];
	FILE *fp;
	int count = 0, i;
	strsncpy(ancpath, strtrim(buf), sizeof (ancpath));
	snprintf(file, sizeof (file), "0Announce%s/.Names", ancpath);
	fp = fopen(file, "rt");
	if (!fp) {
		printf("Cann't open %s! -%s-", file, strtrim(" a "));
		return -1;
	}
	if (anc_readtitle(fp, title, sizeof (title))) {
		fclose(fp);
		return -1;
	}
	while (!anc_readitem(fp, file, sizeof (file), title, sizeof (title))) {
		if (anc_hidetitle(title))
			continue;
		if (strlen(title) > 39)
			title[38] = 0;
		memmove(&files[1], &files[0], 256 * 4);
		memmove(&titles[1], &titles[0], 80 * 4);
		strcpy(files[0], file);
		strcpy(titles[0], strtrim(title));
		count++;
	}
	fclose(fp);

	if (count > 5)
		count = 5;

	printf
	    ("<script>function aanc(file,title){this.file=file;this.title=title;}"
	     "var lastanc=new Array('%s'", ancpath);
	for (i = 0; i < count; i++) {
		printf(",");
		printf("new aanc('%s','%s')", files[i],
		       nohtml(void1(titles[i])));
	}
	printf(");</script>");
	return 0;
}

int
showfirstanc(char *buf)
{
	char ancpath[256], file[256], title[80];
	FILE *fp;
	int count = 0;
	strsncpy(ancpath, strtrim(buf), sizeof (ancpath));
	snprintf(file, sizeof (file), "0Announce%s/.Names", ancpath);
	fp = fopen(file, "rt");
	if (!fp) {
		printf("Cann't open %s! -%s-", file, strtrim(" a "));
		return -1;
	}
	if (anc_readtitle(fp, title, sizeof (title))) {
		fclose(fp);
		return -1;
	}
	printf
	    ("<script>function aanc(file,title){this.file=file;this.title=title;}"
	     "var firstanc=new Array('%s'", ancpath);
	while (!anc_readitem(fp, file, sizeof (file), title, sizeof (title))) {
		if (anc_hidetitle(title))
			continue;
		if (strlen(title) > 39)
			title[38] = 0;
		printf(",");
		printf("new aanc('%s','%s')", file, nohtml(void1(title)));
		count++;
		if (count >= 5)
			break;
	}
	printf(");</script>");
	fclose(fp);
	return 0;
}

int
showoneboardscript(char *aboard)
{
	struct boardmem *brd;
	char board[40];
	strsncpy(board, strtrim(aboard), sizeof (board));
	brd = getbcache(board);
	if (brd == NULL)
		return -1;
	printf("<script>var oneboardscript=");
	oneboardscript(brd);
	printf(";</script>");
	return 0;
}

int
shownewpost(char *board)
{
	char buf[80];
	int i, count = 0, total;
	FILE *fp;
	struct fileheader fh;
	board = strtrim(board);
	snprintf(buf, sizeof (buf), "boards/%s/.DIR", board);
	total = file_size(buf) / sizeof (fh);
	fp = fopen(buf, "rb");
	if (fp == NULL) {
		printf("Cann't Open!");
		return -1;
	}
	printf("<script>newpost=new Array(");
	i = total - 1;
	while (i >= 0 && fseek(fp, i * sizeof (fh), SEEK_SET) == 0) {
		i--;
		if (fread(&fh, sizeof (fh), 1, fp) != 1)
			break;
		if (fh.thread != fh.filetime)
			continue;
		if (bytenum(fh.sizebyte) < 10)
			continue;
		if (count > 0)
			printf(",");
		printf("new alm(%d,'%s','%s')", fh.thread,
		       nohtml(void1(fh.title)), fh2owner(&fh));
		count++;
		if (count >= 10)
			break;
	}
	printf(");</script>");
	fclose(fp);
	return 0;
}

int
shownewmark(char *board)
{
	char buf[80];
	int i, count = 0, total, gotdigest = 0;
	FILE *fp;
	struct fileheader fh;
	board = strtrim(board);
	snprintf(buf, sizeof (buf), "boards/%s/.DIR", board);
	total = file_size(buf) / sizeof (fh);
	fp = fopen(buf, "rb");
	if (fp == NULL) {
		printf("Cann't Open!");
		return -1;
	}
	printf("<script>newmark=new Array(");
	i = total - 1;
	while (i >= 0 && fseek(fp, i * sizeof (fh), SEEK_SET) == 0) {
		i--;
		if (fread(&fh, sizeof (fh), 1, fp) != 1)
			break;
		if (fh.thread != fh.filetime)
			continue;
		if (!(fh.accessed & FH_MARKED))
			continue;
		if (!gotdigest && (fh.accessed & FH_DIGEST)) {
			gotdigest = 1;
			fh.thread = -fh.thread;
		}
		if (bytenum(fh.sizebyte) < 10)
			continue;
		if (count > 0)
			printf(",");
		printf("new alm(%d,'%s','%s')", fh.thread,
		       nohtml(void1(fh.title)), fh2owner(&fh));
		count++;
		if (count >= 10)
			break;
	}
	printf(");</script>");
	fclose(fp);
	return 0;
}
