#include "bbslib.h"

int
bbsplan_main()
{
	struct mmapfile mf = { ptr:NULL };
	char plan[256];
	html_header(1);
	check_msg();
	printf("<body><center>\n");
	if (!loginok || isguest)
		http_fatal("匆匆过客不能设置说明档，请先登录");
	changemode(EDITUFILE);
	sethomefile(plan, currentuser->userid, "plans");
	if (!strcasecmp(getparm("type"), "update"))
		save_plan(plan);
	printf("%s -- 设置个人说明档 [%s]<hr>\n", BBSNAME, currentuser->userid);
	printf("<form name=form1 method=post action=bbsplan?type=update>\n");
	printf("<table border=1><tr><td>");
	printubb("form1", "text");
	printf
	    ("<textarea  onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}'  onkeypress='if(event.keyCode==10) return document.form1.submit()' name=text rows=20 cols=80 wrap=virtual>\n");
	MMAP_TRY {
		if (mmapfile(plan, &mf) >= 0)
			ansi2ubb(mf.ptr, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	printf("</textarea></table>\n");
	printf("<input type=submit value=存盘> ");
	printf("<input type=reset value=复原>\n");
	printf("<hr></body></html>\n");
	http_quit();
	return 0;
}

int
save_plan(char *plan)
{
	char *text = getparm("text");
	int use_ubb = strlen(getparm("useubb"));
	if (use_ubb)
		ubb2ansi(text, plan);
	else
		f_write(plan, text);
	printf("个人说明档修改成功。");
	http_quit();
	return 0;
}
