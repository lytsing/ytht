#include "bbslib.h"

void
printBlogEditPostSideBox(struct Blog *blog, time_t fileTime)
{
	printf("<div id=sidebox>");
	printf("<h3>编辑本文的</h3><ul>");
	printf
	    ("<li><a href=\"blogpost?T=%d\">标题、分类、正文</a></li>",
	     (int) fileTime);
	printf("<li><a href=\"blogeditabstract?T=%d\">摘要</a></li>",
	       (int) fileTime);
	printf("</ul>");
	printf("</div>");
}

void
printAbstractBox(struct Blog *blog, int n)
{
	struct BlogHeader *blh = &blog->index[n];
	printf("<script>\nfunction submitForm() {updateRTE('rte1');"
	       "document.form1.aBstract.value=document.form1.rte1.value;"
	       "return false;}\ninitRTE('/images/', '/', '/blog.css');</script>\n");
	printf("<br />编辑本文的摘要：");
	if (blh->hasAbstract)
		printf
		    ("<br>(如果不想要摘要，将摘要内容删除，然后提交即可)");
	printf("<form name=form1 action=blogeditabstract method=post>");
	printf("<input type=hidden name=T value=\"%d\">", blh->fileTime);
	printf("<input type=hidden name=doit value=\"1\">");
	printf
	    ("<div style='position:absolute;visibility:hidden;display:none'>");
	printf("<textarea name=aBstract>");	//"abstract" is a key word of javascript
	if (blh->hasAbstract) {
		FILE *fp;
		char buf[256];
		setBlogAbstract(buf, blog->userid, blh->fileTime);
		fp = fopen(buf, "r");
		if (fp) {
			while (fgets(buf, sizeof (buf), fp)) {
				printf("%s", nohtml_textarea(buf));
			}
			fclose(fp);
		}
	}
	printf("</textarea></div>");
	printf
	    ("<script>writeRichText('rte1',document.form1.aBstract.value,0,'200px',true,false);</script>");
	printf
	    ("<input type=submit value=\"确定\" onclick=\"this.value='摘要提交中，请稍候...';"
	     "this.disabled=true;submitForm();form1.submit();\"></form>");
}

int
doSaveAbstract(struct Blog *blog, time_t fileTime, char *abstract)
{
	char filename[80], *ptr;
	sprintf(filename, "bbstmpfs/tmp/%d.tmp", thispid);
	saveTidyHtml(filename, abstract, 1);
	if (file_size(filename) <= 6) {
		blogSaveAbstract(blog, fileTime, NULL);
	} else {
		ptr = readFile(filename);
		if (ptr) {
			blogSaveAbstract(blog, fileTime, ptr);
			free(ptr);
		}
	}
	unlink(filename);
	return 0;
}

int
blogeditabstract_main()
{
	struct Blog blog;
	int n, size;
	time_t fileTime;

	printXBlogHeader();

	if (!loginok || isguest) {
		printf("您尚未登录。");
		printXBlogEnd();
		return 0;
	}

	fileTime = atoi(getparm("T"));
	if (atoi(getparm("doit"))) {
		char buf[128];
		if (openBlogW(&blog, currentuser->userid) >= 0) {
			doSaveAbstract(&blog, fileTime, getparm("abstract"));
			snprintf(buf, sizeof (buf), "blogread?U=%s&T=%d",
				 blog.config->useridEN, (int) fileTime);
			redirect(buf);
			closeBlog(&blog);
		}
		printXBlogEnd();
		return 0;
	}

	if (openBlog(&blog, currentuser->userid) < 0) {
		printf("您尚未建立 blog。");
		printXBlogEnd();
		return 0;
	}

	check_msg();
	printBlogHeader(&blog);
	printBlogEditPostSideBox(&blog, fileTime);
	n = findBlogArticle(&blog, fileTime);
	if (n < 0) {
		printf("<b>找不到该文章！</b>");
		printXBlogEnd();
		closeBlog(&blog);
		return 0;
	}
	printf("<div id=contents>");
	if ((size = atoi(getparm("s")))) {
		printf("<div>文章已经保存到您的 blog。<br>"
		       "这篇文章长达 %d 字节，建议写一短摘要，"
		       "这样文章列表显示比较美观。</div>", size);
	}
	printAbstractBox(&blog, n);
	printf("<hr>");
	printBlogArticle(&blog, n);
	printf("</div>");
	closeBlog(&blog);
	printXBlogEnd();
	return 0;
}
