#include "bbslib.h"
#if defined(ENABLE_GHTHASH) && defined(ENABLE_FASTCGI)
#include <ght_hash_table.h>
#endif
#include "sys/shm.h"
#include "stdarg.h"
//#ifdef GPROF
//#include <iconv_glibc.h>
//#else
#include <iconv.h>
//#endif

char needcgi[STRLEN];
char rframe[STRLEN];

struct wwwstyle wwwstyle[NWWWSTYLE] = {
	{"蓝色梦想(小字体)", CSSPATH "bbssg.css", CSSPATH "left.css"},
	{"蓝色梦想(大字体)", CSSPATH "bbslg.css", CSSPATH "leftlg.css"},
	{"红粉佳人(小字体)", CSSPATH "bbssm.css", CSSPATH "leftsm.css"},
	{"红粉佳人(大字体)", CSSPATH "bbslm.css", CSSPATH "leftlm.css"},
	{"九九年的回忆(小字体)", CSSPATH "bbss99.css", CSSPATH "lefts99.css"},
	{"九九年的回忆(大字体)", CSSPATH "bbsl99.css", CSSPATH "leftl99.css"},
	//{"自定义的界面", "bbsucss/ubbs.css", "bbsucss/uleft.css"}
};
struct wwwstyle *currstyle = wwwstyle;
int wwwstylenum = 0;
int contentbg = 0;

const struct UBB2ANSI ubb[] = {
	{"color=white", "\033[30m"},
	{"color=red", "\033[31m"},
	{"color=green", "\033[32m"},
	{"color=yellow", "\033[33m"},
	{"color=blue", "\033[34m"},
	{"color=purple", "\033[35m"},
	{"color=cyan", "\033[36m"},
	{"color=black", "\033[37m"},
	{"bcolor=white", "\033[40m"},
	{"bcolor=red", "\033[41m"},
	{"bcolor=green", "\033[42m"},
	{"bcolor=yellow", "\033[43m"},
	{"bcolor=blue", "\033[44m"},
	{"bcolor=purple", "\033[45m"},
	{"bcolor=cyan", "\033[46m"},
	{"bcolor=black", "\033[47m"},
/* 颜色的标签必须在此之前设置，其他标签在此之后设置，为了提高效率 */
	{"/color", "\033[m"},
	{"highlight", "\033[1m"},
	{"normal", "\033[2m"},
	{"bold", "\033[101m"},
	{"/bold", "\033[102m"},
	{"italic", "\033[111m"},
	{"/italic", "\033[112m"},
	{"underline", "\033[4m"},
	{"/underline", "\033[m"},
	{"ubb off", "\033[999m"},
#define UBB_BEGIN_NOUBB 25
	{"ubb on", "\033[998m"},
#define UBB_END_NOUBB 26
	{"", ""}
};

const struct ANSI2HTML_UBB ubb2[] = {
	{"30", "<font class=c30>", 16, "[color=white]"},
	{"31", "<font class=c31>", 16, "[color=red]"},
	{"32", "<font class=c32>", 16, "[color=green]"},
	{"33", "<font class=c33>", 16, "[color=yellow]"},
	{"34", "<font class=c34>", 16, "[color=blue]"},
	{"35", "<font class=c35>", 16, "[color=purple]"},
	{"36", "<font class=c36>", 16, "[color=cyan]"},
	{"37", "<font class=c37>", 16, "[color=black]"},
	{"40", "<font class=b40>", 16, "[bcolor=white]"},
	{"41", "<font class=b41>", 16, "[bcolor=red]"},
	{"42", "<font class=b42>", 16, "[bcolor=green]"},
	{"43", "<font class=b43>", 16, "[bcolor=yellow]"},
	{"44", "<font class=b44>", 16, "[bcolor=blue]"},
	{"45", "<font class=b45>", 16, "[bcolor=purple]"},
	{"46", "<font class=b46>", 16, "[bcolor=cyan]"},
	{"47", "<font class=b47>", 16, "[bcolor=black]"},
#define UBB_BEGIN_OTHER 16
	{"101", "<b>", 3, "[bold]"},
	{"102", "</b>", 4, "[/bold]"},
	{"111", "<i>", 3, "[italic]"},
	{"112", "</i>", 4, "[/italic]"},
#define UBB_BEGIN_UNDERLINE 20
	{"4", "<u>", 3, "[underline]"},
	{"", "", 0, ""}
};

int underline = 0;
int highlight = 0;
int lastcolor = 37;
int useubb = 1;
int noansi = 0;

int usedMath = 0;		//本页面中曾经使用数学公式
int usingMath = 0;		//当前文章（当前hsprintf方式）在使用数学公式
int withinMath = 0;		//正在数学公式中

int loginok = 0;
int isguest = 0;
int tempuser = 0;
int utmpent = 0;
int thispid;
time_t now_t;

jmp_buf cgi_start;

struct userec *currentuser;
struct user_info *u_info;
struct wwwsession *w_info;
struct UTMPFILE *shm_utmp;
struct BCACHE *shm_bcache;
struct UCACHE *shm_ucache;
struct UINDEX *uindexshm;
struct WWWCACHE *wwwcache;
struct mmapfile mf_badwords = { ptr:NULL };
struct mmapfile mf_sbadwords = { ptr:NULL };
struct mmapfile mf_pbadwords = { ptr:NULL };
char fromhost[60];
char realfromhost[60];
struct in_addr from_addr;
int via_proxy = 0;

struct boardmem *getbcache();
char *anno_path_of();
void updatelastboard(void);

int
file_has_word(char *file, char *word)
{
	FILE *fp;
	char buf[256], buf2[256];
	fp = fopen(file, "r");
	if (fp == 0)
		return 0;
	while (1) {
		bzero(buf, 256);
		if (fgets(buf, 255, fp) == 0)
			break;
		sscanf(buf, "%s", buf2);
		if (!strcasecmp(buf2, word)) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

int
junkboard(char *board)
{
	// 请自定义junkboard.
	return file_has_word("etc/junkboards", board);
}

//因为IE的bug, IE 的textarea不能正确处理&nbsp;
char *
nohtml_textarea(const char *s)
{
	static char buf[2048];
	int i = 0;

	while (*s && i < sizeof (buf) - 6) {
		if (*s == '<') {
			strcpy(buf + i, "&lt;");
			i += 4;
		} else if (*s == '>') {
			strcpy(buf + i, "&gt;");
			i += 4;
		} else if (*s == '&') {
			strcpy(buf + i, "&amp;");
			i += 5;
		} else {
			buf[i] = *s;
			i++;
		}
		s++;
	}
	buf[i] = 0;
	return buf;
}

char *
nohtml(const char *s)
{
	static char buf[1024];
	int i = 0;
	int lastnbsp = 0;

	while (*s && i < 1010) {
		if (0 && *s == ' ' && !lastnbsp) {
			lastnbsp = 1;
			strcpy(buf + i, "&nbsp;");
			i += 6;
			s++;
			continue;
		}
		if (*s == '<') {
			strcpy(buf + i, "&lt;");
			i += 4;
		} else if (*s == '>') {
			strcpy(buf + i, "&gt;");
			i += 4;
		} else if (*s == '&') {
			strcpy(buf + i, "&amp;");
			i += 5;
		} else if (*s == '\'') {
			strcpy(buf + i, "&#39;");
			i += 5;
		} else if (*s == '\"') {
			strcpy(buf + i, "&quot;");
			i += 6;
		} else if (*s == '\033') {
			;
		} else {
			buf[i] = *s;
			i++;
		}
		lastnbsp = 0;
		s++;
	}
	buf[i] = 0;
	return buf;
}

char *
getsenv(char *s)
{
	char *t = getenv(s);
	if (t)
		return t;
	return "";
}

int
http_quit()
{
	longjmp(cgi_start, 1);
}

void
http_fatal(char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, 1023, fmt, ap);
	va_end(ap);
	buf[1023] = 0;
	html_header(1);
	printf("<br>错误! %s! <br><br>\n", buf);
	fputs("<a href=javascript:history.go(-1)>快速返回上一步</a>", stdout);
	//fputs("<BR><BR><BR><BR><a href=\"/\" target=_top> 回到进站页面</a>", stdout);
	http_quit();
}

static inline void
strnncpy(char *s, int *l, const char *s2, int len)
{
	memcpy(s + (*l), s2, len);
	(*l) += len;
}

static void
ansifont(const char *ansi, int len, struct bbsfont *bf)
{
	int n;
	char buf[30], *ptr;
	len++;
	if (len > sizeof (buf))
		len = sizeof (buf);
	strsncpy(buf, ansi, len);
	ptr = strtok(buf, ";m");
	if (ptr == NULL) {
		bzero(bf, sizeof (*bf));
		bf->color = 7;
		return;
	}
	while (ptr) {
		n = atoi(ptr);
		if (n == 0) {
			bzero(bf, sizeof (*bf));
			bf->color = 7;
		} else if (n > 29 && n < 38) {
			bf->color = n - 30;
		} else if (n > 39 && n < 48) {
			bf->bgcolor = n - 40;
		} else if (n == 1) {
			bf->hilight = 1;
		} else if (n == 2) {
			bf->hilight = 0;
		} else if (n == 101) {
			bf->bold = 1;
		} else if (n == 102) {
			bf->bold = 0;
		} else if (n == 111) {
			bf->italic = 1;
		} else if (n == 112) {
			bf->italic = 0;
		} else if (n == 4) {
			bf->underline = 1;
		}
		ptr = strtok(NULL, ";m");
	}
}

const char *(colorlo[]) = {
"white", "red", "green", "yellow", "blue", "purple", "cyan", "black"};
const char *(colorhi[]) = {
"#f0f0f0", "#e00000", "#008000", "#808000", "#0000ff",
	    "#d000d0", "#33a0a0", "#808080"};

static char *
fontstr(struct bbsfont *bf, char *buf)
{
	const char **color = colorlo;
	if (noansi) {
		buf[0] = 0;
		return NULL;
	}
	if (bf->hilight)
		color = colorhi;
	if (bf->bgcolor == 0 && bf->color == 7 && bf->bold == 0
	    && bf->italic == 0 && bf->underline == 0 && bf->hilight == 0)
		return NULL;
	strcpy(buf, "<font style='");
	if (bf->bgcolor) {
		strcat(buf, "background-color: ");
		strcat(buf, color[bf->bgcolor]);
		strcat(buf, ";");
	}
	if (bf->color != 7) {
		strcat(buf, "color: ");
		strcat(buf, color[bf->color]);
		strcat(buf, ";");
	}
	if (bf->bold)
		strcat(buf, "font-weight: bold;");
	if (bf->italic)
		strcat(buf, "font-style: italic;");
	if (bf->underline)
		strcat(buf, "text-decoration: underline;");
	strcat(buf, "'>");
	return buf;
}

void
hsprintf(char *s, char *s0)
{
	static const char specchar[256] = {['\\'] = 1,['$'] = 1,['"'] =
		    1,['&'] = 1,['<'] = 1,['>'] = 1,[' '] = 1,['\n'] =
		    1,['\r'] = 1,['\033'] = 1
	};
	int c, m, i, len;
	static int dbchar = 0;
	int lastdb;
	char buf[256];
	static struct bbsfont bf;
	static int infont;
	if (s == NULL) {
		bzero(&bf, sizeof (bf));
		bf.color = 7;
		infont = 0;
		withinMath = 0;
		usingMath = 0;
		usedMath = 0;
		contentbg = 0;
		dbchar = 0;
		return;
	}
	len = 0;
	for (i = 0; (c = s0[i]); i++) {
		lastdb = dbchar;
		if (dbchar)
			dbchar = 0;
		else if (c & 0x80)
			dbchar = 1;

		if (!specchar[(unsigned char) c]) {
			s[len++] = c;
			continue;
		}
		if (lastdb && len && c != '\n') {
			s[len++] = c;
			continue;
		}
		switch (c) {
		case '\\':
			if (usingMath && !withinMath && s0[i + 1] == '[') {
				strnncpy(s, &len, "<div class=math>", 16);
				i++;
				withinMath = 2;
			} else if (usingMath && withinMath == 2
				   && s0[i + 1] == ']') {
				strnncpy(s, &len, "</div>", 6);
				i++;
				withinMath = 0;
			} else if (usingMath && s0[i + 1] == '$') {
				s[len++] = '$';
				i++;
			} else if (quote_quote)
				strnncpy(s, &len, "\\\\", 2);
			else
				s[len++] = c;
			break;
		case '$':
			if (usingMath && !withinMath) {
				strnncpy(s, &len, "<span class=math>", 17);
				withinMath = 1;
			} else if (usingMath && withinMath == 1) {
				strnncpy(s, &len, "</span>", 7);
				withinMath = 0;
			} else
				s[len++] = c;
			break;
		case '"':
			if (quote_quote)
				strnncpy(s, &len, "\\\"", 2);
			else
				s[len++] = c;
			break;
		case '&':
			strnncpy(s, &len, "&amp;", 5);
			break;
		case '<':
			strnncpy(s, &len, "&lt;", 4);
			break;
		case '>':
			strnncpy(s, &len, "&gt;", 4);
			break;
		case ' ':
			if (!i || s0[i - 1] != ' ') {
				s[len++] = c;
			} else {
				strnncpy(s, &len, "&nbsp;", 6);
			}
			break;
		case '\r':
			break;
		case '\n':
			if (withinMath) {
				s[len++] = ' ';
				break;
			}
			if (quote_quote)
				strnncpy(s, &len, " \\\n<br>", 7);
			else
				strnncpy(s, &len, "\n<br>", 5);
			break;
		case '\033':
			if (s0[i + 1] == '\n') {
				i++;
				continue;
			}
			if (s0[i + 1] == '\r' && s0[i + 2] == '\n') {
				i += 2;
				continue;
			}
			if (s0[i + 1] == '<') {
				for (m = 2; s0[i + m] && s0[i + m] != '>';
				     m++) ;
				if (s0[i + m])
					i += m;
				continue;
			}
			if (s0[i + 1] != '[')
				continue;
			if (s0[i + 2] == '<') {
				int savelen = 0;
				int indb = 0;
				savelen = len;
				for (m = i + 2; s0[m]; m++) {
					if (indb) {
						s[len++] = s0[m];
						indb = 0;
						continue;
					}
					if (quote_quote
					    && (s0[m] == '\"'
						|| s0[m] == '\\')) {
						s[len++] = '\\';
						s[len++] = s0[m];
					} else if (quote_quote
						   && s0[m - 1] == '<'
						   && s0[m] == '/'
						   && tolower(s0[m + 1]) ==
						   's') {
						//Avoiding </script>
						s[len++] = '\\';
						s[len++] = '\n';
						s[len++] = '/';
					} else {
						s[len++] = s0[m];
						if (s0[m] & 0x80)
							indb = 1;
					}
					if (strchr(">\r\n", s0[m]))
						break;
				}
				if (indb)
					s[len++] = ' ';
				if (s0[m] != '>') {
					i = i + 2;
					len = savelen;
					continue;
				}
				if (infont) {
					strnncpy(s, &len, "</font>", 7);
					infont = 0;
				}
				m++;
				i = m - 1;
				if (fontstr(&bf, buf)) {
					infont = 1;
					strnncpy(s, &len, buf, strlen(buf));
				}
				break;
			}
			for (m = i + 2; s0[m] && m < i + 24; m++)
				if (strchr("0123456789;", s0[m]) == 0)
					break;
			if (s0[m] != 'm') {
				i = m;
				continue;
			}
			ansifont(s0 + i + 2, m - (i + 2), &bf);
			if (infont) {
				strnncpy(s, &len, "</font>", 7);
				infont = 0;
			}
			if (fontstr(&bf, buf)) {
				infont = 1;
				strnncpy(s, &len, buf, strlen(buf));
			}
			i = m;
			break;
		default:
			s[len++] = c;
		}
	}
	s[len] = 0;
}

char *
titlestr(const char *s)
{
	return nohtml(s);
}

int
hprintf(char *fmt, ...)
{
	static char buf[32768], buf2[1024];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf2, 1023, fmt, ap);
	va_end(ap);
	buf2[1023] = 0;
	hsprintf(buf, buf2);
	fputs(buf, stdout);
	return 0;
}

static void
fqhprintf(FILE * output, char *str)
{
	static char buf[8096];
	hsprintf(buf, str);
	fputs(buf, output);
}

int
fhhprintf(FILE * output, char *fmt, ...)
{
	char buf0[1024], buf[1024], *s;
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(buf, 1023, fmt, ap);
	va_end(ap);
	buf[1023] = 0;
	s = buf;
	if (w_info->link_mode) {
		fqhprintf(output, buf);
		return 0;
	}
	if (len < 10 || !strchr(s + 3, ':')) {
		fqhprintf(output, buf);
		return 0;
	}
	if (strstr(s, "\033[<")
	    || (!strcasestr(s, "http://") && !strcasestr(s, "ftp://")
		&& !strcasestr(s, "mailto:"))) {
		fqhprintf(output, buf);
		return 0;
	}
	if (!strncmp(s, "标  题:", 7)) {
		fqhprintf(output, buf);
		return 0;
	}
	len = 0;
	while (s[0]) {
		if (!strncasecmp(s, "http://", 7)
		    || !strncasecmp(s, "mailto:", 7)
		    || !strncasecmp(s, "ftp://", 6)) {
			char *tmp, *noh, tmpchar;
			if (len > 0) {
				buf0[len] = 0;
				fqhprintf(output, buf0);
				len = 0;
			}
			tmp = s;
			while (*s && !strchr("<>\'\" \r\t)(,;\n\033", *s))
				s++;
			tmpchar = *s;
			*s = 0;
			if (1) {
				if (!strcasecmp(s - 4, ".gif")
				    || !strcasecmp(s - 4, ".jpg")
				    || !strcasecmp(s - 4, ".bmp")
				    || !strcasecmp(s - 5, ".jpeg")) {
					fprintf(output, "<IMG SRC='%s'>",
						nohtml(tmp));
					*s = tmpchar;
					continue;
				}
			}
			noh = nohtml(tmp);
			fprintf(output, "<a target=_blank href='%s'>%s</a>",
				noh, noh);
			*s = tmpchar;
			continue;
		} else {
			buf0[len] = s[0];
			if (len < sizeof (buf0) - 1)
				len++;
			s++;
		}
	}
	if (len) {
		buf0[len] = 0;
		fqhprintf(output, buf0);
	}
	return 0;
}

char parm_name[256][80], *parm_val[256];
struct parm_file parm_valfile[256];
int parm_num = 0;

void
parm_add(char *name, char *val)
{
	int len = strlen(val);
	if (parm_num >= 255)
		http_fatal("too many parms.");
	parm_val[parm_num] = calloc(len + 1, 1);
	if (parm_val[parm_num] == 0)
		http_fatal("memory overflow2 %d %d", len, parm_num);
	strsncpy(parm_name[parm_num], name, 78);
	strsncpy(parm_val[parm_num], val, len + 1);
	parm_num++;
}

void
parm_addfile(char *name, char *filename, char *content, int contentlen)
{
	int len = strlen(filename);
	if (!len)
		return;
	if (parm_num >= 255)
		http_fatal("too many parms.");

	parm_valfile[parm_num].content = malloc(contentlen + 1);
	if (parm_valfile[parm_num].content == NULL) {
		http_fatal("Out of memory");
	}
	memcpy(parm_valfile[parm_num].content, content, contentlen);
	parm_valfile[parm_num].content[contentlen] = 0;
	parm_valfile[parm_num].len = contentlen;

	parm_valfile[parm_num].filename = malloc(len + 1);
	if (parm_valfile[parm_num].filename == NULL) {
		free(parm_valfile[parm_num].content);
		http_fatal("Out of memory");
	}
	memcpy(parm_valfile[parm_num].filename, filename, len + 1);

	strsncpy(parm_name[parm_num], name, 78);
	parm_num++;
}

static char *domainname[] = {
	MY_BBS_DOMAIN,
	NULL
};

char *specname[] = {
	"1999",
	"2001",
	"www",
	"bbs",
	"0",
	NULL
};

int
isaword(char *dic[], char *buf)
{
	char **a;
	for (a = dic; *a != NULL; a++)
		if (!strcmp(buf, *a))
			return 1;
	return 0;
}

struct wwwsession guest = {
      t_lines:20,
      link_mode:0,
      def_mode:0,
      att_mode:0,
      doc_mode:0,
      edit_mode:1,
};

int
url_parse()
{
	char *url, *end, name[STRLEN], *p, *extraparam;
	url = getenv("SCRIPT_URL");
	if (NULL == url) {
		char buf[1024];
		url = getenv("REQUEST_URI");
		if (NULL == url)
			return -1;
		strsncpy(buf, url, sizeof (buf));
		if ((end = strchr(buf, '?')))
			*end = 0;
		setenv("SCRIPT_URL", buf, 0);
	}
	strcpy(name, "/");
#ifdef USESESSIONCOOKIE
	if (strcmp(url, "/")) {
		strsncpy(name, getparm("SESSION"), sizeof (name));
	}
	if (!strncmp(url, "/" SMAGIC, sizeof (SMAGIC))) {
		//兼容 telnet
		if (!name[1]) {
			snprintf(name, STRLEN, "%s", url + sizeof (SMAGIC));
			p = strchr(name, '/');
			if (p)
				*p = 0;
		}

		p = strchr(url + sizeof (SMAGIC), '/');
		if (NULL != p) {
			url = strchr(url + 1, '/');
		} else
			return -1;

	}
#else
	if (!strncmp(url, "/" SMAGIC, sizeof (SMAGIC))) {
		snprintf(name, STRLEN, "%s", url + sizeof (SMAGIC));
		p = strchr(name, '/');
		if (NULL != p) {
			*p = 0;
			url = strchr(url + 1, '/');
		} else
			return -1;
	}
#endif
	extraparam = strchr(name, '_');
	if (extraparam) {
		*extraparam = 0;
		extraparam++;
	} else
		extraparam = "";
	extraparam_init(extraparam);
	loginok = user_init(&currentuser, &u_info, name);
	if (loginok)
		w_info = &(u_info->wwwinfo);
	else
		w_info = &guest;

	snprintf(needcgi, STRLEN, "%s", url + 1);
	if ((end = strchr(needcgi, '?')) != NULL)
		*end = 0;

	if (!*needcgi || nologin) {
		strcpy(needcgi, "bbsindex");
		strcpy(rframe, "");
		strsncpy(name, getsenv("HTTP_HOST"), 70);
		p = strchr(name, '.');
		if (p != NULL && isaword(domainname, p + 1)) {
			*p = 0;
			strsncpy(rframe, name, 60);
		}
		if (rframe[0] && isaword(specname, rframe))
			rframe[0] = 0;
		return 0;
	}
	if (NULL != (end = strchr(needcgi, '/')))
		*end = 0;
	return 0;
}

static void
http_parm_free(void)
{
	int i;
	for (i = 0; i < parm_num; i++) {
		if (parm_val[i]) {
			free(parm_val[i]);
			parm_val[i] = NULL;
		} else {
			free(parm_valfile[i].filename);
			free(parm_valfile[i].content);
			parm_valfile[i].filename = NULL;
		}
	}
	parm_num = 0;
}

static void
parm_init_parsestring(char *buf, char *delim)
{
	char *t2, *t3;
	t2 = strtok(buf, delim);
	while (t2) {
		t3 = strchr(t2, '=');
		if (t3 != 0) {
			t3[0] = 0;
			t3++;
			__unhcode(t3);
#ifdef USEBIG5
			sbig2gb(t3, strlen(t3));
#endif
			parm_add(strtrim(t2), t3);
		}
		t2 = strtok(0, delim);
	}
}

static void
skipLine(char **buf, int *len)
{
	while (**buf && **buf != '\r' && **buf != '\n') {
		(*buf)++;
		(*len)--;
	}
	if (**buf == '\r') {
		(*buf)++;
		(*len)--;
	}
	if (**buf == '\n') {
		(*buf)++;
		(*len)--;
	}
}

static void
parm_init_parseformdata(char *buf, int len, char *boundary)
{
	char *cdline, *parmName, *parmFile, *parmContent;
	int contentlen;
	char *ptr, *contentstr = "Content-Disposition: form-data; name=\"";
	int blen, clen;
	blen = strlen(boundary);
	clen = strlen(contentstr);
	while (1) {
		//the input should begin with a boundary line
		if (strncmp(buf, boundary, blen))
			break;
		//skip the boundary line
		buf += blen;
		len -= blen;
		skipLine(&buf, &len);
		//if there is no more segment
		if (*buf == 0 || !strncmp(buf, "--", 2)) {
			break;
		}
		//the Content-Disposition line
		// Content-Disposition: form-data; name="uploadfile"; filename="file1.txt"
		// Content-Disposition: form-data; name="submit-name"
		if (strncasecmp(buf, contentstr, clen))
			break;
		cdline = buf;	//The Content-Disposition line
		skipLine(&buf, &len);
		*(buf - 1) = 0;
		//skip the mime lines
		while (*buf && *buf != '\r' && *buf != '\n') {
			skipLine(&buf, &len);
		}
		skipLine(&buf, &len);

		//The content of the file or the value of form parameter
		parmContent = buf;
		ptr = memmem(parmContent, len, boundary, blen);
		if (!ptr)
			break;
		len -= ptr - buf;
		buf = ptr;
		contentlen = buf - parmContent;
		if (contentlen > 0 && parmContent[contentlen - 1] == '\n')
			contentlen--;
		if (contentlen > 0 && parmContent[contentlen - 1] == '\r')
			contentlen--;
		parmContent[contentlen] = 0;

		//Look for the value of name in the Content-Disposition line
		parmName = cdline + clen;
		ptr = strchr(parmName, '\"');
		if (!ptr)
			break;
		*ptr = 0;
		ptr++;
		//Look for "filename" in the Content-Disposition line
		while (*ptr && !isalpha(*ptr))
			ptr++;
		if (!strncasecmp(ptr, "filename=\"", strlen("filename=\""))) {
			parmFile = ptr + strlen("filename=\"");
			ptr = strchr(parmFile, '\"');
			if (ptr)
				*ptr = 0;
			//IE 竟然送全路径名给服务器，太愚蠢了！
			if ((ptr = strrchr(parmFile, '\\')))
				parmFile = ptr + 1;
			//errlog("name=%s filename=%s %s", parmName, parmFile, strtrim(parmName));
			parm_addfile(parmName, strtrim(parmFile), parmContent,
				     contentlen);
		} else {
			__unhcode(parmContent);
#ifdef USEBIG5
			sbig2gb(parmContent, strlen(parmContent));
#endif
			parm_add(parmName, parmContent);
		}
	}
}

void
http_parm_init(void)
{
	char *buf, buf2[1024];
	char *ptr;
	int n;
	http_parm_free();

	n = atoi(getsenv("CONTENT_LENGTH"));
	if (n > 5000000)
		http_fatal("File size is too big.");
	buf = malloc(n + 1);
	if (NULL == buf)
		http_fatal("Out of memory.");
	n = fread(buf, 1, n, stdin);
	buf[n] = 0;

	ptr = getsenv("CONTENT_TYPE");
	if (strncasecmp(ptr, "multipart/form-data; boundary=", 30))
		parm_init_parsestring(buf, "&");
	else {
		ptr = strchr(ptr, '=');
		if (!ptr)
			http_fatal("Bad http request");
		ptr++;
		buf2[0] = '-';
		buf2[1] = '-';
		strsncpy(buf2 + 2, ptr, sizeof (buf2) - 2);
		parm_init_parseformdata(buf, n, buf2);
	}
	free(buf);
	strsncpy(buf2, getsenv("QUERY_STRING"), 1024);
	parm_init_parsestring(buf2, "&");
	strsncpy(buf2, getsenv("HTTP_COOKIE"), 1024);
	parm_init_parsestring(buf2, ";");
}

int
cache_header(time_t t, int age)
{
	char *old;
	time_t oldt = 0;
	char buf[STRLEN];
	if (!t)
		return 0;
	old = getenv("HTTP_IF_MODIFIED_SINCE");
	if (old) {
		struct tm tm;
		bzero(&tm, sizeof (tm));
#ifndef CYGWIN
		if (strptime(old, "%a, %d %b %Y %H:%M:%S %Z", &tm))
#else				//CYGWIN
		if (strptime(old, "%a, %d %b %Y %H:%M:%S GMT", &tm))
#endif				//CYGWIN

#ifdef __GLIBC__
			oldt = mktime(&tm) - timezone;
#else
			oldt = mktime(&tm) + 28800;
//fixme 28800 is only suitable for China. 28800=8*3600
#endif
		else {
			strsncpy(buf, old, STRLEN - 10);
			strcat(buf, "\n");
			f_append("not-known-time", buf);
		}
	}
	if (oldt >= t) {
		printf("Status: 304\r\n");
		t = now_t + age;
		if (strftime(buf, STRLEN, "%a, %d %b %Y %H:%M:%S", gmtime(&t)))
			printf("Expires: %s GMT\r\n\r\n", buf);
		//printf("Cache-Control: max-age=%d\r\n\r\n", age);
		return 1;
	}
	if (strftime(buf, STRLEN, "%a, %d %b %Y %H:%M:%S", gmtime(&t)))
		printf("Last-Modified: %s GMT\r\n", buf);
	t = now_t + age;
	if (strftime(buf, STRLEN, "%a, %d %b %Y %H:%M:%S", gmtime(&t)))
		printf("Expires: %s GMT\r\n", buf);
	//printf("Cache-Control: max-age=%d\r\n", age);
	return 0;
}

int cssv;

void
html_header(int mode)
{
	static int hasrun;
	static int lastcheck = 0;

	if (now_t != lastcheck) {
		cssv = cssVersion();
		lastcheck = now_t;
	}

	if (!mode) {
		hasrun = 0;
		return;
	}
	if (hasrun) {
		return;
	}
	hasrun = 1;

	printf("Content-type: text/html; charset=%s\r\n\r\n", CHARSET);
	printf("<html>\n");
#if 0
	//This doesn't work for some places!
	printf("<!--%d;%d;%d;%d-->", thispid, sizeof (struct wwwsession),
	       wwwcache->www_visit, wwwcache->home_visit);
#endif
	switch (mode) {
	case 1:
	case 11:		//bbsgetmsg
		printf
		    ("<head><meta http-equiv='Content-Type' content='text/html; charset=%s'>\n",
		     CHARSET);
		printf("<link rel=stylesheet type=text/css href='%s?%d'>\n",
		       currstyle->cssfile, cssv);
#if 0
		printf("<style type=text/css>"
		       "div.content1 {line-height: 1.5em; font-size: 10.5pt; border-left:0.5pt solid blue; border-right:0.5pt solid blue; padding:2pt 5pt 2pt 5pt; background-color:e9e9ed; text-align: left}\n"
		       "div.content0 {line-height: 1.5em; font-size: 10.5pt; border-left:0.5pt solid blue; border-right:0.5pt solid blue; padding:2pt 5pt 2pt 5pt; background-color:f4f4f8; text-align: left}\n"
		       "div.line {border-top:1px solid BLUE; padding:0pt 0pt 0pt 0pt}\n"
		       "</style>");
#endif
		printf("<script src=" BBSJS "?%d></script>\n", cssv);
		break;
	case 2:
		printf
		    ("<head><meta http-equiv=Content-Type content=\"text/html; charset=%s\">",
		     CHARSET);
		printf
		    ("<link rel='stylesheet' type='text/css' href=%s?%d>",
		     currstyle->leftcssfile, cssv);
		printf("<script src=" BBSLEFTJS "?%d></script>\n", cssv);
		break;
	case 3:
		//printf("<meta http-equiv=\"pragma\" content=\"no-cache\">");
		//break;
	case 4:
		printf
		    ("<head><meta http-equiv=Content-Type content=\"text/html; charset=%s\">",
		     CHARSET);
	default:
		break;
	}
}

int
__to16(char c)
{
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	if (c >= '0' && c <= '9')
		return c - '0';
	return 0;
}

void
__unhcode(char *s)
{
	int m, n;
	for (m = 0, n = 0; s[m]; m++, n++) {
		if (s[m] == '+') {
			s[n] = ' ';
			continue;
		}
		if (s[m] == '%') {
			if (!s[m + 1] || !s[m + 2])
				break;
			s[n] = __to16(s[m + 1]) * 16 + __to16(s[m + 2]);
			m += 2;
			continue;
		}
		s[n] = s[m];
	}
	s[n] = 0;
}

char *
getparm(char *var)
{
	int n;
	for (n = parm_num - 1; n >= 0; n--)
		if (!strcasecmp(parm_name[n], var))
			return parm_val[n];
	return "";
}

char *
getparm2(char *v1, char *v2)
{
	char *ptr;
	ptr = getparm(v1);
	if (!ptr[0])
		ptr = getparm(v2);
	return ptr;
}

struct parm_file *
getparmfile(char *var)
{
	int n;
	for (n = parm_num - 1; n >= 0; n--) {
		if (!strcasecmp(parm_name[n], var)) {
			if (!parm_valfile[n].filename)
				return NULL;
			return &parm_valfile[n];
		}
	}
	return NULL;
}

char *
getparmboard(char *buf, int size)
{
	if (size < 24)
		return "";
	strsncpy(buf, getparm2("B", "board"), size);
	return buf;
}

int
shm_init(struct bbsinfo *bbsinfo)
{
	shm_utmp = bbsinfo->utmpshm;
	shm_bcache = bbsinfo->bcacheshm;
	uindexshm = bbsinfo->uindexshm;
	return 0;
}

int
addextraparam(char *ub, int size, int n, int param)
{
	char *base;
	int len = strlen(ub), i;
	base = strchr(ub, '_');
	if (!base) {
		len = strlen(ub);
		if (len > size - 3)
			return -1;
		base = ub + len;
		*base = '_';
	}
	base++;
	if (base + n + 1 >= ub + size)
		return -1;
	for (i = 0; base[i] && i < n; i++) ;
	for (; i < n; i++)
		base[i] = '_';
	base[i++] = 'A' + param % 26;
	base[i] = 0;
	return 0;
}

void
extraparam_init(unsigned char *extrastr)
{
	int i;
	if (*extrastr) {
		i = *extrastr - 'A';
		if (i < 0 || i >= NWWWSTYLE)
			i = 0;
		wwwstylenum = i;
		currstyle = &wwwstyle[i];
		extrastr++;
	} else
		currstyle = &wwwstyle[1];
}

int
user_init(struct userec **x, struct user_info **y, unsigned char *ub)
{
	struct userec *x2;
	static struct userec *guestrec = NULL;
	char sessionid[31];
	int ret = 0;
	int i;
	int uid;
	utmpent = 0;
	isguest = 0;
	tempuser = 0;
	if (guestrec == NULL) {
		if (getuser("guest", &guestrec) <= 0)
			guestrec = NULL;
	}
	*x = guestrec;
	*y = NULL;
	if (strlen(ub) < 33) {
		if (strlen(ub) >= 8) {	//从 telnet 来的临时用户
			i = myatoi(ub, 3);
			uid = myatoi(ub + 3, 4);
			strcpy(sessionid, ub + 7);
			if (i < 0 || i >= MAXACTIVE)
				goto OUT_INIT;
			if (uid <= 0 || uid > MAXUSERS)
				goto OUT_INIT;
			(*y) = &(shm_utmp->uinfo[i]);
			if ((*y)->fromIP != from_addr.s_addr) {
				goto OUT_INIT;
			}
			if (strncmp((*y)->sessionid, sessionid, 2)) {
				goto OUT_INIT;
			}
			if ((*y)->active == 0) {
				goto OUT_INIT;
			}
			if (!strcasecmp((*y)->userid, "new")) {
				goto OUT_INIT;
			}
			if (!strcmp((*y)->userid, "guest"))
				isguest = 1;
			if (!isguest) {
				if (getuserbynum(uid, &x2) <= 0)
					goto OUT_INIT;
			} else {
				x2 = guestrec;
			}
			if (x2 == 0 || strcmp(x2->userid, (*y)->userid)) {
				goto OUT_INIT;
			}
			utmpent = i + 1;
			*x = x2;
			tempuser = 1;
		}
		goto OUT_INIT;
	}
	ub[37] = 0;
	i = myatoi(ub, 3);
	uid = myatoi(ub + 3, 4);
	strsncpy(sessionid, ub + 7, sizeof(sessionid));
	if (i < 0 || i >= MAXACTIVE) {
		goto OUT_INIT;
	}
	if (uid <= 0 || uid > MAXUSERS)
		goto OUT_INIT;
	(*y) = &(shm_utmp->uinfo[i]);
	if ((*y)->wwwinfo.ipmask) {
		if (((*y)->fromIP << (*y)->wwwinfo.ipmask) !=
		    (from_addr.s_addr << (*y)->wwwinfo.ipmask)) {
			goto OUT_INIT;
		}
	} else if ((*y)->fromIP != from_addr.s_addr) {
		goto OUT_INIT;
	}
	if (strcmp((*y)->sessionid, sessionid)) {
		goto OUT_INIT;
	}
	if ((*y)->active == 0) {
		goto OUT_INIT;
	}
	if ((*y)->userid[0] == 0)
		goto OUT_INIT;
	if (!strcasecmp((*y)->userid, "new"))
		goto OUT_INIT;
	if (now_t - u_info->lasttime > 18 * 60 || u_info->wwwinfo.iskicked)	//18分钟是留下一个窗口,防止一个u_info被刷新为0两次
		goto OUT_INIT;
	else
		u_info->lasttime = now_t;
	if (!strcasecmp((*y)->userid, "guest"))
		isguest = 1;
	if (!isguest) {
		if (getuserbynum(uid, &x2) < 0)
			goto OUT_INIT;
	} else {
		x2 = guestrec;
	}
	if (x2 == 0 || strcmp(x2->userid, (*y)->userid))
		goto OUT_INIT;
	utmpent = i + 1;
	ret = 1;
	*x = x2;
      OUT_INIT:
	if (ret)
		return 1;
	else {
		if (!tempuser)
			*x = guestrec;
		return 0;
	}
}

#ifdef INTERNET_EMAIL
static int
post_imail(char *userid, char *title, char *file, char *id,
	   char *nickname, char *ip, int sig)
{
	char *ptr;

	if (strlen(userid) > 100)
		http_fatal("错误的收信人地址");
	for (ptr = userid; *ptr; ptr++) {
		if (strchr(";~|`!$#%%&^*()'\"<>?/ ", *ptr) || !isprint(*ptr))
			http_fatal("错误的收信人地址");
	}

	return bbs_sendmail_noansi(file, title, userid, id);
}
#endif

static void
sig_append(FILE * fp, char *id, int sig)
{
	FILE *fp2;
	char path[256];
	char buf[256];
	int total, hasnl = 1, i, emptyline = 0;
	if (USERPERM(currentuser, PERM_DENYSIG))
		return;
	if (sig <= -2)
		sig = random() % (-1 - sig);
	if (sig < 0 || sig > 10)
		return;
	sethomefile(path, id, "signatures");
	fp2 = fopen(path, "r");
	if (fp2 == 0)
		return;
	for (total = 0; total < 100; total++) {
		if (fgets(buf, 255, fp2) == 0)
			break;
		if (total < sig * 6)
			continue;
		if (total >= (sig + 1) * 6)	// || buf[0] == '\r' || buf[0] == '\n')
			break;
		if (buf[0] == '\r' || buf[0] == '\n') {
			emptyline++;
			continue;
		}
		for (i = 0; i < emptyline; i++)
			fputs("\n", fp);
		emptyline = 0;
		fputs(buf, fp);
		hasnl = (strchr(buf, '\n') == NULL) ? 0 : 1;
	}
	fclose(fp2);
	if (!hasnl)
		fputs("\n", fp);
}

int
post_mail(char *userid, char *title, char *file, char *id,
	  char *nickname, char *ip, int sig, int mark)
{
	char buf[256];
	FILE *fp;

	snprintf(buf, sizeof (buf), ".bbs@%s", MY_BBS_DOMAIN);
	if (strstr(userid, buf)
	    || strstr(userid, ".bbs@localhost")) {
		char *pos;
		pos = strchr(userid, '.');
		*pos = '\0';
	}
	if (strstr(userid, "@")) {
#ifdef INTERNET_EMAIL
		return post_imail(userid, title, file, id, nickname, ip, sig);
#else
		return 0;
#endif
	}
	fp = fopen(file, "a");
	if (!fp)
		return -1;
	fputs("\n--\n", fp);
	sig_append(fp, id, sig);
	fprintf(fp, "\033[m\033[1;%dm※ 来源:．%s %s [FROM: %.20s]\033[m\n",
		31 + rand() % 7, BBSNAME, "http://" MY_BBS_DOMAIN, ip);
	fclose(fp);
	mail_file(file, userid, title, id);
	tracelog("%s mail %s", currentuser->userid, userid);
	return 0;
}

int
post_article_1984(char *board, char *title, char *file, char *id,
		  char *nickname, char *ip, int sig, int mark,
		  int outgoing, char *realauthor, int thread)
{
	FILE *fp, *fp2;
	char buf3[1024], buf[80];
	struct fileheader header;
	int t;
	struct tm *n;
	n = localtime(&now_t);
	sprintf(buf, "boards/.1984/%04d%02d%02d", n->tm_year + 1900,
		n->tm_mon + 1, n->tm_mday);
	if (!file_isdir(buf))
		if (mkdir(buf, 0770) < 0)
			return -1;

	bzero(&header, sizeof (header));
	fh_setowner(&header, id, 0);
	strcpy(buf3, buf);
	t = trycreatefile(buf3, "M.%d.A", now_t, 100);
	if (t < 0)
		return -1;
	header.filetime = t;
	if (thread != -1)
		header.thread = thread;
	strsncpy(header.title, title, sizeof (header.title));
	fp = fopen(buf3, "w");
	if (NULL == fp)
		return -1;
	fp2 = fopen(file, "r");
	fprintf(fp,
		"发信人: %s (%s), 信区: %s\n标  题: %s\n发信站: %s (%24.24s), %s)\n\n",
		id, nickname, board, title, BBSNAME, Ctime(now_t),
		outgoing ? "转信(" MY_BBS_DOMAIN : "本站(" MY_BBS_DOMAIN);
	if (fp2 != 0) {
		while (1) {
			int retv;
			retv = fread(buf3, 1, sizeof (buf3), fp2);
			if (retv <= 0)
				break;
			fwrite(buf3, 1, retv, fp);
		}
		fclose(fp2);
	}
	fprintf(fp, "\n--\n");
	sig_append(fp, id, sig);
	fprintf(fp, "\033[m\033[1;%dm※ 来源:．%s %s [FROM: %.20s]\033[m\n",
		31 + rand() % 7, BBSNAME, "http://" MY_BBS_DOMAIN, ip);
	fclose(fp);
	sprintf(buf3, "%s/M.%d.A", buf, t);
	header.sizebyte = numbyte(eff_size(buf3));
	sprintf(buf3, "%s/.DIR", buf);
	append_record(buf3, &header, sizeof (header));
	return t;
}

static int
watch_board(char *bname)
{
	struct boardmem *x;
	x = getbcache(bname);
	if (x == 0)
		return 0;
	return (x->header.flag2 & WATCH_FLAG);
}

static char *
setbfile(buf, boardname, filename)
char *buf, *boardname, *filename;
{
	sprintf(buf, "boards/%s/%s", boardname, filename);
	return buf;
}

static void
log_article(struct fileheader *fh, char *oldpath, char *board)
{
	int count;
	int t;
	char filepath[STRLEN], newfname[STRLEN];
	char buf[256];
	struct fileheader postfile;
	if (!strcmp(fh->owner, "deliver"))
		return;
	if (watch_board(board))
		return;
	snprintf(buf, sizeof (buf), "..%s", oldpath + 6);
	memcpy(&postfile, fh, sizeof (struct fileheader));
	if (strncmp(fh->title, "Re: ", 4))
		snprintf(postfile.title, sizeof (postfile.title), "%s %s",
			 board, fh->title);
	else
		snprintf(postfile.title, sizeof (postfile.title), "Re: %s %s",
			 board, fh->title + 4);
	t = postfile.filetime;
	count = 0;
	while (1) {
		sprintf(newfname, "M.%d.A", t);
		setbfile(filepath, "allarticle", newfname);
		if (symlink(buf, filepath) == 0) {
			postfile.filetime = t;
			break;
		}
		t += 10;
		if (count++ > MAX_POSTRETRY)
			break;
	}
	append_record("boards/allarticle/.DIR", &postfile,
		      sizeof (struct fileheader));
	updatelastpost("allarticle");
}

int
post_article(char *board, char *title, char *file, char *id,
	     char *nickname, char *ip, int sig, int mark,
	     int outgoing, char *realauthor, int thread)
{
	FILE *fp, *fp2;
	char buf3[1024];
	struct fileheader header;
	int t;
	bzero(&header, sizeof (header));
	if (strcasecmp(id, "Anonymous"))
		fh_setowner(&header, id, 0);
	else
		fh_setowner(&header, realauthor, 1);
	setbfile(buf3, board, "");
	t = trycreatefile(buf3, "M.%d.A", now_t, 100);
	if (t < 0)
		return -1;
	header.filetime = t;
	strsncpy(header.title, title, sizeof (header.title));
	header.accessed |= mark;
	if (outgoing)
		header.accessed |= FH_INND;
	fp = fopen(buf3, "w");
	if (NULL == fp)
		return -1;
	fp2 = fopen(file, "r");
	fprintf(fp,
		"发信人: %s (%s), 信区: %s\n标  题: %s\n发信站: %s (%24.24s), %s)\n\n",
		id, nickname, board, title, BBSNAME, Ctime(now_t),
		outgoing ? "转信(" MY_BBS_DOMAIN : "本站(" MY_BBS_DOMAIN);
	if (fp2 != 0) {
		while (1) {
			int retv;
			retv = fread(buf3, 1, sizeof (buf3), fp2);
			if (retv <= 0)
				break;
			fwrite(buf3, 1, retv, fp);
		}
		fclose(fp2);
	}
	fprintf(fp, "\n--\n");
	sig_append(fp, id, sig);
	fprintf(fp, "\033[m\033[1;%dm※ 来源:．%s %s [FROM: %.20s]\033[m\n",
		31 + rand() % 7, BBSNAME, "http://" MY_BBS_DOMAIN, ip);
	fclose(fp);
	sprintf(buf3, "boards/%s/M.%d.A", board, t);
	header.sizebyte = numbyte(eff_size(buf3));
	if (eff_size_isjunk)
		header.accessed |= FH_DEL;
	if (thread == -1)
		header.thread = header.filetime;
	else
		header.thread = thread;
	setbfile(buf3, board, ".DIR");
	append_record(buf3, &header, sizeof (header));
	if (outgoing && innd_board(board))
		outgo_post(&header, board, id, nickname);
	updatelastpost(board);
	sprintf(buf3, "boards/%s/M.%d.A", board, t);
	if (!hideboard(board))
		log_article(&header, buf3, board);
	return t;
}

int
securityreport(char *title, char *content)
{
	char fname[STRLEN];
	FILE *se;
	sprintf(fname, "bbstmpfs/tmp/security.%s.%05d", currentuser->userid,
		getpid());
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "系统安全记录系统\n原因：\n%s\n", content);
		fprintf(se, "以下是部分个人资料\n");
		fprintf(se, "光临机器: %s", realfromhost);
		fclose(se);
		post_article("syssecurity", title, fname, currentuser->userid,
			     currentuser->username, realfromhost, -1, 0, 0,
			     currentuser->userid, -1);
		unlink(fname);
	}
	return 0;
}

char *
anno_path_of(char *board)
{
	struct boardmem *bp;
	static char buf[256];
	bp = getbcache(board);
	if (!bp)
		return "";
	snprintf(buf, sizeof (buf), "/groups/GROUP_%c/%s",
		 bp->header.sec1[0], bp->header.filename);
	return buf;
}

int
has_BM_perm(struct userec *user, struct boardmem *x)
{
	if (user_perm(user, PERM_BLEVELS))
		return 1;
	if (!user_perm(user, PERM_BOARDS))
		return 0;
	if (x == 0)
		return 0;
	return chk_BM(user, &(x->header), 0);
}

//是否可以阅读版面文章  
int
has_read_perm(struct userec *user, char *board)
{
	return has_read_perm_x(user, getbcache(board));
}

int
has_read_perm_x(struct userec *user, struct boardmem *x)
{
	/* 版面不存在返回0, p和z版面返回1, 有权限版面返回1. */
	char fn[256];
	if (x == 0)
		return 0;
	if (x->header.flag & CLOSECLUB_FLAG) {
		if (!loginok && !tempuser)
			return 0;
		sprintf(fn, "boards/%s/club_users", x->header.filename);
		return file_has_word(fn, user->userid);
	}
	if (x->header.level == 0)
		return 1;
	if (x->header.level & (PERM_POSTMASK | PERM_NOZAP))
		return 1;
	if (!user_perm(user, PERM_BASIC))
		return 0;
	if (user_perm(user, x->header.level))
		return 1;
	return 0;
}

//是否可以看到版面名称；不一定可以看到版面文章
int
has_view_perm(struct userec *user, char *board)
{
	return has_view_perm_x(user, getbcache(board));
}

int
has_view_perm_x(struct userec *user, struct boardmem *x)
{
	/* 版面不存在返回0, p和z版面返回1, 有权限版面返回1. */
	char fn[256];
	if (x == 0)
		return 0;
	if (x->header.flag & CLOSECLUB_FLAG) {
		if (!loginok && !tempuser)
			return 0;
		if (x->header.flag & CLUBLEVEL_FLAG) {
			sprintf(fn, "boards/%s/club_users", x->header.filename);
			return file_has_word(fn, user->userid);
		} else
			return 1;
	}
	if (x->header.level == 0)
		return 1;
	if (x->header.level & (PERM_POSTMASK | PERM_NOZAP))
		return 1;
	if (!user_perm(user, PERM_BASIC))
		return 0;
	if (user_perm(user, x->header.level))
		return 1;
	return 0;
}

int
hideboard(char *bname)
{
	struct boardmem *x;
	if (bname[0] <= 32)
		return 1;
	x = getbcache(bname);
	if (x == 0)
		return 0;
	return hideboard_x(x);
}

int
hideboard_x(struct boardmem *x)
{
	if (x->header.flag & CLOSECLUB_FLAG)
		return 1;
	if (x->header.level & PERM_NOZAP)
		return 0;
	return (x->header.level & PERM_POSTMASK) ? 0 : x->header.level;
}

int
innd_board(char *bname)
{
	struct boardmem *x;
	x = getbcache(bname);
	if (x == 0)
		return 0;
	return (x->header.flag & INNBBSD_FLAG);
}

int
njuinn_board(char *bname)
{
	struct boardmem *x;
	x = getbcache(bname);
	if (x == 0)
		return 0;
	return (x->header.flag2 & NJUINN_FLAG);
}

int
political_board(char *bname)
{
	struct boardmem *x;
	x = getbcache(bname);
	if (x == 0)
		return 0;
	if (x->header.flag & POLITICAL_FLAG)
		return 1;
	else
		return 0;
}

int
anony_board(char *bname)
{
	struct boardmem *x;
	x = getbcache(bname);
	if (x == 0)
		return 0;
	return (x->header.flag & ANONY_FLAG);
}

int
noadm4political(bname)
char *bname;
{
	if (!shm_utmp->watchman || now_t < shm_utmp->watchman)
		return 0;
	return political_board(bname);

}

int
has_post_perm(struct userec *user, struct boardmem *x)
{
	char buf3[256];
	if (!loginok || isguest || !x || !has_read_perm_x(user, x))
		return 0;

	sprintf(buf3, "boards/%s/deny_users", x->header.filename);
	if (file_has_word(buf3, user->userid))
		return 0;
	sprintf(buf3, "boards/%s/deny_anony", x->header.filename);
	if (file_has_word(buf3, user->userid))
		return 0;
	if (!strcasecmp(x->header.filename, DEFAULTBOARD))
		return 1;
	if (user_perm(user, PERM_SYSOP))
		return 1;
	if (!user_perm(user, PERM_BASIC))
		return 0;
	if (!user_perm(user, PERM_POST))
		return 0;
	if (!strcasecmp(x->header.filename, "Arbitration"))
		return 1;	//封全站不包含Arbitration
	if (file_has_word("deny_users", user->userid))
		return 0;
	if (!(x->header.level & PERM_NOZAP) && x->header.level
	    && !user_perm(user, x->header.level))
		return 0;
	if (x->header.flag & CLUB_FLAG) {
		sprintf(buf3, "boards/%s/club_users", x->header.filename);
		if (file_has_word(buf3, user->userid))
			return 1;
		else
			return 0;
	}
	return 1;
}

int
has_vote_perm(struct userec *user, struct boardmem *x)
{
	char buf3[256];
	if (!loginok || isguest || !x || !has_read_perm_x(user, x))
		return 0;
	if (!strcasecmp(x->header.filename, "sysop"))
		return 1;
	if (user_perm(user, PERM_SYSOP))
		return 1;
	if (!user_perm(user, PERM_BASIC))
		return 0;
	if (!user_perm(user, PERM_POST))
		return 0;
	if (x->header.flag & CLUB_FLAG) {
		sprintf(buf3, "boards/%s/club_users", x->header.filename);
		if (file_has_word(buf3, user->userid))
			return 1;
		else
			return 0;
	}
	if (!(x->header.level & PERM_NOZAP) && x->header.level
	    && !user_perm(user, x->header.level))
		return 0;
	return 1;
}

struct boardmem *
numboard(const char *numstr)
{
	int n;
	if (!isdigit(*numstr))
		return NULL;
	n = atoi(numstr);
	if (n < 0 || n >= MAXBOARD || n >= shm_bcache->number)
		return NULL;
	if (!shm_bcache->bcache[n].header.filename[0])
		return NULL;
	return &shm_bcache->bcache[n];
}

#if defined(ENABLE_GHTHASH) && defined(ENABLE_FASTCGI)
struct boardmem *
getbcache(const char *board)
{
	int i, j;
	static int num = 0;
	char upperstr[STRLEN];
	static ght_hash_table_t *p_table = NULL;
	static time_t uptime = 0;
	struct boardmem *ptr;
	if (board[0] == 0)
		return 0;
	if (p_table
	    && (num != shm_bcache->number || shm_bcache->uptime > uptime)) {
		//errlog("getbcache: num %d, bcache->number %d, would reload",
		//       num, shm_bcache->number);
		ght_finalize(p_table);
		p_table = NULL;
	}
	if (p_table == NULL) {
		num = 0;
		p_table = ght_create(MAXBOARD);
		for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++) {
			num++;
			if (!shm_bcache->bcache[i].header.filename[0])
				continue;
			strsncpy(upperstr,
				 shm_bcache->bcache[i].header.filename,
				 sizeof (upperstr));
			for (j = 0; upperstr[j]; j++)
				upperstr[j] = toupper(upperstr[j]);
			ght_insert(p_table, &shm_bcache->bcache[i], j,
				   upperstr);
		}
		uptime = now_t;
	}
	strsncpy(upperstr, board, sizeof (upperstr));
	for (j = 0; upperstr[j]; j++)
		upperstr[j] = toupper(upperstr[j]);
	ptr = ght_get(p_table, j, upperstr);
	if (ptr)
		return ptr;
	return numboard(board);
}
#else
struct boardmem *
getbcache(const char *board)
{
	int i;
	if (board[0] == 0)
		return 0;

	for (i = 0; i < MAXBOARD && i < shm_bcache->number; i++)
		if (!strcasecmp(board, shm_bcache->bcache[i].header.filename))
			return &shm_bcache->bcache[i];
	return numboard(board);
}
#endif

int
getbnumx(struct boardmem *x1)
{
	if (!x1)
		return -1;
	return x1 - (struct boardmem *) (shm_bcache);
}

int
getbnum(char *board)
{
	struct boardmem *x1 = getbcache(board);
	if (!x1)
		return -1;
	return x1 - (struct boardmem *) (shm_bcache);
}

struct boardmem *
getboard(char board[80])
{
	struct boardmem *x1;
	x1 = getbcache(board);
	if (x1 == 0)
		return NULL;
	if (!has_read_perm_x(currentuser, x1))
		return NULL;
	strcpy(board, x1->header.filename);
	return x1;
}

struct boardmem *
getboard2(char board[80])
{
	struct boardmem *x1;
	x1 = getbcache(board);
	if (x1 == 0)
		return NULL;
	if (!has_view_perm_x(currentuser, x1))
		return NULL;
	strcpy(board, x1->header.filename);
	return x1;
}

int
send_msg(char *myuserid, int i, char *touserid, int topid, char *msg,
	 int offline)
{
	struct msghead head, head2;
	head.time = now_t;
	head.sent = 0;
	head.mode = 5;		//mode 5 for www msg
	strncpy(head.id, currentuser->userid, IDLEN + 2);
	head.frompid = 1;
	head.topid = topid;
	memcpy(&head2, &head, sizeof (struct msghead));
	head2.sent = 1;
	strncpy(head2.id, touserid, IDLEN + 2);

	if (save_msgtext(touserid, &head, msg) < 0)
		return -2;
	if (save_msgtext(currentuser->userid, &head2, msg) < 0)
		return -2;
	if (offline)
		return 1;
	if (topid != 1)
		kill(topid, SIGUSR2);
	else
		shm_utmp->uinfo[i].unreadmsg++;
	return 1;
}

int
user_perm(struct userec *x, int level)
{
	return (x->userlevel & level);
}

int
count_online()
{
	return shm_utmp->activeuser;
}

struct override fff[MAXFRIENDS];
int friendnum = 0;
int
loadfriend(char *id)
{
	FILE *fp;
	char file[256];
	sethomefile(file, id, "friends");
	fp = fopen(file, "r");
	friendnum = 0;
	if (fp) {
		friendnum = fread(fff, sizeof (fff[0]), MAXFRIENDS, fp);
		fclose(fp);
	}
	return 0;
}

static int
cmpfuid(a, b)
unsigned *a, *b;
{
	return *a - *b;
}

int
initfriends(struct user_info *u)
{
	int i, fnum = 0;
	char buf[128];
	FILE *fp;
	memset(u->friend, 0, sizeof (u->friend));
	sethomefile(buf, u->userid, "friends");
	u->fnum = file_size(buf) / sizeof (struct override);
	if (u->fnum <= 0)
		return 0;
	u->fnum = (u->fnum >= MAXFRIENDS) ? MAXFRIENDS : u->fnum;
	loadfriend(u->userid);
	for (i = 0; i < u->fnum; i++) {
		u->friend[i] = getuser(fff[i].id, NULL);
		if (u->friend[i])
			fnum++;
		else
			fff[i].id[0] = 0;
	}
	qsort(u->friend, u->fnum, sizeof (u->friend[0]), (void *) cmpfuid);
	if (fnum == u->fnum)
		return fnum;
	fp = fopen(buf, "w");
	for (i = 0; i < u->fnum; i++) {
		if (fff[i].id[0])
			fwrite(&(fff[i]), sizeof (struct override), 1, fp);
	}
	fclose(fp);
	u->fnum = fnum;
	return fnum;
}

int
isfriend(char *id)
{
	int n, num;
	if (!loginok || isguest)
		return 0;
	if ((num = getuser(id, NULL)) <= 0)
		return 0;
	for (n = 0; n < u_info->fnum; n++)
		if (num == u_info->friend[n])
			return 1;
	return 0;
}

struct override bbb[MAXREJECTS];
int badnum = 0;
int
loadbad(char *id)
{
	FILE *fp;
	char file[256];
	sethomefile(file, id, "rejects");
	fp = fopen(file, "r");
	if (fp) {
		badnum = fread(fff, sizeof (fff[0]), MAXREJECTS, fp);
		fclose(fp);
	}
	return 0;
}

int
isbad(char *id)
{
	int n;
	if (!loginok || isguest)
		return 0;
	loadbad(currentuser->userid);
	for (n = 0; n < badnum; n++)
		if (!strcasecmp(id, bbb[n].id))
			return 1;
	return 0;
}

void
changemode(int mode)
{
	if (!loginok || u_info->mode == mode)
		return;
	u_info->mode = mode;
	if (mode != BACKNUMBER && mode != READING && mode != POSTING)
		updatelastboard();
	return;
}

char *
flag_str_bm(int access)
{
	static char buf[80];
	strcpy(buf, "  ");
	if (access & FH_DIGEST)
		buf[0] = 'G';
	if (access & FH_MARKED)
		buf[0] = 'M';
	if ((access & FH_MARKED) && (access & FH_DIGEST))
		buf[0] = 'B';
	if (access & FH_ATTACHED)
		buf[1] = '@';
	if (access & FH_DEL)
		buf[0] = 'T';
	return buf;
}

char *
flag_str(int access)
{
	static char buf[80];
	strcpy(buf, "  ");
	if (access & FH_DIGEST)
		buf[0] = 'G';
	if (access & FH_MARKED)
		buf[0] = 'M';
	if ((access & FH_MARKED) && (access & FH_DIGEST))
		buf[0] = 'B';
	if (access & FH_ATTACHED)
		buf[1] = '@';
	return buf;
}

char *
flag_str2(int access, int has_read)
{
	static char buf[3];
	buf[0] = 'N';
	buf[1] = 0;
	buf[2] = 0;
	if (access) {
		if ((access & FH_MARKED) && (access & FH_DIGEST))
			buf[0] = 'B';
		else if (access & FH_DIGEST)
			buf[0] = 'G';
		else if (access & FH_MARKED)
			buf[0] = 'M';
		if (access & FH_ATTACHED)
			buf[1] = '@';
	}
	if (has_read)
		buf[0] = tolower(buf[0]);
	if (buf[0] == 'n') {
		buf[0] = buf[1];
		buf[1] = 0;
	}
	return buf;
}

char *
userid_str(char *s)
{
	static char buf[512];
	char buf2[256], tmp[256], *ptr, *ptr2;
	strsncpy(tmp, s, 255);
	buf[0] = 0;
	ptr = strtok(tmp, " ,();\r\n\t");
	while (ptr && strlen(buf) < 400) {
		if ((ptr2 = strchr(ptr, '.'))) {
			ptr2[1] = 0;
			strcat(buf, ptr);
		} else {
			//ptr = nohtml(ptr);
			sprintf(buf2, "<a href=qry?U=%s>%s</a>", ptr, ptr);
			strcat(buf, buf2);
		}
		ptr = strtok(0, " ,();\r\n\t");
		if (ptr)
			strcat(buf, " ");
	}
	return buf;
}

int
set_my_cookie()
{
	char buf[256];
	w_info->t_lines = 20;
	if (readuservalue(currentuser->userid, "t_lines", buf, sizeof (buf)) >=
	    0)
		w_info->t_lines = atoi(buf);
	if (readuservalue(currentuser->userid, "link_mode", buf, sizeof (buf))
	    >= 0)
		w_info->link_mode = atoi(buf);
	if (readuservalue(currentuser->userid, "def_mode", buf, sizeof (buf)) >=
	    0)
		w_info->def_mode = atoi(buf);
	if (readuservalue(currentuser->userid, "edit_mode", buf, sizeof (buf))
	    >= 0)
		w_info->edit_mode = atoi(buf);
/*	if (readuservalue(currentuser->userid, "att_mode", buf, sizeof (buf)) >=
	    0) w_info->att_mode = atoi(buf);
*/
	w_info->att_mode = 0;
	w_info->doc_mode = 1;
	if (w_info->t_lines < 10 || w_info->t_lines > 40)
		w_info->t_lines = 20;
	return 0;
}

int
cmpboard(b1, b2)
struct boardmem **b1, **b2;
{
	return strcasecmp((*b1)->header.filename, (*b2)->header.filename);
}

int
cmpboardscore(b1, b2)
struct boardmem **b1, **b2;
{
	return ((*b2)->score - (*b1)->score);
}

int
cmpboardinboard(b1, b2)
struct boardmem **b1, **b2;
{
	return ((*b2)->inboard - (*b1)->inboard);
}

int
cmpuser(a, b)
struct user_info **a, **b;
{
	return strcasecmp((*a)->userid, (*b)->userid);
}

char *
utf8_decode(char *src)
{
	static iconv_t cd = (iconv_t) - 1;
	static char out[2048];
	char *outbuf;
	char *inbuf;
	int flen, tlen, n;
	if (cd == (iconv_t) - 1) {
		cd = iconv_open("GB2312", "UTF-8");
		if (cd == (iconv_t) - 1)
			return src;
	}
	flen = strlen(src);
	if (flen > 2000)
		return src;
	tlen = flen;
	outbuf = &(out[0]);
	inbuf = src;
	n = iconv(cd, &inbuf, &flen, &outbuf, &tlen);
	if (n != (size_t) - 1) {
		*outbuf = 0;
		return out;
	} else
		return src;
}

char mybrd[GOOD_BRC_NUM][80];
int mybrdnum = 0;
void
fdisplay_attach(FILE * output, FILE * fp, char *currline, char *nowfile)
{
	char buf[1024], *attachfile, *download, *ext, *ptr;
	int pic;
	static int ano = 0;
	off_t size;
	if (NULL == fp) {
		ano = 0;
		return;
	}
	strncpy(buf, currline, sizeof (buf));
	buf[1023] = 0;
	attachfile = buf + 10;
	ptr = attachfile;
	while (*ptr) {
		if (*ptr == '\n' || *ptr == '\r') {
			*ptr = 0;
			break;
		}
		if ((*ptr > 0 && *ptr < ' ') || isspace(*ptr)
		    || strchr("\\/~`!@#$%^&*()|{}[];:\"'<>,?", *ptr)) {
			*ptr = '_';
		}
		ptr++;
	}
	if (strlen(attachfile) < 2)
		return;

	download = attachdecode(FCGI_ToFILE(fp), nowfile, attachfile);
	if (download == NULL) {
		fprintf(output, "不能正确解码的附件内容...");
		return;
	}
	if ((ext = strrchr(attachfile, '.')) != NULL) {
		if (!strcasecmp(ext, ".bmp") || !strcasecmp(ext, ".jpg")
		    || !strcasecmp(ext, ".gif")
		    || !strcasecmp(ext, ".jpeg")
		    || !strcasecmp(ext, ".png")
		    || !strcasecmp(ext, ".pcx"))
			pic = 1;
		else if (!strcasecmp(ext, ".swf"))
			pic = 2;
		else
			pic = 0;
	} else
		pic = 0;
	size = file_size(download);
	if ((ext = strrchr(download, ':')) != NULL)
		*ext = '/';
	download += sizeof (ATTACHCACHE);
	switch (pic) {
	case 1:
		fprintf
		    (output,
		     "%d 附图: %s (%ld 字节)<br><img src='/attach/%s'></img>",
		     ++ano, attachfile, size, download);
		break;
	case 2:
		fprintf(output,
			"%d Flash动画: <a href='/attach/%s'>%s</a> (%ld 字节)<br>"
			"<OBJECT><PARAM NAME='MOVIE' VALUE='/attach/%s'>"
			"<EMBED SRC='/attach/%s'></EMBED></OBJECT>",
			++ano, download, attachfile, size, download, download);
		break;
	default:
		fprintf
		    (output,
		     "%d 附件: <a href='/attach/%s'>%s</a> (%ld 字节)",
		     ++ano, download, attachfile, size);
	}
	if (quote_quote)
		fprintf(output, "\\");
}

void
printhr()
{
	printf("<div class=line></div>");
}

void
updatelastboard(void)
{
	struct boardmem *last;
	if (u_info->curboard) {
		last = &(shm_bcache->bcache[u_info->curboard - 1]);
		if (last->inboard > 0)
			last->inboard--;
		if (now_t > w_info->lastinboardtime
		    && w_info->lastinboardtime != 0)
			tracelog("%s use %s %ld", currentuser->userid,
				 last->header.filename,
				 (long int) (now_t - w_info->lastinboardtime));
		else
			tracelog("%s use %s 1", currentuser->userid,
				 last->header.filename);
		u_info->curboard = 0;
	}
}

void
updateinboard(struct boardmem *x)
{
	int bnum;
	if (!loginok)
		return;
	bnum = x - (struct boardmem *) (shm_bcache);
	if (bnum + 1 == u_info->curboard)
		return;
	updatelastboard();
	u_info->curboard = bnum + 1;
	w_info->lastinboardtime = now_t;
	x->inboard++;
	return;
}

double *
system_load()
{
	static double load[3] = {
		0, 0, 0
	};
#if defined(LINUX) || defined(BSD44) || defined(CYGWIN)
#ifndef	CYGWIN
	getloadavg(load, 3);
#endif				//CYGWIN
#else
	struct statstime rs;
	rstat("localhost", &rs);
	load[0] = rs.avenrun[0] / (double) (1 << 8);
	load[1] = rs.avenrun[1] / (double) (1 << 8);
	load[2] = rs.avenrun[2] / (double) (1 << 8);
#endif
	return load;
}

int
setbmstatus(struct userec *u, int online)
{
	return dosetbmstatus(shm_bcache->bcache, u->userid, online,
			     u_info->invisible);
}

int
count_uindex(int uid)
{
	int i, uent, count = 0;
	struct user_info *uentp;
	if (uid <= 0 || uid > MAXUSERS)
		return 0;
	for (i = 0; i < 6; i++) {
		uent = uindexshm->user[uid - 1][i];
		if (uent <= 0)
			continue;
		uentp = &(shm_utmp->uinfo[uent - 1]);
		if (!uentp->active || !uentp->pid || uentp->uid != uid)
			continue;
		if (uentp->pid > 1 && kill(uentp->pid, 0) < 0) {
			uindexshm->user[uid - 1][i] = 0;
			continue;
		}
		count++;
	}
	return count;
}

int
dofilter(char *title, char *fn, int level)
{
	struct mmapfile *mb;
	char *bf;
	switch (level) {
	case 1:
		mb = &mf_badwords;
		bf = BADWORDS;
		break;
	case 0:
		mb = &mf_sbadwords;
		bf = SBADWORDS;
		break;
	case 2:
		mb = &mf_pbadwords;
		bf = PBADWORDS;
		break;
	default:
		return 1;
	}
	if (mmapfile(bf, mb) < 0)
		goto CHECK2;

	if (filter_article(title, fn, mb)) {
		if (level != 2)
			return 1;
		return 2;
	}
      CHECK2:
	if (level != 1)
		return 0;
	mb = &mf_pbadwords;
	bf = PBADWORDS;
	if (mmapfile(bf, mb) < 0)
		return 0;
	if (filter_article(title, fn, mb))
		return 2;
	else
		return 0;
}

int
dofilter_edit(char *title, char *buf, int level)
{
	struct mmapfile *mb;
	char *bf;
	switch (level) {
	case 1:
		mb = &mf_badwords;
		bf = BADWORDS;
		break;
	case 0:
		mb = &mf_sbadwords;
		bf = SBADWORDS;
		break;
	case 2:
		mb = &mf_pbadwords;
		bf = PBADWORDS;
		break;
	default:
		return 1;
	}
	if (mmapfile(bf, mb) < 0)
		goto CHECK2;

	if (filter_string(buf, mb)
	    || filter_string(title, mb)) {
		if (level != 2)
			return 1;
		return 2;
	}
      CHECK2:
	if (level != 1)
		return 0;
	mb = &mf_pbadwords;
	bf = PBADWORDS;
	if (mmapfile(bf, mb) < 0)
		return 0;
	if (filter_string(buf, mb)
	    || filter_string(title, mb))
		return 2;
	else
		return 0;
}

int
search_filter(char *pat1, char *pat2, char *pat3)
{
	if (mmapfile(BADWORDS, &mf_badwords) < 0)
		return 0;
	if (filter_string(pat1, &mf_badwords)
	    || filter_string(pat2, &mf_badwords)
	    || filter_string(pat3, &mf_badwords)) {
		return -1;
	}
	return 0;
}

void
ansi2ubb(const char *s, int size, FILE * fp)
{
	int i = 0, m, c, n;
	char ansibuf[80];
	char *tmp;
	char ch;
	//int flag = 0; // void1
	for (; i < size; i++) {
		/* compatible with "void1" */
		ch = s[i];
		/*if (i == 0)
		   if ((unsigned char)s[0] >= 128)
		   flag = 1;
		   if (!flag) {
		   if (i < size - 1)
		   if ((unsigned char)s[i + 1] >= 128)
		   flag = 1;
		   } else {
		   flag = 0;
		   if (i < size - 1) {
		   if ((unsigned char)s[i + 1] < 32)
		   ch = 32;
		   } else if (i == size - 1)
		   ch = 32;
		   } */
		/* end void1 */
		/* compatible with "nohtml" */
		if (ch == '<') {
			fputs("&lt;", fp);
			continue;
		}
		if (ch == '>') {
			fputs("&gt;", fp);
			continue;
		}
		if (ch == '&') {
			fputs("&amp;", fp);
			continue;
		}
		/* end nohtml */
		if (ch == '\033') {
			if (s[i + 1] != '[') {
				fputc(ch, fp);
				continue;
			}
			for (m = i + 2; (m < size) && (m < i + 24); m++)
				if (strchr("0123456789;", s[m]) == 0)
					break;
			strsncpy(ansibuf, &s[i + 2], m - (i + 2) + 1);
			if (s[m] != 'm') {
				fputc(ch, fp);
				continue;
			}
			if (!useubb) {
				if (!strcmp(ansibuf, "998")) {
					useubb = 1;
					fputs("[ubb on]", fp);
					i = m;
					continue;
				} else {
					fputc(ch, fp);
					continue;
				}
			}
			i = m;
			if (ansibuf[0] == 0) {
				if (highlight) {
					//fputs("[normal]", fp);
					highlight = 0;
				}
				if (underline) {
					fputs("[/underline]", fp);
					underline = 0;
				}
				fputs("[/color]", fp);
			}
			tmp = strtok(ansibuf, ";");
			while (tmp) {
				n = atoi(tmp);
				if (n == 999) {
					useubb = 0;
					fputs("[ubb off]", fp);
					tmp = strtok(0, ";");
					continue;
				}
				if (n == 0) {
					if (highlight) {
						//fputs("[normal]", fp);
						highlight = 0;
					}
					if (underline) {
						fputs("[/underline]", fp);
						underline = 0;
					}
					fputs("[/color]", fp);
					tmp = strtok(0, ";");
					continue;
				}
				if (n == 1) {
					fputs("[highlight]", fp);
					highlight = 1;
					tmp = strtok(0, ";");
					continue;
				}
				if (n == 2) {
					fputs("[normal]", fp);
					highlight = 0;
					tmp = strtok(0, ";");
					continue;
				}
				if ((n >= 30) && (n <= 37)) {
					fputs(ubb2[n - 30].ubb, fp);
					tmp = strtok(0, ";");
					continue;
				} else if ((n >= 40) && (n <= 47)) {
					fputs(ubb2[n - 32].ubb, fp);
					tmp = strtok(0, ";");
					continue;
				}
				c = UBB_BEGIN_OTHER;
				while (ubb2[c].ansi[0]) {
					if (!strncmp(tmp, ubb2[c].ansi, 8)) {
						fputs(ubb2[c].ubb, fp);
						break;
					}
					c++;
				}
				if (c == UBB_BEGIN_UNDERLINE)
					underline = 1;
				tmp = strtok(0, ";");
			}
		} else
			fputc(ch, fp);
	}
}

int
ubb2ansi(const char *s, const char *fn)
{
	FILE *fp;
	int i, m, c;
	char ubbtag[64];
	fp = fopen(fn, "w");
	underline = 0;
	highlight = 0;
	lastcolor = 37;
	useubb = 1;
	if (!fp)
		return -1;
	for (i = 0; s[i]; i++) {
		if (s[i] != '[') {
			fputc(s[i], fp);
			continue;
		}
		for (m = 0; s[i + m + 1] && (m < 60); m++) {
			if (s[i + m + 1] != ']')
				ubbtag[m] = s[i + m + 1];
			else
				break;
		}
		ubbtag[m] = 0;
		if (s[i + m + 1] != ']') {
			fputc(s[i], fp);
			continue;
		}
		if (!useubb) {
			if (!strcmp(ubbtag, ubb[UBB_END_NOUBB].ubb)) {
				i += m + 1;
				useubb = 1;
				fputs(ubb[UBB_END_NOUBB].ansi, fp);
				continue;
			} else {
				fputc(s[i], fp);
				continue;
			}
		}
		c = 0;
		while (ubb[c].ubb[0]) {
			if (!strncmp(ubb[c].ubb, ubbtag, 32)) {
				i += m + 1;
				fputs(ubb[c].ansi, fp);
				if (c == UBB_BEGIN_NOUBB)
					useubb = 0;
				break;
			}
			c++;
		}
		if (ubb[c].ubb[0] == 0) {
			fputc(s[i], fp);
			continue;
		}
	}
	fclose(fp);
	return 0;
}

void
printubb(const char *form, const char *textarea)
{
	printf
	    ("使用UBB CODE<input type=checkbox name=useubb checked>&nbsp;<a href=home/boards/BBSHelp/html/ubb/ubbintro.htm target=_blank>UBB简介</a>\n");
	printf
	    ("<br>前景色:<select onchange='javascript:document.%s.%s.value+=this.value;document.%s.%s.focus();'><option value='[/color]' selected>默认<option value='[color=black]'>黑色</option></option><option value='[color=red]'>红色</option><option value='[color=green]'>绿色</option><option value='[color=yellow]'>黄色</option><option value='[color=blue]'>蓝色</option><option value='[color=purple]'>紫色</option><option value='[color=cyan]'>青色</option><option value='[color=white]'>白色</option></select>\n",
	     form, textarea, form, textarea);
	printf
	    ("&nbsp;背景色:<select onchange='javascript:document.%s.%s.value+=this.value;document.%s.%s.focus();'><option value='[/color]' selected>默认<option value='[bcolor=black]'>黑色</option></option><option value='[bcolor=red]'>红色</option><option value='[bcolor=green]'>绿色</option><option value='[bcolor=yellow]'>黄色</option><option value='[bcolor=blue]'>蓝色</option><option value='[bcolor=purple]'>紫色</option><option value='[bcolor=cyan]'>青色</option><option value='[bcolor=white]'>白色</option></select>\n",
	     form, textarea, form, textarea);
	printf
	    ("&nbsp;亮度:<select onchange='javascript:document.%s.%s.value+=this.value;document.%s.%s.focus();'><option value='[normal]' selected>默认亮度</option><option value='[highlight]'>高亮度</option></select>\n",
	     form, textarea, form, textarea);
	printf
	    ("&nbsp;<b>B</b>:<select onchange='javascript:document.%s.%s.value+=this.value;document.%s.%s.focus();'><option value='[/bold]' selected>常规</option><option value='[bold]'>加粗</option></select>\n",
	     form, textarea, form, textarea);
	printf
	    ("&nbsp;<i>I</i>:<select onchange='javascript:document.%s.%s.value+=this.value;document.%s.%s.focus();'><option value='[/italic]' selected>常规</option><option value='[italic]'>斜体</option></select>\n",
	     form, textarea, form, textarea);
	printf
	    ("&nbsp;<u>U</u>:<select onchange='javascript:document.%s.%s.value+=this.value;document.%s.%s.focus();'><option value='[/underline]' selected>常规</option><option value='[underline]'>底线</option></select>\n",
	     form, textarea, form, textarea);
}

int
mmap_getline(char *ptr, int max_size)
{
	int n = 0;
	while (n < max_size) {
		if (ptr[n] == '\n') {
			n++;
			break;
		}
		n++;
	}
	return n;
}

int
countln(fname)
char *fname;
{
	FILE *fp;
	char tmp[256];
	int count = 0;
	if ((fp = fopen(fname, "r")) == NULL)
		return 0;
	while (fgets(tmp, sizeof (tmp), fp) != NULL)
		count++;
	fclose(fp);
	return count;
}

#ifdef USESESSIONCOOKIE
char sessionCookie[STRLEN];
#endif

char *
makeurlbase(int uent, int uid)
{
	static char urlbase[STRLEN * 2];
	char *ptr;
	ptr = getparm("usehost");	//For https->http redirection
	if (strlen(ptr) >= STRLEN)
		ptr = "";
	//max 26*26*26-1 = 17575 online,enough?
	strcpy(urlbase, ptr);
	ptr = urlbase + strlen(urlbase);
#ifdef USESESSIONCOOKIE
	strcpy(ptr, "/" SMAGIC "/");
	if (uent >= 0) {
		myitoa(uent, sessionCookie, 3);
		myitoa(uid, sessionCookie + 3, 4);
		strcpy(sessionCookie + 7, shm_utmp->uinfo[uent].sessionid);
	} else {
		strcpy(sessionCookie, "");
	}
	addextraparam(sessionCookie, sizeof (sessionCookie), 0, wwwstylenum);
	//errlog("sessionCookie: %s", sessionCookie);
#else
	if (uent >= 0) {
		strcpy(ptr, "/" SMAGIC);
		myitoa(uent, ptr + sizeof (SMAGIC), 3);
		myitoa(uid, ptr + sizeof (SMAGIC) + 3, 4);
		strcpy(ptr + sizeof (SMAGIC) + 7,
		       shm_utmp->uinfo[uent].sessionid);
	} else
		strcpy(ptr, "/" SMAGIC);
	addextraparam(urlbase, sizeof (urlbase), 0, wwwstylenum);
	strcat(ptr, "/");
#endif
	return urlbase;
}

void
updatewwwindex(struct boardmem *x)
{
	char buf[STRLEN * 2];
	int size;
	if (now_t - x->wwwindext < 30)
		return;
	x->wwwindext = now_t;
	sprintf(buf, "ftphome/root/boards/%s/html/index.htm",
		x->header.filename);
	x->wwwindex = access(buf, R_OK) ? 0 : 1;
	sprintf(buf, "ftphome/root/boards/%s/html/icon.gif",
		x->header.filename);
	x->wwwicon = access(buf, R_OK) ? 0 : 1;
	size = file_size(buf);
	if (size > 10 && size < 12 * 1024)
		x->wwwicon = 1;
	else
		x->wwwicon = 0;
	sprintf(buf, "boards/.backnumbers/%s/.DIR", x->header.filename);
	x->wwwbkn = access(buf, R_OK) ? 0 : 1;
	sprintf(buf, "vote/%s/notes", x->header.filename);
	x->hasnotes = access(buf, R_OK) ? 0 : 1;
}

void
changeContentbg()
{
	contentbg = (contentbg + 1) % 2;
}
