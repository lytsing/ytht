#include "bbslib.h"

int
bbsselstyle_main()
{
	char name[STRLEN], *p, *url;
	int i;
#ifdef USESESSIONCOOKIE
	char session[STRLEN];
	strsncpy(session, getparm("SESSION"), sizeof (session));
#endif
	url = getenv("SCRIPT_URL");
	if (NULL == url)
		return -1;
	name[0] = 0;
	if (!strncmp(url, "/" SMAGIC, sizeof (SMAGIC))) {
		snprintf(name, STRLEN, "%s", url + sizeof (SMAGIC));
		p = strchr(name, '/');
		if (NULL != p) {
			*p = 0;
		} else {
			http_fatal("Incorrect url");
			return -1;
		}
	}
	html_header(1);
	check_msg();
	printf("<body><center><br><h2>选择界面风格</h2><table>");
	for (i = 0; i < NWWWSTYLE; i++) {
		addextraparam(name, sizeof (name), 0, i);
		printf("<tr><td><li><a");
#ifdef USESESSIONCOOKIE
		if (session[1]) {
			addextraparam(session, sizeof (session), 0, i);
			printf
			    (" onclick=\"document.cookie='SESSION=%s; path=/';return true;\"",
			     session);
		}
#endif
		printf(" href=/" SMAGIC "%s/?t=%d target=_top>%s</a></td></tr>",
		       name, now_t, wwwstyle[i].name);
	}
	if (strcmp(currentuser->userid, "guest"))
		printf
		    ("<tr><td><li><a href=bbsdefcss>定义自己的界面</a></td></tr>");
	printf("</table></body>");
	http_quit();
	return 0;
}
