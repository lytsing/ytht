#include "bbslib.h"

static void
modifySubjects(struct Blog *blog)
{
	int i, hide, count = 0;
	char *title, buf[80];
	printf("修改标题...<br>");
	for (i = 0; i < blog->nSubject; ++i) {
		sprintf(buf, "title%d", i);
		title = getparm(buf);
		sprintf(buf, "hide%d", i);
		if (*getparm(buf))
			hide = 1;
		else
			hide = 0;
		if (!strcmp(title, blog->subject[i].title) &&
		    blog->subject[i].hide == hide)
			continue;
		blogModifySubject(blog, i, title, hide);
		printf("编号: %d，标题：%s，隐藏：%s<br>",
		       i, title, hide ? "是" : "否");
		count++;
	}
	if (!count)
		printf("没有进行任何修改！<br>");
	printf("<br>");
}

static void
addSubject(struct Blog *blog)
{
	int i, hide;
	char *title;
	printf("添加栏目...");
	title = getparm("title");
	if (!*title) {
		printf("没写栏目标题啊？<br>");
		return;
	}
	if (*getparm("hide"))
		hide = 1;
	else
		hide = 0;
	for (i = 0; i < blog->nSubject; i++) {
		if (!*blog->subject[i].title)
			break;
	}
	if (blogModifySubject(blog, i, title, hide) >= 0) {
		printf("编号：%d，标题：%s；隐藏：%s<br><br>",
		       i, title, hide ? "是" : "否");
	} else {
		printf("没有添加成功...<br><br>");
	}
}

static void
deleteSubject(struct Blog *blog)
{
	int i;
	char *id = getparm("subjectID");
	if (!*id)
		return;
	i = atoi(id);
	if (i < 0 || i >= blog->nSubject)
		return;
	if (blog->subject[i].count)
		return;
	*blog->subject[i].title = 0;
}

int
blogeditsubject_main()
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
		printf("您尚未建立 blog。");
		goto END;
	}
	printBlogHeader(&blog);
	printBlogSettingSideBox(&blog);
	printf("<div id=contents>");

	cmd = getparm("cmd");
	if (!strcmp(cmd, "modify"))
		modifySubjects(&blog);
	else if (!strcmp(cmd, "add"))
		addSubject(&blog);
	else if (!strcmp(cmd, "delete"))
		deleteSubject(&blog);

	printf("<br><b>修改现有栏目</b>：<br>"
	       "<form action=blogeditsubject method=post>"
	       "<input type=hidden name=cmd value=modify><table border=1>");
	printf
	    ("<tr><td>编号</td><td>标题</td><td>&nbsp;</td><td>文章数</td><td></td></tr>");
	for (i = 0; i < blog.nSubject; i++) {
		if (!*blog.subject[i].title && !blog.subject[i].count)
			continue;
		printf("<tr>");
		printf("<td>%d</td>", i);
		printf
		    ("<td><input name=\"title%d\" type=text value=\"%s\" /></td>",
		     i, nohtml(blog.subject[i].title));
		printf
		    ("<td><input name=\"hide%d\" type=checkbox%s> 隐藏</td>",
		     i, blog.subject[i].hide ? " checked" : "");
		printf("<td>%d 篇</td><td>&nbsp;", blog.subject[i].count);
		if (!blog.subject[i].count) {
			printf
			    ("<a href=\"blogeditsubject?cmd=delete&amp;subjectID=%d\">删除</a>",
			     i);
		}
		printf("</td></tr>");
	}
	closeBlog(&blog);

	printf("</table>");
	printf("<input type=submit value=\"修改\"></form>");

	printf("<br><br><b>添加新栏目</b>：<br>"
	       "<form action=blogeditsubject method=post>"
	       "<input type=hidden name=cmd value=add>");
	printf("标题：<input name=title><br>");
	printf("<input name=hide type=checkbox> 隐藏该栏目<br>");
	printf("<input type=submit value=\"添加\">");
	printf("</form>");
	printf("</div>");
      END:
	printXBlogEnd();
	return 0;
}
