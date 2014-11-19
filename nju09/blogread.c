#include <time.h>
#include "bbslib.h"
#include "blogdisplayip.h"

int filt_domain(const char* from, char* to);
char* mask_ip(const char* ip_from, char* ip_to);

int
printPrevNext(struct Blog *blog, int n)
{
	if (blog->nIndex == 1)
		return 0;
	printf("<div class=prevnext>");
	if (n < blog->nIndex - 1)
		printf
		    ("<span class=postprev><a href=\"blogread?U=%s&amp;T=%d\">&lt; 上一篇 %s</a></span>",
		     blog->config->useridEN, blog->index[n + 1].fileTime,
		     nohtml(blog->index[n + 1].title));
	if (n)
		printf
		    ("<span class=postnext><a href=\"blogread?U=%s&amp;T=%d\">下一篇 %s &gt;</a></span>",
		     blog->config->useridEN, blog->index[n - 1].fileTime,
		     nohtml(blog->index[n - 1].title));
	printf("</div>");
	return 0;
}

int
printBlogArticle(struct Blog *blog, int n)
{
	struct BlogHeader *blh;
	char filePath[80];
	int i, s, t;
	if (n < 0 || n >= blog->nIndex)
		return -1;
	blh = &blog->index[n];
	setBlogPost(filePath, blog->userid, blh->fileTime);
	printf("<h2 class=posttitle>");
	printf("%s", nohtml(blh->title));
	printf("</h2>");
	printf("<small>%s 发表于 %s</small>", blog->config->useridUTF8,
	       Ctime(blh->fileTime));

	printf("<div class=postcontent>");
	showfile(filePath);
	printf("</div>");

	printf("<div class=postfoot>");
	s = blh->subject;
	printf("栏目：%s", nohtml(blog->subject[s].title));
	printf(" | Tags：");
	for (i = 0; i < MAXTAG; ++i) {
		t = blh->tag[i];
		if (t < 0 || t >= blog->nTag)
			continue;
		if (! *blog->tag[t].title)
			continue;
		printf("%s ", nohtml(blog->tag[t].title));
	}
	printf(" | 评论(%d)", blh->nComment);
	if (!strcmp(currentuser->userid, blog->userid))
		printf(" | <a href=blogpost?T=%d>编辑</a>", blh->fileTime);
	printf("</div>");
	return 0;
}

void
printCommentFile(char *fileName, int commentTime, int i)
{
	struct mmapfile mf = { ptr:NULL };
	char *ptr, buf[256], *comment, *from = "", *who = "", *by = "";
	char time_buf[32] = {0};
	char from_buf[64] = {0};
	char domain_buf[64] = {0};
	char ip_buf[32] = {0};
	char* p = NULL;
	strftime(time_buf, sizeof(time_buf), "%F %T", localtime(&commentTime));

	if (mmapfile(fileName, &mf) < 0)
		return;
	comment = strstr(mf.ptr, "\n\n");
	if (!comment)
		goto END;
	comment += 2;
	buf[0] = '\n';
	strsncpy(buf + 1, mf.ptr, min(sizeof (buf) - 1, comment - mf.ptr));
	//errlog("%s-%s", fileName, buf);
	if ((ptr = strstr(buf, "\nF:"))) {
		from = ptr + 3;
	}
	if ((ptr = strstr(buf, "\nW:"))) {
		who = ptr + 3;
	}
	if ((ptr = strstr(buf, "\nB:"))) {
		by = ptr + 3;
	}
	if ((ptr = strchr(who, '\n')))
		*ptr = 0;
	if ((ptr = strchr(by, '\n')))
		*ptr = 0;
	if ((ptr = strchr(from, '\n')))
		*ptr = 0;

	printf("<div class=\"titleTip\"> <div class=\"name\">");
	if (strchr(by, '@')) {
		printf("<b>%d.</b>糊涂", i + 1);
		p = display_ip((from));
		gb2utf8(from_buf, sizeof (from_buf), p);

		if (filt_domain(from_buf, domain_buf) != 0)
			printf(from_buf);
		else 
			printf(domain_buf);

		free(p);

		// 保持现有的代码兼容
		if (who)
			printf("网友 <span class=\"ip\"> ip %s</span> <a href=\"mailto:%s\">%s</a>", mask_ip(from, ip_buf), by, who);
		else 
			printf("网友 <span class=\"ip\"> ip %s</span>  %s", mask_ip(from, ip_buf), by);
	} else {
		//errlog("=%s=", by);
		ptr = strchr(by, ' ');
		if (ptr) {
			*ptr = 0;
			ptr++;
		} else
			ptr = by;
		printf("<b>%d.</b> <a href=\"qry?U=%s\">%s</a>", i + 1, ptr,
		       by);
	}
	printf("</div><div class=\"title\"> 回复于 %s </div>", time_buf);
	printf("</div>");

	printf("<div class=commenthead>");
	fwrite(comment, mf.size - (comment - mf.ptr), 1, stdout);
	printf("</div>");
      END:
	mmapfile(NULL, &mf);
}

void
printComments(struct Blog *blog, int fileTime)
{
	char buf[120];
	struct mmapfile mf = { ptr:NULL };
	struct BlogCommentHeader *bch;
	int count, i = 0;

	setBlogCommentIndex(buf, blog->userid, fileTime);
	if (mmapfile(buf, &mf) < 0)
		goto END;
	bch = (struct BlogCommentHeader *) mf.ptr;
	count = mf.size / sizeof (*bch);
	printf("<div class=\"endReview\">");
	for (i = 0; i < count; ++i) {
		setBlogComment(buf, blog->userid, fileTime, bch[i].commentTime);
		printCommentFile(buf, bch[i].commentTime, i);
	}
	printf("</div>");
	mmapfile(NULL, &mf);
      END:
	if (!i)
		printf("<div class=commenthead>目前还没有评论</div>");
}

void
printCommentBox(struct Blog *blog, int fileTime)
{
//	if (!loginok || isguest)
//		return;
	printf("<script>\nfunction submitForm() {updateRTE('rte1');"
	       "document.form1.comment.value=document.form1.rte1.value;"
	       "return false;}\ninitRTE('/images/', '/', '/blog.css');</script>\n");
	printf("<br><br>就本文发表评论：<br>");
	printf("<form name=form1 action=blogcomment method=post>");
	printf("<input type=hidden name=U value=\"%s\">",
	       blog->config->useridEN);
	printf("<input type=hidden name=T value=\"%d\">", fileTime);
	if (!loginok || isguest) {
		printf("您的 网名: <input type=text name=who><br />");
		printf("您的 email：<input type=text name=by><br />");
	}
	printf("<div style='position:absolute;visibility:hidden;display:none'>"
	       "<textarea name=comment></textarea></div>");
	printf
	    ("<script>writeRichText('rte1','',0,'300px',true,false);</script>");
	printf
	    ("<input type=submit value=\"确定\" onclick=\"this.value='评论提交中，请稍候...';"
	     "this.disabled=true;submitForm();form1.submit();\"></form>");
}

void
print_add_new_comment(struct Blog *blog, int fileTime)
{
	printf("<div style=\"margin-top:20px;\"> Add your own comment:</div>");
	printf("<form id=\"cform\">");

	printf("<input type=hidden name=U value=\"%s\">",
	       blog->config->useridEN);
	printf("<input type=hidden name=T value=\"%d\">", fileTime);
	if (!loginok || isguest) {
		printf("您的 网名: <input type=text name=who><br />");
		printf("您的 email：<input type=text name=by><br />");
	}
	
	printf("Comment: <textarea name=\"comment\" id=\"comment_text\"></textarea>");
	printf("</form>\n");
	printf("<button onclick=\"addcomment()\">Add Comment </button>");
	
	/*
	printf("<script>\nfunction submitForm() {updateRTE('rte1');"
	       "document.cform.comment.value=document.cform.rte1.value;"
	       "return false;}\ninitRTE('/images/', '/', '/blog.css');</script>\n");
	printf("<br><br>就本文发表评论：<br>");
	printf("<form id=cform name=cform>");
	printf("<input type=hidden name=U value=\"%s\">",
	       blog->config->useridEN);
	printf("<input type=hidden name=T value=\"%d\">", fileTime);
	if (!loginok || isguest) {
		printf("您的 网名: <input type=text name=who><br />");
		printf("您的 email：<input type=text name=by><br />");
	}
	printf("<div style='position:absolute;visibility:hidden;display:none'>"
	       "<textarea name=comment></textarea></div>");
	printf
	    ("<script>writeRichText('rte1','',0,'300px',true,false);</script>");
	printf
	    ("<input type=submit value=\"确定\" onclick=\"this.value='评论提交中，请稍候...';"
	     "this.disabled=true;submitForm();addcomment();\"></form>");
	     */
	printf("<script>"
			"function addcomment()"
			"{"
				"new Ajax.Updater(\'comments\', \'blogcomment\',"
				"{"
					"method: \'post\',"
					"parameters: $(\'cform\').serialize(),"
					"onSuccess: function() {"
						"$(\'comment_text\').value = \'\';"
					"}"
				"} );"
			"}"
			"</script>");

	return;
}

int
blogread_main()
{
	char userid[IDLEN + 1];
	struct Blog blog;
	int n, fileTime;

	printXBlogHeader();
	check_msg();
	changemode(READING);
	strsncpy(userid, getparm2("U", "userid"), sizeof (userid));
	fileTime = atoi(getparm("T"));
	if (openBlog(&blog, userid) < 0) {
		printf("该用户尚未建立 blog。");
		printXBlogEnd();
		return 0;
	}
	printBlogHeader(&blog);
	printBlogSideBox(&blog);
	printf("<div id=contents>");
	n = findBlogArticle(&blog, fileTime);
	if (n < 0) {
		printf("<b>找不到该文章！</b>");
	} else {
		printPrevNext(&blog, n);
		printBlogArticle(&blog, n);
		printf("<a name=comment></a>");
		printf("<div id=\"comments\">");
		printComments(&blog, fileTime);
		printf("</div>");
		//printCommentBox(&blog, fileTime);
		print_add_new_comment(&blog, fileTime);
	}
	printf("</div>");
	printBlogFooter(&blog);
	if (loginok && !isguest) {
		if (strcmp(currentuser->userid, blog.userid))
			blogLog("READ %s %d %s", blog.userid, fileTime,
				currentuser->userid);
	} else {
		//log up to the subnet
		blogLog("Read %s %d %s", blog.userid, fileTime, fromhost);
	}
	closeBlog(&blog);
	printXBlogEnd();
	return 0;
}

// 广东省深圳市南山区电信 => 广东深圳
int
filt_domain(const char* from, char* to)
{
	char* p;
	char* p2;

	if ((p = strstr(from, "省")) != NULL) {
		strncpy(to, from, p - from);
		p += 3;
	} else {
		return -1;
	}

	if ((p2 = strstr(p, "市")) != NULL) {
		strncat(to, p,  p2 - p);
	} else {
		return -1;
	}

	return 0;
}

// 116.24.86.89 => 116.24.*.*
char*
mask_ip(const char* ip_from, char* ip_to)
{
	char* ret = ip_to;
	char* p = NULL;
	char* p2 = NULL;

	if ((p = strchr(ip_from, '.')) != NULL) {
		++p;

		if ((p2 = strchr(p, '.')) != NULL) {
			strncpy(ip_to, ip_from, p2 - ip_from);
			strcat(ip_to, ".*.*");
		}
	}

	return ret;
}
