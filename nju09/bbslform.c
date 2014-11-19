#include "bbslib.h"
#define LOGTMP HTMPATH "/login.htm"

int
bbslform_main()
{
#if 0
      static struct mmapfile mf = { ptr:NULL };
	html_header(1);
	if (loginok && !isguest)
		http_fatal("你已经登录了啊...");
	if (mmapfile(LOGTMP, &mf) < 0)
		http_fatal("无法打开模板文件");
	fwrite(mf.ptr, 1, mf.size, stdout);
#endif
	html_header(2);
	printf("<script src=" BBSLEFTJS "></script>\n"
	       "<title>登录本站</title>"
	       "<body leftmargin=0 topmargin=0 marginwidth=0 marginheight=0>\n");
#ifdef HTTPS_DOMAIN
	printf("<br>请在左边菜单输入密码以登录。<br>");
	printf("<center>[<a href='javascript:window.close()'>关闭</a>]</center>");
#else
	printf("<nobr><table width=124>\n");
	printf("<form name=l action=bbslogin?url=1 method=post target=_top>");
	printf("<tr><td>"
	       "<input type=hidden name=lastip1 value=''>"
	       "<input type=hidden name=lastip2 value=''>"
	       "帐号<input type=text name=id maxlength=%d size=11><br>"
	       "密码<input type=password name=pw maxlength=%d size=11><br>"
	       "范围<select name=ipmask>\n"
	       "<option value=0 selected>单IP</option>\n"
	       "<option value=1>2 IP</option>\n"
	       "<option value=2>4 IP</option>\n"
	       "<option value=3>8 IP</option>\n"
	       "<option value=4>16 IP</option>\n"
	       "<option value=5>32 IP</option>\n"
	       "<option value=6>64 IP</option>\n"
	       "<option value=7>128 IP</option>\n"
	       "<option value=8>256 IP</option>\n"
	       "<option value=15>32768 IP</option></select>"
	       "<a href=/ipmask.html target=_blank class=red>?</a>\n<br>&nbsp;"
	       "<input type=submit value=登录>&nbsp;"
	       "<input type=submit value=注册 onclick=\"{top.location.href='/"
	       SMAGIC "/bbsreg';return false}\">\n"
	       "</td></tr></form></table>\n", IDLEN, PASSLEN - 1);
#endif
	printf("</body></html>");
	return 0;
}
