#include "bbslib.h"

int
bbsmypic_main()
{
	char userid[sizeof (currentuser->userid)], filename[256];
	struct userec *x;
      struct mmapfile mf = { ptr:NULL };
	strsncpy(userid, getparm2("U", "userid"), sizeof (userid));
	if (getuser(userid, &x) <= 0) {
		html_header(1);
		printf("啊，没这个人啊？！");
		http_quit();
	}
	sethomefile(filename, x->userid, "mypic");
	if (cache_header(file_time(filename), 3600 * 24 * 100)) {
		return 0;
	}
	MMAP_TRY {
		if (mmapfile(filename, &mf)) {
			MMAP_UNTRY;
			http_fatal("错误的文件名");
		}
		printf("Content-type: %s\r\n", get_mime_type("mypic.gif"));
		printf("Content-Length: %d\r\n\r\n", mf.size);
		fwrite(mf.ptr, 1, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END {
		mmapfile(NULL, &mf);
	}
	return 0;
}

void
printmypic(char *userid)
{
	char buf[80];
	int t;
	sethomefile(buf, userid, "mypic");
	t = file_time(buf);
	printf("<img src=/" SMAGIC "/mypic?U=%s&t=%d alt=''"
	       " onLoad='javascript:if(this.width>120||this.height>160){"
	       "if(this.width/this.height>0.75)this.width=120;"
	       "else this.height=160;}'>", userid, t);
}

int
printmypicbox(char *userid)
{
	struct userec *x;
	int t = 0;
	char buf[80];

	if (getuser(userid, &x) <= 0)
		return -1;
	if (x->mypic) {
		sethomefile(buf, userid, "mypic");
		t = file_time(buf);
	}

	printf("<script>printMypic('%s','%s',%d,%d,%d,%d,'%s',%d,'%s',%d);</script>",
	       SMAGIC, userid, t, x->numposts, countlife(x),
	       countexp(x), cuserexp(countexp(x)),
	       countperf(x), cperf(countperf(x)), x->hasblog);
#if 0
	printf("<div class=mypic>");
	printf("<a href=qry?U=%s><b>%s</b></a><br><center>",
	       x->userid, x->userid);
	if (x->mypic)
		printmypic(x->userid);
	else
		printf("<img src=/defaultmypic.gif>");
	printf("</center><font class=f1>");
	printf("文章：%d<br>生命：%d", x->numposts, countlife(x));
	printf("<br>经验：%d(%s)", countexp(x), cuserexp(countexp(x)));
	printf("<br>表现：%d(%s)", countperf(x), cperf(countperf(x)));
	printf("</font></div>");
#endif
	return 0;
}
