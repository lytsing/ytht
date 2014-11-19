#include "bbslib.h"
int
bbssouke_main()
{
	char buf[1024];
	html_header(1);
	if (atoi(getparm("SVR")) == 1)
		strcpy(buf, "http://proxy.souke.net/cgi/SearchCGI.php?");
	else
		strcpy(buf, "http://bbs.souke.net/cgi-bin/SearchCGI?");
	strcat(buf, "Site=");
	strsncpy(buf + strlen(buf), getparm("Site"), 5);
	strcat(buf, "&Region=");
	strsncpy(buf + strlen(buf), getparm("Region"), 5);
	strcat(buf, "&KeyWord=");
	strsncpy(buf + strlen(buf), getparm("KeyWord"), 100);
	if (dofilter(buf, buf, 1)) {
		printf("<script>history.go(-1);</script>");
	} else {
		redirect(buf);
	}
	return 0;
}

void
printSoukeForm()
{
	return;
	if (!strcmp(MY_BBS_ID, "YTHT")) {
		printf("<form action='bbssouke' name=formsouke>\n"
		       "<input name=KeyWord class=FormText size=18 maxlength=255>\n"
		       "<input type=hidden name=Site value=1>"
		       "<input type=hidden name=Region value=0>\n"
		       "<input type=hidden name=SVR value=0>"
		       "<br><input class=btn name=button type=submit value=搜客搜索 onClick='this.form.SVR.value=\"0\"; this.form.submit();' width=80>\n"
		       "<input class=btn name=button1 type=button value='海外通道' onClick='this.form.SVR.value=\"1\"; this.form.submit();' width=80>\n</form>");
	}
}
