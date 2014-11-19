#ifndef __BBSLIB_H
#define __BBSLIB_H
#include <sys/mman.h>
#include "ythtlib.h"
#include "ythtbbs.h"
#include "bbs.h"
#include "blog.h"
#ifndef ENABLE_FASTCGI
#define FCGI_ToFILE(x) (x)
#define FCGI_FILE FILE
#else
#include <fcgi_stdio.h>
#endif

#define CSS_FILE 	"/bbs.css"
#define BIG5FIRSTPAGE   "/" BASESMAGICBI5 "/bbsindex"

#ifndef USEBIG5
#define FIRST_PAGE      "/"
#define CHARSET		"GBK"
#define CSSPATH		"/"
#else
#define FIRST_PAGE      "/" SMAGIC "/bbsindex"
#define CHARSET		"Big5"
#define CSSPATH		"/big5/"
#endif

//#define HTTPS_DOMAIN	MY_BBS_DOMAIN
#define USESESSIONCOOKIE

#define BBSJS		CSSPATH "function.js"
#define BBSLEFTJS	CSSPATH "func.js"

#define NAVFILE		"nav.txt"
#define MAXWWWCLIENT MAXACTIVE
#define CACHE_ABLE	0x100

#define DEFAULT_PROXY_PORT 8080

struct wwwstyle {
	char *name;
	char *cssfile;
	char *leftcssfile;
};

#define NWWWSTYLE (6)
extern struct wwwstyle wwwstyle[];
extern struct wwwstyle *currstyle;
extern int wwwstylenum;
extern int contentbg;

//#define SECNUM 13
#define BBSNAME MY_BBS_NAME
#define BBSHOME MY_BBS_HOME
#define BBSHOST MY_BBS_DOMAIN
#define LDEB if(!strcmp(currentuser->userid,"lepton"))
#define PATHLEN         1024

#define cssVersion() file_time(HTMPATH "/cssVersion")

#define NTHREADPERPAGE 20
#define NPOSTPERPAGE 15

//extern const char seccodes[SECNUM];
//extern const char secname[SECNUM][2][20];
extern char needcgi[STRLEN];
extern char rframe[STRLEN];
extern time_t thisversion;
extern struct bbsinfo bbsinfo;

void getsalt(char *salt);	//生成密码加密用两字节salt
int junkboard(char *board);

extern int loginok;
extern int isguest;
extern int tempuser;
extern int utmpent;
extern volatile int incgiloop;
extern int thispid;
extern time_t now_t;
extern time_t starttime;
extern jmp_buf cgi_start;
extern struct userec *currentuser;
extern struct user_info *u_info;
extern struct wwwsession *w_info;
extern struct UTMPFILE *shm_utmp;
extern struct BCACHE *shm_bcache;
extern struct WWWCACHE *wwwcache;
extern struct UINDEX *uindexshm;
extern char fromhost[60];
extern char realfromhost[60];
extern struct in_addr from_addr;
extern int via_proxy;
extern int quote_quote;
extern struct userec *ummap_ptr;
extern int ummap_size;
extern char ATT_ADDRESS[20];
extern char ATT_PORT[10];
//#define redirect(x)	printf("<meta http-equiv='Refresh' content='0; url=%s'>\n", x)
#define redirect(x) printf("<meta http-equiv='Refresh' content='0; url=%s'>\n" \
		"<script>location.replace('%s');</script>", x, x);
//#define refreshto(x, t)	printf("<meta http-equiv='Refresh' content='%d; url=%s'>\n", t, x)
#define refreshto(x, t) printf("<meta http-equiv='Refresh' content='%d; url=%s'>\n" \
		"<script>setTimeout(\"location.replace('%s')\", %d);</script>", t, x, x, (t)*1000);
//#define cgi_head()	printf("Content-type: text/html; charset=%s\n\n", CHARSET)

struct parm_file {
	char *filename;
	int len;
	char *content;
};

extern char parm_name[256][80], *parm_val[256];
extern struct parm_file parm_valfile[256];
extern int parm_num;
extern struct override fff[MAXFRIENDS];
extern int friendnum;
extern struct override bbb[MAXREJECTS];
extern int badnum;

#define MAXHOTBOARD 30
struct hotboard {
	const struct sectree *sec;
	char bname[MAXHOTBOARD][24];
	char title[MAXHOTBOARD][24];
	int bnum[MAXHOTBOARD];
	int count;
	int max;
	time_t uptime;
};

struct deny {
	char id[80];
	char exp[80];
	int free_time;
};

struct cgi_applet {
	int (*main) (void);
	char *(name[5]);
	double utime;
	double stime;
	int count;
};

extern struct deny denyuser[256];
extern int denynum;
extern int nologin;
extern char mybrd[GOOD_BRC_NUM][80];
extern int mybrdnum;

struct emotion {
	char *(smilename[4]);
	char *filename;
};

/* begin UBB */
/* translate name to alias. For example, [color=red] => \033[31m */
struct UBB2ANSI {
	const char *ubb;
	const char *ansi;
};

/* translate alias back to name */
struct ANSI2HTML_UBB {
	const char *ansi;
	const char *html;
	const int htmllen;
	const char *ubb;
};

struct bbsfont {
	int color;
	int bgcolor;
	int underline;
	int bold;
	int italic;
	int hilight;
};

extern const struct UBB2ANSI ubb[];
extern const struct ANSI2HTML_UBB ubb2[];
extern int underline;
extern int highlight;
extern int lastcolor;
extern int useubb;
extern int noansi;
extern int usedMath;
extern int usingMath;
extern int withinMath;
extern int stripHeadTail;
/* end UBB */
extern char *specname[];
extern char decode64[256];
extern const char mybase64[65];
extern struct szm_ctx * tjpg_ctx;

#define FCGI_printf printf
int fhhprintf(FCGI_FILE *output, char *fmt, ...) __attribute__((format(printf, 2, 3)));
void http_fatal(char *fmt, ...) __attribute__((format(printf, 1, 2)));
int hprintf(char *fmt, ...) __attribute__((format(printf, 1, 2)));
#undef FCGI_printf

//outstr: 输出一个常量字符串
#define printstr(x) fwrite(x, sizeof(x)-1, 1, stdout)

#ifndef MAKE_PROTO
#include "proto.h"
#endif
#endif
