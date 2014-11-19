#include "bbslib.h"

int
printDraftArticle(struct Blog *blog, int n)
{
	struct BlogHeader *blh;
	char fileName[20], filePath[80];
	if (n < 0 || n >= blog->nDraft)
		return -1;
	blh = &blog->draft[n];
	sprintf(fileName, "D%d", (int) blh->fileTime);
	setBlogFile(filePath, blog->userid, fileName);
	printf("<div class=\"line\"></div>");
	printf("<div class=\"content%d\">", contentbg);
	printf("<b>%s</b><br />", nohtml(blh->title));
	showfile(filePath);
	printf("<br /><a href=blogpost?draftID=%d>编辑</a>", blh->fileTime);
	printf(" <a href=blogdraftdelete?T=%d>删除</a>", blh->fileTime);
	printf("</div>");
	return 0;
}

int
blogdraftread_main()
{
	struct Blog blog;
	int i, fileTime;

	printXBlogHeader();
	check_msg();
	changemode(READING);
	fileTime = atoi(getparm("T"));
	if(!loginok||isguest) {
		printf("请先登录");
		printXBlogEnd();
		return 0;
	}
	if (openBlogD(&blog, currentuser->userid) < 0) {
		printf("您尚未建立 blog。");
		printXBlogEnd();
		return 0;
	}
	printBlogHeader(&blog);
	printBlogSideBox(&blog);
	for (i = 0; i < blog.nDraft; ++i) {
		if (blog.draft[i].fileTime == fileTime)
			break;
	}
	printf("草稿");
	printf("<div id=contents>");
	if (i >= blog.nDraft) {
		printf("<b>找不到该文章！</b>");
	} else {
		printDraftArticle(&blog, i);
	}
	printf("</div>");
	printBlogFooter(&blog);
	closeBlog(&blog);
	printXBlogEnd();
	return 0;
}
