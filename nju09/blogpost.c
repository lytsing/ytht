#include "bbslib.h"

void
printUploadUTF8()
{
	int i = u_info - &(shm_utmp->uinfo[0]);
	unsigned char buf[4];
	if (!loginok || isguest)
		return;
	myitoa(i, buf, 3);
	printf
	    (" [<a href=\"/cgi-bin/upload/%s%s\" target=uploadytht>添加/删除附件</a>]",
	     buf, u_info->sessionid);
}

void
printSelectSubject(struct Blog *blog, struct BlogHeader *blh)
{
	int i;
	printf("栏目：<select name=subject>");
	for (i = 0; i < blog->nSubject; ++i) {
		char *selected = "";
		if (!*blog->subject[i].title)
			continue;
		if (blh && blh->subject == i)
			selected = " selected";
		printf("<option value=%d%s>%s</option>", i, selected,
		       nohtml(blog->subject[i].title));
	}
	printf("<option value=\"-1\">草稿</option>");
	printf("</select>");
}

void
printSelectTag(struct Blog *blog, struct BlogHeader *blh)
{
	int i;
	printf("<input type=hidden name=tag value=\"\">");
	printf("<nobr>选择 Tags (最多 %d 个)：</nobr>", MAXTAG);
	printf("<table><tr><td>");
	printf
	    ("<select name=taglist1 multiple size=\"%d\" width=\"300\" style=\"width:100px\">", MAXTAG);
	for (i = 0; i < blog->nTag; ++i) {
		if (!*blog->tag[i].title)
			continue;
		if (blh && tagedAs(blh, i))
			printf("<option value=%d selected>%s</option>", i,
			       nohtml(blog->tag[i].title));
		else
			printf("<option value=%d>%s</option>", i,
			       nohtml(blog->tag[i].title));
	}
	printf("</select>");
	printf("</td><td align=center valign=middle>");
	printf("<input type=button value=\"添加>>\" "
	       "onClick=\"movelist(this.form.taglist1,this.form.taglist2,%d,this.form.tag)\"><br>", MAXTAG);
	printf("<input type=button value=\"<<去掉\" "
	       "onClick=\"movelist(this.form.taglist2,this.form.taglist1,0,this.form.tag)\">");
	printf("</td><td>");
	printf
	    ("<select name=taglist2 multiple size=\"%d\" width=\"300\" style=\"width:100px\"></select>", MAXTAG);
	printf("</td></tr></table>");
	printf
	    ("<script>movelist(document.form1.taglist1,document.form1.taglist2,%d,document.form1.tag)</script>", MAXTAG);
}

void
printTextarea(char *text, int size)
{
	int n;
	char buf[256];
	while (size) {
		n = min(sizeof (buf) - 1, size);
		memcpy(buf, text, n);
		buf[n] = 0;
		fputs(nohtml_textarea(buf), stdout);
		text += n;
		size -= n;
	}
}

int
blogpost_main()
{
	struct Blog blog;
	struct BlogHeader *blh = NULL;
	char buf[256], editFile[256];
	int i, draftID = 0, fileTime = 0;
	FILE *fp;

	printXBlogHeader();
	if (!loginok || isguest) {
		printf("请先登录。");
		goto END;
	}
	printf("<script>\nfunction submitForm() {updateRTE('rte1');"
	       "document.form1.content.value=document.form1.rte1.value;"
	       "return false;}\ninitRTE('/images/', '/', '/blog.css');</script>\n");

	if (openBlogD(&blog, currentuser->userid) < 0) {
		printf("您的 blog 尚未建立");
		goto END;
	}

	draftID = atoi(getparm("draftID"));
	fileTime = atoi(getparm("T"));

	if (draftID) {
		for (i = 0; i < blog.nDraft; ++i) {
			if (blog.draft[i].fileTime == draftID) {
				blh = &blog.draft[i];
				sprintf(buf, "D%d", draftID);
				setBlogFile(editFile, blog.userid, buf);
				break;
			}
		}
		if (i >= blog.nDraft)
			draftID = 0;
	} else if (fileTime) {
		i = findBlogArticle(&blog, fileTime);
		if (i < 0)
			fileTime = 0;
		else {
			blh = &blog.index[i];
			sprintf(buf, "%d", fileTime);
			setBlogFile(editFile, blog.userid, buf);
		}
	}

	printBlogHeader(&blog);
	if (fileTime)
		printBlogEditPostSideBox(&blog, fileTime);

	check_msg();
	changemode(POSTING);
	// TODO: make it autosave, just as qq mail
	// add by deli
	printf("<div class=\"tips\">");
	printf("贴心小提示：<br />");
	printf("写blog要养成的好习惯,请先在记事本上编辑好了再拷贝粘贴提交，以防网络断线,网站服务器运行不正常等 . </div>" );

	printf("<div id=contents><div class=swidth>");

	printf("<form name=form1 method=post action=blogsend>");
	printf
	    ("标题：<input type=text name=title size=50 maxlength=50 value=\"%s\"><br />",
	     blh ? nohtml(blh->title) : "");
	printSelectSubject(&blog, blh);
	printf("<br />");
	printSelectTag(&blog, blh);
	printf("<div style='position:absolute;visibility:hidden;display:none'>"
	       "<textarea name=content>");
	fp = fopen(editFile, "r");
	if (!fp) {
		draftID = 0;
		fileTime = 0;
	} else {
		while (fgets(buf, sizeof (buf), fp)) {
			printf("%s", nohtml_textarea(buf));
		}
		fclose(fp);
	}
	printf("</textarea></div>");
	if (draftID) {
		printf("<input type=hidden name=draftID value=\"%d\">",
		       draftID);
	} else if (fileTime) {
		printf("<input type=hidden name=T value=\"%d\">",
		       fileTime);
	}
	printf
	    ("<script>writeRichText('rte1',document.form1.content.value,'100%%','500px',true,false);</script>");
	printf
	    ("<table width=100%%><tr><td align=center>"
	     "<input type=submit value=发表 onclick=\"this.value='文章提交中，请稍候...';"
	     "this.disabled=true;submitForm();form1.submit();\"> </td><td align=center>"
	     "<input type=reset value=清除 onclick='return confirm(\"确定要全部清除吗?\")'>"
	     "</td></tr></table>");
	printf("</form>");

	printf("</div></div>");
	closeBlog(&blog);
      END:
	printXBlogEnd();

	return 0;
}

