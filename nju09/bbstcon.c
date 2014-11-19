#include "bbslib.h"

int
bbstcon_main()
{
	char board[80], dir[80];
	struct fileheader *x;
	struct boardmem *x1;
	int num = 0, firstnum = 0, found = 0, start, total, count;
	int thread;
      struct mmapfile mf = { ptr:NULL };
	html_header(1);
	check_msg();
	getparmboard(board, sizeof (board));
	thread = atoi(getparm("th"));
	printf("</head><body><div class=swidth>\n");
	if ((x1 = getboard(board)) == NULL)
		http_fatal("错误的讨论区");
	start = atoi(getparm("start"));
	brc_initial(currentuser->userid, board);
	sprintf(dir, "boards/%s/.DIR", board);
	MMAP_TRY {
		if (mmapfile(dir, &mf) == -1) {
			MMAP_UNTRY;
			http_fatal("目录错误");
		}
		total = mf.size / sizeof (struct fileheader);
		if (start <= 0 || start > total) {
			start =
			    Search_Bin((struct fileheader *) mf.ptr, thread, 0,
				       total - 1);
			if (start < 0)
				start = -(start + 1);
		} else {
			start--;
		}
		printWhere(x1, "主题文章阅读");
		printf("<div class=hright>"
		       "<a href=tfind?B=%d&th=%d>同主题索引</a> <a href=doc?B=%d&S=%d>回文章列表</a>"
		       "</div><br>", getbnumx(x1), thread, getbnumx(x1), start);
		for (num = start, count = 0; num < total; num++) {
			x = (struct fileheader *) (mf.ptr +
						   num *
						   sizeof (struct fileheader));
			if (x->thread != thread) {
				continue;
			}
			if (!found) {
				printf
				    ("<div class=title>话题：%s</div><div class=wline3></div>",
				     void1(titlestr(x->title)));
				found = 1;
				firstnum = num - 1;
			}
			count++;
			show_file(x1, x, num, count);
		}
	}
	MMAP_CATCH {
		found = 0;
	}
	MMAP_END mmapfile(NULL, &mf);
	if (found == 0)
		http_fatal("错误的文件名");
	printf("<div class=line></div>");
	printf("[<a href='javascript:history.go(-1)'>返回上一页</a>]");
	printf("[<a href=bbsdoc?B=%d&start=%d>本讨论区</a>]", getbnumx(x1),
	       firstnum - 4);
	processMath();
	printf("</center></div></body>\n");
	brc_update(currentuser->userid);
	http_quit();
	return 0;
}

int
fshow_file(FILE * output, char *board, struct fileheader *x)
{
	char path[80];
	char interurl[256];
	if ((x->accessed & FH_MATH)) {
		usedMath = 1;
		usingMath = 1;
		withinMath = 0;
	} else {
		usingMath = 0;
	}
	binarylinkfile(fh2fname(x));
	sprintf(path, "boards/%s/%s", board, fh2fname(x));
	if (!w_info->doc_mode && !hideboard(board)
	    && (via_proxy
		|| (wwwcache->accel_addr.s_addr && wwwcache->accel_port))) {
		sprintf(path, "boards/%d/%s", getbnum(board), fh2fname(x));
		if (via_proxy)
			snprintf(interurl, sizeof (interurl),
				 "/" SMAGIC "/%s+%d%s%s", path,
				 x->edittime ? (x->edittime - x->filetime) : 0,
				 usingMath ? "m" : "",
				 stripHeadTail ? "s" : "");
		else
			snprintf(interurl, sizeof (interurl),
				 "http://%s:%d/" SMAGIC "/%s+%d%s%s",
				 inet_ntoa(wwwcache->text_accel_addr),
				 wwwcache->text_accel_port, path,
				 x->edittime ? (x->edittime - x->filetime) : 0,
				 usingMath ? "m" : "",
				 stripHeadTail ? "s" : "");
		fprintf(output, "<script>docStr=''</script>");
		fprintf(output, "<script src=\"%s\"></script>", interurl);
		ttid(1);
		fprintf(output, "<div id=tt%d class=content0></div>", ttid(0));
		fprintf(output,
			"<script>document.getElementById('tt%d').innerHTML=docStr+conPadding;</script>",
			ttid(0));
		return 0;
	}
	ttid(1);
	fprintf(output, "<div id=tt%d class=content0></div>", ttid(0));
	fputs("<script>var docStr = \"", output);
	fshowcon(output, path, 2);
	fprintf(output,
		"\";\ndocument.getElementById('tt%d').innerHTML=docStr+conPadding;</script>",
		ttid(0));
	return 0;
}

int
show_file(struct boardmem *brd, struct fileheader *x, int num, int floor)
{
	//printf("<div>");
	if (!strchr(x->owner, '.'))
		stripHeadTail = 1;
	printf("<a name=#%d></a>", x->filetime);
	printf("<div class=line></div>");
	printmypicbox(fh2owner(x));
	if (bytenum(x->sizebyte) > 40) {
		printf
		    ("<div class=hright>[<a href='pst?B=%d&F=%s&num=%d'>回复本文</a>]</div>",
		     getbnumx(brd), fh2fname(x), num + 1);
	}
	if (stripHeadTail)
		printf
		    ("<div class=tcontent1><b>%s</b></div>",
		     void1(titlestr(x->title)));
	printf("<div class=scontent1>");
	if (floor > 0) {
		if (floor == 1)
			printf("<font color=red>楼主</font> ");
		else
			printf("第<font color=red>%d</font>楼 ", floor);
	}
	printf("<b>%s</b><span> 发表于 ",
	       userid_str(fh2owner(x)));
	printf("<script>pNumDate(%d)</script> ", x->filetime);
	//printf("[<a href='pstmail?B=%d&F=%s&num=%d'>回信给作者</a>]",
	//       getbnumx(brd), fh2fname(x), num);
#ifdef ENABLE_MYSQL
	if (bytenum(x->sizebyte) > 40) {
		printf("[星级: %d ", x->staravg50 / 50);
		printf("评价人数: %d]", x->hasvoted);
	}
#endif
	printf("</span></div>");
	//printf("<div class=dline></div>");
	//printf("<div class=content0 style='padding-top:5px'>");
	fshow_file(stdout, brd->header.filename, x);
	printf("<div class=scontent1 style='text-align:right'>");
	printf("[<a href=con?B=%d&F=%s&N=%d&T=%d>本篇全文</a>]",
	       getbnumx(brd), fh2fname(x), num + 1, feditmark(x));
	printf("[<a href='pst?B=%d&F=%s&num=%d'>回复本文</a>]",
	       getbnumx(brd), fh2fname(x), num + 1);
#ifdef ENABLE_MYSQL
	if (loginok && now_t - x->filetime <= 3 * 86400
	    && bytenum(x->sizebyte) > 40) {
		printf("<script>eva('%d','%s');</script>", getbnumx(brd),
		       fh2fname(x));
	}
#endif
	printf("</div>");
	printf ("<div style=\"clear:both\"></div>");
	brc_add_read(x);
	//changeContentbg();
	stripHeadTail = 0;
	return 0;
}
