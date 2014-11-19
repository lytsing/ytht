#include "bbslib.h"
#if defined(ENABLE_GHTHASH) && defined(ENABLE_FASTCGI)
#include <ght_hash_table.h>
#endif

#ifndef ENABLE_FASTCGI
int looponce = 0;
#define FCGI_Accept() looponce--
#endif

struct cgi_applet applets[] = {
//      {bbsusr_main, {"bbsusr", NULL}},
	{bbsucss_main, {"bbsucss", NULL}},
	{bbsdefcss_main, {"bbsdefcss", NULL}},
	{bbstop10_main, {"bbstop10", NULL}},
	{bbsdoc_main, {"bbsdoc", "doc", NULL}},
	{bbscon_main, {"bbscon", "con", NULL}},
	{bbsbrdadd_main, {"bbsbrdadd", "brdadd", NULL}},
	{bbsboa_main, {"bbsboa", "boa", NULL}},
	{bbsall_main, {"bbsall", NULL}},
	{bbsanc_main, {"bbsanc", "anc", NULL}},
	{bbs0an_main, {"bbs0an", "0an", NULL}},
	{bbslogout_main, {"bbslogout", NULL}},
	{bbsleft_main, {"bbsleft", NULL}},
	{bbslogin_main, {"bbslogin", NULL}},
	{bbsbadlogins_main, {"bbsbadlogins", NULL}},
	{bbsqry_main, {"bbsqry", "qry", NULL}},
	{bbsnot_main, {"bbsnot", "not", NULL}},
	{bbsfind_main, {"bbsfind", NULL}},
	{bbsfadd_main, {"bbsfadd", NULL}},
	{bbsfdel_main, {"bbsfdel", NULL}},
	{bbsfall_main, {"bbsfall", NULL}},
	{bbsfriend_main, {"bbsfriend", NULL}},
	{bbsfoot_main, {"bbsfoot", NULL}},
	{bbsform_main, {"bbsform", NULL}},
	{bbspwd_main, {"bbspwd", NULL}},
	{bbsplan_main, {"bbsplan", NULL}},
	{bbsinfo_main, {"bbsinfo", NULL}},
	{bbsmypic_main, {"bbsmypic", "mypic", NULL}},
	{bbsmybrd_main, {"bbsmybrd", NULL}},
	{bbssig_main, {"bbssig", NULL}},
	{bbspst_main, {"bbspst", "pst", NULL}},
	{bbsgcon_main, {"bbsgcon", "gcon", NULL}},
	{bbsgdoc_main, {"bbsgdoc", "gdoc", NULL}},
	{bbsdel_main, {"bbsdel", "del", NULL}},
	{bbsdelmail_main, {"bbsdelmail", NULL}},
	{bbsmailcon_main, {"bbsmailcon", NULL}},
	{bbsmail_main, {"bbsmail", "mail", NULL}},
	{bbsdelmsg_main, {"bbsdelmsg", NULL}},
	{bbssnd_main, {"bbssnd", NULL}},
	{bbsnotepad_main, {"bbsnotepad", NULL}},
	{bbsmsg_main, {"bbsmsg", NULL}},
	{bbssendmsg_main, {"bbssendmsg", NULL}},
#ifdef ENABLE_EMAILREG
	{bbsreg_main, {"bbsreg", NULL}},
	{bbsemailreg_main, {"bbsemailreg", NULL}},
	{bbsemailconfirm_main, {"bbsemailconfirm", "emailconfirm", NULL}},
#else
	{bbsreg_main, {"bbsreg", "bbsemailreg", NULL}},
#endif
#ifdef ENABLE_INVITATION
	{bbsinvite_main, {"bbsinvite", "invite", NULL}},
	{bbsinviteconfirm_main, {"bbsinviteconfirm", "inviteconfirm", NULL}},
#endif
	{bbsscanreg_main, {"bbsscanreg", NULL}},
	{bbsmailmsg_main, {"bbsmailmsg", NULL}},
	{bbssndmail_main, {"bbssndmail", NULL}},
	{bbsnewmail_main, {"bbsnewmail", NULL}},
	{bbspstmail_main, {"bbspstmail", "pstmail", NULL}},
	{bbsgetmsg_main, {"bbsgetmsg", NULL}},
	//{bbschat_main, {"bbschat", NULL}},
	{bbscloak_main, {"bbscloak", NULL}},
	{bbsmdoc_main, {"bbsmdoc", "mdoc", NULL}},
	{bbsnick_main, {"bbsnick", NULL}},
	{bbstfind_main, {"bbstfind", "tfind", NULL}},
	{bbsadl_main, {"bbsadl", NULL}},
	{bbstcon_main, {"bbstcon", "tcon", NULL}},
	{bbstdoc_main, {"bbstdoc", "tdoc", NULL}},
	{bbsdoreg_main, {"bbsdoreg", NULL}},
	{bbsmywww_main, {"bbsmywww", NULL}},
	{bbsccc_main, {"bbsccc", "ccc", NULL}},
	//{bbsufind_main, {"bbsufind", NULL}},
	{bbsclear_main, {"bbsclear", "clear", NULL}},
	{bbsstat_main, {"bbsstat", NULL}},
	{bbsedit_main, {"bbsedit", "edit", NULL}},
	{bbsman_main, {"bbsman", "man", NULL}},
	{bbsparm_main, {"bbsparm", NULL}},
	{bbsfwd_main, {"bbsfwd", "fwd", NULL}},
	{bbsmnote_main, {"bbsmnote", NULL}},
	{bbsdenyall_main, {"bbsdenyall", NULL}},
	{bbsdenydel_main, {"bbsdenydel", NULL}},
	{bbsdenyadd_main, {"bbsdenyadd", NULL}},
	{bbstopb10_main, {"bbstopb10", NULL}},
	{bbsbfind_main, {"bbsbfind", "bfind", NULL}},
	{bbsx_main, {"bbsx", NULL}},
	{bbseva_main, {"bbseva", "eva", NULL}},
	{bbsvote_main, {"bbsvote", "vote", NULL}},
	{bbsshownav_main, {"bbsshownav", NULL}},
	{bbsbkndoc_main, {"bbsbkndoc", "bkndoc", NULL}},
	{bbsbknsel_main, {"bbsbknsel", "bknsel", NULL}},
	{bbsbkncon_main, {"bbsbkncon", "bkncon", NULL}},
	{bbshome_main, {"bbshome", "home", NULL}},
	{bbsindex_main, {"bbsindex", "index", NULL}},
//      {bbsupload_main, {"bbsupload", NULL}},
	{bbslform_main, {"bbslform", NULL}},
	{bbslt_main, {"bbslt", NULL}},
	{bbsdt_main, {"bbsdt", NULL}},
	{regreq_main, {"regreq", NULL}},
	{bbsselstyle_main, {"bbsselstyle", NULL}},
	{bbscon1_main, {"bbscon1", "c1", NULL}},
	{bbsattach_main, {"attach", NULL}},
	{bbskick_main, {"kick", NULL}},
	{bbsshowfile_main, {"bbsshowfile", "showfile", NULL}},
	{bbsincon_main, {"boards", NULL}},
	{bbssetscript_main, {"setscript", NULL}},
	{bbscccmail_main, {"bbscccmail", NULL}},
	{bbsfwdmail_main, {"bbsfwdmail", NULL}},
	{bbsscanreg_findsurname_main, {"bbsscanreg_findsurname", NULL}},
	{bbsnt_main, {"bbsnt", NULL}},
	{bbstopcon_main, {"bbstopcon", NULL}},
	{bbsdrawscore_main, {"bbsdrawscore", NULL}},
	{bbsmyclass_main, {"bbsmyclass", NULL}},
	{bbssearchboard_main, {"bbssearchboard", NULL}},
	{bbsrss_main, {"bbsrss", NULL}},
	{bbslastip_main, {"bbslastip", NULL}},
	{bbsself_photo_vote_main, {"selfvote", NULL}},
	{bbsspam_main, {"bbsspam", NULL}},
	{bbsspamcon_main, {"bbsspamcon", NULL}},
	{bbssouke_main, {"bbssouke", NULL}},
	{bbsboardlistscript_main,
	 {"bbsboardlistscript", "boardlistscript", NULL}},
	{bbsolympic_main, {"bbsolympic", "olympic", NULL}},
	{bbsicon_main, {"bbsicon", "icon", NULL}},
	{bbsnewattach_main, {"newattach", "b", NULL}},
	{bbstmpleft_main, {"bbstmpleft", "tmpleft", NULL}},
	{bbsdlprepare_main, {"bbsdlprepare", NULL}},
	{bbsupload_main, {"bbsupload", NULL}},
	{blogblog_main, {"blogblog", "blog", NULL}}, 
	{blogread_main, {"blogread", NULL}}, 
	{blogpost_main, {"blogpost", NULL}}, 
	{blogsend_main, {"blogsend", NULL}}, 
	{blogeditabstract_main, {"blogeditabstract", NULL}}, 
	{blogeditconfig_main, {"blogeditconfig", NULL}}, 
	{blogeditsubject_main, {"blogeditsubject", NULL}}, 
	{blogedittag_main, {"blogedittag", NULL}}, 
	{blogeditpost_main, {"blogeditpost", NULL}}, 
	{blogdraft_main, {"blogdraft", NULL}}, 
	{blogdraftread_main, {"blogdraftread", NULL}}, 
	{blogcomment_main, {"blogcomment", NULL}}, 
	{blogsetup_main, {"blogsetup", NULL}}, 
	{blogpage_main, {"blogpage", NULL}}, 
	{blogrss2_main, {"blogrss2", NULL}}, 
	{blogatom_main, {"blogatom", NULL}}, 
	{NULL}
};

#include <sys/times.h>
char *cginame = NULL;
static unsigned int rt = 0;
volatile int shouldExit = 0;
int accepting = 0;

void
logtimeused()
{
	char buf[1024];
	struct cgi_applet *a = applets;
	while (a->main) {
		sprintf(buf, "%s %d %f %f\n", a->name[0], a->count, a->utime,
			a->stime);
		f_append(MY_BBS_HOME "/gprof/cgirtime", buf);
		a++;
	}
}

static void
cgi_time(struct cgi_applet *a)
{
	static struct rusage r0, r1;
	getrusage(RUSAGE_SELF, &r1);

	if (a == NULL) {
		r0 = r1;
		return;
	}
	a->count++;
	a->utime +=
	    1000. * (r1.ru_utime.tv_sec - r0.ru_utime.tv_sec) +
	    (r1.ru_utime.tv_usec - r0.ru_utime.tv_usec) / 1000.;
	a->stime +=
	    1000. * (r1.ru_stime.tv_sec - r0.ru_stime.tv_sec) +
	    (r1.ru_stime.tv_usec - r0.ru_stime.tv_usec) / 1000.;
	r0 = r1;
}

void
wantquit(int signal)
{
	if (!accepting)
		shouldExit = 1;
	else
		exit(6);
}

#if defined(ENABLE_GHTHASH) && defined(ENABLE_FASTCGI)
struct cgi_applet *
get_cgi_applet(char *needcgi)
{
	static ght_hash_table_t *p_table = NULL;
	struct cgi_applet *a;

	if (p_table == NULL) {
		int i;
		a = applets;
		p_table = ght_create(250);
		while (a->main != NULL) {
			a->count = 0;
			a->utime = 0;
			a->stime = 0;
			for (i = 0; a->name[i] != NULL; i++)
				ght_insert(p_table, (void *) a,
					   strlen(a->name[i]),
					   (void *) a->name[i]);
			a++;
		}
	}
	if (p_table == NULL)
		return NULL;
	return ght_get(p_table, strlen(needcgi), needcgi);
}
#else
struct cgi_applet *
get_cgi_applet(char *needcgi)
{
	struct cgi_applet *a;
	int i;
	a = applets;
	while (a->main != NULL) {
		for (i = 0; a->name[i] != NULL; i++)
			if (!strcmp(needcgi, a->name[i]))
				return a;
		a++;
	}
	return NULL;
}
#endif

int nologin = 1;

#ifdef ENABLE_FASTCGI
void *myout;
#else
FILE *myout;
#endif

char *myoutbuf = NULL;
int myoutsize;
FILE oldout;

#ifdef USEBIG5
//don't delete this function although it is unused now
//it will perhaps be used in the furture.
void
start_outcache()
{
	if (myoutbuf) {
		free(myoutbuf);
		myoutbuf = NULL;
	}
#ifdef HAVE_OPEN_MEMSTREAM
	myout = open_memstream(&myoutbuf, &myoutsize);
#else				//HAVE_OPEN_MEMSTREAM
	myout = tmpfile();
#endif				//HAVE_OPEN_MEMSTREAM
	if (NULL == myout) {
		errlog("can't open mem stream...");
		exit(14);
	}
	oldout = *stdout;
#ifdef ENABLE_FASTCGI
	stdout->stdio_stream = myout;
	stdout->fcgx_stream = 0;
#else
	*stdout = *myout;
#endif
}

void
no_outcache()
{
	fclose(stdout);
	*stdout = oldout;
	if (myoutbuf) {
		free(myoutbuf);
		myoutbuf = NULL;
	}
}

void
end_outcache()
{
	char *ptr, *ptr2;
	int len, doconvert = 0;
#ifndef HAVE_OPEN_MEMSTREAM
	myoutsize = ftell(stdout);
	rewind(stdout);
	myoutbuf = (char *) malloc(myoutsize + 1);
	fread(myoutbuf, myoutsize, 1, stdout);
	myoutbuf[myoutsize] = '\0';
#endif				//HAVE_OPEN_MEMSTREAM
	fclose(stdout);
	*stdout = oldout;
	if (!myoutbuf)
		return;
	ptr = strstr(myoutbuf, "\r\n\r\n");
	if (!ptr) {
		printf("\r\n\r\nfaint! 出毛病了...");
		goto END;
	}
	if ((ptr2 = strnstr(myoutbuf, "Content-type: ", ptr - myoutbuf))) {
		len = myoutsize - (ptr - myoutbuf) - 4;
		printf("Content-Length: %d\r\n", len);
		if (strnstr(ptr2, "text", ptr - ptr2)
		    || strnstr(ptr2, "x-javascript", ptr - ptr2))
			doconvert = 1;
	}
	if (doconvert) {
		sgb2big(myoutbuf, myoutsize);
	}
	//fputs(myoutbuf, stdout);
	fwrite(myoutbuf, myoutsize, 1, stdout);
      END:
	free(myoutbuf);
	myoutbuf = NULL;
}
#endif				//USEBIG5

void
get_att_server()
{
	FILE *fp;
	char *ptr;
	char buf[128], *tmp;
	unsigned long accel_ip, text_accel_ip, accel_port, text_accel_port,
	    validproxy[MAX_PROXY_NUM];
	int i;
	struct in_addr proxy_ip;
	i = accel_ip = accel_port = text_accel_port = text_accel_ip = 0;
	bzero(validproxy, sizeof (validproxy));
	fp = fopen("ATT_SERVER", "r");
	if (!fp)
		goto END;

	while (fgets(buf, sizeof (buf), fp) && i < MAX_PROXY_NUM + 1) {
		if (buf[0] == '#')
			continue;
		strtok(buf, "\n");
		if (i) {
			if (inet_aton(buf, &proxy_ip)) {
				validproxy[i - 1] = proxy_ip.s_addr;
				i++;
			}
			continue;
		}
		ptr = strchr(buf, ':');
		if (ptr)
			*ptr = 0;
		if (!inet_aton(buf, &proxy_ip))
			continue;
		accel_ip = proxy_ip.s_addr;
		if (ptr)
			accel_port = atoi(ptr + 1);
		else
			accel_port = DEFAULT_PROXY_PORT;
		i++;
		tmp = strchr(ptr + 1, ' ');
		if (!tmp)
			continue;
		ptr = strchr(tmp + 1, ':');
		if (ptr)
			*ptr = 0;
		if (!inet_aton(tmp + 1, &proxy_ip))
			continue;
		text_accel_ip = proxy_ip.s_addr;
		if (ptr)
			text_accel_port = atoi(ptr + 1);
		else
			text_accel_port = DEFAULT_PROXY_PORT;

	}
	fclose(fp);
      END:
	if (!text_accel_ip || !text_accel_port) {
		text_accel_port = accel_port;
		text_accel_ip = accel_ip;
	}
	wwwcache->accel_addr.s_addr = accel_ip;
	wwwcache->accel_port = accel_port;
	wwwcache->text_accel_addr.s_addr = text_accel_ip;
	wwwcache->text_accel_port = text_accel_port;

	for (i = 0; i < MAX_PROXY_NUM; i++)
		wwwcache->validproxy[i] = validproxy[i];
}

time_t thisversion;
struct bbsinfo bbsinfo;
struct szm_ctx *tjpg_ctx;
int canResizePIC = 0;

static void
get_tjpg_server(void)
{
	FILE *fp;
	char buf[256];
	fp = fopen("TJPG_SERVER", "r");
	if (!fp)
		return;
	fgets(buf, sizeof (buf), fp);
	strtok(buf, "\n");
	fclose(fp);
	tjpg_ctx = szm_init(buf);
	mkdir("szmcache", 0700);
}

int
main(int argc, char *argv[])
{
	struct cgi_applet *a = NULL;
	struct rlimit rl;
	int i;
	struct in_addr in;
	seteuid(BBSUID);
	setuid(BBSUID);
	setgid(BBSGID);
	cgi_time(NULL);
	rl.rlim_cur = 100 * 1024 * 1024;
	rl.rlim_max = 100 * 1024 * 1024;
	setrlimit(RLIMIT_CORE, &rl);
	thispid = getpid();
	now_t = time(NULL);
	srand(now_t * 2 + thispid);
	if (initbbsinfo(&bbsinfo) < 0)
		exit(0);
	if (uhash_uptime() == 0) {
		exit(-1);
	}
	wwwcache = bbsinfo.wwwcache;
	thisversion = file_time(argv[0]);
#ifndef USEBIG5
	if (thisversion > wwwcache->www_version)
		wwwcache->www_version = thisversion;
#endif
	html_header(0);
	if (geteuid() != BBSUID)
		http_fatal("uid error.");
	chdir(BBSHOME);
	shm_init(&bbsinfo);
	signal(SIGTERM, wantquit);
	if (access("NOLOGIN", F_OK))
		nologin = 0;
	get_att_server();
	get_tjpg_server();
#ifdef ENABLE_FASTCGI
	atexit(FCGI_Finish);
#endif
	while (1) {
		accepting = 1;
		if (shouldExit || wwwcache->www_version > thisversion
		    || rt++ > 400000) {
			accepting = 0;
			break;
		}
		if (FCGI_Accept() < 0) {
			accepting = 0;
			break;
		}
		accepting = 0;
#ifdef USEBIG5
		start_outcache();
#endif
		cginame = NULL;
		if (setjmp(cgi_start)) {
#ifdef USEBIG5
			end_outcache();
#endif
			cgi_time(a);
			continue;
		}
		html_header(0);
		hsprintf(NULL, NULL);
		now_t = time(NULL);
		via_proxy = 0;
		strsncpy(realfromhost, getsenv("REMOTE_ADDR"),
			 sizeof (realfromhost));
		inet_aton(realfromhost, &from_addr);
		in = from_addr;
		in.s_addr &= 0x0000ffff;
		strsncpy(fromhost, inet_ntoa(in), sizeof (fromhost));
		for (i = 0; wwwcache->validproxy[i] && i < MAX_PROXY_NUM; i++) {
			if (from_addr.s_addr == wwwcache->validproxy[i]) {
				via_proxy = 1;
				break;
			}
		}
		if (via_proxy
		    || from_addr.s_addr == wwwcache->accel_addr.s_addr) {
			char *ptr, *p;
			ptr = getenv("HTTP_X_FORWARDED_FOR");
			if (!ptr)
				ptr = getsenv("REMOTE_ADDR");
			p = strrchr(ptr, ',');
			if (p != NULL) {
				while (!isdigit(*p) && *p)
					p++;
				if (*p)
					strsncpy(realfromhost, p,
						 sizeof (realfromhost));
				else
					strsncpy(realfromhost, ptr,
						 sizeof (realfromhost));
			} else
				strsncpy(realfromhost, ptr,
					 sizeof (realfromhost));
			inet_aton(realfromhost, &from_addr);
			in = from_addr;
			in.s_addr &= 0x0000ffff;
			strsncpy(fromhost, inet_ntoa(in), sizeof (fromhost));
		}
		http_parm_init();
		if (url_parse())
			http_fatal("%s 没有实现的功能1!",
				   getsenv("SCRIPT_URL"));
		brc_expired();
		a = get_cgi_applet(needcgi);
		if (a != NULL) {
			cginame = a->name[0];
			//access(getsenv("QUERY_STRING"), F_OK);
			wwwcache->www_visit++;
			(*(a->main)) ();
#ifdef USEBIG5
			end_outcache();
#endif
			cgi_time(a);
			continue;
		}
		http_fatal("%s 没有实现的功能2!", getsenv("SCRIPT_URL"));
//              end_outcache();
	}
	logtimeused();
	exit(5);
}
