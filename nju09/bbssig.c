#include "bbslib.h"
FILE *fp;

int
bbssig_main()
{
	struct mmapfile mf = { ptr:NULL };
	char path[256];
	html_header(1);
	if (!loginok || isguest)
		http_fatal("匆匆过客不能设置签名档，请先登录");
	changemode(EDITUFILE);
	printf("<body><center>%s -- 设置签名档 [使用者: %s]<hr>\n",
	       BBSNAME, currentuser->userid);
	sprintf(path, "home/%c/%s/signatures",
		mytoupper(currentuser->userid[0]), currentuser->userid);
	if (!strcasecmp(getparm("type"), "1"))
		save_sig(path);
	printf("<form name=form1 method=post action=bbssig?type=1>\n");
	printf("签名档每6行为一个单位, 第1行到第6行为第一个, 第7行到第12行开始为第2个, 依此类推.回车为换行标记<br>"
	       "(<a href=bbscon?B=Announce&F=M.1047666649.A>"
	       "[临时公告]关于图片签名档的大小限制</a>)<br>");
	printubb("form1", "text");
	printf
	    ("<textarea  onkeydown='if(event.keyCode==87 && event.ctrlKey) {document.form1.submit(); return false;}'  onkeypress='if(event.keyCode==10) return document.form1.submit()' name=text rows=20 cols=80>\n");
	MMAP_TRY {
		if (mmapfile(path, &mf) >= 0) 
			ansi2ubb(mf.ptr, mf.size, stdout);
	}
	MMAP_CATCH {
	}
	MMAP_END mmapfile(NULL, &mf);
	printf("</textarea><br>\n");
	printf("<input type=submit value=存盘> ");
	printf("<input type=reset value=复原>\n");
	printf("</form><hr></body>\n");
	http_quit();
	return 0;
}

void
save_sig(char *path)
{
	char *buf;
	int use_ubb = strlen(getparm("useubb"));
	buf = getparm("text");
	if (use_ubb)
		ubb2ansi(buf, path);
	else
		f_write(path, buf);
	printf("签名档修改成功。");
	http_quit();
}
