#include "bbslib.h"
char *size_str(int size);

void
printdocform(char *cginame, int bnum)
{
	printf("<script>docform('%s','%d');</script>", cginame, bnum);
}

void
noreadperm(char *board, char *cginame)
{
	printf("��������������һ�����ֲ����棬ֻ�о��ֲ���Ա���Է��ʡ���Ҫ");
	if (!loginok || isguest) {
		printf("��¼����");
	}
	printf("����������������þ��ֲ���<p>");
	printf("<p><a href=javascript:history.go(-1)>���ٷ���</a>");
	http_quit();
}

//��lepton���������д��һ������
void
nosuchboard(char *board, char *cginame)
{
	int i, j = 0;
	char buf[128];
	int closed = 0;
	printf("û������������������ܵ�ѡ��:<p>");
	printf("<table width=300>");
	board = strtrim(board);
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		if (!strcasestr(shm_bcache->bcache[i].header.filename, board) &&
		    !(strcasestr(shm_bcache->bcache[i].header.title, board)))
			continue;
		if (!has_view_perm_x(currentuser, &(shm_bcache->bcache[i])))
			continue;
		closed = (shm_bcache->bcache[i].header.flag & CLOSECLUB_FLAG);
		printf("<tr><td>");
		printf("<a href=%s?B=%d>%s (%s)</a>",
		       cginame, getbnumx(&shm_bcache->bcache[i]),
		       void1(titlestr(shm_bcache->bcache[i].header.title)),
		       shm_bcache->bcache[i].header.filename);
		printf("</td></tr>");
		j++;
		if (j == 1)
			sprintf(buf, "%s?B=%d", cginame,
				getbnumx(&shm_bcache->bcache[i]));
	}
	printf("</table>");
	if (!j)
		printf
		    ("ร�����İ������ˣ����Ǹ�������һ���������, û�н��� \"%s\" ����������",
		     board);
	if (j == 1 && !closed)
		redirect(buf);
	printf("<p><a href=javascript:history.go(-1)>���ٷ���</a>");
	http_quit();
}

void
printhrwhite()
{
	printf("<div class=wline></div>");
}

void
printboardhot(struct boardmem *x)
{
	char path[STRLEN];
	sprintf(path, "boards/%s/TOPN", x->header.filename);
	if (showfile(path) >= 0)
		printhr();
	else
		printhrwhite();
}

void
printrelatedboard(char *board)
{
	int i;
	struct boardaux *boardaux = getboardaux(getbnum(board));
	if (!boardaux || !boardaux->nrelate)
		return;
	printf("<div class=wline3></div>");
	printf("�����������˻�ȥ���°���:");
	for (i = 0; i < boardaux->nrelate; i++) {
		printf(" <a href=doc?B=%d>&lt;%s&gt;</a>",
		       getbnum(boardaux->relate[i].filename),
		       void1(titlestr(boardaux->relate[i].title)));
	}
}

void
printWhere(struct boardmem *x, char *str)
{
	int i;
	char buf[10];
	printf("<div class=hleft>");
	printf("<script>checkFrame();</script>");
	for (i = 0; x->header.sec1[i]; i++) {
		strsncpy(buf, x->header.sec1, i + 2);
		printf("<a href=boa?secstr=%s class=blk>%s</a> �� ",
		       buf, nohtml(getsectree(buf)->title));
	}
	printf("<a href=home?B=%d class=blk>%s(%s)</a> ", getbnumx(x),
	       nohtml(x->header.title), x->header.filename);
	if (!str)
		printf(" (<a href=brdadd?B=%d class=blk>Ԥ��</a>)",
		       getbnumx(x));
	else
		printf("�� %s", str);
	printf("</div>");
}

void
printboardtop(struct boardmem *x, int num, char *infostr)
{
	char *board = x->header.filename;
	//char *c1 = "whi", *c2 = "blu";
	char bmbuf[IDLEN * 4 + 4];
	char sbmbuf[IDLEN * 12 + 12];
	printWhere(x, NULL);
	printf("<div class=hright>����[%s]",
	       userid_str(bm2str(bmbuf, &(x->header))));
	if (x->ban)
		printf("<a href=bbsshowfile?F=banAlert class=red>%s!!!</a>",
		       (x->ban == 1) ? "" : "<b>");
	if (strlen(sbm2str(sbmbuf, &(x->header)))) {
		printf("<br>С����[%s]", userid_str(sbmbuf));
	}
	printf("</div>");
	printf("<center><font size=+2>%s</font></center>", void1(titlestr(x->header.title)));
	updatewwwindex(x);
	//For javascript function tabs(...), check html/function.js
	printf("<script>tabs('%d',%d,%d,", getbnumx(x), num, x->wwwindex);
	if (!politics(board) && x->wwwbkn)
		printf("1,");
	else
		printf("0,");
	printf("'%s','%s',%d,%d,%d);</script>", anno_path_of(board),
	       infostr ? infostr : "", x->inboard,
	       (x->header.flag & VOTE_FLAG) ? 1 : 0,
	       x->hasnotes || has_BM_perm(currentuser, x));
}

// start �� 1 ��ʼ
int
getdocstart(int total, int lines)
{
	char *ptr;
	int start;
	ptr = getparm("S");
	if (!ptr[0])
		ptr = getparm("start");
//      if (sscanf(ptr, "%d", &start) != 1)
//              start = 0;
	start = atoi(ptr);
	if (!*ptr || start == -100 || start > total - lines + 1)
		start = total - lines + 1;
	if (start <= 0)
		start = 1;
	return start;
}

void
bbsdoc_helper(char *cgistr, int start, int total, int lines)
{
	if (start > 1)
		printf("<a href=%s&S=%d>��һҳ</a> ", cgistr, start - lines);
	else
		printf("��һҳ ");
	if (start < total - lines + 1)
		printf("<a href=%s&S=%d>��һҳ</a> ", cgistr, start + lines);
	else
		printf("��һҳ ");
	printf
	    ("<a href=# onclick='javascript:{location=location;return false;}' class=blu>ˢ��</a> ");
}

#if 0
static int
istoday(time_t t)
{
	struct tm *tm;
	int year, yday;
	tm = localtime(&now_t);
	if (NULL == tm)
		return 0;
	year = tm->tm_year;
	yday = tm->tm_yday;
	tm = localtime(&t);
	if (NULL == tm)
		return 0;
	if (year == tm->tm_year && yday == tm->tm_yday)
		return 1;
	return 0;
}
#endif

int
bbsdoc_main()
{
	char board[80], dir[80], genbuf[STRLEN], title[STRLEN];
	struct boardmem *x1;
	struct fileheader *x, x2;
      struct mmapfile mf = { ptr:NULL };
	int j, i = 0, start, total, fd = -1, sizebyte;
	struct boardaux *boardaux;
	changemode(READING);
	getparmboard(board, sizeof (board));
	x1 = getboard2(board);
	boardaux = getboardaux(getbnum(board));
	if (!x1 || !boardaux) {
		html_header(1);
		nosuchboard(board, "doc");
	}
	if (!has_read_perm_x(currentuser, x1)) {
		html_header(1);
		noreadperm(board, "doc");
	}
	updateinboard(x1);
	sprintf(dir, "boards/%s/.DIR", board);
//      if(cache_header(file_time(dir),10))
//              return 0; 
	html_header(1);
	check_msg();
	total = x1->total;
	start = getdocstart(total + boardaux->ntopfile, w_info->t_lines);
	brc_initial(currentuser->userid, board);
	printf("</head><body>\n<nobr><center>");
	sprintf(genbuf, "����%dƪ", total);
	printboardtop(x1, 3, genbuf);
	printboardhot(x1);
	printf("<table cellSpacing=0 cellPadding=2>\n");
	printf
	    ("<tr class=docbgcolor><td>���</td><td>״̬</td><td>����</td><td>"
	     "ʱ��</td><td>����</td><td>�Ǽ�</td></tr>\n");
	MMAP_TRY {
		mmapfile(dir, &mf);
		total = mf.size / sizeof (struct fileheader);
		if (total <= 0) {
			printf
			    ("<tr><td colspan=6><h3>��������Ŀǰû������</h3></td></tr>");
		}
		printf("<script>docItemInit('%d',%d,%d);", getbnumx(x1), start,
		       (int) now_t);
		x = (struct fileheader *) mf.ptr;
		for (i = 0; i < w_info->t_lines; i++) {
			j = start + i - 1;
			if (j < 0 || j >= total)
				break;
			while (x[j].sizebyte == 0) {
				char filename[80];
				sprintf(filename, "boards/%s/%s", board,
					fh2fname(&(x[j])));
				sizebyte = numbyte(eff_size(filename));
				fd = open(dir, O_RDWR);
				if (fd < 0)
					break;
				flock(fd, LOCK_EX);
				lseek(fd,
				      j * sizeof (struct fileheader), SEEK_SET);
				if (read(fd, &x2, sizeof (x2)) == sizeof (x2)
				    && x[j].filetime == x2.filetime) {
					x2.sizebyte = sizebyte;
					lseek(fd, -1 * sizeof (x2), SEEK_CUR);
					write(fd, &x2, sizeof (x2));
				}
				flock(fd, LOCK_UN);
				close(fd);
				fd = -1;
				break;
			}
			printf("docItem('%s','%s','%s',%d,",
			       flag_str2(x[j].accessed, !brc_un_read(&(x[j]))),
			       fh2owner(&(x[j])), fh2fname(&(x[j])),
			       feditmark(&x[j]));
			printf("'%s',%d,%d,%d);\n",
			       scriptstr(x[j].title), bytenum(x[j].sizebyte),
			       x[j].staravg50 / 50, x[j].hasvoted);
		}
	}
	MMAP_CATCH {
		if (fd != -1)
			close(fd);
	}
	MMAP_END mmapfile(NULL, &mf);
	printf("</script>\n");
	if (i < w_info->t_lines) {
		//��ʾ�ö�����
		for (j = 0; j < boardaux->ntopfile; j++) {
			struct fileheader *tfh = &boardaux->topfile[j];
			char buf[STRLEN];
			char *picNew = "";

			sprintf(buf, "boards/%s/%s", board, fh2fname(tfh));
			if (now_t - file_time(buf) < 24 * 3600)
				picNew = "<img src=/new.gif>";

			printf
			    ("<tr><td colspan=2><b>[��ʾ]</b></td><td>%s</td><td>%6.6s</td>",
			     userid_str(fh2owner(tfh)),
			     (Ctime(tfh->filetime) + 4));
			strncpy(title, tfh->title, 60);
			title[60] = 0;
			printf
			    ("<td colspan=3><a href=bbstopcon?B=%d&F=%s>%s%s</a>%s%s</td></tr>",
			     getbnumx(x1), fh2fname(tfh),
			     strncmp(title, "Re: ", 4) ? "�� " : "",
			     void1(titlestr(title)),
			     size_str(bytenum(tfh->sizebyte)), picNew);
		}
	}

	printf("</table>");
	printhr();
	docinfostr(x1, 0, has_BM_perm(currentuser, x1));
	printf(" <a href=pst?B=%d class=red>��Ҫ�������£�</a> \n",
	       getbnumx(x1));
	printf("<a href=bfind?B=%d>���ڲ�ѯ</a> ", getbnumx(x1));
	printf("<a href=clear?B=%d&S=%d>���δ��</a> ", getbnumx(x1), start);
	sprintf(genbuf, "doc?B=%d", getbnumx(x1));
	bbsdoc_helper(genbuf, start, total + boardaux->ntopfile,
		      w_info->t_lines);
	printdocform("doc", getbnumx(x1));
	printrelatedboard(board);
	printf("</body>");
	http_quit();
	return 0;
}

char *
size_str(int size)
{
	static char buf[256];
	if (size < 1000) {
		sprintf(buf, "(<font class=tea>%d��</font>)", size);
	} else {
		sprintf(buf, "(<font class=red>%d.%dǧ��</font>)", size / 1000,
			(size / 100) % 10);
	}
	return buf;
}

char *
short_Ctime(time_t t)
{
	char *ptr = Ctime(t) + 4;
	ptr[12] = 0;
	//��20Сʱ�ڵý�������
//      if (now_t - t < 20 * 3600)
	ptr += 7;
	return ptr;
}

void
docinfostr(struct boardmem *brd, int mode, int haspermm)
{
	if (mode == 0)
		printf("[һ��ģʽ <a href=tdoc?B=%d>����ģʽ</a>",
		       getbnumx(brd));
	else
		printf("[<a href=doc?B=%d>һ��ģʽ</a> ����ģʽ",
		       getbnumx(brd));
	if (haspermm) {
		if (mode == 2)
			printf(" ����ģʽ");
		else
			printf(" <a href=mdoc?B=%d>����ģʽ</a>",
			       getbnumx(brd));
	}
	printf("]");
}
