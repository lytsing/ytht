/*
 * xlat   %ds:(%ebx)
 * out    %al, (%dx)
 * mov    %0xae, %al
 * shld   0xd0d5(%eax)
 *     -- by TripleX
 */
#include "bbslib.h"
#include <iconv.h>

#define HOT_LINE_MAXLEN 200
#define HOT_LINE_MAXNUM 120

#define RSS_TITLE_LEN    120
#define RSS_AUTHOR_LEN   16
#define RSS_BOARD_LEN    32
#define RSS_DESC_LEN     256
#define RSS_MAX_THREAD	10

/* 评分等级　*/
const char *score_str =
    "\xe8\xaf\x84\xe5\x88\x86\xe7\xad\x89\xe7\xba\xa7\x0a\x00";
/* 作者　*/
const char *author_str = "\xe4\xbd\x9c\xe8\x80\x85\x0a\x00";

typedef struct _hot_article {
	int score;
	int count;
	int thread;
	char title[RSS_TITLE_LEN];
	char author[RSS_AUTHOR_LEN];
	char board[RSS_BOARD_LEN];
	//      char description[RSS_DESC_LEN];
} hot_article;

static int hot_count = 0;
hot_article hot_table[HOT_LINE_MAXNUM];

char *
nonohtml(const char *s)
{
	char buf[1024];
	strsncpy(buf, nohtml(s), sizeof(buf));
	return nohtml(buf);
}
	
static void
bbsrss_header(char *encode)
{
	printf("Content-type: text/xml\r\n\r\n");
	printf("<?xml version=\"1.0\" encoding=\"%s\"?>\n", encode);
	printf
	    ("<!DOCTYPE rss [<!ENTITY %% HTMLlat1 PUBLIC \"-//W3C//ENTITIES Latin 1 for XHTML//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml-lat1.ent\">]>\n");
}

static int
bbsrss_filter_key(char **inbuf, int *inbytesleft, char **outbuf,
		  int *outbytesleft)
{
	while ((*inbytesleft) > 0) {
		if (!(*outbytesleft > 0))
			break;
		if (*(*inbuf) == '<') {
			if ((*outbytesleft) >= 8) {
				memcpy((*outbuf), "&amp;lt;", 8);
				(*outbytesleft) -= 8;
				(*outbuf) += 8;
				(*inbytesleft) -= 1;
				(*inbuf)++;
				continue;
			} else {
				break;
			}
		} else if (*(*inbuf) == '>') {
			if ((*outbytesleft) >= 8) {
				memcpy((*outbuf), "&amp;gt;", 8);
				(*outbytesleft) -= 8;
				(*outbuf) += 8;
				(*inbytesleft) -= 1;
				(*inbuf)++;
				continue;
			} else {
				break;
			}
		} else if (*(*inbuf) == '&') {
			if ((*outbytesleft) >= 5) {
				memcpy((*outbuf), "&amp;amp;", 9);
				(*outbytesleft) -= 5;
				(*outbuf) += 5;
				(*inbytesleft) -= 1;
				(*inbuf)++;
				continue;
			} else {
				break;
			}
		} else {
			if ((*outbytesleft) >= 1) {
				**outbuf = **inbuf;
				(*outbytesleft) -= 1;
				(*outbuf)++;
				(*inbytesleft) -= 1;
				(*inbuf)++;
				continue;
			} else {
				break;
			}
		}
	}
	return 0;
}

static void
bbsrss_parse_hot(iconv_t cd, char *buf)
{
	int len;
	char *ptr, *next;
	char *inbuf, *outbuf;
	size_t inbytesleft, outbytesleft;
	hot_article *hot;
	char title_buf[RSS_TITLE_LEN / 2];

	hot = &hot_table[hot_count];
	ptr = buf;
	next = strchr(ptr, ' ');
	if (NULL == next)
		return;
	*next = '\0';
	hot->score = *ptr - '0';	//ascii only

	ptr = next + 1;
	next = strchr(ptr, ' ');
	if (NULL == next)
		return;
	*next = '\0';
	hot->count = atoi(ptr);

	ptr = next + 1;
	next = strchr(ptr, ' ');
	if (NULL == next)
		return;
	*next = '\0';
	len = strlen(ptr);
	if (len >= RSS_BOARD_LEN)
		return;
	memcpy(hot->board, ptr, len + 1);

	ptr = next + 1;
	next = strchr(ptr, ' ');
	if (NULL == next)
		return;
	*next = '\0';
	len = strlen(ptr);
	if (len >= RSS_AUTHOR_LEN)
		return;
	inbuf = ptr;
	outbuf = hot->author;
	inbytesleft = len;
	outbytesleft = RSS_AUTHOR_LEN - 1;
	iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	*outbuf = '\0';

	ptr = next + 1;
	next = strchr(ptr, ' ');
	if (NULL == next)
		return;
	*next = '\0';
	hot->thread = atoi(ptr);

	ptr = next + 1;
	/*next = strchr(ptr, ' ');
	   if(NULL == next)
	   return;
	   *next = '\0'; */
	strtok(ptr, "\n");
	len = strlen(ptr);
	if (len >= RSS_TITLE_LEN / 2)
		return;
	inbuf = ptr;
	outbuf = title_buf;
	inbytesleft = len;
	outbytesleft = sizeof (title_buf) - 1;
	bbsrss_filter_key(&inbuf, &inbytesleft, &outbuf, &outbytesleft);
	*outbuf = '\0';

	//      ptr = title_buf;
	inbuf = title_buf;
	outbuf = hot->title;
	inbytesleft = sizeof (title_buf) - 1;
	outbytesleft = RSS_TITLE_LEN - 1;
	iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
	*outbuf = '\0';
	hot_count++;
}

static void
rss_head(char *title, char *link, char *description)
{
	printf("<rss version=\"0.91\">\n");
	printf("<channel>\n");
	printf(" <title>Technology! Thought! Truth! - %s</title>\n", title);
	printf(" <link>http://%s</link>\n", link);
	printf(" <description>%s</description>\n", description);
	printf(" <language>zh-cn</language>\n");
}

static void
rss_end(void)
{
	printf("</channel>\n");
	printf("</rss>\n");
}

static void
hot_body(void)
{
	int i;
	hot_article *hot;
	rss_head(MY_BBS_NAME " - 热门话题", MY_BBS_DOMAIN "/", MY_BBS_NAME " BBS");
	for (i = 0; i < hot_count; i++) {
		hot = &hot_table[i];
		printf(" <item>\n");
		printf("  <title>%s</title>\n", hot->title);
		printf("  <link>http://" MY_BBS_DOMAIN "/" SMAGIC
		       "/bbsnt?B=%d&amp;th=%d</link>\n", getbnum(hot->board),
		       hot->thread);
		/*
		printf
		    ("  <description>%s  %d/%d &lt;BR&gt; %s  %s</description>\n",
		     score_str, hot->score, hot->count, author_str,
		     hot->author);
		*/
		printf
		    ("  <description>评分等级  %d/%d &lt;br&gt; 发布者  %s</description>\n",
		     hot->score, hot->count, hot->author);
		printf(" </item>\n");
	}
	rss_end();
	return;
}

static int
bbsrss_showhot()
{
	static time_t last_access_time;
	time_t ft;
	FILE *fp;
	char buf[HOT_LINE_MAXLEN];
	iconv_t cd;

	ft = file_time("wwwtmp/navpart.txt");
	if (ft > last_access_time) {
		fp = fopen("wwwtmp/navpart.txt", "r");
		if (fp == NULL)
			return -1;
		hot_count = 0;
		//cd = iconv_open("UTF-8", "GBK");
		cd = iconv_open("GBK", "GBK");
		if (cd < 0)
			return -1;
		while (hot_count < HOT_LINE_MAXNUM
		       && fgets(buf, sizeof (buf), fp) != NULL) {
			bbsrss_parse_hot(cd, buf);
		}
		iconv_close(cd);
		fclose(fp);
		last_access_time = ft;
	}
	hot_body();
	return 0;
}

static int
listallchannel(void)
{
	struct boardmem *x;
	int i;
	rss_head(MY_BBS_NAME " - 看版列表", MY_BBS_DOMAIN "/", MY_BBS_NAME " BBS");
	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
		x = &(shm_bcache->bcache[i]);
		if (x->header.filename[0] <= 32 || x->header.filename[0] > 'z')
			continue;
		if (x->header.flag & CLOSECLUB_FLAG)
			continue;
		if (x->header.level
		    && !(x->header.level & (PERM_POSTMASK | PERM_NOZAP)))
			continue;
		printf("<item>\n");
		printf("<title>%s</title>\n", x->header.filename);
		printf("<link>http://" MY_BBS_DOMAIN "/" SMAGIC
		       "/doc?B=%d</link>\n", getbnumx(x));
		printf("<description>%s</description>\n", x->header.filename);
		printf("</item>\n");
	}
	rss_end();
	return 0;
}

static int
get_thread_id(char *board, struct fileheader *tdocdata)
{
	char buf[80];
	struct fileheader *data;
	int total = 0, sum = 0, i;
	struct mmapfile mf = { ptr:NULL };
	snprintf(buf, sizeof (buf), "boards/%s/.DIR", board);
	MMAP_TRY {
		if (mmapfile(buf, &mf) < 0) {
			MMAP_UNTRY;
			return -1;
		}
		data = (void *) mf.ptr;
		total = mf.size / sizeof (struct fileheader);
		if (total <= 0) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			return 0;
		}
		for (i = total - 1; i >= 0 && i < total; i--) {
			if (data[i].thread != data[i].filetime)
				continue;
			memcpy(&(tdocdata[sum]), &(data[i]),
			       sizeof (struct fileheader));
			sum++;
			if (sum >= RSS_MAX_THREAD)
				break;
		}
	}
	MMAP_CATCH {
		sum = 0;
	}
	MMAP_END mmapfile(NULL, &mf);
	return sum;
}

static int
get_single_thread_id(char *board, int thread, struct fileheader *tdocdata)
{
	char buf[80];
	struct fileheader *x;
	int total = 0, start, i, b, num;
	struct mmapfile mf = { ptr:NULL };
	snprintf(buf, sizeof (buf), "boards/%s/.DIR", board);

	MMAP_TRY {
		if (mmapfile(buf, &mf) == -1) {
			MMAP_UNTRY;
			return -1;
		}
		total = mf.size / sizeof (struct fileheader);
		x = (struct fileheader *) mf.ptr;
		start = Search_Bin((struct fileheader *)mf.ptr, thread, 0, total - 1);
		if (start < 0) {
			b = i = 0;
			start = -(start + 1);
		} else {
			b = i = 1;
			memcpy(tdocdata, x + start, sizeof (struct fileheader));
		}
		for (num = total - 1; i < RSS_MAX_THREAD && num >= start; num--) {
			if (x[num].thread != thread)
				continue;
			memcpy(tdocdata + RSS_MAX_THREAD - 1 - i, x + num,
			       sizeof (struct fileheader));
			i++;
		}
		memmove(tdocdata + b, tdocdata + RSS_MAX_THREAD - i,
			sizeof (struct fileheader) * (i - b));
	}
	MMAP_CATCH {
		i = 0;
	}
	MMAP_END mmapfile(NULL, &mf);
	return i;
}

static int
listchannel(char *board)
{
	int i, count;
	struct fileheader *tmp;
	char titlebuf[STRLEN], buf[STRLEN];
	struct mmapfile mf = { ptr:NULL };

	tmp = malloc(sizeof (struct fileheader) * RSS_MAX_THREAD);
	if (NULL == tmp)
		return -1;
	count = get_thread_id(board, tmp);
	if (count < 0) {
		free(tmp);
		return -1;
	}

	snprintf(titlebuf, sizeof(titlebuf), MY_BBS_NAME " - %s", board);
	snprintf(buf, sizeof (buf), MY_BBS_DOMAIN "/" SMAGIC "/doc?B=%d",
		 getbnum(board));
	rss_head(titlebuf, buf, board);

	for (i = 0; i < count; i++) {
		printf("<item>\n");
		printf("<title>%s</title>\n", nonohtml(tmp[i].title));
		printf("<link>http://" MY_BBS_DOMAIN "/" SMAGIC
		       "/con?B=%d&amp;F=%s&amp;T=%d</link>\n", getbnum(board),
		       fh2fname(tmp + i),
		       tmp[i].edittime ? (tmp[i].edittime -
					  tmp[i].filetime) : 0);
		printf("<description>\n");
		printf
		    ("[发表时间]:%s [贴子长度]:%d字节 [精彩度]:%d [推荐人数]:%d  [正文]: ",
		     Ctime(tmp[i].filetime), bytenum(tmp[i].sizebyte),
		     tmp[i].staravg50 / 50, tmp[i].hasvoted);
		snprintf(buf, sizeof (buf), "boards/%s/%s", board,
			 fh2fname(tmp + i));
		if (!mmapfile(buf, &mf)) {
			//fwrite(mf.ptr, mf.size, 1, stdout);
			mmapfile(NULL, &mf);
		}
		printf("\n</description>\n");
		printf("<author>%s</author>\n", fh2owner(tmp + i));	//作者
		printf("<source>%s</source>\n", board);	//所属版面名称
		//printf("<guid>%d</guid>\n", tmp[i].filetime);
		printf("</item>\n");
	}
	free(tmp);
	rss_end();
	return 0;
}

static int
listchannel_thread(char *board, int thread)
{
	int i, count;
	struct fileheader *tmp;
	char buf[STRLEN];
	struct mmapfile mf = { ptr:NULL };

	tmp = malloc(sizeof (struct fileheader) * RSS_MAX_THREAD);
	if (NULL == tmp)
		return -1;
	count = get_single_thread_id(board, thread, tmp);
	if (count < 0) {
		free(tmp);
		return -1;
	}
	snprintf(buf, sizeof (buf), MY_BBS_DOMAIN "/" SMAGIC "/doc?B=%d",
		 getbnum(board));
	rss_head(board, buf, board);

	for (i = 0; i < count; i++) {
		printf("<item>\n");
		if (i)
			printf("<title>Re:%d-%s</title>\n", i, nonohtml(tmp[i].title));
		else
			printf("<title>%s</title>\n", nonohtml(tmp[i].title));
		printf("<link>http://" MY_BBS_DOMAIN "/" SMAGIC
		       "/con?B=%d&amp;F=%s&amp;T=%d</link>\n", getbnum(board),
		       fh2fname(tmp + i),
		       tmp[i].edittime ? (tmp[i].edittime -
					  tmp[i].filetime) : 0);
		printf("<description>\n");
		if (i)
			printf("[回]");
		printf
		    ("[发表时间]:%s [贴子长度]:%d字节 [精彩度]:%d [推荐人数]:%d  [正文]: ",
		     Ctime(tmp[i].filetime), bytenum(tmp[i].sizebyte),
		     tmp[i].staravg50 / 50, tmp[i].hasvoted);
		snprintf(buf, sizeof (buf), "boards/%s/%s", board,
			 fh2fname(tmp + i));
		if (!mmapfile(buf, &mf)) {
			fwrite(mf.ptr, mf.size, 1, stdout);
			mmapfile(NULL, &mf);
		}
		printf("\n</description>\n");
		printf("<author>%s</author>\n", fh2owner(tmp + i));	//作者
		printf("<source>%s</source>\n", board);	//所属版面名称
		//printf("<guid>%d</guid>\n", tmp[i].filetime);
		printf("</item>\n");
	}
	free(tmp);
	rss_end();
	return 0;
}

int
bbsrss_main(void)
{
	char rssid[32];
	struct boardmem *x;
	int thread;

	strsncpy(rssid, getparm("rssid"), sizeof (rssid));

	if (strcmp(rssid, "hot") == 0) {
		//bbsrss_header("UTF-8");
		bbsrss_header("GBK");
		bbsrss_showhot();
		return 0;
	} else if (strcmp(rssid, "all") == 0) {
		bbsrss_header("GBK");
		listallchannel();
		return 0;
	} else if ((x = getboard(rssid))) {
		bbsrss_header("GBK");
		thread = atoi(getparm("thread"));
		if (thread)
			listchannel_thread(x->header.filename, thread);
		else
			listchannel(x->header.filename);
		return 0;
	}

	http_fatal("oh, we did not support this feature.....\n");
	return 0;
}
