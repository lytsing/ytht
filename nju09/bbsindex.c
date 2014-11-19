#include "bbslib.h"

#define ONEFACEPATH "/face"

#define NFACE 3
struct wwwface {
	char *bgcolor;
	char *color;
	char *figure;
	char *stamp;
	char *logo;
	char *board;
};
static struct wwwface *pface;
static struct wwwface oneface = {
	NULL, NULL, NULL, NULL, NULL
};
static struct wwwface bbsface[NFACE] = {
	{"#000000", "#99ccff", "/ytht2men.jpg", NULL, NULL},
	{"white", "#99ccff", "/ythtBlkRedGry.gif", NULL, NULL},
	{"white", "black", "/cai.jpg", "/stamp.gif", "/logo.gif"}
};

int
checkfile(char *fn, int maxsz)
{
	char path[456];
	int sz;
	sprintf(path, HTMPATH "%s", fn);
	sz = file_size(path);
	if (sz < 100 || sz > maxsz)
		return -1;
	return 0;
}

int
loadoneface()
{
	FILE *fp;
	static char buf[256], figure[256 + 100], stamp[356], logo[356];
	char *ptr;

	fp = fopen(HTMPATH ONEFACEPATH "/config", "r");
	if (!fp)
		return -1;
	if (fgets(buf, sizeof (buf), fp) == NULL) {
		fclose(fp);
		return -1;
	}
	fclose(fp);
	ptr = buf;
	oneface.bgcolor = strsep(&ptr, " \t\r\n");
	oneface.color = strsep(&ptr, " \t\r\n");
	oneface.figure = strsep(&ptr, " \t\r\n");
	oneface.stamp = strsep(&ptr, " \t\r\n");
	oneface.logo = strsep(&ptr, " \t\r\n");
	oneface.board = strsep(&ptr, " \t\r\n");
	if (!oneface.logo)
		return -2;
	if (strstr(oneface.figure, "..") ||
	    strstr(oneface.stamp, "..") || strstr(oneface.logo, ".."))
		return -3;
	sprintf(figure, ONEFACEPATH "/%s", oneface.figure);
	oneface.figure = figure;
	if (checkfile(figure, 40000))	//回头减小到15K，不要把这个数字改大！
		return -4;
	if (!strcasecmp(oneface.stamp, "NULL"))
		oneface.stamp = NULL;
	else {
		sprintf(stamp, ONEFACEPATH "/%s", oneface.stamp);
		oneface.stamp = stamp;
		if (checkfile(stamp, 4000))
			return -5;
	}
	if (!strcasecmp(oneface.logo, "NULL"))
		oneface.logo = NULL;
	else {
		sprintf(logo, ONEFACEPATH "/%s", oneface.logo);
		oneface.logo = logo;
		if (checkfile(logo, 6500))
			return -6;
	}
	if (oneface.board && (!strcasecmp(oneface.board, "NULL")
			      || !strcasecmp(oneface.board, "Painter")))
		oneface.board = NULL;
	return 0;
}

int
showannounce()
{
      static struct mmapfile mf = { ptr:NULL };
	if (mmapfile("0Announce/announce", &mf) < 0 || mf.size <= 10)
		return -1;
	printf("<br><br><fieldset style=\"width: 740px;\">"
	       "<legend align=\"center\"><font size=4 color=%s>:: 公告 ::</font></legend>"
	       "<div align=\"left\" style=\"margin-top:8px; margin-left:15px; margin-right:15px;margin-bottom:10px;font-color:%s;font-size:9pt\">",
	       pface->color, pface->color);
	fwrite(mf.ptr, mf.size, 1, stdout);
	printf("</div></fieldset>");
	return 0;
}

void
loginwindow()
{

//int n=random()%NFACE;
	int n = 2;
	if (!loadoneface())
		pface = &oneface;
	else
		pface = &(bbsface[n]);
	html_header(4);
	if (!strcmp(MY_BBS_NAME, "TTTAN")) {
		printf("<title>Technology! Thought! Truth! - %s</title>\n",
		       MY_BBS_NAME);
	} else {
		printf("<title>%s</title>\n", MY_BBS_NAME);
	}
	printf("<style type=\"text/css\">\n"
	       ".textstyle{font-family:tahoma;font-size:10.5pt;color:%s;}\n"
	       ".textstylebold{font-family:tahoma;font-size:10.5pt;color:%s;font-weight:bold;}\n"
	       ".inputbox{border-style:solid;border-width:1px 1px 1px 1px;color:#336699;font-family:tahoma;font-size:9pt;}"
	       ".button{font-family:tahoma;font-size:9pt;position:relative;top:1px;}\n"
	       "a{text-decoration:none;color:%s;}\n"
	       "a:hover{color:#000000;background-color:#b7dbff;}\n"
	       "a:visited{color:%s;}\n"
	       "body{background-color:%s;}\n"
	       "</style>\n", pface->color, pface->color, pface->color,
	       pface->color, pface->bgcolor);
	printf(
		      //"<STYLE type=text/css>*{ font-family:arial,sans-serif;}\n"
		      //     "A{COLOR: %s; text-decoration: none;}"
		      //     "</STYLE>\n"
		      "<script>function sf(){document.l.id.focus();}\n"
		      "function st(){document.l.t.value=(new Date()).valueOf();}\n"
		      "function lg(){self.location.href='/" SMAGIC
		      "/bbslogin?id=guest&ipmask=8&t='+(new Date()).valueOf();}\n"
		      "</script>\n"
		      "</head>\n<BODY text=%s leftmargin=1 MARGINWIDTH=1>\n<br><br>"
		      "<center>", pface->color);
	if (pface->board != NULL) {
		if (strchr(pface->board, '.')) {
			printf("<a href=%s target=_blank>", pface->board);
			printf
			    ("<IMG src=%s border=0 alt='进入 %s'>\n",
			     pface->figure, pface->board);
		} else {
			printf("<a href=http://%s." MY_BBS_DOMAIN ">",
			       pface->board);
			printf
			    ("<IMG src=%s border=0 alt='进入 %s 讨论区'>\n",
			     pface->figure, pface->board);
		}
		printf("</a>");
	}

	else
		printf("<IMG src=%s border=0 alt=''><br>&nbsp;\n",
		       pface->figure);
	//if (pface->stamp)
	//      printf
	//          ("<table width=75%%><tr><td align=right><IMG src=%s border=0 alt=''></td></tr></table>",
	//           pface->stamp);
	printf
	    ("<table align=\"center\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\" class=\"textstyle\"><tr><td>");
#ifdef HTTPS_DOMAIN
	printf("<form name=l action=https://" HTTPS_DOMAIN "/" SMAGIC
	       "/bbslogin method=post>");
	printf("<input type=hidden name=usehost value='http%s://%s:%s'>",
	       strcasecmp(getsenv("HTTPS"), "ON") ? "" : "s",
	       getsenv("HTTP_HOST"), getsenv("SERVER_PORT"));
#else
	printf("<form name=l action=/" SMAGIC "/bbslogin method=post>");
#endif
	printf
	    ("<table class=\"textstyle\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\"><tr><td>");
	if (pface->logo)
		printf("<IMG src=%s border=0 alt=''></td><td>", pface->logo);
	else
		printf("<b><font size=4px>" MY_BBS_NAME "</font></b>");
	printf
	    ("<td> 用户 <input class=\"inputbox\" maxLength=%d size=8 name=id>"
	     " 密码 <input class=\"inputbox\" type=password maxLength=%d size=8 name=pw>"
	     "<input type=submit value=登录 class=\"button\">"
	     "<input type=hidden name=t value=''>"
	     "<input type=hidden name=lastip1 value=''>"
	     "<input type=hidden name=lastip2 value=''></td><td>&nbsp;"
#ifndef USEBIG5
	     "<a href=/ipmask.html target=_blank>验证范围</a></td><td><select name=ipmask class=\"inputbox\">"
#else
	     "<a href=/big5ipmask.html target=_blank>验证范围</a></td><td><select name=ipmask class=\"inputbox\">"
#endif
	     "<option value=0 selected>单IP</option>"
	     "<option value=3>8 IP</option>"
	     "<option value=6>64 IP</option>"
	     "<option value=8>256 IP</option>"
	     "<option value=15>32K IP</option>"
	     "</select></td><td>", IDLEN, PASSLEN - 1);
	if (strcmp(MY_BBS_ID, "YTHT"))
		printf("&nbsp;<a href=/" SMAGIC
		       "/bbsemailreg>自动注册</a>&nbsp;"
		       "<a href='javascript:lg();'>游客请进</a>"
		       "</td></tr></table></form>");
	printf("</td></tr></table>\n" "<script>sf();st();</script>");
	printf("<br><div class=\"textstyle\"><center>");
	printf("<script>function langpage(lan) { "
	       "var p=lan;if(lan=='GB')p='';"
	       "exDate = new Date;exDate.setMonth(exDate.getMonth()+9);"
	       "document.cookie='uselang='+lan+';path=/;expires=' + exDate.toGMTString();"
	       "self.location.replace('/" BASESMAGIC
	       "'+p+'/bbsindex');}</script>");
	printf("<a href=\"javascript:langpage('GB');\">简体版[GB]</a>　"
	       "<a href=\"javascript:langpage('BIG5');\">繁体版[BIG5]</a>　");
#ifdef HTTPS_DOMAIN
	printf("<a href=\"https://" HTTPS_DOMAIN "/\">安全传输[HTTPS]</a>　");
#endif
	printf("<!--a href='telnet://" MY_BBS_DOMAIN "'>Telnet登录" MY_BBS_ID
	       "</a>　"
	       "<a href=\"javascript:window.external.AddFavorite('http://"
	       MY_BBS_DOMAIN "/','◆" MY_BBS_LOC MY_BBS_NAME "◆')\">"
	       "将本站加入收藏夹</a>　<a href=\"mailto:" ADMIN_EMAIL
	       "\">联系站务组</a-->");
	//if (!strcmp(MY_BBS_ID, "YTHT"))
	//      printf("　<a href=/ythtsearch.htm target=_blank>搜索本站</a>");
	showannounce();
	printf("</center></div>");
	setlastip();
	printf("</CENTER></BODY></HTML>");
}

int
setlastip()
{
	printf("<script>document.write('<script src=/" SMAGIC
	       "/bbslastip?n=1&t='+(new Date()).valueOf()+'><\\/script>');\n"
	       "document.write('<script src=/" SMAGIC
	       "/bbslastip?n=2&t='+(new Date()).valueOf()+'><\\/script>');</script>");
	return 0;
}

void
shownologin()
{
	int n = 0;
      static struct mmapfile mf = { ptr:NULL };
	html_header(4);
	printf
	    ("<STYLE type=text/css>A{COLOR: #99ccff; text-decoration: none;}</STYLE>"
	     "</head><BODY text=#99ccff bgColor=%s leftmargin=1 MARGINWIDTH=1><br>"
	     "<CENTER>", bbsface[n].bgcolor);
	printf("<IMG src=%s border=0 alt='' width=70%%><BR>",
	       bbsface[n].figure);
	printf("<b>停站通知</b><br>");
	if (!mmapfile("NOLOGIN", &mf))
		fwrite(mf.ptr, mf.size, 1, stdout);
	printf("</CENTER></BODY></HTML>");
	return;
}

void
checklanguage()
{
	char *scripturl = getsenv("SCRIPT_URL");
	int tobig = 0;
	while (!strcmp("/", scripturl)) {
		char lang[6];
		char *langcookie = getparm("uselang");
		if (!strcmp(langcookie, "GB"))
			return;
		if (!strcmp(langcookie, "BIG5")) {
			tobig = 1;
			break;
		}
		strsncpy(lang, getsenv("HTTP_ACCEPT_LANGUAGE"), 6);
		if (!strncasecmp(lang, "zh-tw", 5)
		    || !strncasecmp(lang, "zh-hk", 5)
		    || !strncasecmp(lang, "zh-mo", 5)) {
			tobig = 1;
		}
		break;
	}
	if (!tobig)
		return;
	//errlog("哦，有繁体使用繁体的？%s", realfromhost);
	html_header(3);
	redirect(BIG5FIRSTPAGE);
	http_quit();

}

static int
printFrameset()
{
	html_header(1);
	printf("<title>%s</title>\n"
	       "<frameset id=fs0 frameborder=1 frameSpacing=1 border=1 cols=\"128,*\">\n"
	       "<frame name=f2 src=bbs%sleft?t=%ld MARGINWIDTH=1 MARGINHEIGHT=1>\n"
	       "<frameset id=fs1 rows=\"18, *, 15\" frameSpacing=0 frameborder=no border=0>\n"
	       "<frame scrolling=no name=fmsg src=\"bbsgetmsg\">\n"
	       "<frame name=f3 src=%s>"
	       "<frame scrolling=no marginwidth=4 marginheight=1 name=f4 src=\""
	       "bbsfoot?t=%ld\">\n"
	       "</frameset>\n"
	       "</frameset>\n", MY_BBS_NAME,
	       strcasecmp(MY_BBS_ID, "ytht") ? "" : "tmp",
	       now_t, bbsred(rframe), now_t);
	return 0;
}

int
bbsindex_main()
{
	char str[20], redbuf[50];
	char *t, *b;
	char *agent;
	t = getparm("t");
	b = getparm("b");
	if (nologin) {
		shownologin();
		http_quit();
		return 0;
	}

	if ((!loginok || isguest) && (rframe[0] == 0) && !t[0] && !b[0]) {
		checklanguage();
#if 0
		if (strcasecmp(FIRST_PAGE, getsenv("SCRIPT_URL"))) {
			html_header(3);
			redirect(FIRST_PAGE);
			http_quit();
		}
#endif

		wwwcache->home_visit++;
		loginwindow();
		http_quit();
	}
	if (!loginok && !t[0] && !b[0]) {
		sprintf(redbuf, "/" SMAGIC "/bbslogin?id=guest&ipmask=8&t=%d",
			(int) now_t);
		html_header(3);
		//printf("<!-- %s -- %s -->", getsenv("SCRIPT_URL"), rframe);
		redirect(redbuf);
		http_quit();
	}
	if (cache_header(1000000000, 86400))
		return 0;

	if (!isguest
	    &&
	    (readuservalue(currentuser->userid, "wwwstyle", str, sizeof (str))
	     || atoi(str) != wwwstylenum)) {
		sprintf(str, "%d", wwwstylenum);
		saveuservalue(currentuser->userid, "wwwstyle", str);
	}
#if 1
	{
		char *ptr;
		char buf[256];
		ptr = getsenv("HTTP_USER_AGENT");
		sprintf(buf, "%-14.14s %.100s", currentuser->userid, ptr);
		addtofile(MY_BBS_HOME "/browser.log", buf);
	}
#endif

	agent = getsenv("HTTP_USER_AGENT");

	if (strstr(agent, "Konqueror")) {
		printFrameset();
		return 0;
	}
	printf("Content-type: text/html; charset=%s\r\n\r\n", CHARSET);
	printf
	    ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" "
	     "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">");
	//<html xmlns="http://www.w3.org/1999/xhtml" xmlns:v="urn:schemas-microsoft-com:vml">
	//printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" "
	//     "\"http://www.w3.org/TR/REC-html40/strict.dtd\">\n");
	printf
	    ("<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\" />\n",
	     CHARSET);
	printf("<title>%s</title>\n", MY_BBS_NAME);
	printf
	    ("<style type=\"text/css\"><!--\n"
	     "html{overflow:hidden;height:100%%;width:100%%;margin:0px;}\n"
	     "--></style>");
	printf
	    ("<script type=\"text/javascript\" src=\"/indexjs.js?%d\"></script>\n",
	     cssVersion());
	printf("</head>"
	       "<body style=\"overflow:hidden;margin:0px;height:100%%;width:100%%;\""
	       " onresize=\"doResize();\" onload=\"doResize();\""
	       " onmouseover=\"doBodyMouseIn();\">\n");
	printf("<span id=\"mw\" style=\"font-size:100pt;\">　</span>");

	if (strstr(agent, "Opera")) {
		printf
		    ("<iframe id=\"f2\" name=\"f2\" frameborder=\"0\" border=\"0\" marginwidth=\"0\""
		     " style=\"position:absolute;top:0;left:0;width:128px;height:100%%;\""
		     " src=\"bbs%sleft?t=%ld\"></iframe>\n",
		     strcasecmp(MY_BBS_ID, "ytht") ? "" : "tmp", now_t);
	}
	printf
	    ("<iframe id=\"fmsg\" name=\"fmsg\" frameborder=\"0\" border=\"0\""
	     " style=\"position:absolute;top:0;left:128px;width:100%%;height:18px;\""
	     " src=\"bbsgetmsg\"></iframe>\n");
	printf("<iframe id=\"f3\" name=\"f3\" frameborder=\"0\" border=\"0\""
	       " style=\"position:absolute;top:18px;left:128px;width:100%%;height:100px;\""
	       " src=\"%s\"></iframe>\n", bbsred(rframe));
	printf("<iframe id=\"f4\" name=\"f4\" frameborder=\"0\" border=\"0\""
	       " style=\"position:absolute;top:150px;left:128px;width:100%%;height:15px;overflow:hidden\""
	       " src=\"bbsfoot?t=%ld\"></iframe>\n", now_t);
	if (!strstr(agent, "Opera")) {
		printf
		    ("<iframe id=\"f2\" name=\"f2\" frameborder=\"0\" border=\"0\" marginwidth=\"0\""
		     " style=\"position:absolute;top:0;left:0;width:128px;height:100%%;\""
		     " src=\"bbs%sleft?t=%ld\"></iframe>\n",
		     strcasecmp(MY_BBS_ID, "ytht") ? "" : "tmp", now_t);
		printf("<script type=\"text/javascript\">"
		       "window.onresize=doResize;doResize();</script>");
	} else {
		//need this 'a' for opera to display correctly always
		printf("a<script type=\"text/javascript\">"
		       "window.onresize=doResize;</script>");
	}
	printf("</body></html>");
	return 0;
}
