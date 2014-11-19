#include "bbslib.h"

int
printDraftAbstract(struct BlogHeader *blh)
{
	printf("\n<div class=line></div>");
	printf("<div class=\"content%d\">", contentbg);
	printf("<b>%s</b> ", nohtml(blh->title));
	printf("<font size=-1>%s</font>", Ctime(blh->fileTime));
	printf
	    ("<div class=scontent%d style=\"text-align:right\"><font size=\"-1\">",
	     contentbg);
	printf("<div class=wline2></div>\n");
	printf("<a href=\"blogdraftread?T=%d\">阅读全文</a>",
	       (int) blh->fileTime);
	printf(" │ <a href=\"blogpost?draftID=%d\">编辑</a>",
	       (int) blh->fileTime);
	printf("</font></div>");
	changeContentbg();
	return 0;
}

int
blogdraft_main()
{
	struct Blog blog;
	int i;

	printXBlogHeader();
	check_msg();
	changemode(READING);
	
	if(!loginok||isguest) {
		printf("请先登录。");
		printXBlogEnd();
		return 0;
	}
	if (openBlogD(&blog, currentuser->userid) < 0) {
		printf("无法打开文件");
		printXBlogEnd();
		return 0;
	}

	printBlogHeader(&blog);
	printBlogSideBox(&blog);
	printf("<div id=contents>");
	for (i = blog.nDraft - 1; i >= 0; --i) {
		printDraftAbstract(&blog.draft[i]);
	}
	printf("<div class=line></div>");
	printf("</div>");
	printBlogFooter(&blog);
	closeBlog(&blog);
	printXBlogEnd();
	return 0;
}
