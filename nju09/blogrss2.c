#include "bbslib.h"

char *
gmtctime(const time_t * timep)
{
	static char buf[80];
	strftime(buf, sizeof (buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(timep));
	return buf;
}

static void
printItem(struct Blog *blog, int i)
{
	struct BlogHeader *blh;
	char buf[256];
	blh = &blog->index[i];
	printf("<item>");
	printf("<title>%s</title>", nohtml(blh->title));
	printf("<author>%s</author>", blog->config->useridUTF8);
	printf("<pubDate>%s</pubDate>", gmtctime(&blh->fileTime));
	printf("<link>http://" MY_BBS_DOMAIN "/" SMAGIC
	       "/blogread?U=%s&amp;T=%d</link>", blog->config->useridEN,
	       (int) blh->fileTime);
	printf("<guid>http://" MY_BBS_DOMAIN "/" SMAGIC
	       "/blogread?U=%s&amp;T=%d</guid>", blog->config->useridEN,
	       (int) blh->fileTime);
	for (i = 0; i < blog->nTag; ++i) {
		if (!*blog->tag[i].title || !blog->tag[i].count
		    || blog->tag[i].hide)
			continue;
		printf("<category>%s</category>", nohtml(blog->tag[i].title));
	}

	printf("<description><![CDATA[");
	if (blh->hasAbstract) {
		setBlogAbstract(buf, blog->userid, blh->fileTime);
		showfile(buf);
		printf("<br /><a href=\"http://" MY_BBS_DOMAIN "/" SMAGIC
		       "/blogread?U=%s&T=%d\">(继续阅读...)</a>",
		       blog->config->useridEN, (int) blh->fileTime);
	} else {
		setBlogPost(buf, blog->userid, blh->fileTime);
		showfile(buf);
	}
	printf("]]></description>");
	printf("</item>");
}

int
blogrss2_main()
{
	char userid[IDLEN + 1];
	struct Blog blog;
	int n, i;

	//errlog("HTTP_IF_MODIFIED_SINCE: %s", getsenv("HTTP_IF_MODIFIED_SINCE"));
	strsncpy(userid, getparm2("U", "userid"), sizeof (userid));
	if (openBlog(&blog, userid) < 0) {
		printf("Content-type: text/xml; charset=utf-8\r\n\r\n");
		printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	       		"<rss version=\"2.0\"><channel>");
		printf("<title>该 Blog 尚不存在</title>");
		printf("<link>http://" MY_BBS_DOMAIN "</link>");
		printf
		    ("<description>该用户不存在，或者尚未建立 Blog。</description>");
		goto END;
	}
	if (cache_header(blog.indexFile.mtime, 3600*2)) {
		//errlog("OK?");
		return 0;
	}
	printf("Content-type: text/xml; charset=utf-8\r\n\r\n");
	printf("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	       "<rss version=\"2.0\"><channel>");
	printf("<title>%s</title>", nohtml(blog.config->title));
	printf("<link>http://" MY_BBS_DOMAIN "/" SMAGIC "/blog?U=%s</link>",
	       blog.config->useridEN);
	printf("<description>%s 的 Blog。</description>",
	       blog.config->useridUTF8);
	//ttl...
	printf("<lastBuildDate>%s</lastBuildDate>",
	       gmtctime(&blog.indexFile.mtime));
	for (i = blog.nIndex - 1, n = 0; i >= 0 && n < 30; --i) {
		if (now_t - blog.index[i].fileTime > 7 * 24 * 3600 && n > 10)
			break;
		printItem(&blog, i);
	}

	closeBlog(&blog);
      END:
	printf("</channel></rss>");
	return 0;
}
