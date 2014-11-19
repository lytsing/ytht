#include "bbslib.h"

static int printmyblogpicbox(struct Blog* blog);

void
printXBlogHeader()
{
	int cssv = 1;
	printf("Content-type: text/html; charset=utf-8\r\n\r\n");
	//printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n");
	//printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	//要使用 Strict，否则 IE6 里面 iframe 编辑框显示不正常
	//printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n");
	printf
	    ("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n");
	//printf("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n");
	printf("<html><head>");
	printf
	    ("<meta http-equiv=Content-Type content=\"text/html; charset=utf-8\">");
	if (0)
		printf
		    ("<link rel=\"stylesheet\" href=\"%s?%d\" type=\"text/css\">",
		     currstyle->cssfile, cssv);
	printf
	    ("<link rel=\"stylesheet\" href=\"/blog.css?%d\" type=\"text/css\">",
	     cssv);
	//title 需要预先有值，否则 mozilla 上 javascript 无法重新设置 title
	printf("<title>%s</title>", MY_BBS_ID);
	printf
	    ("<script type=\"text/javascript\" src=\"/function.utf8.js?%d\"></script>",
	     cssv);
	printf("<script type=\"text/javascript\" src=\"/blog.js?%d\"></script>",
	       cssv);
	printf
	    ("<script type=\"text/javascript\" src=\"/richtext.utf8.js\"></script>\n");
	// add prototype.js
	printf("<script type=\"text/javascript\" src=\"/prototype.js\"></script>\n");
	printf("</head><body><div id=all><div id=all1><div id=all2>");
}

void
printXBlogEnd()
{
	printf("</div></div></div></body></html>");
}

void
printBlogHeader(struct Blog *blog)
{
	char *nh = nohtml(blog->config->title);
	printf("<script>document.title='%s - %s';</script>", nh,
	       blog->config->useridUTF8);
	printf("<div id=topbar>");
	printf
	    ("<span id=title><a href=\"blog?U=%s\">%s</a></span> <strong>%s的Blog</strong>",
	     blog->config->useridEN, nh, blog->config->useridUTF8);
	printf("<span id=toolbar>");
	printf("<a href=blogpost>写新日志</a> | "
	       "<a href=blogeditconfig>管理页面</a> | "
	       "<a href=blogpage>Blog社区</a> | "
	       "<a href=bbsindex target=_blank>论坛</a>");
	printf("</span>");
	printf("</div>");
}

void
printBlogSideBox(struct Blog *blog)
{
	int i;
#if 1
	char *ptr;
	printf("<script>var articleList=[");
	for (i = 0; i < blog->nIndex; ++i) {
		printf("%d%s", (int) blog->index[i].fileTime,
		       (i == blog->nIndex - 1) ? "" : ",");
	}
	printf("];</script>");
#endif

	printf("<div id=sidebox>");

	printf("<h3>个人档案</h3>");
	printmyblogpicbox(blog);
#if 1
	if (*(ptr = getparm("start"))) {
		printf
		    ("<script>showCalendar(%s,\"blog?U=%s\",articleList);</script>",
		     ptr, blog->config->useridEN);
	} else {
		printf
		    ("<script>showCalendar(%d,\"blog?U=%s\",articleList);</script>",
		     time(NULL), blog->config->useridEN);
	}
#endif
	printf("<h3>栏目</h3><ul>");
	if (!strcmp(currentuser->userid, blog->userid))
		printf("<li><a href=blogdraft>草稿</a></li>");
	for (i = 0; i < blog->nSubject; ++i) {
		if (!*blog->subject[i].title	//|| !blog->subject[i].count
		    || blog->subject[i].hide)
			continue;
		printf
		    ("<li><a href=\"blog?U=%s&amp;subject=%d\">%s(%d)</a></li>",
		     blog->config->useridEN, i, blog->subject[i].title,
		     blog->subject[i].count);
	}
	printf("</ul>");
	printf("<h3>Tags</h3><ul>");

	for (i = 0; i < blog->nTag; ++i) {
		if (!*blog->tag[i].title || !blog->tag[i].count
		    || blog->tag[i].hide)
			continue;
		printf
		    ("<li><a href=\"blog?U=%s&amp;tag=%d\">%s(%d)</a></li>",
		     blog->config->useridEN, i, blog->tag[i].title,
		     blog->tag[i].count);
	}

	printf("</ul>");
	printf("<h3>订阅</h3><ul>");
	printf("<li><a href=\"blogrss2?U=%s\">RSS 2.0</a></li>",
	       blog->config->useridEN);
	printf("<li><a href=\"blogatom?U=%s\">Atom 1.0</a></li>",
	       blog->config->useridEN);
	printf("</ul>");
	printf("</div>");
}

void
printBlogFooter(struct Blog *blog)
{
	printf("<div id=footbar></div>");
}

int
printAbstract(struct Blog *blog, int n)
{
	struct BlogHeader *blh;
	char buf[80];
	int i, t;
	if (n < 0 || n >= blog->nIndex)
		return -1;

	blh = &blog->index[n];
	printf("<div class=post>");
	printf("<h2 class=posttitle>");
	printf("<a href=\"blogread?U=%s&amp;T=%d\">%s</a>",
	       blog->config->useridEN, (int) blh->fileTime, nohtml(blh->title));
	printf("</h2>");
	printf("<div class=postcontent>");

	if (blh->hasAbstract) {
		setBlogAbstract(buf, blog->userid, blh->fileTime);
		showfile(buf);
		printf("<br /><a href=blogread?U=%s&T=%d>(继续阅读...)</a>",
		       blog->config->useridEN, (int) blh->fileTime);
	} else {
		setBlogPost(buf, blog->userid, blh->fileTime);
		showfile(buf);
		printf("<br />(全文完)");
	}
	printf("</div>");

	printf("<div class=postfoot>");
	printf("%s", Ctime(blh->fileTime));
	printf(" | %s", nohtml(blog->subject[blh->subject].title));
	// FIXME: by deli if i < 6, it will print the tag twice
	for (i = 0; i < MAXTAG; ++i) {
		t = blh->tag[i];
		if (t < 0 || t >= blog->nTag)
			continue;
		if (!*blog->tag[t].title)
			continue;
		printf(" | <a href=\"blog?U=%s&amp;tag=%d\">%s</a>%s", 
			blog->config->useridEN, i, blog->tag[t].title, i ? " " : "");
	}

	printf(" | <a href=blogread?U=%s&amp;T=%d#comment>评论(%d)</a>",
	       blog->config->useridEN, (int) blh->fileTime, blh->nComment);
	printf("</div>");

	printf("</div>");	//class=post

	return 0;
}

void
printAbstractsTime(struct Blog *blog, int start, int end)
{
	int i;
	for (i = blog->nIndex - 1; i >= 0; --i) {
		if (blog->index[i].fileTime < start)
			break;
		if (blog->index[i].fileTime >= end)
			continue;
		printAbstract(blog, i);
	}
}

void
printPages(char *link, int count, int currPage)
{
	int i;
	int nPage = (count + 9) / 10;
	if (nPage <= 1)
		return;
	printf("<div>第");
	for (i = 0; i < nPage; ++i) {
		if (currPage == i)
			printf(" <a href=%s&page=%d class=blu><b>%d</b></a>",
			       link, i + 1, i + 1);
		else
			printf(" <a href=%s&page=%d class=blu>%d</a>", link,
			       i + 1, i + 1);
	}
	printf(" 页</div>");
}

void
printAbstractsSubject(struct Blog *blog, int subject, int page)
{
	int i, j, count;
	char buf[80];
	if (subject >= blog->nSubject || subject < 0)
		return;
	count = blog->subject[subject].count;
	if (page * 10 >= count)
		page = 0;
	sprintf(buf, "blog?U=%s&subject=%d", blog->config->useridEN, subject);
	printPages(buf, count, page);
	for (i = blog->nIndex - 1, j = 0; i >= 0 && j < page * 10; --i) {
		if (blog->index[i].subject == subject)
			++j;
	}
	for (j = 0; i >= 0 && j < 10; --i) {
		if (blog->index[i].subject == subject) {
			printAbstract(blog, i);
			++j;
		}
	}
}

int
tagedAs(struct BlogHeader *blh, int tag)
{
	int i;
	for (i = 0; i < 6; ++i)
		if (blh->tag[i] == tag)
			return 1;
	return 0;
}

void
printAbstractsTag(struct Blog *blog, int tag, int page)
{
	int i, j, count;
	char buf[80];
	if (tag >= blog->nTag || tag < 0)
		return;
	count = blog->tag[tag].count;
	if (page * 10 >= count)
		page = 0;
	sprintf(buf, "blog?U=%s&tag=%d", blog->config->useridEN, tag);
	printPages(buf, count, page);
	for (i = blog->nIndex - 1, j = 0; i >= 0 && j < page * 10; --i) {
		if (tagedAs(&blog->index[i], tag))
			++j;
	}
	for (j = 0; i >= 0 && j < 10; --i) {
		if (tagedAs(&blog->index[i], tag)) {
			printAbstract(blog, i);
			++j;
		}
	}
}

void
printAbstractsAll(struct Blog *blog, int page)
{
	int i, j;
	char buf[80];
	if (page * 10 >= blog->nIndex)
		page = 0;
	sprintf(buf, "blog?U=%s", blog->config->useridEN);
	printPages(buf, blog->nIndex, page);

	for (i = blog->nIndex - 1 - page * 10, j = 0; i >= 0 && j < 10; --i) {
		printAbstract(blog, i);
		++j;
	}
}

int
blogblog_main()
{
	char userid[IDLEN + 1], *ptr;
	struct Blog blog;
	int page;

	printXBlogHeader();
	check_msg();
	changemode(READING);
	strsncpy(userid, getparm2("U", "userid"), sizeof (userid));
	if (!*userid && loginok && !isguest) {
		strsncpy(userid, currentuser->userid, sizeof (userid));
	}
	if (openBlog(&blog, userid) < 0) {
		if (!loginok || isguest || strcmp(userid, currentuser->userid)) {
			printf("该用户尚未建立 blog。");
		} else {
			printf
			    ("<br /><br />您还没有建立 blog, 希望建立 blog 吗？<br />");
			printf
			    ("－－<a href=blogsetup><b>是啊，现在建立</b></a><br />");
			printf
			    ("－－<a href=\"javascript:history.go(-1)\"><b>算了，回头再说</b><br />");
		}
		printXBlogEnd();
		return 0;
	}
	printBlogHeader(&blog);
	printBlogSideBox(&blog);
	printf("<div id=contents>");
	page = atoi(getparm("page")) - 1;
	if (page < 0)
		page = 0;

	if (blog.nIndex == 0) {
		printf("目前还没有日志");
	} else if (*(ptr = getparm("subject"))) {
		printAbstractsSubject(&blog, atoi(ptr), page);
	} else if (*(ptr = getparm("tag"))) {
		printAbstractsTag(&blog, atoi(ptr), page);
	} else if (*(ptr = getparm("start"))) {
		printAbstractsTime(&blog, atoi(ptr), atoi(getparm("end")));
	} else {
		printAbstractsAll(&blog, page);
	}
	printf("<div class=line></div>");
	printf("</div>");
	printBlogFooter(&blog);

	if (loginok && !isguest) {
		if (strcmp(currentuser->userid, blog.userid))
			blogLog("READ %s 0 %s", blog.userid,
				currentuser->userid);
	} else {
		blogLog("Read %s 0 %s", blog.userid, fromhost);
	}

	closeBlog(&blog);
	printXBlogEnd();
	return 0;
}

static int
printmyblogpicbox(struct Blog* blog)
{
	struct userec *x;

	if (getuser(blog->userid, &x) <= 0)
		return -1;

	printf("<div class=\"mypic\">");
	printf("<center>");

	if (x->mypic)
		printmypic(x->userid);
	else
		printf("<img src=/defaultmypic.gif>");
	printf("</center>");
	printf("文章：%d 评论:<br />", blog->nIndex);
	printf("经验：");
	printf("</div>");

	return 0;
}

