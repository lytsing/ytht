/*
 * Filename: blogedipost.c
 * Description: for management the blog articles, pack up, delete...
 * @author deli
 * @date 2008.12.14
 * $id$
 */

#include "bbslib.h"

static void
packing_blog(struct Blog *blog)
{
	printf("<b>打包我的Blog</b><br />");
	printf("[tar.gz]格式 [chm]格式<br />");
}

static void
printPostTitleList(struct Blog *blog)
{
	struct BlogHeader* blh;
	//char buf[80] = {0};
	int i;

	printf("<b>文章列表</b><br>");
	printf("<div class=post>");
	printf("文章总数: %d 篇<br >", blog->nIndex);

	for (i = blog->nIndex; i > 0; --i) {
		blh = &blog->index[i-1];
		printf("<li>");
		printf("<a href=\"blogread?U=%s&amp;T=%d\">%s</a>",
			blog->config->useridEN, (int)blh->fileTime, nohtml(blh->title));
		printf("</li>");
	}

	printf("</div>");
}

int
blogeditpost_main(void)
{
	struct Blog blog;
/*    int i;*/
/*    char *cmd;*/

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
	packing_blog(&blog);
	printPostTitleList(&blog);

	closeBlog(&blog);
	printf("</div>");

END:
	printXBlogEnd();

	return 0;
}

