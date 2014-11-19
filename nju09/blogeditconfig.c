#include "bbslib.h"

void
modifyConfig(struct Blog *blog, int echo)
{
	int count = 0;
	char *title;
	if (echo)
		printf("修改 bLog...<br>");
	title = getparm("title");
	if (title[0] && strcmp(title, blog->config->title)) {
		strsncpy(blog->config->title, title,
			 sizeof (blog->config->title));
		utf8cut(blog->config->title, sizeof (blog->config->title));
		if (!*blog->config->title) {
			strcpy(blog->config->title, "No title");
		}
		count++;
	}
	if (!count && echo)
		printf("没有进行任何修改！<br>");
	if (echo)
		printf("<br>");
}

void
printBlogSettingSideBox(struct Blog *blog)
{
	printf("<div id=sidebox>");
	printf("<ul>");
	printf("<li><a href=blogeditconfig>基本设置</a></li>");
	printf("<li><a href=blogeditsubject>栏目设置</a></li>");
	printf("<li><a href=blogedittag>Tag 设置</a></li>");
	printf("<li><a href=blogeditpost>文章管理</a></li>"); // add by deli
	printf("</ul>");
	printf("</div>");
}

int
blogeditconfig_main()
{
	struct Blog blog;
	char *cmd;

	printXBlogHeader();
	check_msg();
	if (!loginok || isguest) {
		printf("请先登录。");
		goto END;
	}
	if (openBlogW(&blog, currentuser->userid) < 0) {
		printf("您尚未建立 blog。");
		goto END;
	}
	printBlogHeader(&blog);
	printBlogSettingSideBox(&blog);
	printf("<div id=contents>");

	cmd = getparm("cmd");
	if (!strcmp(cmd, "modify"))
		modifyConfig(&blog, 1);

	printf("<br><b>修改 blog 的设置</b>：<br>"
	       "<form action=blogeditconfig method=post>"
	       "<input type=hidden name=cmd value=modify>");
	printf("标题：<input type=text name=title value=\"%s\">",
	       nohtml(blog.config->title));
	printf("<input type=submit value=\"修改\"></form>");

	closeBlog(&blog);
	printf("</div>");
      END:
	printXBlogEnd();
	return 0;
}
