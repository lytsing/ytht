#include "bbslib.h"

static char *
atomctime(const time_t * timep)
{
	static char buf[80];
	strftime(buf, sizeof (buf), "%Y-%m-%dT%H:%M:%SZ", gmtime(timep));
	return buf;
}

static char *
makeTag(struct Blog *blog, const time_t * timep)
{
	static char buf[128];
	struct tm *tm;
	if (timep) {
		tm = gmtime(timep);
		snprintf(buf, sizeof (buf), "tag:%s,%d-%02d-%02d:/%s/%ld",
			 MY_BBS_DOMAIN, tm->tm_year + 1900, tm->tm_mon + 1,
			 tm->tm_mday, blog->config->useridEN, (long) *timep);
	} else {
		tm = gmtime(&blog->config->createTime);
		snprintf(buf, sizeof (buf), "tag:%s,%d-%02d-%02d:/%s",
			 MY_BBS_DOMAIN, tm->tm_year + 1900, tm->tm_mon + 1,
			 tm->tm_mday, blog->config->useridEN);
	}
	return buf;
}

static void
printEntry(struct Blog *blog, int i)
{
	struct BlogHeader *blh;
	char buf[256];
	blh = &blog->index[i];
	printf("<entry>");
	printf("<title>%s</title>", nohtml(blh->title));
	printf("<link href=\"blogread?U=%s&amp;T=%d\" />",
	       blog->config->useridEN, (int) blh->fileTime);
	printf("<id>%s</id>", makeTag(blog, &blh->fileTime));
	printf("<updated>%s</updated>", atomctime(&blh->fileTime));

	//printf("<author><name>%s</name></author>", blog->config->useridUTF8);
	//printf("<issued>%s</issued>", gmtctime(&blh->fileTime));
	/*
	   for (i = 0; i < blog->nTag; i++) {
	   if (!*blog->tag[i].title || !blog->tag[i].count
	   || blog->tag[i].hide)
	   continue;
	   printf("<category>%s</category>", nohtml(blog->tag[i].title));
	   } */

	printf("<summary type=\"html\"><![CDATA[");
	if (blh->hasAbstract) {
		setBlogAbstract(buf, blog->userid, blh->fileTime);
		showfile(buf);
		printf
		    ("<br /><a href=\"blogread?U=%s&amp;T=%d\">(继续阅读...)</a>",
		     blog->config->useridEN, (int) blh->fileTime);
	} else {
		setBlogPost(buf, blog->userid, blh->fileTime);
		showfile(buf);
	}
	printf("]]></summary>");
	printf("</entry>");
}

int
blogatom_main()
{
	char userid[IDLEN + 1];
	struct Blog blog;
	int n, i;

	printf("Content-type: text/xml; charset=utf-8\r\n\r\n");
	printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	       "<feed xmlns=\"http://www.w3.org/2005/Atom\" xml:lang=\"zh-CN\">");

	strsncpy(userid, getparm2("U", "userid"), sizeof (userid));
	if (openBlog(&blog, userid) < 0) {
		printf("<title>该 Blog 尚不存在</title>");
		printf("<link href=\"http://" MY_BBS_DOMAIN "\" />");
		goto END;
	}
	printf("<id>%s</id>", makeTag(&blog, NULL));
	printf("<title>%s</title>", nohtml(blog.config->title));
	printf("<link href=\"blog?U=%s\" />", blog.config->useridEN);
	printf("<link rel=\"self\" href=\"blogatom?U=%s\" />",
	       blog.config->useridEN);
	printf("<author><name>%s</name></author>", blog.config->useridUTF8);
	printf("<updated>%s</updated>", atomctime(&blog.indexFile.mtime));
	//tagline...
	//id...
	//generator...

	for (i = blog.nIndex - 1, n = 0; i >= 0 && n < 30; i--) {
		if (now_t - blog.index[i].fileTime > 7 * 24 * 3600 && n > 10)
			break;
		printEntry(&blog, i);
	}

	closeBlog(&blog);
      END:
	printf("</feed>");
	return 0;
}
