#include "bbslib.h"


int
bbsdefcss_main()
{
	char *ptr, buf[256];
	int type;
	html_header(1);
	check_msg();
	printf("<body>");
	if (!loginok || isguest)
		http_fatal("匆匆过客不能定制界面");
	changemode(GMENU);
	type = atoi(getparm("type"));
	if (type > 0){
		sethomefile(buf, currentuser->userid, "ubbs.css");
		ptr=getparm("ucss");
		f_write(buf,ptr);
		sethomefile(buf, currentuser->userid, "uleft.css");
		ptr=getparm("uleftcss");
		f_write(buf,ptr);
		//个人文集的css,等把那边弄好后再打开。
#if 0
		sethomefile(buf, currentuser->userid, "upercor.css");
		ptr=getparm("upercorcss");
		f_write(buf,ptr);
#endif
		printf("WWW用户定制样式设定成功.<br>\n");
		printf("[<a href='javascript:history.go(-2)'>返回</a>]");
		return 0;
	}
	printf("<table align=center><form action=bbsdefcss method=post>\n");
	printf("<tr><td>\n");
	printf("<input type=hidden name=type value=1>");
	printf("<font color=red> 个人CSS模式允许用户自定义WWW的各种显示效果。 注意，下面是自于当前的界面[%s]所定义的css。确定后将覆盖原有的所有自定义css。</font><br><br>\n",currstyle->name);
	printf
	     ("当前的主窗口的CSS设置如下：<br><textarea name=ucss rows=10 cols=76>");
	if(strstr(currstyle->cssfile,"ubbs.css"))
		sethomefile(buf, currentuser->userid, "ubbs.css");
	else
		sprintf(buf,HTMPATH "%s",currstyle->cssfile);
	showfile(buf);
	printf
	     ("</textarea><br><br>当前的左侧选单的CSS设置如下(body.foot是底行的css)：<br><textarea name=uleftcss rows=5 cols=76>");
	if(strstr(currstyle->leftcssfile,"uleft.css"))
		sethomefile(buf, currentuser->userid, "uleft.css");
	else
		sprintf(buf,HTMPATH "%s",currstyle->leftcssfile);
	showfile(buf);
	printf("</textarea><br><br>");
#if 0
	printf("当前的个人文集的CSS设置如下：<br><textarea name=upercorcss rows=5 cols=76>");
	sethomefile(buf, currentuser->userid, "upercor.css");
	if(!file_exist(buf))
		sprintf(buf,HTMPATH "%s",currstyle->cssfile);
	showfile(buf);
#endif
	printf
	    ("</textarea></td></tr><tr><td align=center><input type=submit value=确定> <input type=reset value=复原>\n");
	printf("</td></tr></form></table></body></html>\n");
	return 0;
}

