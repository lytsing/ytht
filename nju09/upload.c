//ylsdd Nov 05, 2002
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include "bbs.h"
#include "ythtlib.h"

struct bbsinfo bbsinfo;
static struct UTMPFILE *shm_utmp;
static struct WWWCACHE *wwwcache;
static char *FileName;		/* The filename, as selected by the user. */
static char *ContentStart;	/* Pointer to the file content. */
static int ContentLength;	/* Bytecount of the content. */
char userattachpath[256];
int attachtotalsize;

static char *
getsenv(char *s)
{
	char *t = getenv(s);
	if (t)
		return t;
	return "";
}

static char *
getreqstr()
{
	static char str[100] = { 0 }, *ptr, *end;
	if (str[0])
		return str;
	ptr = getenv("SCRIPT_URL");
	if (!ptr) {
		ptr = getenv("REQUEST_URI");
		if (ptr) {
			if ((end = strchr(ptr, '?')))
				*end = 0;
		}
		setenv("SCRIPT_URL", ptr, 0);
	}
	strsncpy(str, getsenv("SCRIPT_URL"), sizeof (str));
	if ((ptr = strchr(str, '&')))
		*ptr = 0;
	return str;
}

int
fixfilename(char *str)
{
	if (!str[0] || !strcmp(str, ".") || !strcmp(str, ".."))
		return -1;
	while (*str) {
		if ((*str > 0 && *str < ' ') || isspace(*str)
		    || strchr("\\/~`!@#$%^&*()|{}[];:\"'<>,?", *str)) {
			*str = '_';
		}
		str++;
	}
	return 0;
}

int
getpathsize(char *path, int showlist)
{
	DIR *pdir;
	struct dirent *pdent;
	char fname[1024];
	int totalsize = 0, size;
	if (showlist)
		printf("已经上载的附件有:<br>");
	pdir = opendir(path);
	if (!pdir)
		return -1;
	while ((pdent = readdir(pdir))) {
		if (!strcmp(pdent->d_name, "..") || !strcmp(pdent->d_name, "."))
			continue;
		if (strlen(pdent->d_name) + strlen(path) >= sizeof (fname)) {
			totalsize = -1;
			break;
		}
		sprintf(fname, "%s/%s", path, pdent->d_name);
		size = file_size(fname);
		if (showlist) {
			printf("<li> %s (<i>%d字节</i>) ", pdent->d_name, size);
			printf("<a href='%s&%s'>删除</a>",
			       getreqstr(), pdent->d_name);
		}
		if (size < 0) {
			totalsize = -1;
			break;
		}
		totalsize += size;
	}
	closedir(pdir);
	if (showlist) {
		printf("<br>大小总计 %d 字节 (最大 %d 字节)<br>", totalsize,
		       MAXATTACHSIZE);
	}
	return totalsize;
}

static void
html_header()
{
#ifndef USEBIG5
	printf("Content-type: text/html; charset=gb2312\r\n\r\n");
#else
	printf("Content-type: text/html; charset=big5\r\n\r\n");
#endif
	printf("<HTML><head></head><body bgcolor=#f0f4f0>\n");
}

static void
http_fatal(char *str)
{
	printf("错误! %s!<br>", str);
	printf("<a href=javascript:history.go(-1)>快速返回</a></body></html>");
	exit(0);
}

/* Skip a line in the input stream. */
static void
SkipLine(char **Input,		/* Pointer into the incoming stream. */
	 int *InputLength)
{				/* Bytes left in the incoming stream. */

	while ((**Input != '\0') && (**Input != '\r')
	       && (**Input != '\n')) {
		*Input = *Input + 1;
		*InputLength = *InputLength - 1;
	}
	if (**Input == '\r') {
		*Input = *Input + 1;
		*InputLength = *InputLength - 1;
	}
	if (**Input == '\n') {
		*Input = *Input + 1;
		*InputLength = *InputLength - 1;
	}
}

static void
GoAhead(char **Input,		/* Pointer into the incoming stream. */
	int *InputLength, int len)
{
	*Input += len;
	*InputLength -= len;
}

/* Accept a single segment from the incoming mime stream. Each field in the
   form will generate a mime segment. Return a pointer to the beginning of
   the Boundary, or NULL if the stream is exhausted. */
static void
AcceptSegment(char **Input,	/* Pointer into the incoming stream. */
	      int *InputLength,	/* Bytes left in the incoming stream. */
	      char *Boundary	/* Character string that delimits segments. */
    )
{
	char *FieldName;	/* Name of the variable from the form. */
	char *ContentEnd;
	char *contentstr, *ptr;
	/* The input stream should begin with a Boundary line. Error-exit if not
	   found. */
	if (strncmp(*Input, Boundary, strlen(Boundary)) != 0)
		http_fatal("文件传送错误 10");
	/* Skip the Boundary line. */
	GoAhead(Input, InputLength, strlen(Boundary));
	SkipLine(Input, InputLength);
	/* Return NULL if the stream is exhausted (no more segments). */
	if ((**Input == '\0') || (strncmp(*Input, "--", 2) == 0))
		http_fatal("文件传送错误 11");
	/* The first line of a segment must be a "Content-Disposition" line. It
	   contains the fieldname, and optionally the original filename. Error-exit
	   if the line is not recognised. */
	contentstr = "content-disposition: form-data; name=\"";
	if (strncasecmp(*Input, contentstr, strlen(contentstr)))
		http_fatal("文件传送错误 12");
	GoAhead(Input, InputLength, strlen(contentstr));
	FieldName = *Input;
	ptr = strchr(*Input, '\"');
	if (!ptr)
		http_fatal("文件传送错误 13");
	*ptr = 0;
	ptr++;
	while (*ptr && !isalpha(*ptr))
		ptr++;
	GoAhead(Input, InputLength, ptr - *Input);
	if (strncasecmp(*Input, "filename=\"", strlen("filename=\"")))
		http_fatal("文件传送错误 14");
	GoAhead(Input, InputLength, strlen("filename=\""));
	FileName = *Input;
	ptr = strchr(*Input, '\"');
	if (!ptr)
		http_fatal("文件传送错误 15");
	*ptr = 0;
	ptr++;
	GoAhead(Input, InputLength, ptr - *Input);
	/* Skip the Disposition line and one or more mime lines, until an empty
	   line is found. */
	SkipLine(Input, InputLength);
	while ((**Input != '\r') && (**Input != '\n'))
		SkipLine(Input, InputLength);
	SkipLine(Input, InputLength);
	/* The following data in the stream is binary. The Boundary string is the
	   end of the data. There may be a CRLF just before the Boundary, which
	   must be stripped. */
	ContentStart = *Input;
	ContentLength = 0;
	while (*InputLength > 0 && memcmp(*Input, Boundary, strlen(Boundary))) {
		GoAhead(Input, InputLength, 1);
		ContentLength++;
	}
	ContentEnd = *Input - 1;
	if ((ContentLength > 0) && (*ContentEnd == '\n')) {
		ContentEnd--;
		ContentLength--;
	}
	if ((ContentLength > 0) && (*ContentEnd == '\r')) {
		ContentEnd--;
		ContentLength--;
	}
}

int
save_attach()
{
	char *ptr, *p0, filename[1024];
	FILE *fp;
	p0 = FileName;
	ptr = strrchr(p0, '/');
	if (ptr) {
		*ptr = 0;
		ptr++;
	} else
		ptr = p0;
	p0 = ptr;
	ptr = strrchr(p0, '\\');
	if (ptr) {
		*ptr = 0;
		ptr++;
	} else
		ptr = p0;
	p0 = ptr;
	if (strlen(p0) >= 40) {
		ptr = strrchr(p0, '.');
		if (ptr) {
			int len = strlen(ptr);
			if (len > 6) {
				ptr[6] = 0;
				len = 6;
			}
			memmove(p0 + 40 - len, ptr, len + 1);
		} else
			p0[40] = 0;
	}
	if (fixfilename(p0))
		http_fatal("无效的文件名");
	sprintf(filename, "%s/%s", userattachpath, p0);
	mkdir(userattachpath, 0760);
	fp = fopen(filename, "w");
	fwrite(ContentStart, 1, ContentLength, fp);
	fclose(fp);
	printf("文件 %s (%d字节) 上载成功<br>", p0, ContentLength);
	return 0;
}

void
do_del()
{
	char str[1024], *p0, *ptr;
	char filename[1024];
	strsncpy(str, getsenv("PATH_INFO"), sizeof (str));
	if (!(p0 = strchr(str, '&')))
		return;
	p0++;
	while ((ptr = strsep(&p0, "&"))) {
		if (strlen(ptr) >= 40)
			ptr[40] = 0;
		if (fixfilename(ptr))
			http_fatal
			    ("无效的文件名，请勿包含特殊字符(问号括号空格等)");
		sprintf(filename, "%s/%s", userattachpath, ptr);
		if (unlink(filename) < 0) {
			printf("删除文件失败，“%s”“%s”<br>\n", ptr,
			       getsenv("PATH_INFO"));
		}
	}
}

void
printuploadform()
{
	char *req = getreqstr();
	printf("<hr>"
	       "<form name=frmUpload action='%s' enctype='multipart/form-data' method=post>"
	       "上载附件: <input type=file name=file>"
	       "<input type=submit value=上载 "
	       "onclick=\"this.value='附件上载中，请稍候...';this.disabled=true;frmUpload.submit();\">"
	       "</form> "
	       "在这里可以为文章附加点小图片小程序啥的, 不要贴太大的东西哦, 文件名里面也不要有括号问号什么的, 否则会粘贴失败哦.<br>"
	       "<b>可以在文章中任意定位附件</b>，只需要在文章编辑框中预期位置上顶头写上“#attach 1.jpg”就可以了(别忘了将 1.jpg 换成所上载的文件名 :) )"
	       "<center><input type=button value='刷新' onclick=\"location='%s';\">&nbsp; &nbsp;"
	       "<input type=button value='完成' onclick='window.close();'></center>",
	       req, req);
}

int
main(int argc, char *argv[], char *environment[])
{
	char *ptr, *buf;
	char utmpnstr[4];
	char Boundary[1024] = "--";
	int len, i, via_proxy = 0;
	struct user_info uin;
	char str[100];
	char fromhost[256];
	struct in_addr from_addr;
	struct rlimit rl;
	rl.rlim_cur = 100 * 1024 * 1024;
	rl.rlim_max = 100 * 1024 * 1024;
	setrlimit(RLIMIT_CORE, &rl);

	html_header();
	seteuid(BBSUID);
	if (geteuid() != BBSUID)
		http_fatal("内部错误 0");
	if (initbbsinfo(&bbsinfo) < 0)
		http_fatal("内部错误 1");

	shm_utmp = bbsinfo.utmpshm;
	wwwcache = bbsinfo.wwwcache;

	//Should use SCRIPT_URL instead of PATH_INFO, since in  PATH_INFO '+' will be converted to ' '
	ptr = strrchr(getreqstr(), '/');
	if (!ptr)
		http_fatal("请先登录 0");
	strsncpy(str, ptr, sizeof (str));
	//strsncpy(str, getsenv("PATH_INFO"), sizeof (str));
	if ((ptr = strchr(str, '&')))
		*ptr = 0;
	if (strlen(str) != 34) {
		//errlog("==%s==%s==%s==", uin.sessionid, str+4,  getsenv("PATH_INFO"));
		http_fatal("请先登录 1");
	}
	strsncpy(utmpnstr, str + 1, 4);
	i = myatoi(utmpnstr, 3);
	if (i < 0 || i > USHM_SIZE)
		http_fatal("请先登录 2");
	uin = shm_utmp->uinfo[i];
	strsncpy(fromhost, getsenv("REMOTE_ADDR"), 32);
	inet_aton(fromhost, &from_addr);
	for (i = 0; wwwcache->validproxy[i] && i < MAX_PROXY_NUM; i++) {
		if (from_addr.s_addr == wwwcache->validproxy[i]) {
			via_proxy = 1;
			break;
		}
	}
	if (via_proxy || from_addr.s_addr == wwwcache->accel_addr.s_addr) {
		char *ptr, *p;
		ptr = getenv("HTTP_X_FORWARDED_FOR");
		if (!ptr)
			ptr = getsenv("REMOTE_ADDR");
		p = strrchr(ptr, ',');
		if (p != NULL) {
			while (!isdigit(*p) && *p)
				p++;
			if (*p)
				strsncpy(fromhost, p, sizeof (fromhost));
			else
				strsncpy(fromhost, ptr, sizeof (fromhost));
		} else
			strsncpy(fromhost, ptr, sizeof (fromhost));
		inet_aton(fromhost, &from_addr);
	}
	if (!uin.active || strcmp(uin.sessionid, str + 4)) {
		//errlog("==%s==%s==%s==", uin.sessionid, str+4,  getsenv("PATH_INFO"));
		http_fatal("请先登录 3");
	}
	if (uin.wwwinfo.ipmask) {
		if ((uin.fromIP << uin.wwwinfo.ipmask) !=
		    (from_addr.s_addr << uin.wwwinfo.ipmask)) {
			http_fatal("请先登录 3.2");
		}
	} else if (uin.fromIP != from_addr.s_addr) {
		http_fatal("请先登录 3.3");
	}
	if (!(uin.userlevel & PERM_POST))
		http_fatal("缺乏 POST 权限");
	snprintf(userattachpath, sizeof (userattachpath), PATHUSERATTACH "/%s",
		 uin.userid);
	mkdir(userattachpath, 0760);
	//clearpath(userattachpath);
	/* Test if the program was started by a METHOD=POST form. */
	if (strcasecmp(getsenv("REQUEST_METHOD"), "post")) {
		do_del();
		attachtotalsize = getpathsize(userattachpath, 1);
		if (attachtotalsize < MAXATTACHSIZE)
			printuploadform();
		printf("</body></html>");
		return 0;
	}
	attachtotalsize = getpathsize(userattachpath, 0);
	if (attachtotalsize < 0)
		http_fatal("无法检测目录大小");
	/* Test if the program was started with ENCTYPE="multipart/form-data". */
	ptr = getsenv("CONTENT_TYPE");
	if (strncasecmp(ptr, "multipart/form-data; boundary=", 30))
		http_fatal("文件传送错误 2");
	/* Determine the Boundary, the string that separates the segments in the
	   stream. The boundary is available from the CONTENT_TYPE environment
	   variable. */
	ptr = strchr(ptr, '=');
	if (!ptr)
		http_fatal("文件传送错误 3");
	ptr++;
	strsncpy(Boundary + 2, ptr, sizeof (Boundary) - 2);
	/* Get the total number of bytes in the input stream from the
	   CONTENT_LENGTH environment variable. */
	len = atoi(getsenv("CONTENT_LENGTH"));
	if (len <= 0 || len > 2000000)
		http_fatal("文件传送错误 4");
	buf = malloc(len + 1);
	if (!buf)
		http_fatal("文件传送错误 5");
	len = fread(buf, 1, len, stdin);
	buf[len] = 0;
	ptr = buf;
	AcceptSegment(&ptr, &len, Boundary);
	if (ContentLength + attachtotalsize > MAXATTACHSIZE) {
		free(buf);
		http_fatal("附件太大, 超过限额");
	}
	save_attach();
	/* Cleanup. */
	free(buf);
	attachtotalsize = getpathsize(userattachpath, 1);
	if (attachtotalsize < MAXATTACHSIZE)
		printuploadform();
	return (0);
}
