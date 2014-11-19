#include "bbslib.h"
extern char *cginame;
int stripHeadTail = 0;
int scriped = 0;

static const int img_fmt_png[148] = {
	0x474e5089, 0x0a1a0a0d, 0x0d000000, 0x52444849, 0xc8000000, 0x14000000,
	0x00000408, 0xbaad6200, 0x02000041, 0x41444917, 0xedda7854, 0xc392d958,
	0x3b430c20, 0xd65ffffd, 0xf821353e, 0xa7da5392, 0x19db33b0, 0x2d921cc0,
	0xb4f18c9b, 0x676d3da7, 0xc9946363, 0x45d83f38, 0x96d7739f, 0xbfef0ac2,
	0x497b67ad, 0x0cdc208b, 0x021c205f, 0x7ee7284c, 0xd1b119db, 0x1213e009,
	0x5c01935d, 0x201d0e16, 0xae76eb4e, 0xb089cb86, 0x77345c05, 0x35011fbf,
	0xd5210bbe, 0x59e86961, 0xe2a13a41, 0x0d938332, 0x4361c392, 0x8dc0f524,
	0x81a72f5c, 0x9905625b, 0xe24cd564, 0x66c73be8, 0xb02d9d6f, 0x4c76cfd8,
	0x53a58724, 0x7a74a360, 0xc6e98c67, 0x57706756, 0xa447b979, 0x05cad381,
	0x5c8ecb43, 0x5aa18031, 0x08f33787, 0xb4d1e21d, 0xd30f6ef4, 0xa0ec50c3,
	0x0cd73522, 0xe199034c, 0x906a6905, 0xe0c2256b, 0xa5a7b67f, 0x7463a992,
	0x79852b12, 0x47354daa, 0xea4dd7c1, 0x213d166f, 0x55848980, 0x980ae842,
	0xadbffda2, 0xc0313a98, 0x62548f25, 0x8d404519, 0x67592baf, 0x152165d2,
	0x8b519cd7, 0x29d8530e, 0x4735fc85, 0x217c2ced, 0xbccd0db8, 0xf8c7b43b,
	0xcaac842e, 0xb5cb838c, 0xf960c25e, 0x5712e062, 0xb44f2b6f, 0xad820120,
	0xbb56aee5, 0xe100155d, 0x9877d053, 0xf0dbe321, 0x783a2e50, 0xbd7832fe,
	0xd1a042aa, 0x63f08f19, 0x78d6583a, 0x5ce2470c, 0xdc15eca1, 0xcc0a4fe2,
	0x625c6e13, 0xdf659703, 0x38fc43ac, 0x3c4e7481, 0x5315248a, 0x207e2594,
	0x9b3e2c68, 0xa294e728, 0xfcd5c19c, 0x83b53a7e, 0x6516008d, 0x74d2a527,
	0xeb7a7b38, 0xa642f853, 0x98c53c79, 0xa834ab62, 0x64220df1, 0xe76211a4,
	0x55d15641, 0x8d3c6199, 0xa4c4d252, 0x0d3abdc2, 0xeba43647, 0xa6e83cc7,
	0x1cf22f9c, 0x5f29aad4, 0xf947850d, 0xe129cdbe, 0xe6286624, 0x5b2e898c,
	0xa214c1c1, 0x229123ca, 0x0315a361, 0x0deb0ef9, 0x1b19f55a, 0x8a5ceafd,
	0xd4ede957, 0xaf77f55b, 0x4d82dc95, 0xb4f69fdc, 0x0ff699a7, 0x9062fe18,
	0x656d662d, 0x00000000, 0x444e4549, 0x826042ae
};

void
resizecachename(char *buf, size_t len, char *fname, int ver, int pos)
{
	char *ptr;
	if (ver > 0)
		snprintf(buf, len, "szmcache/%s%d.%d", fname, pos, ver);
	else
		snprintf(buf, len, "szmcache/%s%d", fname, pos);
	ptr = strchr(buf, '/') + 1;
	while ((ptr = strchr(ptr, '/'))) {
		*ptr = '_';
	}
}

int
showresizecache(char *resizefn)
{
      struct mmapfile mf = { ptr:NULL };
	MMAP_TRY {
		if (mmapfile(resizefn, &mf) < 0) {
			MMAP_UNTRY;
			MMAP_RETURN(-1);
		}
		printf("Content-type: image/jpeg\r\n");
		printf("Content-Length: %d\r\n\r\n", mf.size);
		fwrite(mf.ptr, 1, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	return 0;
}

int
showbinaryattach(char *filename)
{
	char *attachname;
	int pos;
	int small;
	int ver = 0;
	int fd;
	unsigned int size;
      struct mmapfile mf = { ptr:NULL };
	char resizefn[256];

//      no_outcache();
	if (cache_header(file_time(filename), 864000))
		return 0;
	attachname = getparm("attachname");
	pos = atoi(getparm("attachpos"));
	small = atoi(getparm("S"));
	ver = atoi(getparm("T"));
	if (small) {
		resizecachename(resizefn, sizeof (resizefn), filename, ver,
				pos);
		if (showresizecache(resizefn) == 0) {
			return 0;
		}
	}
	MMAP_TRY {
		if (mmapfile(filename, &mf) < 0) {
			MMAP_UNTRY;
			http_fatal("无法打开附件 1");
			MMAP_RETURN(-1);
		}
		if (pos >= mf.size - 4 || pos < 1) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("无法打开附件 2");
			MMAP_RETURN(-1);
		}
		if (mf.ptr[pos - 1] != 0) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("无法打开附件 3");
			MMAP_RETURN(-1);
		}
		size = ntohl(*(unsigned int *) (mf.ptr + pos));
		if (pos + 4 + size > mf.size) {
			mmapfile(NULL, &mf);
			MMAP_UNTRY;
			http_fatal("无法打开附件 4");
			MMAP_RETURN(-1);
		}
		while (small) {
			int ret, t_size;
			void *out;
			if (memcmp(mf.ptr + pos + 4, "BM", 2))
				ret =
				    szm_box_jpg(tjpg_ctx, mf.ptr + pos + 4,
						size, 480, &out, &t_size);
			else
				ret =
				    szm_box_bmp(tjpg_ctx, mf.ptr + pos + 4,
						size, 480, &out, &t_size);
			switch (ret) {
			case -SZM_ERR_CONNECT:
			case -SZM_ERR_BROKEN:
				printf("Status: 404\r\n\r\n");
				break;
			case 0:
				printf("Content-type: image/jpeg\r\n");
				printf("Content-Length: %d\r\n\r\n", t_size);
				fwrite(out, 1, t_size, stdout);
				fd = open(resizefn, O_WRONLY | O_CREAT, 0600);
				if (fd >= 0) {
					write(fd, out, t_size);
					close(fd);
				}
				free(out);
				break;
			default:
				printf("Content-type: image/png\r\n\r\n");
				fwrite((void *) img_fmt_png, 1,
				       sizeof (img_fmt_png), stdout);
				fd = open(resizefn, O_WRONLY | O_CREAT, 0600);
				if (fd >= 0) {
					write(fd, img_fmt_png,
					      sizeof (img_fmt_png));
					close(fd);
				}
				break;
			}
			break;
		}
		if (!small) {
			printf("Content-type: %s\r\n",
			       get_mime_type(attachname));
			printf("Content-Length: %d\r\n\r\n", size);
			fwrite(mf.ptr + pos + 4, 1, size, stdout);
		}
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	return 0;
}

char *
binarylinkfile(char *f)
{
	static char *lf = "";
	if (f)
		lf = f;
	return lf;
}

void
fprintbinaryattachlink(FILE * fp, int ano, char *attachname, int pos, int size)
{
	char *ext, link[256], slink[256], *ptr, board[40],
	    *urlencoded_attachname;
	int pic = 0;
	int atthttp = 0;
	struct boardmem *brd;
	int resize = 0;

	void1(attachname);
	urlencoded_attachname = urlencode(attachname);
	//check read_perm for guest, refer to has_read_perm()
	getparmboard(board, sizeof (board));
	brd = getboard(board);
	if (brd && !(brd->header.flag & CLOSECLUB_FLAG) && !brd->header.level) {
		ptr = "/" BASESMAGIC "/";
		if (w_info->att_mode == 0 && !via_proxy)
			atthttp = 1;
	} else {
		ptr = "";
	}
	if (!wwwcache->accel_addr.s_addr || !wwwcache->accel_port)
		atthttp = 0;

	if ((ext = strrchr(attachname, '.')) != NULL) {
		if (!strcasecmp(ext, ".bmp") || !strcasecmp(ext, ".jpg")
		    || !strcasecmp(ext, ".gif") || !strcasecmp(ext, ".jpeg")
		    || !strcasecmp(ext, ".png") || !strcasecmp(ext, ".pcx"))
			pic = 1;
		else if (!strcasecmp(ext, ".swf"))
			pic = 2;
		else if (!strcasecmp(ext, ".tiff") || !strcasecmp(ext, ".tif"))
			pic = 3;
		else if (!strcasecmp(ext, ".wmv"))
			pic = 4;
		else if (!strcasecmp(ext, ".mpg") || !strcasecmp(ext, ".mpeg"))
			pic = 5;
		else if (!strcasecmp(ext, ".mp3") || !strcasecmp(ext, ".wav"))
			pic = 6;
		else
			pic = 0;
	}

	if (tjpg_ctx && 1 == pic
	    && (!strcasecmp(ext, ".bmp") || !strcasecmp(ext, ".jpg")
		|| !strcasecmp(ext, ".jpeg"))
	    && size > 70000)
		resize = 1;

	if (!strcmp(cginame, "bbscon") || !strcmp(cginame, "bbsnt")
	    || !strcmp(cginame, "bbstcon")) {
		//同上
		if (!atthttp) {
			if (!*ptr)
				resize = 0;
#if 0
			//use bbsnewattach
			snprintf(link, sizeof (link), "%sb/%d/%s/%d/%s", ptr,
				 getbnumx(brd), binarylinkfile(NULL), pos,
				 attachname);
			snprintf(link, sizeof (link), "%sb/%d/%s/%d/%s", ptr,
				 getbnumx(brd), binarylinkfile(NULL), pos,
				 urlencoded_attachname);
#else
			//Or use bbsattach, if want to resize pictures
			snprintf(link, sizeof (link),
				 "%sattach/bbscon/%s?B=%d&amp;F=%s&amp;attachpos=%d&amp;attachname=/%s",
				 ptr, urlencoded_attachname, getbnumx(brd),
				 binarylinkfile(NULL), pos,
				 urlencoded_attachname);
#endif
		} else if (wwwcache->accel_addr.s_addr && wwwcache->accel_port) {
			snprintf(link, sizeof (link),
				 "http://%s:%d%sattach/bbscon/%s?B=%d&amp;F=%s&amp;attachpos=%d&amp;attachname=/%s",
				 inet_ntoa(wwwcache->accel_addr),
				 wwwcache->accel_port,
				 ptr, urlencoded_attachname, getbnumx(brd),
				 binarylinkfile(NULL), pos,
				 urlencoded_attachname);
			//} else {
			//      snprintf(link, sizeof (link),
			//               "http://%s:8080/%s/%s/%d/%s", MY_BBS_IP,
			//               board, binarylinkfile(), pos, attachname);
			//      resize = 0;
		}
	} else {
		snprintf(link, sizeof (link),
			 "attach/%s/%s?%s&amp;attachpos=%d&amp;attachname=/%s",
			 cginame, urlencoded_attachname,
			 getsenv("QUERY_STRING"), pos, urlencoded_attachname);
		resize = 0;
	}

	while (resize) {
		char *t1, *t2;
		if (wwwcache->text_accel_addr.s_addr
		    && wwwcache->text_accel_port) {
			t1 = strstr(link, BASESMAGIC);
			if (!t1)
				break;
			snprintf(slink, sizeof (slink), "http://%s:%d/%s",
				 inet_ntoa(wwwcache->text_accel_addr),
				 wwwcache->text_accel_port, t1);
		} else
			strcpy(slink, link);
		t1 = strchr(slink, '?');
		t2 = strchr(link, '?');
		if (!t1 || !t2)
			break;
		sprintf(t1 + 1, "S=1&amp;%s", t2 + 1);
		break;
	}

	if (1 == pic && !resize)
		strcpy(slink, link);

	switch (pic) {
	case 1:
		fprintf(fp, "%d 附图: %s (%s%d 字节%s)<br>"
			"<a href='%s' target='_blank'>"
			"<img src='%s' border='0' alt='按此在新窗口浏览图片' "
			"onload='javascript:if(this.width>document.body.clientWidth-30)this.width=document.body.clientWidth-30' "
			"endimg></a>", ano, attachname,
			resize ? "原始图大小 " : "", size,
			resize ? "，点击看大图" : "", link, slink);
		break;
	case 2:
		fprintf(fp,
			"%d Flash动画: "
			"<a href='%s'>%s</a> (%d 字节)<br>"
			"<OBJECT width=560 height=480><PARAM NAME='MOVIE' VALUE='%s'>"
			"<EMBED width=560 height=480 SRC='%s'></EMBED></OBJECT>",
			ano, link, attachname, size, link, link);
		break;
	case 3:
		fprintf(fp, "%d 附图: <a href='%s'>%s</a> (%d 字节)<br>"
			"<object width=560 height=480 classid='CLSID:106E49CF-797A-11D2-81A2-00E02C015623'>"
			"<param name='src' value='%s'>"
			"<param name='negative' value='yes'>"
			"<embed src='%s' type='image/tiff' negative='yes' width=560 height=480>"
			"</boject>", ano, link, attachname, size, link, link);
		break;
	case 4:
		fprintf(fp,
			"%d 视频: "
			"<a href='%s'>%s</a> (%d 字节)<br>"
			"<embed width=560 height=480 src='%s%s'></embed>",
			ano, link, attachname, size, strncmp(link, "http://",
							     7) ? "http://"
			MY_BBS_DOMAIN "/" : "", link);
		break;
	case 5:
		fprintf(fp, "%d 视频: "
			"<a href='%s'>%s</a> (%d 字节)<br>"
			"<object classid='CLSID:05589FA1-C356-11CE-BF01-00AA0055595A'>"
			"<param name='Filename' value='%s'>"
			"<param name='AutoStart' value='-1'>"
			//"<param name='Appearance' value='0'>"
			"<embed src='%s%s' type='application/x-mplayer2' width=560 height=480></embed>"
			"</object>", ano, link, attachname, size, link,
			strncmp(link, "http://",
				7) ? "http://" MY_BBS_DOMAIN "/" : "", link);
		break;
	case 6:
		fprintf(fp, "%d 音频: "
			"<a href='%s'>%s</a> (%d 字节)<br>"
			"<object width=300 height=42>"
			"<param name=src value='%s'>"
			"<param name=autoplay value=true>"
			"<param name=controller value=true>"
			"<embed src='%s' autostart=true loop=false width=300 height=42 controller=true>"
			"</embed></object>", ano, link, attachname, size, link,
			link);
		break;
	default:
		fprintf(fp,
			"%d 附件: <a href='%s'>%s</a> (%d 字节)",
			ano, link, attachname, size);
	}
}

int
ttid(int i)
{
	static int n = 0;
	n += i;
	return n % 2000 + now_t;
}

char *
memStripHeadTail(char *ptr00, int *len)
{
	char *ptr, *ptr0;
	ptr0 = ptr00;
	ptr = memmem(ptr0, *len, "\n\n", 2);
	if (!ptr)
		return ptr0;
	*len -= (ptr - ptr0) + 2;
	ptr0 = ptr + 2;

	while (*len > 0 && (*ptr0 == '\n' || *ptr0 == '\r')) {
		(*len)--;
		ptr0++;
	}

	for (ptr = ptr0 + *len - 5; ptr > ptr0; ptr--) {
		if (*ptr == '\n')
			break;
	}
	if (ptr > ptr0) {
		if (memmem(ptr, *len - (ptr - ptr0), "※ 来源", 7))
			*len = (ptr - ptr0);
	} else {
		scriped = ptr0 - ptr00;
		return ptr0;
	}
	if (*len > 3 && ptr[-1] == '-' && ptr[-2] == '-' && ptr[-3] == '\n')
		*len -= 3;
	scriped = ptr0 - ptr00;
	return ptr0;
}

/* show_iframe: 0  Show "<div id=tt*></div>, and show the content as script.
		1  Show "<div id=tt*></div>, and the link to the script
		2  only show the content of the file as script
		3  show the content as pure html
 */

int
fshowcon(FILE * output, char *filename, int show_iframe)
{
      struct mmapfile mf = { ptr:NULL };
	if (show_iframe != 2) {
		fprintf(output, "<div id='tt%d' class=content%d></div>",
			ttid(1), contentbg);
		fprintf(output, "<script>docStr='';</script>");
		if (show_iframe == 1) {
			char interurl[256];
			if (via_proxy)
				snprintf(interurl, sizeof (interurl),
					 "/" SMAGIC "/%s+%d%s%s", filename,
					 atoi(getparm("T")),
					 usingMath ? "m" : "",
					 stripHeadTail ? "s" : "");
			else
				snprintf(interurl, sizeof (interurl),
					 "http://%s:%d/" SMAGIC "/%s+%d%s%s",
					 inet_ntoa(wwwcache->text_accel_addr),
					 wwwcache->text_accel_port, filename,
					 atoi(getparm("T")),
					 usingMath ? "m" : "",
					 stripHeadTail ? "s" : "");

			fprintf(output,
				"<script charset=" CHARSET
				" src=\"%s\"></script>", interurl);
			fprintf(output,
				"<script>document.getElementById('tt%d').innerHTML=docStr;</script>\n</td></tr></table>\n",
				ttid(0));
			return 0;
		}
		if (show_iframe != 3)
			fputs("<script>var docStr = \"", output);
	}
	if (show_iframe != 3)
		quote_quote = 1;
	if (mmapfile(filename, &mf) < 0)
		goto END;
	MMAP_TRY {
		if (stripHeadTail) {
			int len = mf.size;
			char *ptr;
			ptr = memStripHeadTail(mf.ptr, &len);
			mem2html(output, ptr, len, filename);
			scriped = 0;
		} else
			mem2html(output, mf.ptr, mf.size, filename);
	}

	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	quote_quote = 0;
      END:
	if (show_iframe != 2 && show_iframe != 3) {
		fprintf(output, "\";\n");
#if 0
		fprintf(output, "docStr=lowerTag(docStr);"
			"docStr=docStr.replace(/<\\/script[^>]*>/g, '<\\/script>\\n');"
			"docStr=docStr.replace(/<script[^>]*>[^\\n]*<\\/script>\\n/g, '');\n");
#endif
		fprintf(output,
			"document.getElementById('tt%d').innerHTML=docStr+conPadding;</script>\n",
			ttid(0));

	}
	return 0;
}

int
mem2html(FILE * output, char *str, int size, char *filename)
{
	char *str0, *ptr, buf[512];
	int lastq = 0, ano = 0, len, size0;
	str0 = str;
	size0 = size;
	underline = 0;
	highlight = 0;
	lastcolor = 37;
	useubb = 1;
	fdisplay_attach(NULL, NULL, NULL, NULL);
	while (size) {
		len = min(size, sizeof (buf) - 1);
		ptr = str;
		str = memchr(ptr, '\n', len);
		if (!str && len == sizeof (buf) - 1) {
			str = memchr(ptr + 1, '\033', len);
			if (str)
				str--;
		}
		if (!str)
			str = ptr + len;
		else {
			str++;
			len = str - ptr;
		}
		size -= len;
		if (len > 10 && !strncmp(ptr, "begin 644 ", 10) && filename) {
			FILE *fp;
			int pos;
			strsncpy(buf, ptr, len);
			fp = fopen(filename, "r");
			if (fp) {
				fseek(fp, str - str0, SEEK_SET);
				ano++;
				ptr = strrchr(filename, '/');
				errlog("old attach %s", filename);
				fdisplay_attach(output, fp, buf, ptr + 1);
				pos = ftell(fp);
				str = str0 + pos;
				size = size0 - pos;
				fclose(fp);
			}
			fprintf(output, "\n<br>");
			continue;
		}
		if (len > 18 && !strncmp(ptr, "beginbinaryattach ", 18)
		    && size >= 5 && !*str) {
			unsigned int attlen;
			strsncpy(buf, ptr, len);
			ptr = strchr(buf, '\r');
			if (ptr)
				*ptr = 0;
			ptr = strchr(buf, '\n');
			if (ptr)
				*ptr = 0;
			ano++;
			ptr = buf + 18;
			attlen = ntohl(*(unsigned int *) (str + 1));
			fprintbinaryattachlink(output, ano, ptr,
					       str - str0 + 1 + scriped,
					       attlen);
			attlen = min(size, (int) (attlen + 5));
			str += attlen;
			size -= attlen;
			continue;
		}
		if (ptr[0] == ':' && ptr[1] == ' ') {
			if (!lastq)
				fprintf(output, "<font class=quote>");
			lastq = 1;
		} else {
			if (lastq)
				fprintf(output, "</font>");
			lastq = 0;
		}
		fhhprintf(output, "%.*s", len, ptr);
	}
	if (lastq)
		fprintf(output, "</font>");
	return 0;
}

int
showcon(char *filename)
{
	int retv;
	printf("<div class=line></div>");
	retv = fshowcon(stdout, filename, 0);
	printf("<div class=line></div>");
	return retv;
}

int
testmozilla()
{
	char *ptr = getsenv("HTTP_USER_AGENT");
	if (strcasestr(ptr, "Mozilla") && !strcasestr(ptr, "compatible"))
		return 1;
	return 0;
}

void
processMath()
{
	if (usedMath) {
		printf("<script src=/jsMath/jsMath.js></script>");
		printf("<script>jsMath.ProcessBeforeShowing();</script>");
	}
}

int
bbscon_main()
{
	char board[80], dir[80], file[80], filename[80];
	char buf[1024];
	int nbuf = 0;
	struct boardmem *bx;
	int num, sametitle;
	int outgoing;
	int inndboard;
	int ret, index[3];
	struct fileheader x[3];
	int filetime;
	int range = 100;

	changemode(READING);

	getparmboard(board, sizeof (board));
	strsncpy(file, getparm2("F", "file"), 32);
	num = atoi(getparm2("N", "num")) - 1;
	sametitle = atoi(getparm("st"));

	if ((bx = getboard(board)) == NULL)
		http_fatal("错误的讨论区");
	if (strncmp(file, "M.", 2) && strncmp(file, "G.", 2))
		http_fatal("错误的参数1");
	if (strstr(file, "..") || strstr(file, "/"))
		http_fatal("错误的参数2");
	sprintf(filename, "boards/%s/%s", board, file);
	if (*getparm("attachname") == '/') {
		showbinaryattach(filename);
		return 0;
	}

	nbuf = 0;
	filetime = atoi(file + 2);

	sprintf(dir, "boards/%s/.DIR", board);
	if (sametitle) {
		ret =
		    get_fileheader_records_cb(dir, filetime, num, 1, x, index,
					      3, (void *) same_thread_inrange,
					      &range);
	} else {
		ret =
		    get_fileheader_records(dir, filetime, num, 1, x, index, 3);
	}

	if (ret == -1)
		http_fatal("本文不存在或者已被删除");
	if (x[1].owner[0] == '-') {
		http_fatal("本文已被删除");
	}
	num = index[1];
	html_header(1);
	if (x[1].accessed & FH_MATH) {
		usingMath = 1;
		usedMath = 1;
		withinMath = 0;
	} else {
		usingMath = 0;
	}
	check_msg();
	printf("</head><body><div class=swidth>\n");
	printWhere(bx, "阅读文章");
	printf
	    ("<div class=hright><a href=\"bbsdoc?B=%d&amp;S=%d\" class=blk>回文章列表</a>"
	     "</div><br>", getbnumx(bx), num - 5);
	printf
	    ("<div class=title>话题：%s</div><div class=wline3></div>",
	     void1(titlestr(x[1].title)));
	if (loginok && !isguest && x[1].accessed & FH_ATTACHED
	    && ((wwwcache->accel_addr.s_addr && wwwcache->accel_port)
		|| via_proxy))
		printf
		    ("<a href='bbsmywww'><font color='red'>看不了图片？</font></a>");
	if (loginok && !isguest &&
	    ((wwwcache->accel_addr.s_addr && wwwcache->accel_port)
	     || via_proxy) && !w_info->doc_mode)
		printf
		    ("<a href='bbsmywww'><font color='red'>看不了文章？</font></a>");

	if (x[1].accessed & FH_ALLREPLY) {
		FILE *fp;
		fp = fopen("bbstmpfs/dynamic/Bvisit_log", "a");
		if (NULL != fp) {
			fprintf(fp,
				"www user %s from %s visit %s %s %s",
				currentuser->userid, fromhost,
				bx->header.filename, fh2fname(&(x[1])),
				ctime(&now_t));
			fclose(fp);
		}
	}
	if (index[0] != NA_INDEX) {
		nbuf += sprintf
		    (buf + nbuf,
		     "[<a href='con?B=%d&F=%s&N=%d&T=%d&st=%d'>",
		     getbnumx(bx), fh2fname(&(x[0])), index[0] + 1,
		     feditmark(&(x[0])), sametitle);
		if (sametitle)
			nbuf += sprintf(buf + nbuf, "同主题");
		nbuf += sprintf(buf + nbuf, "上篇</a>]");
	}
	nbuf +=
	    sprintf(buf + nbuf,
		    "[<a href='doc?B=%d&S=%d'>本讨论区</a>]",
		    getbnumx(bx), (num > 4) ? (num - 4) : 1);
	if (index[2] != NA_INDEX) {
		nbuf += sprintf
		    (buf + nbuf,
		     "[<a href='con?B=%d&F=%s&N=%d&T=%d&st=%d'>",
		     getbnumx(bx), fh2fname(&(x[2])), index[2] + 1,
		     feditmark(&(x[2])), sametitle);
		if (sametitle)
			nbuf += sprintf(buf + nbuf, "同主题");
		nbuf += sprintf(buf + nbuf, "下篇</a>]");
	}
	brc_initial(currentuser->userid, board);
	brc_add_read(&(x[1]));
	brc_update(currentuser->userid);
	x[1].title[sizeof (x[1].title) - 1] = 0;

	outgoing = (x[1].accessed & FH_INND)
	    || strchr(x[1].owner, '.');
	inndboard = bx->header.flag & INNBBSD_FLAG;
	if (!(x[1].accessed & FH_NOREPLY))
		nbuf += sprintf(buf + nbuf,
				"[<a href='pst?B=%d&amp;F=%s&amp;num=%d%s'>回复文章</a>]",
				getbnumx(bx), file, num + 1,
				outgoing ? "" : (inndboard ? "&amp;la=1" : ""));

//add by lepton for evaluate
//      if (!sametitle)
	nbuf +=
	    sprintf(buf + nbuf,
		    "[<a href='bbsnt?B=%d&amp;th=%d'>同主题阅读</a>]\n",
		    getbnumx(bx), x[1].thread);
	nbuf +=
	    sprintf(buf + nbuf,
		    "[<a href='tcon?B=%d&amp;th=%d&amp;start=%d'>从这里展开</a>]\n",
		    getbnumx(bx), x[1].thread, num + 1);
	printf("<div class=line></div>");
	printmypicbox(fh2owner(&(x[1])));
	printf("<div class=scontent0><center>");
	fputs(buf, stdout);
#ifdef ENABLE_MYSQL
	if (bytenum(x[1].sizebyte) > 40) {
		printf("[本篇星级: %d]", x[1].staravg50 / 50);
		printf("[评价人数: %d]", x[1].hasvoted);
	}
#endif
	binarylinkfile(getparm2("F", "file"));
	printf("</div><div class=dline></div>");
	if (hideboard(board)
	    || (!via_proxy
		&& (!wwwcache->accel_addr.s_addr || !wwwcache->accel_port))
	    || w_info->doc_mode) {
		//if (!strcmp(currentuser->userid, "ylsdd"))
		//      errlog("%s--%s--%d--%s", dirinfo->owner,
		//             fh2owner(dirinfo), nbuf, dirinfo->title);
		fshowcon(stdout, filename, 0);
	} else {
		sprintf(filename, "boards/%d/%s", getbnumx(bx), file);
		fshowcon(stdout, filename, 1);
	}
	printf("<div class=dline></div><div class=scontent0><center>");
	printf("[<a href='fwd?B=%d&F=%s'>寄回信箱</a>] ", getbnumx(bx), file);
	printf("[<a href='ccc?B=%d&F=%s'>转贴</a>] ", getbnumx(bx), file);
	if (!strncmp(currentuser->userid, x[1].owner, IDLEN + 1)) {
		//|| has_BM_perm(currentuser, bx)) {
		printf
		    ("[<a onclick='return confirm(\"你真的要删除本文吗?\")' href='del?B=%d&amp;F=%s'>删除</a>]",
		     getbnumx(bx), file);
		printf("[<a href='edit?B=%d&amp;F=%s'>修改</a>]", getbnumx(bx),
		       file);
	}
	fputs(buf, stdout);
	printf("<br>");
#ifdef ENABLE_MYSQL
	if (loginok && now_t - filetime <= 5 * 86400
	    && bytenum(x[1].sizebyte) > 40) {
		printf(" <script>eva('%s','%s');</script>", board, file);
	}
#endif
	printf("</div><div class=line></div><br>");
	processMath();
	printReplyForm(bx, &(x[1]), num);
	printf("</center></div></body></html>\n");
	return 0;
}

int
printReplyForm(struct boardmem *bx, struct fileheader *x, int num)
{
	int outgoing, inndboard, njuinnboard;
	char buf[80];
	extern int cssv;
	if (!loginok || isguest || !user_perm(currentuser, PERM_POST))
		return 0;
	if (!w_info->edit_mode || x->accessed & FH_NOREPLY)
		return 0;

	printf("<script>function showReply() {"
	       "document.getElementById('fakereplybox').innerHTML='';"
	       "d=document.getElementById('replybox');"
	       "d.style.visibility='visible';%s"
	       "window.scrollBy(0,500);"
	       "document.getElementById('rte1').contentWindow.focus();}</script>",
	       testmozilla()?"":"d.style.display='block';");
	printf
	    ("<div id=fakereplybox align=left onclick='showReply();'>发表评论<br>"
	     "<textarea cols=50 rows=4></textarea></div>");

	printf("<div id=replybox style='visibility:hidden;%s'>", testmozilla()?"":"display:none");
	printf("<script src=" CSSPATH "richtext.js></script>");
	printf("<script>\nfunction submitForm() {updateRTE('rte1');"
	       "if(isRichText)document.form1.text.value=html2ansi(document.form1.rte1.value);"
	       "else document.form1.text.value=document.form1.rte1.value;"
	       "return false;}\ninitRTE('/images/', '/', '%s?%d');</script>\n",
	       currstyle->cssfile, cssv);
	snprintf(buf, sizeof (buf), "&ref=%s&rid=%d", fh2fname(x), num);
	printf("<form name=form1 method=post action=bbssnd?B=%d&th=%d%s>\n",
	       getbnumx(bx), x->thread, buf);
	printf("<div class=line></div><div class=content1>");
	printf("<div class=hright align=right>");
	printselsignature();
	printf("<br>");
	printuploadattach();
	printf("<br>");
	printusemath(0);
	printf("</div>");
	printf("<b>发表评论</b><br>");
	printf
	    ("使用标题: <input type=text name=title size=50 maxlength=100 value='%s%s'>",
	     strncmp(x->title, "Re: ", 4) ? "Re: " : "",
	     void1(nohtml(x->title)));

	outgoing = (x->accessed & FH_INND) || strchr(x->owner, '.');
	inndboard = bx->header.flag & INNBBSD_FLAG;
	njuinnboard = bx->header.flag2 & NJUINN_FLAG;
	printf
	    ("<br>[<a target=_self href=bbspst?inframe=1&B=%d&file=%s&num=%d&la=%d",
	     getbnumx(bx), fh2fname(x), num + 1, !outgoing);
	printf("&fullquote=1>引用全文(将丢弃所更改内容)</a>] ");
	if (inndboard || njuinnboard)
		printf
		    (" <font class=red>转信</font><input type=checkbox name=outgoing %s>\n",
		     outgoing ? "checked" : "");
	if (anony_board(bx->header.filename))
		printf("匿名<input type=checkbox name=anony %s>\n",
		       strcmp(bx->header.filename,
			      DEFAULTANONYMOUS) ? "" : "checked");

	printf("<div style='height:350'>");
	printf
	    ("<div style='position:absolute;visibility:hidden;display:none'>"
	     "<textarea name=text>");
	sprintf(buf, "boards/%s/%s", bx->header.filename, fh2fname(x));
	printquote(buf, bx->header.filename, fh2owner(x), 0);
	printf("</textarea>");
	printf("</div>");
	printf
	    ("<script>writeRichText('rte1',ansi2html(document.form1.text.value),0,'100%%',true,false);</script>");
	printf("</div>");
	printf
	    ("<center><input type=submit value=发表 onclick=\"this.value='文章提交中，请稍候...';"
	     "this.disabled=true;submitForm();form1.submit();\"></center>");
	printf("</div><div class=line></div></form>");
	//printf ("<script>enableDesignMode(\"rte1\", ansi2html(document.form1.text.value), false);</script>");
	printf("</div>");
	return 0;
}
