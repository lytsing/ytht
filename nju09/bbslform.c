#include "bbslib.h"
#define LOGTMP HTMPATH "/login.htm"

int
bbslform_main()
{
#if 0
      static struct mmapfile mf = { ptr:NULL };
	html_header(1);
	if (loginok && !isguest)
		http_fatal("���Ѿ���¼�˰�...");
	if (mmapfile(LOGTMP, &mf) < 0)
		http_fatal("�޷���ģ���ļ�");
	fwrite(mf.ptr, 1, mf.size, stdout);
#endif
	html_header(2);
	printf("<script src=" BBSLEFTJS "></script>\n"
	       "<title>��¼��վ</title>"
	       "<body leftmargin=0 topmargin=0 marginwidth=0 marginheight=0>\n");
#ifdef HTTPS_DOMAIN
	printf("<br>������߲˵����������Ե�¼��<br>");
	printf("<center>[<a href='javascript:window.close()'>�ر�</a>]</center>");
#else
	printf("<nobr><table width=124>\n");
	printf("<form name=l action=bbslogin?url=1 method=post target=_top>");
	printf("<tr><td>"
	       "<input type=hidden name=lastip1 value=''>"
	       "<input type=hidden name=lastip2 value=''>"
	       "�ʺ�<input type=text name=id maxlength=%d size=11><br>"
	       "����<input type=password name=pw maxlength=%d size=11><br>"
	       "��Χ<select name=ipmask>\n"
	       "<option value=0 selected>��IP</option>\n"
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
	       "<input type=submit value=��¼>&nbsp;"
	       "<input type=submit value=ע�� onclick=\"{top.location.href='/"
	       SMAGIC "/bbsreg';return false}\">\n"
	       "</td></tr></form></table>\n", IDLEN, PASSLEN - 1);
#endif
	printf("</body></html>");
	return 0;
}
