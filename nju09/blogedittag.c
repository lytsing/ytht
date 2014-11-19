#include "bbslib.h"

static void
modifyTags(struct Blog *blog)
{
	int i, hide, count = 0;
	char *title, buf[80];
	printf("修改标题...<br>");
	for (i = 0; i < blog->nTag; ++i) {
		sprintf(buf, "title%d", i);
		title = getparm(buf);
		sprintf(buf, "hide%d", i);
		if (*getparm(buf))
			hide = 1;
		else
			hide = 0;
		if (!strcmp(title, blog->tag[i].title) &&
		    blog->tag[i].hide == hide)
			continue;
		blogModifyTag(blog, i, title, hide);
		printf("编号: %d，Tag：%s，隐藏：%s<br>",
		       i, title, hide ? "是" : "否");
		count++;
	}
	if (!count)
		printf("没有进行任何修改！<br>");
	printf("<br>");
}

static void
addTag(struct Blog *blog)
{
	int i, hide;
	char *title;
	printf("添加 Tag ...");
	title = getparm("title");
	if (!*title) {
		printf("这个 Tag 叫什么？<br>");
		return;
	}
	if (*getparm("hide"))
		hide = 1;
	else
		hide = 0;
	for (i = 0; i < blog->nTag; i++) {
		if (!*blog->tag[i].title)
			break;
	}
	if (blogModifyTag(blog, i, title, hide) >= 0) {
		printf("编号：%d，Tag：%s；隐藏：%s<br><br>",
		       i, title, hide ? "是" : "否");
	} else {
		printf("没有添加成功...<br><br>");
	}
}

static void
deleteTag(struct Blog *blog)
{
	int i;
	char *id = getparm("tagID");
	if (!*id)
		return;
	i = atoi(id);
	if (i < 0 || i >= blog->nTag)
		return;
	if (blog->tag[i].count)
		return;
	*blog->tag[i].title = 0;
}

int
blogedittag_main()
{
	struct Blog blog;
	int i;
	char *cmd;

	printXBlogHeader();
	check_msg();
	changemode(READING);
	if (!loginok || isguest) {
		printf("请先登录。");
		goto END;
	}
	if (openBlogW(&blog, currentuser->userid) < 0) {
		printf("您尚未建立。");
		goto END;
	}

	printBlogHeader(&blog);
	printBlogSettingSideBox(&blog);
	printf("<div id=contents>");

	cmd = getparm("cmd");
	if (!strcmp(cmd, "modify"))
		modifyTags(&blog);
	else if (!strcmp(cmd, "add"))
		addTag(&blog);
	else if (!strcmp(cmd, "delete"))
		deleteTag(&blog);

	printf("<br><b>修改现有 Tag</b>：<br>"
	       "<form action=blogedittag method=post>"
	       "<input type=hidden name=cmd value=modify><table border=1>");
	printf
	    ("<tr><td>编号</td><td>Tag</td><td>&nbsp;</td><td>文章数</td><td></td></tr>");
	for (i = 0; i < blog.nTag; ++i) {
		if (!*blog.tag[i].title && !blog.tag[i].count)
			continue;
		printf("<tr>");
		printf("<td>%d</td>", i);
		printf
		    ("<td><input name=\"title%d\" type=text value=\"%s\" /></td>",
		     i, nohtml(blog.tag[i].title));
		printf
		    ("<td><input name=\"hide%d\" type=checkbox%s> 隐藏</td>",
		     i, blog.tag[i].hide ? " checked" : "");
		printf("<td>%d 篇</td><td>&nbsp;", blog.tag[i].count);
		if (!blog.tag[i].count) {
			printf
			    ("<a href=\"blogedittag?cmd=delete&amp;tagID=%d\">删除</a>",
			     i);
		}
		printf("</td></tr>");
	}
	closeBlog(&blog);
	printf("</table>");
	printf("<input type=submit value=\"修改\"></form>");

	printf("<br><br><b>添加新 Tag</b>：<br>"
	       "<form action=blogedittag method=post>"
	       "<input type=hidden name=cmd value=add>");
	printf("Tag：<input name=title><br>");
	printf("<input name=hide type=checkbox> 隐藏该 Tag<br>");
	printf("<input type=submit value=\"添加\">");
	printf("</form>");
	printf("</div>");
      END:
	printXBlogEnd();
	return 0;
}
