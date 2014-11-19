#include "bbslib.h"
static char *stat1(struct fileheader *data, int from, int total, int *hasmore);

int
bbstdoc_main()
{
	char board[80], buf[80];
	struct boardmem *x1;
	struct fileheader *data = NULL;
	int i, j, total = 0, sum = 0;
	int start, direction, num;
	int first = 0, last = 0;
	int nothingmore = 0;
	int hasmore = 0;
      struct mmapfile mf = { ptr:NULL };
	static struct fileheader *tdocdata = NULL;
	static int *tdocfrom = NULL;
	if (NULL == tdocdata) {
		tdocdata = malloc(50 * sizeof (struct fileheader));
		if (NULL == tdocdata)
			http_fatal("faint, memory leak?");
	}
	if (NULL == tdocfrom) {
		tdocfrom = malloc(50 * sizeof (int));
		if (NULL == tdocfrom)
			http_fatal("faint, memory leak?");
	}
	changemode(READING);
	getparmboard(board, sizeof (board));
	x1 = getboard2(board);
	if (x1 == 0) {
		html_header(1);
		nosuchboard(board, "bbstdoc");
	}
	if (!has_read_perm_x(currentuser, x1)) {
		html_header(1);
		noreadperm(board, "bbstdoc");
	}
	start = atoi(getparm("S"));
	direction = atoi(getparm("D"));
	if (direction == 0)
		direction = -1;
	/**
	 * 如果start参数存在，那么从 filetime 的文章分界，direction方向里挑出足够多主题显示
	 * 否则 从start为最后一篇文章 
	 */
	updateinboard(x1);
	strcpy(board, x1->header.filename);
	sprintf(buf, "boards/%s/.DIR", board);
	if (cache_header(file_time(buf), 10))
		return 0;
	html_header(1);
	printf("</head><body>");
	check_msg();
	mmapfile(NULL, &mf);
	MMAP_TRY {
		if (mmapfile(buf, &mf) < 0) {
			MMAP_UNTRY;
			http_fatal("无法读取文章列表");
		}
		data = (void *) mf.ptr;
		total = mf.size / sizeof (struct fileheader);
		printf("<nobr><center>\n");
		sprintf(buf, "文章%d篇", total);
		printboardtop(x1, 3, buf);
		if (total <= 0) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("本讨论区目前没有文章");
		}
		if (start == 0) {
			num = total - 1;
			direction = -1;
		} else {
			num =
			    Search_Bin((struct fileheader *) mf.ptr, start, 0,
				       total - 1);
			if (1 == direction) {
				if (num < 0)
					num = -(num + 1);
				else
					num++;
			} else {
				if (num < 0)
					num = -(num + 1);
				num--;
			}
		}
		for (i = num; i >= 0 && i < total; i += direction) {
			if (data[i].thread != data[i].filetime)
				continue;
			memcpy(&(tdocdata[sum]), &(data[i]),
			       sizeof (struct fileheader));
			tdocfrom[sum] = i;
			last = data[i].filetime;
			sum++;
			if (sum == 1)
				first = data[i].filetime;
			if (sum >= w_info->t_lines)
				break;
		}
		if (i < 0 || i >= total)
			nothingmore = 1;
		docinfostr(x1, 1, has_BM_perm(currentuser, x1));
		printf(" <a href=bbspst?B=%d>发表文章</a> ", getbnumx(x1));
		if (!(nothingmore && direction == -1))
			printf("<a href=tdoc?B=%d&S=%d&D=-1>上一页</a> ",
			       getbnumx(x1), (direction == 1) ? first : last);
		if (!(nothingmore && direction == 1))
			printf("<a href=tdoc?B=%d&S=%d&D=1>下一页</a> ",
			       getbnumx(x1), (direction == 1) ? last : first);
		printf("<a href=tdoc?B=%d&S=1&D=1>第一页</a> ", getbnumx(x1));
		printf("<a href=tdoc?B=%d>最后一页</a> ", getbnumx(x1));
		printf
		    ("<a href=# onclick='javascript:{location=location;return false;}' class=blu>刷新</a> ");
		printhr();
		printf("<table cellSpacing=0 cellPadding=2>\n");
		printf
		    ("<tr class=docbgcolor><td>原序号<td>状态<td>作者<td>日期<td>标题<td>回帖/推荐度\n");
		if (sum == 0)
			printf("<tr><td>啊，一篇都没有</td></tr>");
		for (i = 0; i < sum; i++) {
			if (direction == -1)
				j = sum - i - 1;
			else
				j = i;
			printf("<tr><td>%d<td>%s<td>%s",
			       tdocfrom[j] + 1, flag_str(tdocdata[j].accessed),
			       userid_str(fh2owner(&tdocdata[j])));
			printf("<td>%6.6s", Ctime(tdocdata[j].filetime) + 4);
			printf
			    ("<td><a href=bbsnt?B=%d&start=%d&th=%d>○ %s </a><td> %s\n",
			     getbnumx(x1), tdocfrom[j], tdocdata[j].thread,
			     void1(titlestr(tdocdata[j].title)),
			     stat1(data, tdocfrom[j], total, &hasmore));
		}
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);

	if (!hasmore) {
		struct boardaux *boardaux;
		boardaux = getboardaux(getbnum(board));
		for (i = 0; boardaux && i < boardaux->ntopfile; i++) {
			struct fileheader *tfh = &boardaux->topfile[i];
			char title[60];
			char *picNew = "";

			sprintf(buf, "boards/%s/%s", board, fh2fname(tfh));
			if (now_t - file_time(buf) < 24 * 3600)
				picNew = "<img src=/new.gif>";

			printf
			    ("<tr><td colspan=2><b>[提示]</b></td><td>%s</td><td>%s</td>",
			     userid_str(fh2owner(tfh)),
			     short_Ctime(tfh->filetime));
			strncpy(title, tfh->title, 60);
			title[60] = 0;
			printf
			    ("<td colspan=2><a href=bbstopcon?B=%d&F=%s>%s%s</a>%s%s</td></tr>",
			     getbnumx(x1), fh2fname(tfh),
			     strncmp(title, "Re: ", 4) ? "○ " : "",
			     void1(titlestr(title)),
			     size_str(bytenum(tfh->sizebyte)), picNew);
		}
	}

	printf("</table>");
	printhr();
	docinfostr(x1, 1, has_BM_perm(currentuser, x1));
	printf("<a href=bbspst?B=%d>发表文章</a> ", getbnumx(x1));
	if (!(nothingmore && direction == -1))
		printf("<a href=tdoc?B=%d&S=%d&D=-1>上一页</a> ", getbnumx(x1),
		       (direction == 1) ? first : last);
	if (!(nothingmore && direction == 1))
		printf("<a href=tdoc?B=%d&S=%d&D=1>下一页</a> ", getbnumx(x1),
		       (direction == 1) ? last : first);
	printf("<a href=tdoc?B=%d&S=1&D=1>第一页</a> ", getbnumx(x1));
	printf("<a href=tdoc?B=%d>最后一页</a> ", getbnumx(x1));
	printf
	    ("<a href=# onclick='javascript:{location=location;return false;}' class=blu>刷新</a>");
	printf("<table border=0 cellspacing=0 cellpading=0><tr><td>"
	       "<form name=docform2 action=\"tdoc\"><input type=submit value=转到>"
	       "<input type=text name=B size=7>讨论区</form></td></tr></table>");
	printrelatedboard(board);
	http_quit();
	return 0;
}

static char *
stat1(struct fileheader *data, int from, int total, int *hasmore)
{
	static char buf[256];
	int thread = data[from].thread;
	int i, re = 0, click = data[from].staravg50 * data[from].hasvoted / 50;
	*hasmore = 0;
	for (i = from; i < total; i++) {
		if (thread == data[i].thread) {
			re++;
			click += data[i].staravg50 * data[i].hasvoted / 50;
		} else if (data[i].thread == data[i].filetime)
			(*hasmore)++;
	}
	sprintf(buf, "<font color=%s>%d</font>/<font color=%s>%d</font>",
		re > 9 ? "red" : "black", re - 1, click ? "red" : "black",
		click);
	return buf;
}
