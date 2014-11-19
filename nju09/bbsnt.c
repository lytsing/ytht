#include "bbslib.h"
#define MAX_OTHERTHREAD 4
int
bbsnt_main()
{
	char board[80], dir[80];
	struct fileheader *x;
	struct boardmem *x1;
	int num = 0, found = 0, start, total;
	int thread;
      struct mmapfile mf = { ptr:NULL };
	struct fileheader *other_thread[MAX_OTHERTHREAD];
	int otnum = 0;

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
		printf
		    ("<div class=hright><a href=bbsdoc?B=%d&start=%d>回文章列表</a></div><br>",
		     getbnumx(x1), start - 4);

		x = (struct fileheader *) (mf.ptr +
					   start * sizeof (struct fileheader));
		for (num = start; num < total; num++, x++) {
			if (x->thread != thread) {
				continue;
			}
			printf
			    ("<div class=title>话题：%s</div><div class=wline3></div>",
			     void1(titlestr(x->title)));
			show_file(x1, x, num, 0);
			found = 1;
			break;
		}
		if (!found)
			printf("<div class=wline3></div>");

		if (found) {
			char *lasttitle = "";
			printf("跟贴 ");
			printf
			    ("<a href=bbstcon?B=%d&start=%d&th=%d>所有跟贴全部展开</a> ",
			     getbnumx(x1), num, thread);
			printf
			    ("<table border=1><tr><td>编号</td><td>状态</td><td>作者</td>"
			     "<td>日期</td><td>标题　　　　　&nbsp;</td><td>星级</td></tr>\n");
#if 1
			printf("<script>docItemInit('%d',0,%d);\n",
			       getbnumx(x1), (int) now_t);
#endif
			for (; num < total; num++, x++) {
				if (x->thread != thread) {
					if (otnum < MAX_OTHERTHREAD
					    && x->thread == x->filetime
					    && strcasecmp(x->owner,
							  "deliver")) {
						other_thread[otnum++] = x;
					}
					continue;
				}
#if 0
				printf("<tr><td>%d</td><td>%s</td>", num + 1,
				       flag_str2(x->accessed, !brc_un_read(x)));
				printf("<td>%s</td>", userid_str(fh2owner(x)));
				printf("<td>%6.6s</td>",
				       Ctime(x->filetime) + 4);
				printf
				    ("<td><a href=con?B=%d&F=%s&N=%d&st=1&T=%d>%s </a>\n",
				     getbnumx(x1), fh2fname(x), num + 1,
				     feditmark(x), nohtml(x->title));
				if (x->sizebyte)
					printf(" %s",
					       size_str(bytenum(x->sizebyte)));
				printf("</td><td class=%s>%d/%d人</td></tr>\n",
				       x->staravg50 ? "red" : "blk",
				       x->staravg50 / 50, x->hasvoted);
#else
				printf("ntItem(%d,'%s','%s','%s',%d,", num + 1,
				       flag_str2(x->accessed, !brc_un_read(x)),
				       fh2owner(x), fh2fname(x), feditmark(x));
				if (strcmp(x->title, lasttitle)) {
					printf("'%s',", scriptstr(x->title));
					lasttitle = x->title;
				} else
					printf("'',");
				printf("%d,%d,%d);\n", bytenum(x->sizebyte),
				       x->staravg50 / 50, x->hasvoted);
#endif
			}
#if 1
			printf("</script>");
#endif
			printf("</table><br>\n");
			if (otnum) {
				printf("本版其他主题<br>");
				printf("<table border=1>\n");
				printf
				    ("<tr><td>作者</td><td>日期</td><td>标题</td></tr>\n");
				for (num = 0; num < otnum; num++) {
					printf("<tr><td>%s</td>",
					       userid_str(fh2owner
							  (other_thread[num])));
					printf("<td>%6.6s</td>",
					       Ctime(other_thread
						     [num]->filetime) + 4);
					printf
					    ("<td><a href=bbsnt?B=%d&th=%d>○ %s </a></td></tr>\n",
					     getbnumx(x1),
					     other_thread[num]->thread,
					     void1(titlestr
						   (other_thread[num]->title)));
				}
			}
			printf("</table><br>\n");
		}
	}
	MMAP_CATCH {
		found = -1;
	}
	MMAP_END mmapfile(NULL, &mf);
	if (found == -1) {
		printf("文件列表更新例外, 请"
		       "<a href=# onclick='javascript:{location=location;return false;}'>刷新</a>"
		       "<script>setTimeout('location.reload()', 1000);</script>");
		http_quit();
		return 0;
	}
	if (found == 0)
		http_fatal("错误的文件名");
	printf("[<a href='javascript:history.go(-1)'>返回上一页</a>]");
	printf("[<a href=bbsdoc?B=%d&start=%d>本讨论区</a>]<hr>", getbnumx(x1),
	       start - 4);
	printrelatedboard(board);
	processMath();
	printf("</center></div></body>\n");
	brc_update(currentuser->userid);
	http_quit();
	return 0;
}
