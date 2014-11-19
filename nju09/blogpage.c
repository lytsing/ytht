#include "bbslib.h"

static void
printNewPost()
{
	char buf[512], *useridEN, *useridUTF8, *ptr;
	int fileTime;
	FILE *fp;
	fp = fopen("blog/blog.newpost.www", "r");
	if (!fp)
		return;
	printf("<h3 style=\"text-align:left\">最新网志</h3><ul>");
	while (fgets(buf, sizeof (buf), fp)) {
		fileTime = atoi(buf);
		if (!(ptr = strchr(buf, '\t')))
			continue;
		ptr++;
		useridEN = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		useridUTF8 = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		printf("<li><a href=\"blogread?U=%s&T=%d\">%s</a> ",
		       useridEN, fileTime, nohtml(ptr));
		printf("(<a href=\"blog?U=%s\">%s</a>)</li>",
		       useridEN, useridUTF8);
	}
	printf("</ul>");
	fclose(fp);
}

static void
printHotPost()
{
	char buf[512], *useridEN, *useridUTF8, *ptr;
	int fileTime;
	FILE *fp;
	fp = fopen("blog/blog.hotpost", "r");
	if (!fp)
		return;
	printf("<h3 style=\"text-align:left\">热门文章</h3><ul>");
	while (fgets(buf, sizeof (buf), fp)) {
		fileTime = atoi(buf);
		if (!(ptr = strchr(buf, '\t')))
			continue;
		ptr++;
		useridEN = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		useridUTF8 = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		printf("<li><a href=\"blogread?U=%s&T=%d\">%s</a> ",
		       useridEN, fileTime, nohtml(ptr));
		printf("(<a href=\"blog?U=%s\">%s</a>)</li>",
		       useridEN, useridUTF8);
	}
	printf("</ul>");
	fclose(fp);
}

static void
printNewBlog()
{
	char buf[512], *useridEN, *useridUTF8, *ptr;
	int fileTime;
	FILE *fp;
	fp = fopen("blog/blog.create.www", "r");
	if (!fp)
		return;
	printf("<h3>最新加入</h3><ul>");
	while (fgets(buf, sizeof (buf), fp)) {
		fileTime = atoi(buf);
		if (!(ptr = strchr(buf, '\t')))
			continue;
		ptr++;
		useridEN = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		useridUTF8 = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		printf("<li><a href=\"blog?U=%s&T=%d\">%s</a> ",
		       useridEN, fileTime, nohtml(ptr));
//		printf(" (<a href=\"blog?U=%s\">%s</a>)</li>",
//		       useridEN, useridUTF8);
		printf(" (<a href=\"qry?U=%s\">%s</a>)</li>",
		       useridEN, useridUTF8);
	}
	printf("</ul>");
	fclose(fp);
}

static void
printHotBlog()
{
	char buf[512], *useridEN, *useridUTF8, *ptr;
	int score;
	FILE *fp;
	fp = fopen("blog/blog.hot.www", "r");
	if (!fp)
		return;
	printf("<h3 style=\"text-align:left\">热门BLOG</h3><ul>");
	while (fgets(buf, sizeof (buf), fp)) {
		score = atoi(buf);
		if (!(ptr = strchr(buf, '\t')))
			continue;
		ptr++;
		useridEN = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		useridUTF8 = ptr;
		if (!(ptr = strchr(ptr, '\t')))
			continue;
		*ptr = 0;
		ptr++;
		printf("<li><a href=\"blog?U=%s\">%s</a> ",
		       useridEN, nohtml(ptr));
		printf(" (%s)</li>", useridUTF8);
	}
	printf("</ul>");
	fclose(fp);
}

int
blogpage_main()
{
	struct Blog blog;

	printXBlogHeader();
	printf("<div id=topbar>");
	printf("<span id=title>真水无香 Blog 社区</span>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
	printf("<span id=toolbar>");
	if (loginok && !isguest)
		printf("<a href=blog>我的Blog</a> |"
		       "<a href=blogpost>写新日志</a> | "
		       "<a href=blogeditconfig>管理页面</a> | ");
	else
		printf("<a href=\"#\" "
		       "onClick=\"open('bbslform','','left=255,top=190,width=130,height=100');return false\">"
		       "请先登录</a> | ");

	printf("<a href=blogpage>Blog社区</a> | "
	       "<a href=bbsindex target=_blank>论坛</a>");
	printf("</span>");
	printf("</div>");
	printf("<div id=pagesidebox>");
	if (!loginok || isguest) {
		printLoginFormUTF8();
	}
	printHotBlog();
	printNewBlog();
	printf("</div>");
	printf("<div style=\"margin-right:220px;\">");
	printf("<div id=newpost>");
	printNewPost();
	printf("</div>");
	printf("<div id=newblog>");
	printf("</div>");
	printf("</div>");
	createBlog("blog");
	if (openBlog(&blog, "blog") >= 0) {
		//...
		closeBlog(&blog);
	}
	printXBlogEnd();
	return 0;
}
